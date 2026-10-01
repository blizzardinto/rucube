#ifndef HUD_HPP
#define HUD_HPP

#include "Console.h"
#include "Cube.h"

// ============================================================================
// 📊 界面信息面板类 (Hud) — 面向对象核心思想教学：
//
// 【1. 概念起源：什么是 HUD？】
// HUD 全称是 "Heads-Up Display"（平视显示器，最初源于战斗机座舱玻璃上的投影）。
// 在游戏开发中，HUD 专门用来代指悬浮在屏幕上的【得分面板、血条、等级、提示信息】。
//
// 【2. 面向对象设计：关注点分离 (Separation of Concerns)】
// - 游戏数据（当前得分多少？消了几行？）属于 Game 核心逻辑管；
// - 界面表现（这些字打印在屏幕第几行第几列？用黄色还是绿色？）由 Hud 管！
// - 这种让“逻辑归逻辑、视觉归视觉”的思想，是现代软件工程（如 MVC 架构）的基石。
//
// 【3. 无状态视图 (Stateless View)】
// 注意观察：Hud 类内部【没有任何成员变量】！
// 它不需要记住上一次得了多少分，每次只要 Game 调用 draw() 把最新数据传给它，
// 它就负责忠实地在屏幕上排版画出来。简单、轻量、永不产生数据不同步的 BUG！
// ============================================================================
class Hud {
public:
    // 统一绘制整个 HUD 界面（包括左侧计分板与右侧 Next 预览框）
    void draw(Console& con, const Cube& next,
              int score, int level, int lines) const;

private:
    // 内部方法：绘制左侧得分榜、等级与操作指引
    void drawScorePanel(Console& con, int score, int level, int lines) const;

    // 内部方法：绘制右侧 Next 预览边框并呼叫 next 方块自绘
    void drawNextBox(Console& con, const Cube& next) const;
};

#endif // HUD_HPP

