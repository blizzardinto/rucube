#include "Board.h"

// ============================================================================
// 状态重置：清空 10x20 的棋盘网格
// ============================================================================
void Board::reset() {
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            mGrid[y][x] = EMPTY;
        }
    }
}

// ============================================================================
// 规则裁判 1：碰撞检测 (canPlace)
//
// 【判决逻辑】：
// 遍历方块包含的 4 个小格子 (x, y)：
// 1. 横向越界：x < 0（撞左墙）或 x >= W（撞右墙）-> 非法！
// 2. 纵向穿底：y >= H（穿破地板）-> 非法！
// 3. 重叠碰撞：如果 y >= 0（进入视野内）且该位置已有固化方块 -> 非法！
// （注意：y < 0 允许通过，因为新方块刚出生在顶部以上，尚未完全降入棋盘）
// ============================================================================
bool Board::canPlace(const Cube& c) const {
    for (const auto& cell : c.cells()) {
        int x = cell.x;
        int y = cell.y;
        if (x < 0 || x >= W)   return false;   // 撞左墙或右墙
        if (y >= H)            return false;   // 撞地板
        if (y >= 0 && mGrid[y][x] != EMPTY) return false; // 与老砖块重叠
    }
    return true; // 4 个格子全都安全合法
}

// ============================================================================
// 规则裁判 2：方块固化 (lock)
// 当活动方块触底无法继续下落时，将它的 4 个小格子“焊死”到棋盘 mGrid 中
// ============================================================================
void Board::lock(const Cube& c) {
    for (const auto& cell : c.cells()) {
        int x = cell.x;
        int y = cell.y;
        if (x >= 0 && x < W && y >= 0 && y < H) {
            // 将方块的 Type 存入网格，以便后续根据类型提取颜色
            mGrid[y][x] = static_cast<int>(c.type());
        }
    }
}

// ============================================================================
// 规则裁判 3：消除满行 (clearLines)
// 
// 【算法精髓教学】：
// 1. 从最底部 (y = H - 1) 往上逐行扫描。
// 2. 如果某一行整行都没有 EMPTY，说明这一行被填满了！
// 3. 将这一行上方的所有行整体向下搬移一行（覆盖掉满行）。
// 4. 最顶层第 0 行重新刷成全 EMPTY。
// 5. 【关键技巧 y++】：因为上面的行刚掉下来填充了当前 y 行，
//    掉下来的这一行可能也是满行！所以 y++ 抵消循环末尾的 y--，
//    让外层循环再次检查当前这一行！
// ============================================================================
int Board::clearLines() {
    int cleared = 0;
    for (int y = H - 1; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < W; x++) {
            if (mGrid[y][x] == EMPTY) {
                full = false;
                break;
            }
        }
        if (full) {
            // 满行消除：从当前 y 行开始，上方所有行整体下移一行
            for (int yy = y; yy > 0; yy--) {
                for (int x = 0; x < W; x++) {
                    mGrid[yy][x] = mGrid[yy - 1][x];
                }
            }
            // 最顶行重置为空
            for (int x = 0; x < W; x++) {
                mGrid[0][x] = EMPTY;
            }
            cleared++;
            y++;   // 重检当前行（新下移过来的行可能也是满的）
        }
    }
    return cleared;
}

// 查询某格是否已被占用
bool Board::occupied(int x, int y) const {
    if (x < 0 || x >= W || y < 0 || y >= H) return false;
    return mGrid[y][x] != EMPTY;
}

// ============================================================================
// 规则助手：落点投影物理模拟 (ghost)
//
// 【实现原理】：
// 1. 复制一个与当前方块完全一致的替身副本 g = c；
// 2. 让替身 g 不断向下 move(0, 1)，直到 canPlace(g) 变成 false（碰底）；
// 3. 往回退一步 move(0, -1)，此时 g 的位置就是方块落到底部时的最终姿态！
// ============================================================================
Cube Board::ghost(const Cube& c) const {
    Cube g = c;
    while (canPlace(g)) {
        g.move(0, 1);
    }
    g.move(0, -1);
    return g;
}

// ============================================================================
// 辅助工具：根据网格数值获取颜色
// ============================================================================
Color Board::cellColor(int v) {
    static const Color map[Cube::Type::COUNT] = {
        Color::BrightCyan,     // I
        Color::BrightYellow,   // O
        Color::BrightMagenta,  // T
        Color::BrightGreen,    // S
        Color::BrightRed,      // Z
        Color::BrightBlue,     // J
        Color::White,          // L
    };
    if (v < 0 || v >= Cube::Type::COUNT) return Color::White;
    return map[v];
}

// ============================================================================
// 棋盘自绘制
// 1. 画青色边框；
// 2. 遍历 10x20 网格：空格画暗蓝对齐点，实心方块画彩色 "[]"。
// ============================================================================
void Board::draw(Console& con, int px, int py) const {
    // 1. 绘制包围棋盘的外边框
    con.drawBox(px, py, W * CELL_W + 2 * BORDER, H + 2 * BORDER, '#', Color::Cyan);

    // 2. 遍历棋盘上每个格子
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (mGrid[y][x] == EMPTY) {
                // 空格子：绘制暗蓝色弱化对齐点，方便玩家目测落点
                con.put(px + BORDER + x * CELL_W,     py + BORDER + y, '.', Color::Blue);
                con.put(px + BORDER + x * CELL_W + 1, py + BORDER + y, ' ', Color::Blue);
            } else {
                // 已固化的彩色方块
                drawCell(con, px, py, x, y, cellColor(mGrid[y][x]));
            }
        }
    }
}

// 在屏幕上绘制 1 个棋盘固化格子
void Board::drawCell(Console& con, int px, int py, int bx, int by, Color fg) const {
    con.put(px + BORDER + bx * CELL_W,     py + BORDER + by, '[', fg);
    con.put(px + BORDER + bx * CELL_W + 1, py + BORDER + by, ']', fg);
}

// ============================================================================
// 绘制活动方块的落点虚影 (Ghost Piece)
// 现代俄罗斯方块必备的人性化设计：用虚线圆点投影预告方块落地姿态
// ============================================================================
void Board::drawGhost(Console& con, const Cube& c, int px, int py) const {
    Cube g = ghost(c);
    // 如果虚影跟当前方块完全重合（说明当前已经在最底部了），无需重复画虚影
    if (g.y() == c.y()) return;

    for (const auto& cell : g.cells()) {
        if (cell.y >= 0) {
            // 虚影使用同色弱化点表示轮廓，不遮挡背景
            con.put(px + BORDER + cell.x * CELL_W,     py + BORDER + cell.y, '.', c.color());
            con.put(px + BORDER + cell.x * CELL_W + 1, py + BORDER + cell.y, ' ', c.color());
        }
    }
}

