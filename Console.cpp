#include "Console.h"

// ==========================================
//  以下所有 Windows 依赖全部藏在 .cpp 中
// ==========================================
#include <windows.h>
#include <conio.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>

// ========== Color → Windows WORD 属性转换 ==========
static WORD colorToAttr(Color fg) {
    WORD attr = 0;
    switch (fg) {
        case Color::Black:         attr = 0; break;
        case Color::Red:           attr = FOREGROUND_RED; break;
        case Color::Green:         attr = FOREGROUND_GREEN; break;
        case Color::Yellow:        attr = FOREGROUND_RED | FOREGROUND_GREEN; break;
        case Color::Blue:          attr = FOREGROUND_BLUE; break;
        case Color::Cyan:          attr = FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        case Color::Magenta:       attr = FOREGROUND_RED | FOREGROUND_BLUE; break;
        case Color::White:         attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        case Color::BrightRed:     attr = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
        case Color::BrightGreen:   attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
        case Color::BrightYellow:  attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
        case Color::BrightBlue:    attr = FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        case Color::BrightCyan:    attr = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        case Color::BrightMagenta: attr = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        case Color::BrightWhite:   attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
    }
    return attr;
}

// ========== 内部实现结构 ==========
struct ConsoleImpl {
    HANDLE    hConsole;
    HANDLE    hInput;
    CHAR_INFO* frameBuf;
};

// ==========================================
//  构造 / 析构
// ==========================================
Console::Console(int width, int height)
    : m_width(width), m_height(height),
      m_impl(nullptr), m_ownProcess(false) {
    auto* impl = new ConsoleImpl();
    impl->hConsole = nullptr;
    impl->hInput   = nullptr;
    impl->frameBuf = new CHAR_INFO[width * height];
    m_impl = impl;
}

Console::~Console() {
    auto* impl = static_cast<ConsoleImpl*>(m_impl);
    if (!impl) return;

    if (impl->hConsole) {
        CONSOLE_CURSOR_INFO cursorInfo = {1, TRUE};
        SetConsoleCursorInfo(impl->hConsole, &cursorInfo);

        if (impl->hInput) {
            DWORD mode = 0;
            GetConsoleMode(impl->hInput, &mode);
            mode |= (ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
            SetConsoleMode(impl->hInput, mode);
        }

        fclose(stdout);
        fclose(stdin);

        if (!m_ownProcess) {
            FreeConsole();
        }
    }

    delete[] impl->frameBuf;
    delete impl;
}

// ==========================================
//  生命周期
// ==========================================
void Console::ensureOwnConsole(int argc, char* argv[]) {
    if (argc >= 2 && strcmp(argv[1], "--game") == 0) {
        m_ownProcess = true;
        return;
    }

    HWND hwnd = GetConsoleWindow();
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId()) {
        m_ownProcess = true;
        return;
    }

    char exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    char cmdLine[MAX_PATH + 32];
    snprintf(cmdLine, sizeof(cmdLine), "\"%s\" --game", exePath);

    STARTUPINFO si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    CreateProcess(NULL, cmdLine, NULL, NULL, FALSE,
                  CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    ExitProcess(0);
}

void Console::init(const char* title) {
    auto* impl = static_cast<ConsoleImpl*>(m_impl);

    // 无论从哪启动（双击 exe / cmd / VSCode 调试 / IDE 终端），
    // 都强制弹出"属于本进程"的全新独立控制台窗口，绝不复用 IDE 的终端。
    // 先 FreeConsole 脱离现有控制台（IDE 的管道/伪控制台、父进程控制台都算），
    // 再 AllocConsole 开一个全新窗口。这两个调用对"无控制台"场景是安全空操作，
    // 因此不做任何探测，保证任何启动方式下画面都不会落到 IDE 输出窗口里。
    FreeConsole();
    AllocConsole();
    m_ownProcess = false;   // 本类分配的窗口，析构时 FreeConsole 关闭
    SetConsoleTitle(title);

    // 关键：不要用 GetStdHandle —— 从 CLion 启动时标准句柄可能被重定向为管道，
    // 子进程继承这些管道后 WriteConsoleOutput 会失败，导致独立窗口黑屏。
    // 这里直接打开"当前进程的控制台"，确保拿到真正的控制台句柄。
    impl->hConsole = CreateFileA("CONOUT$", GENERIC_WRITE | GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL, OPEN_EXISTING, 0, NULL);
    impl->hInput   = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL, OPEN_EXISTING, 0, NULL);

    if (impl->hConsole == NULL || impl->hConsole == INVALID_HANDLE_VALUE ||
        impl->hInput   == NULL || impl->hInput   == INVALID_HANDLE_VALUE) {
        return;  // 拿不到控制台句柄，无法渲染
    }

    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$",  "r", stdin);

    // 调整窗口/缓冲区大小。Windows 规定"窗口不能大于缓冲区"，
    // 所以必须：先缩窗口 → 再设缓冲区 → 最后放大窗口。
    // 顺序错了在默认控制台比目标大的机器上会静默失败，导致黑屏。
    SMALL_RECT minRect  = {0, 0, 1, 1};
    SMALL_RECT fullRect = {0, 0, (SHORT)(m_width - 1), (SHORT)(m_height - 1)};
    COORD bufSize = {(SHORT)m_width, (SHORT)m_height};

    SetConsoleWindowInfo(impl->hConsole, TRUE, &minRect);
    SetConsoleScreenBufferSize(impl->hConsole, bufSize);
    SetConsoleWindowInfo(impl->hConsole, TRUE, &fullRect);

    CONSOLE_CURSOR_INFO cursorInfo = {1, FALSE};
    SetConsoleCursorInfo(impl->hConsole, &cursorInfo);

    DWORD mode = 0;
    GetConsoleMode(impl->hInput, &mode);
    mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    SetConsoleMode(impl->hInput, mode);

    // ==========================================
    //  窗口属性：锁定为固定大小，禁止拖拽/最大化
    // ==========================================
    // 注意重新获取：上面可能刚 AllocConsole，句柄已经变了
    HWND gameWnd = GetConsoleWindow();
    if (gameWnd) {
        // 去掉"拖边框改大小"(WS_THICKFRAME) 和"最大化按钮"(WS_MAXIMIZEBOX)
        LONG_PTR style = GetWindowLongPtr(gameWnd, GWL_STYLE);
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        SetWindowLongPtr(gameWnd, GWL_STYLE, style);

        // 同步删掉系统菜单（Alt+Space）里的"大小"和"最大化"项，
        // 否则键盘 Alt+Space 仍能触发
        HMENU hMenu = GetSystemMenu(gameWnd, FALSE);
        if (hMenu) {
            DeleteMenu(hMenu, SC_SIZE,     MF_BYCOMMAND);
            DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);
        }

        // 通知 Windows 重新计算非客户区，让样式立刻生效
        SetWindowPos(gameWnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOACTIVATE | SWP_FRAMECHANGED);
        // 保留右上角 X 关闭按钮：窗口现在是本进程新分配的，
        // 用户随时可以关窗退出（点 X 会走 CTRL_CLOSE，由系统结束进程）。
    }

    frameClear();
    framePresent();
}

