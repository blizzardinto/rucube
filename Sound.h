#ifndef SOUND_HPP
#define SOUND_HPP

// ==========================================
//  Sound — 复古音效（Windows Beep 蜂鸣器）
//  在独立线程中播放，不阻塞游戏主循环
// ==========================================

// ========== 音效种类 ==========
enum class Sfx {
    Start,      // 开始游戏：上行音阶
    Eat,        // 吃到普通食物（苹果）：短促一声
    EatBonus,   // 吃到高级食物（葡萄）：两声上行
    SpeedUp,    // 速度提升：快速三连音
    Die,        // 死亡：下行音阶
    GameOver,   // 游戏结束：经典下行旋律 + 低音收尾
    Rotate,     // 方块翻转：短促滴声
    Drop,       // 方块落地：低沉撞击"咚"
    Clear1,     // 消除 1 行
    Clear2,     // 消除 2 行
    Clear3,     // 消除 3 行
    Clear4,     // 消除 4 行（Tetris！快速上行琶音 + 高音爆发）
};

// 播放音效（异步，立即返回；同一时刻只播一个，重叠时忽略新的）
// force = true 时强制播放：等待当前音效播完再播（用于 GameOver 等重要音效，
// 避免被上一个短暂音效"吞"掉）
void playSfx(Sfx s, bool force = false);

// 开关音效（静音用）
void setSoundEnabled(bool on);
bool soundEnabled();

// ---------- 背景音乐 ----------
// 游戏进行中的循环 BGM（程序化生成的 chiptune，无需外部文件）
void startMusic();  // 开始循环播放（重复调用安全）
void stopMusic();   // 停止播放（重复调用安全）

#endif // SOUND_HPP
