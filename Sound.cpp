#include "Sound.h"

#include <windows.h>
#include <mmsystem.h>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <thread>
#include <vector>

namespace {

std::atomic<bool> g_enabled{true};
std::atomic<bool> g_playing{false};

// ============================================================
//  背景音乐：程序化生成 chiptune WAV，循环播放
// ============================================================

constexpr int    kSampleRate = 22050;   // 采样率
constexpr double kEighth     = 0.21;    // 八分音符时长（秒），约 143 BPM
constexpr double kMelodyAmp  = 0.13;   // 旋律音量
constexpr double kBassAmp   = 0.10;    // 低音音量
constexpr double kSfxAmp    = 0.70;    // 音效音量（比背景音乐响）

// 背景音乐缓冲（WAV 文件镜像，PlaySound 播放期间必须保持有效）
std::vector<BYTE> g_music;
bool              g_musicBuilt = false;

// midi 音符号 -> 频率
double noteFreq(int midi) {
    return 440.0 * std::pow(2.0, (midi - 69) / 12.0);
}

// 旋律：64 个八分音符（4 个乐句 x 16），0 表示休止符
// 原创的轻快循环：C 大调琶音上行 + 结尾回落
const int kMelody[] = {
    // 乐句 1（C 和弦）
    72, 76, 79, 76,  72, 76, 79, 76,
    // 乐句 2（Dm 和弦）
    74, 77, 81, 77,  74, 77, 81, 77,
    // 乐句 3（Em 和弦，推向高处）
    76, 79, 83, 79,  76, 79, 83, 79,
    // 乐句 4（F -> G，收尾回落）
    77, 81, 84, 81,  79,  0, 76,  0,
};
constexpr int kMelodyLen = sizeof(kMelody) / sizeof(kMelody[0]);

// 低音：32 个四分音符（每个 = 2 个八分音符），跟随和弦根音
const int kBass[] = {
    48, 55, 48, 55,   // C3 G3
    50, 57, 50, 57,   // D3 A3
    52, 55, 52, 55,   // E3 G3
    53, 55, 50, 43,   // F3 G3 D3 G2（收尾）
};
constexpr int kBassLen = sizeof(kBass) / sizeof(kBass[0]);

// 在 [out, out+n) 追加一段方波音符，带 5ms 淡入淡出防爆音
void addNote(std::vector<int16_t>& out, double freq, int n, double amp) {
    if (n <= 0) return;
    const int fade = kSampleRate / 200;   // 5ms
    for (int i = 0; i < n; i++) {
        double t = (double)i / kSampleRate;
        // 方波（占空比 50%）
        double s = (std::fmod(freq * t, 1.0) < 0.5) ? 1.0 : -1.0;
        // 边缘淡入淡出
        double env = 1.0;
        if (i < fade)            env = (double)i / fade;
        if (i > n - 1 - fade)    env = (double)(n - 1 - i) / fade;
        out.push_back((int16_t)(s * amp * env * 32767.0));
    }
}

// 生成完整 WAV 文件镜像（44 字节头 + PCM 数据）
void buildMusic() {
    // ---- 合成音频 ----
    std::vector<int16_t> samples;

    // 旋律轨
    {
        std::vector<int16_t> track;
        int perNote = (int)(kSampleRate * kEighth);
        for (int i = 0; i < kMelodyLen; i++) {
            if (kMelody[i] == 0) {
                track.insert(track.end(), perNote, 0);
            } else {
                addNote(track, noteFreq(kMelody[i]), perNote, kMelodyAmp);
            }
        }
        samples = std::move(track);
    }

    // 低音轨（叠加混音）
    {
        int perNote = (int)(kSampleRate * kEighth * 2);  // 四分音符
        std::vector<int16_t> bassTrack;
        for (int i = 0; i < kBassLen; i++) {
            addNote(bassTrack, noteFreq(kBass[i]), perNote, kBassAmp);
        }
        // 叠加（自动对齐长度）
        size_t n = std::min(samples.size(), bassTrack.size());
        for (size_t i = 0; i < n; i++) {
            int32_t v = (int32_t)samples[i] + (int32_t)bassTrack[i];
            if (v > 32767)  v = 32767;
            if (v < -32768) v = -32768;
            samples[i] = (int16_t)v;
        }
    }

    // ---- 写 WAV 头 ----
    uint32_t dataSize = (uint32_t)(samples.size() * sizeof(int16_t));
    g_music.reserve(44 + dataSize);

    auto pushU32 = [&](uint32_t v) {
        for (int i = 0; i < 4; i++) g_music.push_back((BYTE)((v >> (8 * i)) & 0xFF));
    };
    auto pushU16 = [&](uint16_t v) {
        for (int i = 0; i < 2; i++) g_music.push_back((BYTE)((v >> (8 * i)) & 0xFF));
    };
    auto pushStr = [&](const char* s, int n) {
        for (int i = 0; i < n; i++) g_music.push_back((BYTE)s[i]);
    };

    pushStr("RIFF", 4);
    pushU32(36 + dataSize);        // ChunkSize
    pushStr("WAVE", 4);
    pushStr("fmt ", 4);
    pushU32(16);                   // Subchunk1Size (PCM)
    pushU16(1);                    // AudioFormat = PCM
    pushU16(1);                    // NumChannels = 单声道
    pushU32(kSampleRate);          // SampleRate
    pushU32(kSampleRate * 2);      // ByteRate = SampleRate * Channels * Bits/8
    pushU16(2);                    // BlockAlign
    pushU16(16);                   // BitsPerSample
    pushStr("data", 4);
    pushU32(dataSize);
    for (int16_t s : samples) {
        g_music.push_back((BYTE)((uint16_t)s & 0xFF));
        g_music.push_back((BYTE)(((uint16_t)s >> 8) & 0xFF));
    }

    g_musicBuilt = true;
}

// ============================================================
//  音效：波形合成 + waveOut 声卡播放（独立于 Beep 音量，更响）
// ============================================================

// 合成并播放一串音符（在后台线程中调用，会阻塞该线程直到播完）
// freq <= 0 表示静默停顿
void playSeqWave(const int* freqs, const int* durs, int n) {
    // ---- 合成 PCM 样本 ----
    std::vector<int16_t> samples;
    for (int i = 0; i < n; i++) {
        int count = (int)(kSampleRate * durs[i] / 1000.0);
        if (freqs[i] > 0) {
            const int fade = kSampleRate / 400;   // 2.5ms 边缘防咔哒声
            for (int j = 0; j < count; j++) {
                double t = (double)j / kSampleRate;
                double s = (std::fmod(freqs[i] * t, 1.0) < 0.5) ? 1.0 : -1.0;  // 方波
                double env = 1.0;
                if (j < fade)         env = (double)j / fade;
                if (j > count - 1 - fade) env = (double)(count - 1 - j) / fade;
                samples.push_back((int16_t)(s * kSfxAmp * env * 32767.0));
            }
        } else {
            samples.insert(samples.end(), count, 0);
        }
    }
    if (samples.empty()) return;

    // ---- 打开波形输出设备并同步播放 ----
    HWAVEOUT hOut = nullptr;
    WAVEFORMATEX fmt{};
    fmt.wFormatTag      = WAVE_FORMAT_PCM;
    fmt.nChannels       = 1;
    fmt.nSamplesPerSec  = kSampleRate;
    fmt.wBitsPerSample  = 16;
    fmt.nBlockAlign     = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

    if (waveOutOpen(&hOut, WAVE_MAPPER, &fmt, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
        return;

    WAVEHDR hdr{};
    hdr.lpData          = (LPSTR)samples.data();
    hdr.dwBufferLength  = (DWORD)(samples.size() * sizeof(int16_t));
    if (waveOutPrepareHeader(hOut, &hdr, sizeof(hdr)) == MMSYSERR_NOERROR) {
        waveOutWrite(hOut, &hdr, sizeof(WAVEHDR));
        // 等待播完（后台线程，不影响游戏主循环）
        while (!(hdr.dwFlags & WHDR_DONE)) Sleep(10);
        waveOutUnprepareHeader(hOut, &hdr, sizeof(hdr));
    }
    waveOutClose(hOut);
}

// 实际播放（在后台线程中执行）
void playSeq(Sfx s) {
    switch (s) {
        case Sfx::Start: {
            const int f[] = { 523, 659, 784, 1047 };   // C E G C 上行
            const int d[] = {  90,  90,  90,   180 };
            playSeqWave(f, d, 4);
            break;
        }
        case Sfx::Eat: {
            const int f[] = { 880 };
            const int d[] = {  45 };
            playSeqWave(f, d, 1);
            break;
        }
        case Sfx::EatBonus: {
            const int f[] = { 988, 1319 };             // 两声快速上行
            const int d[] = {  60,   90 };
            playSeqWave(f, d, 2);
            break;
        }
        case Sfx::SpeedUp: {
            const int f[] = { 660, 880, 1175 };
            const int d[] = {  70,  70,  110 };
            playSeqWave(f, d, 3);
            break;
        }
        case Sfx::Die: {
            const int f[] = { 600, 450, 300, 200 };    // 下行滑落
            const int d[] = { 130, 130, 130, 320 };
            playSeqWave(f, d, 4);
            break;
        }
        case Sfx::GameOver: {
            // 经典 game over 旋律：E5 -> C5 -> G4 -> E4 下行，
            // 最后落在一个低沉长音上，辨识度高、有"结束感"
            const int f[] = { 659, 523, 392, 330, 196 };
            const int d[] = { 180, 180, 180, 180, 500 };
            playSeqWave(f, d, 5);
            break;
        }
        case Sfx::Rotate: {
            const int f[] = { 1320 };              // 短促高频"滴"
            const int d[] = {   35 };
            playSeqWave(f, d, 1);
            break;
        }
        case Sfx::Drop: {
            // 低频"咚"，用短促下降滑音模拟撞击感
            const int f[] = { 220, 150 };
            const int d[] = {  50,  70 };
            playSeqWave(f, d, 2);
            break;
        }
        case Sfx::Clear1: {
            const int f[] = { 660 };
            const int d[] = { 110 };
            playSeqWave(f, d, 1);
            break;
        }
        case Sfx::Clear2: {
            const int f[] = { 660, 880 };          // 两声上行
            const int d[] = { 100, 140 };
            playSeqWave(f, d, 2);
            break;
        }
        case Sfx::Clear3: {
            const int f[] = { 523, 659, 880 };     // 三声快速上行
            const int d[] = {  80,  80, 140 };
            playSeqWave(f, d, 3);
            break;
        }
        case Sfx::Clear4: {
            // Tetris！快速上行琶音 + 高音长爆发，爽感拉满
            const int f[] = { 523, 659, 784, 1047, 1319, 1568 };
            const int d[] = {  55,  55,  55,   55,   55,  450 };
            playSeqWave(f, d, 6);
            break;
        }
    }
}

} // namespace

void playSfx(Sfx s, bool force) {
    if (!g_enabled.load()) return;

    if (force) {
        // 强制播放：等当前音效播完（音效都很短，最长约 1s），避免被吞
        while (g_playing.load()) Sleep(10);
        g_playing.store(true);
    } else {
        // 同一时刻只播一个音效，正在播时忽略新请求
        bool expected = false;
        if (!g_playing.compare_exchange_strong(expected, true)) return;
    }

    std::thread([s] {
        playSeq(s);
        g_playing.store(false);
    }).detach();
}

void setSoundEnabled(bool on) {
    g_enabled.store(on);
    if (!on) stopMusic();
}

bool soundEnabled() {
    return g_enabled.load();
}

void startMusic() {
    if (!g_enabled.load()) return;
    if (!g_musicBuilt) buildMusic();
    if (g_music.empty()) return;
    // 异步循环播放内存中的 WAV；重复调用时重启
    PlaySound((LPCSTR)g_music.data(), NULL,
              SND_MEMORY | SND_LOOP | SND_ASYNC | SND_NODEFAULT);
}

void stopMusic() {
    PlaySound(NULL, NULL, 0);
}
