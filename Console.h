#ifndef CONSOLE_HPP
#define CONSOLE_HPP

// ==========================================
//  Console — 独立控制台窗口 + 双缓冲渲染
//  头文件不依赖 <windows.h>，所有 Windows 类型藏在 .cpp 中
// ==========================================

// ========== 颜色枚举 ==========
enum class Color {
    Black,
    Red,
    Green,
    Yellow,
    Blue,
    Cyan,
    Magenta,
    White,

    BrightRed,
    BrightGreen,
    BrightYellow,
    BrightBlue,
    BrightCyan,
    BrightMagenta,
    BrightWhite,
};

// ========== 按键枚举 ==========
enum class Key {
    None = 0,

    // 字母 / 数字（ASCII）
    A = 'a', B = 'b', C = 'c', D = 'd', E = 'e', F = 'f',
    G = 'g', H = 'h', I = 'i', J = 'j', K = 'k', L = 'l',
    M = 'm', N = 'n', O = 'o', P = 'p', Q = 'q', R = 'r',
    S = 's', T = 't', U = 'u', V = 'v', W = 'w', X = 'x',
    Y = 'y', Z = 'z',

    Num0 = '0', Num1 = '1', Num2 = '2', Num3 = '3', Num4 = '4',
    Num5 = '5', Num6 = '6', Num7 = '7', Num8 = '8', Num9 = '9',

    Space  = ' ',
    Enter  = 13,
    Escape = 27,
    Backspace = 8,
    Tab    = 9,

    // 方向键（自定义值，避开 ASCII 冲突）
    Up    = 256,
    Down  = 257,
    Left  = 258,
    Right = 259,

    // 功能键
    F1 = 260, F2 = 261, F3 = 262, F4 = 263,
    F5 = 264, F6 = 265, F7 = 266, F8 = 267,
    F9 = 268, F10 = 269, F11 = 270, F12 = 271,
};

// ========== Console 类 ==========
class Console {
public:
    Console(int width, int height);
    ~Console();

    // 禁止拷贝
    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    // --- 生命周期 ---
    void ensureOwnConsole(int argc, char* argv[]);
    void init(const char* title = "Console Game");

    // --- 帧缓冲 ---
    void frameClear(Color bg = Color::Black);
    void put(int x, int y, char ch, Color fg = Color::White);
    void print(int x, int y, const char* str, Color fg = Color::White);
    void printfAt(int x, int y, Color fg, const char* fmt, ...);
    void framePresent();

    // --- 几何图形 ---
    void drawBox(int x, int y, int w, int h, char ch, Color fg);
    void fillBox(int x, int y, int w, int h, char ch, Color fg);
    void drawHLine(int x, int y, int len, char ch, Color fg);
    void drawVLine(int x, int y, int len, char ch, Color fg);

    // --- 输入 ---
    bool hasKey() const;
    Key  readKey();

    // --- 属性 ---
    int width()  const;
    int height() const;

    void sleep(int msec) const;

private:
    int   m_width;
    int   m_height;
    void* m_impl;   // 隐藏实现（HANDLE、CHAR_INFO 等）
    bool  m_ownProcess;
};

#endif // CONSOLE_HPP