#ifndef CONSOLE_HPP
#define CONSOLE_HPP

// ============================================================================
// 🖥️ 控制台平台抽象层 (Console) — 面向对象与现代 C++ 核心思想教学：
//
// 【1. 为什么游戏画面不会闪烁？—— 经典“双缓冲 (Double Buffering)”原理】
// 很多初学者写控制台游戏，喜欢用 system("cls") 清屏再用 cout 打印。
// 结果画面疯狂闪烁，眼睛都看花了！为什么？
// 因为屏幕每秒刷新 60 次，当屏幕刷新到一半，你的新画面还没画完，玩家就看到了空白和撕裂！
//
// 本项目解决方案：
// 内存中开辟一块看不见的“后台离线画布 (Back Buffer)”。
// 每一帧你可以在上面随便画（frameClear、put、print）；
// 画好之后，调用 framePresent()，底层调用 Windows 的 WriteConsoleOutput 函数，
// 在 1 微秒之内把整张画“瞬间盖”到屏幕上！完全彻底告别闪烁！
//
// 【2. 高级 C++ 设计模式：PIMPL 模式 (Pointer to Implementation)】
// 细心的你会发现：头文件里完全没有 #include <windows.h>！
// 为什么要藏起来？
// 因为 <windows.h> 包含了几万行底层的复杂定义和宏（比如 min, max 会污染全局）。
// 我们在 private 里只放了一个隐藏指针 void* m_impl，所有的 Windows 类型全藏在 Console.cpp 里。
// 这叫“接口与实现分离”，既让头文件清爽，又能成倍提升编译速度！
//
// 【3. C++ 资源管理：为什么要写 = delete 禁止拷贝？】
// Console(const Console&) = delete;
// 操作系统窗口和显存缓冲区是“独占资源”。如果允许随手拷贝 Console 对象，
// 两个对象指向同一块内存，析构时就会产生致命的“双重释放 (Double Free)”崩溃。
// C++11 的 = delete 明确告诉编译器：“禁止复制这个对象”！
// ============================================================================

// ----------------------------------------------------------------------------
// 强类型颜色枚举
// C++ 知识点：enum class 是作用域安全的，必须写 Color::Red，不会污染全局命名空间
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// 统一按键映射枚举（将 Windows 复杂的按键事件抽象为简单枚举）
// ----------------------------------------------------------------------------
enum class Key {
    None = 0,

    // 常用字母键 (ASCII 映射)
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

    // 方向键（自定义大数值，避开 0~255 的 ASCII 冲突）
    Up    = 256,
    Down  = 257,
    Left  = 258,
    Right = 259,

    // 功能键
    F1 = 260, F2 = 261, F3 = 262, F4 = 263,
    F5 = 264, F6 = 265, F7 = 266, F8 = 267,
    F9 = 268, F10 = 269, F11 = 270, F12 = 271,
};

// ----------------------------------------------------------------------------
// Console 类：提供纯净跨平台风格的 2D 字符渲染与输入接口
// ----------------------------------------------------------------------------
class Console {
public:
    Console(int width, int height); // 指定后台缓冲区宽度和高度
    ~Console();

    // 禁止拷贝构造与拷贝赋值（独占资源保护）
    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    // --- 生命周期与配置 ---
    void ensureOwnConsole(int argc, char* argv[]);
    void init(const char* title = "Console Game");

    // --- 双缓冲离线画布绘制 ---
    void frameClear(Color bg = Color::Black);                         // 1. 清空后台离线画布
    void put(int x, int y, char ch, Color fg = Color::White);         // 2. 绘制单字符
    void print(int x, int y, const char* str, Color fg = Color::White); // 3. 绘制普通文本
    void printfAt(int x, int y, Color fg, const char* fmt, ...);      // 4. 格式化打印文本
    void framePresent();                                              // 5. 瞬间呈现到前台物理屏幕！

    // --- 几何与边框绘制工具 ---
    void drawBox(int x, int y, int w, int h, char ch, Color fg);      // 空心矩形
    void fillBox(int x, int y, int w, int h, char ch, Color fg);      // 实心矩形
    void drawHLine(int x, int y, int len, char ch, Color fg);         // 水平线
    void drawVLine(int x, int y, int len, char ch, Color fg);         // 垂直线

    // --- 非阻塞键盘事件输入 ---
    bool hasKey() const;                                              // 是否有按键等待处理（非阻塞）
    Key  readKey();                                                   // 读取队首按键

    // --- 尺寸查询与延时 ---
    int width()  const;
    int height() const;
    void sleep(int msec) const;

private:
    int   m_width;
    int   m_height;
    void* m_impl;         // PIMPL 模式：隐藏 Windows 底层原生实现结构体
    bool  m_ownProcess;
};

#endif // CONSOLE_HPP