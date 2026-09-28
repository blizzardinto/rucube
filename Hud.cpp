#include "Hud.h"
#include "GameConfig.h"

void Hud::draw(Console& con, const Cube& next,
               int score, int level, int lines) const {
    drawScorePanel(con, score, level, lines);
    drawNextBox(con, next);
}

// ==========================================
//  左侧信息面板
// ==========================================
void Hud::drawScorePanel(Console& con, int score, int level, int lines) const {
    con.print(6, 4,  "=== TETRIS ===", Color::BrightYellow);

    con.print(6, 8,  "Score:", Color::BrightGreen);
    con.printfAt(6, 9, Color::BrightWhite, "%8d", score);

    con.print(6, 12, "Level:", Color::BrightGreen);
    con.printfAt(6, 13, Color::BrightWhite, "%8d", level);

    con.print(6, 16, "Lines:", Color::BrightGreen);
    con.printfAt(6, 17, Color::BrightWhite, "%8d", lines);

    con.print(4, 21, "-------------------------", Color::Cyan);
    con.print(4, 23, " Left/A   : move left",   Color::White);
    con.print(4, 24, " Right/D  : move right",  Color::White);
    con.print(4, 25, " Up/W/J   : rotate",      Color::White);
    con.print(4, 26, " Down/S   : soft drop",   Color::White);
    con.print(4, 27, " Space    : hard drop",   Color::White);
    con.print(4, 28, " P:pause  Q:quit",        Color::White);
}

// ==========================================
//  Next 预览框（框 + 标题 + 居中方块）
// ==========================================
void Hud::drawNextBox(Console& con, const Cube& next) const {
    con.drawBox(NEXT_X, NEXT_Y, NEXT_W, NEXT_H, '#', Color::Cyan);
    con.print(NEXT_X + 2, NEXT_Y, " NEXT ", Color::BrightCyan);

    // 让方块在框"内部区域"居中自绘
    next.drawPreview(con,
                     NEXT_X + BORDER, NEXT_Y + BORDER,
                     NEXT_W - 2 * BORDER, NEXT_H - 2 * BORDER);
}
