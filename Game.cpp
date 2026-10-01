#include "Game.h"
#include "Sound.h"
#include <cstdlib>
#include <ctime>

// ============================================================================
// 构造函数：初始化所有子对象
// C++ 知识点：当 Game 实例化时，它内部的 mCon, mCurrent, mNext 也会依次被构造。
// ============================================================================
Game::Game()
    : mCon(SCREEN_W, SCREEN_H),
      mCurrent(Cube::Type::I),
      mNext(Cube::Type::I) {
}

// ============================================================================
// 析构函数：释放资源与退出清理
// C++ 知识点：RAII (Resource Acquisition Is Initialization) 理念！
// 当 Game 生命周期结束（程序退出）时，析构函数自动执行，确保音乐安全停止。
// ============================================================================
Game::~Game() {
    stopMusic();
}

// ============================================================================
// 难度与重力公式算法
//
// 【🎮 游戏设计小秘密：为什么很多初学者写的方块后面速度会暴走？】
// 错误做法：每升一级，下落间隔减少 50ms（例如 500ms -> 450ms -> ... -> 50ms）。
// 速度 = 1000 / 间隔。当从 100ms 降到 50ms 时，速度直接翻倍！到了后期玩家根本没法玩。
//
// 正确做法（本项目使用）：让【每秒下落格数 (格/秒)】线性增加！
// - 1 级：每秒下落 2.0 格（间隔 500ms）
// - 2 级：每秒下落 2.4 格（间隔 416ms）
// - 每升一级平稳增加 0.4 格/秒，最高封顶 8 格/秒（125ms）。丝滑且公平！
// ============================================================================
int Game::dropIntervalFor(int level) {
    double speed = 2.0 + (level - 1) * 0.4;
    if (speed > 8.0) speed = 8.0;
    return static_cast<int>(1000.0 / speed);
}

// 阶梯计分规则：一次消 1/2/3/4 行分别得 100/300/500/800 分，并乘上等级倍率
int Game::lineScore(int lines, int level) {
    static const int base[5] = {0, 100, 300, 500, 800};
    return base[lines] * level;
}

// ============================================================================
// 引擎初次启动初始化
// ============================================================================
void Game::init() {
    // 1. 初始化控制台：锁定 80x30 尺寸、隐藏光标、配置双缓冲
    mCon.init("Tetris - Arrows/WASD to play, Q to quit");
    // 2. 播撒随机数种子（以当前时间为基准，保证每局方块序列不同）
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}

// ============================================================================
// 单局游戏重置（开局或再来一局）
// ============================================================================
void Game::reset() {
    // 1. 棋盘归零清空
    mBoard.reset();

    // 2. 随机生成首个方块与预览方块
    mCurrent = Cube(Cube::randomType());
    mNext    = Cube(Cube::randomType());

    // 3. 重置各项数值状态
    mScore   = 0;
    mLines   = 0;
    mLevel   = 1;
    mRunning = true;
    mPaused  = false;
    mQuit    = false;

    // 4. 防御性检查：如果一开局出生点就放不下，直接结束
    if (!mBoard.canPlace(mCurrent)) {
        mRunning = false;
    }

    // 5. 播放开场音效与启动背景音乐
    playSfx(Sfx::Start);
    startMusic();

    // 6. 初始化重力计时器
    mDropTimer    = 0;
    mDropInterval = dropIntervalFor(mLevel);
}

// ============================================================================
// 触底锁定与推进下一块 (lockAndAdvance)
// 整个游戏逻辑的核心状态转换函数：
// 1. 固化当前块 -> 2. 消除满行 -> 3. 计分与音效 -> 4. 升级判断 -> 5. 换新方块
// 返回值：true 表示新方块在出生点就卡住了（顶到天花板 -> 游戏结束！）
// ============================================================================
bool Game::lockAndAdvance() {
    // 1. 固化到底板网格
    mBoard.lock(mCurrent);

    // 2. 检查消行
    int cleared = mBoard.clearLines();

    // 3. 根据消行数量播放对应音效（消 4 行 Tetris 琶音爆发！）
    switch (cleared) {
        case 1: playSfx(Sfx::Clear1); break;
        case 2: playSfx(Sfx::Clear2); break;
        case 3: playSfx(Sfx::Clear3); break;
        case 4: playSfx(Sfx::Clear4); break;   // 爽感拉满的大招音效
        default: playSfx(Sfx::Drop); break;    // 没消行就播沉闷的落地"咚"
    }

    // 4. 增加分数并计算等级提升
    if (cleared > 0) {
        mScore += lineScore(cleared, mLevel);
        mLines += cleared;

        int newLevel = 1 + mLines / 10;   // 每消除 10 行升一级
        if (newLevel > mLevel) {
            mLevel = newLevel;
            mDropInterval = dropIntervalFor(mLevel);
            playSfx(Sfx::SpeedUp);        // 升级三连音
        }
    }

    // 5. 换新方块：将 Next 方块交给 Current，并重新随机抽取一个新的 Next
    mCurrent = mNext;
    mNext    = Cube(Cube::randomType());

    // 6. 如果新方块放不进棋盘，说明方块堆积顶到天花板了，触发 Game Over
    return !mBoard.canPlace(mCurrent);
}

