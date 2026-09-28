#ifndef HUD_HPP
#define HUD_HPP

#include "Console.h"
#include "Cube.h"

// ==========================================
//  Hud — 游戏界面外壳（左侧信息面板 + Next 预览框）
//
//  职责划分：
//   - Hud  只画"UI 外壳"：标题、分数、操作说明、预览框边框；
//   - Cube 负责画自己（预览里的方块由 Cube::drawPreview 自绘）。
// ==========================================
class Hud {
public:
    void draw(Console& con, const Cube& next,
              int score, int level, int lines) const;

private:
    void drawScorePanel(Console& con, int score, int level, int lines) const;
    void drawNextBox(Console& con, const Cube& next) const;
};

#endif // HUD_HPP