// ==========================================
//  帧缓冲
// ==========================================
void Console::frameClear(Color bg) {
    auto* impl = static_cast<ConsoleImpl*>(m_impl);
    WORD attr = colorToAttr(bg);
    for (int i = 0; i < m_width * m_height; i++) {
        impl->frameBuf[i].Char.AsciiChar = ' ';
        impl->frameBuf[i].Attributes = attr;
    }
}

void Console::put(int x, int y, char ch, Color fg) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return;
    auto* impl = static_cast<ConsoleImpl*>(m_impl);
    int idx = y * m_width + x;
    impl->frameBuf[idx].Char.AsciiChar = ch;
    impl->frameBuf[idx].Attributes = colorToAttr(fg);
}

void Console::print(int x, int y, const char* str, Color fg) {
    while (*str) {
        put(x++, y, *str, fg);
        str++;
    }
}

void Console::printfAt(int x, int y, Color fg, const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    print(x, y, buf, fg);
}

void Console::framePresent() {
    auto* impl = static_cast<ConsoleImpl*>(m_impl);
    COORD bufSize  = {(SHORT)m_width, (SHORT)m_height};
    COORD bufCoord = {0, 0};
    SMALL_RECT region = {0, 0, (SHORT)(m_width - 1), (SHORT)(m_height - 1)};
    WriteConsoleOutput(impl->hConsole, impl->frameBuf, bufSize, bufCoord, &region);
}

// ==========================================
//  几何图形
// ==========================================
void Console::drawBox(int x, int y, int w, int h, char ch, Color fg) {
    for (int i = 0; i < w; i++) {
        put(x + i, y,         ch, fg);
        put(x + i, y + h - 1, ch, fg);
    }
    for (int j = 0; j < h; j++) {
        put(x,         y + j, ch, fg);
        put(x + w - 1, y + j, ch, fg);
    }
}

void Console::fillBox(int x, int y, int w, int h, char ch, Color fg) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            put(x + i, y + j, ch, fg);
        }
    }
}

void Console::drawHLine(int x, int y, int len, char ch, Color fg) {
    for (int i = 0; i < len; i++) put(x + i, y, ch, fg);
}

void Console::drawVLine(int x, int y, int len, char ch, Color fg) {
    for (int j = 0; j < len; j++) put(x, y + j, ch, fg);
}

// ==========================================
//  输入
// ==========================================
bool Console::hasKey() const {
    return _kbhit() != 0;
}

Key Console::readKey() {
    int key = _getch();
    if (key == 0 || key == 0xE0) {
        int ext = _getch();
        switch (ext) {
            case 72: return Key::Up;
            case 80: return Key::Down;
            case 75: return Key::Left;
            case 77: return Key::Right;
            case 59: return Key::F1;
            case 60: return Key::F2;
            case 61: return Key::F3;
            case 62: return Key::F4;
            case 63: return Key::F5;
            case 64: return Key::F6;
            case 65: return Key::F7;
            case 66: return Key::F8;
            case 67: return Key::F9;
            case 68: return Key::F10;
            case 133: return Key::F11;
            case 134: return Key::F12;
            default: return static_cast<Key>(ext);
        }
    }
    return static_cast<Key>(key);
}

// ==========================================
//  属性
// ==========================================
int Console::width()  const { return m_width; }
int Console::height() const { return m_height; }

void Console::sleep(int msec) const{
    Sleep(msec);
}