// ============================================================================
// 阶段 1：处理玩家键盘输入 (processInput)
//
// 【💡 优秀代码设计技巧：试探性更新 (Trial / Speculative Move)】
// 玩家按左键时，我们怎么知道能不能左移？
// 做法非常优雅：
//   Cube t = mCurrent;          // 先拷贝一个临时替身
//   t.move(-1, 0);              // 让替身试探性地往左挪一步
//   if (mBoard.canPlace(t)) {   // 请裁判看看替身的位置是否合法
//       mCurrent = t;           // 合法！才真正更新当前方块！
//   }
// 这样原方块永远不会因为撞墙而产生“穿模”或闪烁！
// ============================================================================
void Game::processInput() {
    // 非阻塞轮询：循环清空键盘输入队列中的所有按键
    while (mCon.hasKey()) {
        Key key = mCon.readKey();

        // 暂停状态下的按键拦截
        if (mPaused) {
            if (key == Key::P || key == Key::Escape) mPaused = false;
            else if (key == Key::Q) { mQuit = true; mRunning = false; }
            continue;
        }

        switch (key) {
            case Key::Escape:
            case Key::Q:
                // 退出游戏
                mQuit = true;
                mRunning = false;
                break;

            case Key::P:
                // 暂停游戏
                mPaused = true;
                break;

            case Key::A:
            case Key::Left: {
                // 左移试探
                Cube t = mCurrent;
                t.move(-1, 0);
                if (mBoard.canPlace(t)) mCurrent = t;
                break;
            }

            case Key::D:
            case Key::Right: {
                // 右移试探
                Cube t = mCurrent;
                t.move(1, 0);
                if (mBoard.canPlace(t)) mCurrent = t;
                break;
            }

            case Key::W:
            case Key::Up:
            case Key::J: {
                // 旋转试探
                Cube t = mCurrent;
                t.rotate();
                if (mBoard.canPlace(t)) {
                    mCurrent = t;
                    playSfx(Sfx::Rotate);   // 旋转成功才播放滴答声
                }
                break;
            }

            case Key::S:
            case Key::Down: {
                // 软降 (Soft Drop)：加速向下走一步，每次奖励 +1 分
                Cube t = mCurrent;
                t.move(0, 1);
                if (mBoard.canPlace(t)) {
                    mCurrent = t;
                    mScore += 1;
                }
                break;
            }

            case Key::Space: {
                // 硬降 (Hard Drop)：一键光速直达底部并立即锁定！
                int dist = 0;
                while (true) {
                    Cube t = mCurrent;
                    t.move(0, 1);
                    if (!mBoard.canPlace(t)) break; // 触底碰壁，停止下落
                    mCurrent = t;
                    dist++;
                }
                mScore += dist * 2; // 硬降额外奖励：每坠落一格奖励 +2 分
                mDropTimer = 0;
                if (lockAndAdvance()) mRunning = false;
                break;
            }

            default:
                break;
        }
    }
}

// ============================================================================
// 阶段 2：物理推进与时钟更新 (update)
//
// 【⏱️ 游戏心跳机制：时间累加器 (Timer Accumulator)】
// 为什么不能直接每次循环都下落？因为每帧才 20ms，一秒会下落 50 格，快得看不清！
// 正确做法：
// 每次主循环把经过的 20ms 累加到 mDropTimer 中。
// 只有当 mDropTimer 达到当前等级的阈值（如 500ms）时，才触发一次向下移动！
// ============================================================================
void Game::update() {
    mDropTimer += FRAME_MS;
    if (mDropTimer >= mDropInterval) {
        mDropTimer = 0; // 重置计时

        // 尝试下落一格
        Cube t = mCurrent;
        t.move(0, 1);
        if (mBoard.canPlace(t)) {
            mCurrent = t; // 能下落，继续下落
        } else if (lockAndAdvance()) {
            // 不能下落说明到底了 -> 固化并换下一块；如果出生点被堵死则游戏结束
            mRunning = false;
        }
    }
}

