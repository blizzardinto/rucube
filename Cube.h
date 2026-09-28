#ifndef CUBE_HPP
#define CUBE_HPP

#include "Console.h"
#include "GameConfig.h"

// ==========================================
//  Cube — 一个正在下落的方块（俄罗斯方块 = Tetromino）
//
//  设计要点：
//   1. Cube 只负责"形状数据 + 位置"，完全不知道 Board 的存在；
//   2. 内部用 4 个格子相对参考点 (mX, mY) 的偏移表示形状；
//   3. move()/rotate() 只改数据不检查碰撞 ——
//      是否合法由外部 Board::canPlace() 判断，
//      旋转失败时可用 restore() 回滚；
//   4. 渲染也归 Cube 自己：draw / drawGhost / drawPreview。
// ==========================================
class Cube {
public:
    enum Type { I, O, T, S, Z, J, L, COUNT };

    struct Cell { int x, y; };

    // 以指定形状生成（出生点在顶部中间）
    explicit Cube(Type t = Type::I);

    // 随机形状（工厂方法）
    static Type randomType();

    // --- 变换（不查碰撞，需配合 Board::canPlace） ---
    void move(int dx, int dy);
    void rotate();      // 顺时针 90°（O 型自动不变）
    void restore();    // 回滚到上一次 rotate() 之前

    // --- 查询 ---
    void  cells(Cell out[4]) const;   // 4 个格子的绝对坐标
    Type  type()  const { return mType; }
    Color color() const;              // 每种形状固定颜色
    int   x() const { return mX; }
    int   y() const { return mY; }

    // 形状包围盒（相对参考点的范围），供居中/预览计算用
    void  bounds(int& minX, int& minY, int& maxX, int& maxY) const;

    // --- 渲染（Cube 负责画自己） ---
    // (px, py) 为棋盘边框左上角的屏幕坐标
    void draw(Console& con, int px, int py) const;               // 实心方块
    // 在预览框"内部区域"居中绘制（供 Hud 的 Next 预览用）
    void drawPreview(Console& con, int innerX, int innerY,
                     int innerW, int innerH) const;

    // 出生位置（Board 宽度的一半偏左），供外部摆放
    static constexpr int SPAWN_X = 3;
    static constexpr int SPAWN_Y = 0;

private:
    Type mType;
    int  mX, mY;           // 参考点（形状包围盒左上角）
    int  mOff[4][2];       // 4 个格子的相对偏移 {dx, dy}
    int  mOld[4][2];       // rotate() 前的备份

    // 画一格（左/右两个字符），供 draw / drawGhost / drawPreview 复用
    static void paintCell(Console& con, int x, int y, Color c,
                          char left, char right);

    static const int   SHAPES[Type::COUNT][4][2];
    static const Color COLORS[Type::COUNT];
};

#endif // CUBE_HPP
