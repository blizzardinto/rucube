#include "Cube.h"
#include <cstdlib>

// ========== 7 种标准形状（相对偏移 {dx, dy}，y 向下） ==========
//   I: 横条    O: 方块    T: T 字
//   S / Z: 反 S / 正 S    J / L: 反 L / 正 L
const int Cube::SHAPES[Cube::Type::COUNT][4][2] = {
    // I
    { {0,1}, {1,1}, {2,1}, {3,1} },
    // O
    { {0,0}, {1,0}, {0,1}, {1,1} },
    // T
    { {0,1}, {1,1}, {2,1}, {1,0} },
    // S
    { {1,0}, {2,0}, {0,1}, {1,1} },
    // Z
    { {0,0}, {1,0}, {1,1}, {2,1} },
    // J
    { {0,0}, {0,1}, {1,1}, {2,1} },
    // L
    { {2,0}, {0,1}, {1,1}, {2,1} },
};

// ========== 每种形状的颜色 ==========
const Color Cube::COLORS[Cube::Type::COUNT] = {
    Color::BrightCyan,     // I
    Color::BrightYellow,   // O
    Color::BrightMagenta,  // T
    Color::BrightGreen,   // S
    Color::BrightRed,     // Z
    Color::BrightBlue,    // J
    Color::White,         // L
};

// ========== 构造 ==========
Cube::Cube(Type t)
    : mType(t), mX(SPAWN_X), mY(SPAWN_Y) {
    for (int i = 0; i < 4; i++) {
        mOff[i][0] = SHAPES[t][i][0];
        mOff[i][1] = SHAPES[t][i][1];
        mOld[i][0] = mOff[i][0];
        mOld[i][1] = mOff[i][1];
    }
}

// ========== 移动 ==========
void Cube::move(int dx, int dy) {
    mX += dx;
    mY += dy;
}

// ========== 旋转（顺时针 90°） ==========
// 屏幕坐标系 y 向下，顺时针旋转等价于 (dx, dy) -> (dy, -dx)，
// 旋转后整体平移使偏移非负（重新贴回包围盒左上角）。
// 该公式对 7 种形状全部适用，O 型旋转后偏移集合不变。
void Cube::rotate() {
    for (int i = 0; i < 4; i++) {
        mOld[i][0] = mOff[i][0];
        mOld[i][1] = mOff[i][1];
    }

    int nx[4], ny[4];
    int minX = 0, minY = 0;
    for (int i = 0; i < 4; i++) {
        nx[i] =  mOff[i][1];
        ny[i] = -mOff[i][0];
        if (nx[i] < minX) minX = nx[i];
        if (ny[i] < minY) minY = ny[i];
    }
    for (int i = 0; i < 4; i++) {
        mOff[i][0] = nx[i] - minX;
        mOff[i][1] = ny[i] - minY;
    }
}

// ========== 回滚旋转 ==========
void Cube::restore() {
    for (int i = 0; i < 4; i++) {
        mOff[i][0] = mOld[i][0];
        mOff[i][1] = mOld[i][1];
    }
}

// ========== 4 个格子的绝对坐标 ==========
void Cube::cells(Cell out[4]) const {
    for (int i = 0; i < 4; i++) {
        out[i].x = mX + mOff[i][0];
        out[i].y = mY + mOff[i][1];
    }
}

// ========== 颜色 ==========
Color Cube::color() const {
    return COLORS[mType];
}

// ========== 随机形状工厂 ==========
Cube::Type Cube::randomType() {
    return static_cast<Type>(rand() % Type::COUNT);
}

// ========== 包围盒（相对参考点的范围） ==========
void Cube::bounds(int& minX, int& minY, int& maxX, int& maxY) const {
    minX = minY = 9999;
    maxX = maxY = -9999;
    for (int i = 0; i < 4; i++) {
        if (mOff[i][0] < minX) minX = mOff[i][0];
        if (mOff[i][0] > maxX) maxX = mOff[i][0];
        if (mOff[i][1] < minY) minY = mOff[i][1];
        if (mOff[i][1] > maxY) maxY = mOff[i][1];
    }
}

// ========== 画一格（两个字符） ==========
void Cube::paintCell(Console& con, int x, int y, Color c, char left, char right) {
    con.put(x,     y, left,  c);
    con.put(x + 1, y, right, c);
}

// ========== 实心方块 ==========
void Cube::draw(Console& con, int px, int py) const {
    Cell cs[4];
    cells(cs);
    for (int i = 0; i < 4; i++) {
        if (cs[i].y >= 0) {   // 顶部边缘外的格子不画
            paintCell(con,
                      px + BORDER + cs[i].x * CELL_W,
                      py + BORDER + cs[i].y,
                      color(), '[', ']');
        }
    }
}

// ========== 预览框内居中绘制 ==========
void Cube::drawPreview(Console& con, int innerX, int innerY,
                       int innerW, int innerH) const {
    int minX, minY, maxX, maxY;
    bounds(minX, minY, maxX, maxY);

    int pw = (maxX - minX + 1) * CELL_W;   // 方块像素宽
    int ph = (maxY - minY + 1);            // 方块像素高
    int ox = innerX + (innerW - pw) / 2;   // 居中起点
    int oy = innerY + (innerH - ph) / 2;

    for (int i = 0; i < 4; i++) {
        int sx = ox + (mOff[i][0] - minX) * CELL_W;
        int sy = oy + (mOff[i][1] - minY);
        paintCell(con, sx, sy, color(), '[', ']');
    }
}