// ============================================================================
// 阶段 3：画面渲染 (render)
// 
// 【🎨 画家算法 (Painter's Algorithm)】
// 计算机图形学经典：由远及近、由底层到顶层一层层盖上去！
// 1. 清空画板 (frameClear)
// 2. 绘制棋盘底板与已锁定的方块 (Board::draw)
// 3. 绘制地面的虚线落点投影 (Board::drawGhost)
// 4. 绘制空中飘着的活动实心方块 (Cube::draw)
// 5. 绘制右侧 HUD 计分板与 Next 预览框 (Hud::draw)
// 6. 顶部打印调试信息
// 7. 将准备好的后台画布一次性推到屏幕上 (framePresent)
// ============================================================================
void Game::render() {
    mCon.frameClear();
    mBoard.draw(mCon, BOARD_PX, BOARD_PY);
    mBoard.drawGhost(mCon, mCurrent, BOARD_PX, BOARD_PY);
    mCurrent.draw(mCon, BOARD_PX, BOARD_PY);
    mHud.draw(mCon, mNext, mScore, mLevel, mLines);
    mCon.printfAt(BOARD_PX + 1, 1, Color::BrightCyan,
                 " Level %d   Speed %dms ", mLevel, mDropInterval);
    mCon.framePresent();
}

// 绘制暂停画面
void Game::renderPaused() {
    mCon.frameClear();
    mBoard.draw(mCon, BOARD_PX, BOARD_PY);
    mCurrent.draw(mCon, BOARD_PX, BOARD_PY);
    mHud.draw(mCon, mNext, mScore, mLevel, mLines);
    mCon.printfAt(BOARD_PX + 2, BOARD_PY + 10, Color::BrightYellow,
                 "  PAUSED  ");
    mCon.printfAt(BOARD_PX + 2, BOARD_PY + 12, Color::White,
                 " P: resume");
    mCon.framePresent();
}

// ============================================================================
// 游戏结束处理：死亡动画闪烁与询问是否再来一局
// ============================================================================
bool Game::showGameOver() {
    // 强制播放死亡终曲（避免被杂音打断）
    playSfx(Sfx::GameOver, true);

    // 红白闪烁 6 次，制造激烈的视觉死亡反馈
    for (int flash = 0; flash < 6; flash++) {
        mCon.frameClear();
        mBoard.draw(mCon, BOARD_PX, BOARD_PY);
        mHud.draw(mCon, mNext, mScore, mLevel, mLines);
        if (flash % 2 == 0) {
            mCon.printfAt(BOARD_PX + 1, BOARD_PY + 9, Color::BrightRed,
                         "  GAME  OVER!  ");
        }
        mCon.framePresent();
        mCon.sleep(200);
    }

    // 停留结算画面
    mCon.frameClear();
    mBoard.draw(mCon, BOARD_PX, BOARD_PY);
    mHud.draw(mCon, mNext, mScore, mLevel, mLines);
    mCon.printfAt(BOARD_PX + 1, BOARD_PY + 9,  Color::BrightRed,   "  GAME  OVER!  ");
    mCon.printfAt(BOARD_PX + 1, BOARD_PY + 11, Color::BrightYellow, " Score: %d", mScore);
    mCon.printfAt(6, 5, Color::BrightYellow, " Play again? (Y/N) ");
    mCon.framePresent();

    // 清除动画期间用户乱按的残留按键
    while (mCon.hasKey()) mCon.readKey();

    // 等待用户选择 Y 或 N
    while (true) {
        if (mCon.hasKey()) {
            Key key = mCon.readKey();
            char c = static_cast<char>(key);
            if (c == 'y' || c == 'Y') {
                return true;  // 再来一局！
            } else if (c == 'n' || c == 'N' || c == 'q' || c == 'Q') {
                return false; // 退出
            }
        }
        mCon.sleep(50);
    }
}

// ============================================================================
// 单局游戏主循环 (The Game Loop)
// ============================================================================
void Game::gameLoop() {
    while (mRunning) {
        processInput();      // 1. 收集输入
        if (!mRunning) break;

        if (mPaused) {
            renderPaused();  // 暂停状态挂起物理更新
            mCon.sleep(100);
            continue;
        }

        update();            // 2. 推进物理与重力
        if (!mRunning) break;

        render();            // 3. 双缓冲渲染
        mCon.sleep(FRAME_MS);// 4. 帧率控制 (20ms 延时)
    }
}

// ============================================================================
// 完整游戏生命周期驱动
// ============================================================================
void Game::run() {
    init(); // 1. 系统初始化

    bool playAgain = true;
    while (playAgain) {
        reset();            // 2. 开启新的一局
        gameLoop();         // 3. 运行游戏主循环

        stopMusic();        // 4. 死亡后关停音乐
        if (mQuit) {
            break;
        }

        playAgain = showGameOver(); // 5. 询问重开
    }

    stopMusic();
}

