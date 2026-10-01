#ifndef GAME_HPP
#define GAME_HPP

#include "Console.h"
#include "Cube.h"
#include "Board.h"
#include "Hud.h"
#include "GameConfig.h"

// ==========================================
//  Game — 俄罗斯方块游戏核心控制器（游戏逻辑主类）
//
//  职责划分（面向对象封装）：
//   1. 统一调度各子模块：Console（输入输出/双缓冲）、Board（底板/碰撞）、
//      Cube（活动方块/下一个预览）、Hud（界面外壳）、Sound（音效/BGM）；
//   2. 维护游戏运行时状态（分数、消行数、等级、重力计时、暂停/退出标记）；
//   3. 封装经典的游戏循环（Game Loop）：输入处理 -> 状态更新/重力 -> 渲染呈现；
//   4. 封装完整的游戏生命周期（启动初始化、单局游戏流程、游戏结束与重开）。
//   使得 main() 仅需创建 Game 实例并调用 run()，保持入口极简清晰。
// ==========================================
class Game {
public:
    static constexpr int FRAME_MS = 20;     // 主循环每帧 20ms，保证按键响应

    Game();
    ~Game();

    // 运行游戏完整生命周期（包含单局游戏、死亡重开循环）
    void run();

private:
    // --- 内部生命周期与循环 ---
    void init();                            // 初始化控制台与随机种子
    void reset();                           // 初始化/重置单局游戏状态
    void gameLoop();                        // 单局游戏主循环
    void processInput();                    // 读取并处理按键事件
    void update();                          // 游戏状态推进（重力下落等）
    void render();                          // 绘制当前游戏帧
    void renderPaused();                    // 绘制暂停画面
    bool showGameOver();                    // 死亡动画闪烁与询问是否重开（返回 true 表示重开）

    // --- 核心规则逻辑 ---
    bool lockAndAdvance();                  // 固化方块、消行计分、生成下一个；返回 true 表示出生点被堵（Game Over）
    static int dropIntervalFor(int level);  // 根据等级计算下落间隔毫秒
    static int lineScore(int lines, int level); // 根据消除行数与等级计算得分

private:
    // --- 子系统与核心对象 ---
    Console mCon;
    Hud     mHud;
    Board   mBoard;
    Cube    mCurrent;
    Cube    mNext;

    // --- 游戏状态数据 ---
    int     mScore        = 0;
    int     mLines        = 0;
    int     mLevel        = 1;
    int     mDropTimer    = 0;              // 重力计时（毫秒）
    int     mDropInterval = 500;            // 当前下落间隔（毫秒）

    bool    mRunning      = false;          // 单局游戏进行中
    bool    mPaused       = false;          // 是否暂停
    bool    mQuit         = false;          // 是否完全退出程序
};

#endif // GAME_HPP
