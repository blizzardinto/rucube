#include "Board.h"

// ==========================================
//  状态
// ==========================================
void Board::reset() {
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            mGrid[y][x] = EMPTY;
}

// ==========================================
//  规则
// ==========================================

// 方块的所有格子都在界内、且不与已固定的格子重叠
bool Board::canPlace(const Cube& c) const {
    Cube::Cell cs[4];
    c.cells(cs);
    for (int i = 0; i < 4; i++) {
        int x = cs[i].x, y = cs[i].y;
        if (x < 0 || x >= W)   return false;   // 左右出界
        if (y >= H)            return false;   // 底部出界
        // y < 0（方块还在顶部边缘之外）视为合法，只检查下方越界
        if (y >= 0 && mGrid[y][x] != EMPTY) return false;
    }
    return true;
}

// 把下落到底的方块写进网格（存 Type 作为颜色索引）
void Board::lock(const Cube& c) {
    Cube::Cell cs[4];
    c.cells(cs);
    for (int i = 0; i < 4; i++) {
        int x = cs[i].x, y = cs[i].y;
        if (x >= 0 && x < W && y >= 0 && y < H) {
            mGrid[y][x] = static_cast<int>(c.type());
        }
    }
}

// 消除满行：从下往上扫描，满行则上方整体下移
int Board::clearLines() {
    int cleared = 0;
    for (int y = H - 1; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < W; x++) {
            if (mGrid[y][x] == EMPTY) { full = false; break; }
        }
        if (full) {
            // 第 y 行及以上整体下移一行
            for (int yy = y; yy > 0; yy--)
                for (int x = 0; x < W; x++)
                    mGrid[yy][x] = mGrid[yy - 1][x];
            // 顶行清空
            for (int x = 0; x < W; x++)
                mGrid[0][x] = EMPTY;
            cleared++;
            y++;   // 同一行再检查一次（刚下移过来的可能也是满行）
        }
    }
    return cleared;
}

bool Board::occupied(int x, int y) const {
    if (x < 0 || x >= W || y < 0 || y >= H) return false;
    return mGrid[y][x] != EMPTY;
}

// ==========================================
//  落点投影：方块一路落到底的位置（副本）
// ==========================================
Cube Board::ghost(const Cube& c) const {
    Cube g = c;
    while (canPlace(g)) g.move(0, 1);
    g.move(0, -1);
    return g;
}

// ==========================================
//  渲染
// ==========================================
Color Board::cellColor(int v) {
    static const Color map[Cube::Type::COUNT] = {
        Color::BrightCyan,     // I
        Color::BrightYellow,   // O
        Color::BrightMagenta,  // T
        Color::BrightGreen,   // S
        Color::BrightRed,     // Z
        Color::BrightBlue,    // J
        Color::White,         // L
    };
    if (v < 0 || v >= Cube::Type::COUNT) return Color::White;
    return map[v];
}

// (px, py) 是棋盘边框左上角的屏幕坐标
void Board::draw(Console& con, int px, int py) const {
    // 边框
    con.drawBox(px, py, W * CELL_W + 2 * BORDER, H + 2 * BORDER, '#', Color::Cyan);

    // 网格
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (mGrid[y][x] == EMPTY) {
                // 空格子：暗色网格点，帮助对齐
                con.put(px + BORDER + x * CELL_W,     py + BORDER + y, '.', Color::Blue);
                con.put(px + BORDER + x * CELL_W + 1, py + BORDER + y, ' ', Color::Blue);
            } else {
                drawCell(con, px, py, x, y, cellColor(mGrid[y][x]));
            }
        }
    }
}

// 在棋盘格坐标 (bx, by) 画一个填满的格子
void Board::drawCell(Console& con, int px, int py, int bx, int by, Color fg) const {
    con.put(px + BORDER + bx * CELL_W,     py + BORDER + by, '[', fg);
    con.put(px + BORDER + bx * CELL_W + 1, py + BORDER + by, ']', fg);
}

// ==========================================
//  画活动方块的落点投影（虚线）
// ==========================================
void Board::drawGhost(Console& con, const Cube& c, int px, int py) const {
    Cube g = ghost(c);
    if (g.y() == c.y()) return;   // 已在落点，实心方块会盖住，无需画

    Cube::Cell cs[4];
    g.cells(cs);
    for (int i = 0; i < 4; i++) {
        if (cs[i].y >= 0) {
            con.put(px + BORDER + cs[i].x * CELL_W,     py + BORDER + cs[i].y, '.', c.color());
            con.put(px + BORDER + cs[i].x * CELL_W + 1, py + BORDER + cs[i].y, ' ', c.color());
        }
    }
}
