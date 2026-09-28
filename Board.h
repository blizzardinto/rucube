#ifndef BOARD_HPP
#define BOARD_HPP

#include "Console.h"
#include "Cube.h"
#include "GameConfig.h"

// ==========================================
//  Board — 俄罗斯方块的底板（游戏区域）
//
//  设计要点：
//   1. 10 x 20 的网格，每格存一个 int：
//      EMPTY(-1) 表示空，否则存 Cube 的颜色索引（Type）；
//   2. Board 是"规则裁判"：所有移动/旋转合法性由 canPlace() 判断；
//   3. lock() 把下落到底的 Cube 固化进网格；
//   4. clearLines() 消除满行并下移，返回消除行数；
//   5. draw() 自绘边框 + 网格（每格 2 字符宽，否则方块会显扁）。
// ==========================================
class Board {
public:
    static const int W     = 10;   // 网格宽（格）
    static const int H     = 20;   // 网格高（格）
    static const int EMPTY = -1;

    Board() { reset(); }

    // --- 状态 ---
    void reset();                                  // 清空（重开一局用）

    // --- 规则 ---
    bool canPlace(const Cube& c) const;            // 方块能否放在当前位置
    void lock(const Cube& c);                       // 固化到底板
    int  clearLines();                              // 消除满行，返回消除数量

    // --- 渲染 ---
    // (px, py) 为棋盘"边框"的屏幕坐标，棋盘占 2*W+2 宽、H+2 高
    void draw(Console& con, int px, int py) const;
    // 画活动方块 c 的落点投影（虚线）。"落点在哪"是 Board 的知识，
    // 所以"算落点 + 画虚线"合并在这里，调用方无需自己 ghost()。
    void drawGhost(Console& con, const Cube& c, int px, int py) const;

    // --- 查询 ---
    bool occupied(int x, int y) const;

private:
    int mGrid[H][W];   // EMPTY 或 Cube::Type

    static Color cellColor(int v);
    // 在棋盘坐标 (bx, by) 画一格（仅 Board::draw 内部使用）
    void drawCell(Console& con, int px, int py, int bx, int by, Color fg) const;
    // 返回当前方块一路落到底（即将锁定位置）的副本，供 drawGhost 用
    Cube ghost(const Cube& c) const;
};

#endif // BOARD_HPP
