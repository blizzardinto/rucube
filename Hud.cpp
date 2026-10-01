#include "Hud.h"
#include "GameConfig.h"

// ============================================================================
// 绘制整个游戏外壳界面：协调左右两个面板
// ============================================================================
void Hud::draw(Console& con, const Cube& next,
               int score, int level, int lines) const {
    drawScorePanel(con, score, level, lines);
    drawNextBox(con, next);
}

// ============================================================================
// 绘制左侧计分与控制说明面板
// 
// 【C++ / 编程教学点】：
// - 界面排版就是算坐标！屏幕左上角为 (0, 0)，通过固定 (X, Y) 坐标把文本整齐输出。
// - "%8d" 是 C 风格格式化占位符，表示“以 8 位宽度右对齐输出整数”，这样即使分数从
//   10 涨到 100000，数字也不会到处乱窜跳动，界面非常整齐专业！
// ============================================================================
void Hud::drawScorePanel(Console& con, int score, int level, int lines) const {
    // 游戏标题
    con.print(6, 4,  "=== TETRIS ===", Color::BrightYellow);

    // 核心数据统计展示
    con.print(6, 8,  "Score:", Color::BrightGreen);
    con.printfAt(6, 9, Color::BrightWhite, "%8d", score);

    con.print(6, 12, "Level:", Color::BrightGreen);
    con.printfAt(6, 13, Color::BrightWhite, "%8d", level);

    con.print(6, 16, "Lines:", Color::BrightGreen);
    con.printfAt(6, 17, Color::BrightWhite, "%8d", lines);

    // 快捷键操作指南
    con.print(4, 21, "-------------------------", Color::Cyan);
    con.print(4, 23, " Left/A   : move left",   Color::White);
    con.print(4, 24, " Right/D  : move right",  Color::White);
    con.print(4, 25, " Up/W/J   : rotate",      Color::White);
    con.print(4, 26, " Down/S   : soft drop",   Color::White);
    con.print(4, 27, " Space    : hard drop",   Color::White);
    con.print(4, 28, " P:pause  Q:quit",        Color::White);
}

// ============================================================================
// 绘制右侧 NEXT 预览框（外框 + 标题 + 委托 Cube 居中自绘）
//
// 【面向对象教学：委托模式 (Delegation)】
// Hud 负责画好青色的外框 NEXT_X, NEXT_Y，写上 "NEXT" 标签；
// 接下来要画方块时，Hud 并不自己去算 7 种方块怎么画，而是把内框范围告诉 next 方块：
// “方块兄弟，这是给你的舞台空间，请你自己画在正中间！”
// 这就叫对象的协同与委托，每个类做好自己的事！
// ============================================================================
void Hud::drawNextBox(Console& con, const Cube& next) const {
    // 1. 画青色外框
    con.drawBox(NEXT_X, NEXT_Y, NEXT_W, NEXT_H, '#', Color::Cyan);
    // 2. 贴上居中标签
    con.print(NEXT_X + 2, NEXT_Y, " NEXT ", Color::BrightCyan);

    // 3. 计算内框有效空间，委托 Cube 自行在内部居中绘制
    next.drawPreview(con,
                     NEXT_X + BORDER, NEXT_Y + BORDER,
                     NEXT_W - 2 * BORDER, NEXT_H - 2 * BORDER);
}

