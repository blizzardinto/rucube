#ifndef GAMECONFIG_HPP
#define GAMECONFIG_HPP

// ==========================================
//  GameConfig — 全局渲染 / 布局约定
//  供 Board / Cube / Hud / Game / main 共用，
//  把分散在各处的魔法数字集中到一处，避免重复。
// ==========================================

// ---- 控制台尺寸 ----
constexpr int SCREEN_W = 80;
constexpr int SCREEN_H = 30;

// ---- 棋盘渲染约定 ----
// 控制台字符高宽比约 1:2，每个格子用 2 个字符宽才能补成正方形
constexpr int CELL_W = 2;    // 每格占的字符宽度
constexpr int BORDER = 1;    // 边框厚度（字符）

// ---- 棋盘在屏幕上的位置（边框左上角坐标） ----
constexpr int BOARD_PX = 30;
constexpr int BOARD_PY = 4;

// ---- Next 预览框（边框左上角 + 尺寸） ----
constexpr int NEXT_X = 58, NEXT_Y = 4;
constexpr int NEXT_W = 12, NEXT_H = 7;

#endif // GAMECONFIG_HPP
