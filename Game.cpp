#include "Game.h"
#include "Sound.h"
#include <cstdlib>
#include <ctime>

Game::Game()
    : mCon(SCREEN_W, SCREEN_H),
      mCurrent(Cube::Type::I),
      mNext(Cube::Type::I) {
}

Game::~Game() {
    stopMusic();
}

int Game::dropIntervalFor(int level) {
    // 关键：让"下落速度（格/秒）"线性增长，而不是"间隔毫秒"线性递减。
    // 若直接线性减间隔，速度 = 1000/间隔 会越加越快，最后一级还断崖式飙升。
    // 这里：level 1 起 2 格/秒，每级 +0.4 格/秒，封顶 8 格/秒（125ms/格）。
    double speed = 2.0 + (level - 1) * 0.4;
    if (speed > 8.0) speed = 8.0;
    return static_cast<int>(1000.0 / speed);
}

int Game::lineScore(int lines, int level) {
    static const int base[5] = {0, 100, 300, 500, 800};
    return base[lines] * level;
}

void Game::init() {
    // ---------- 初始化控制台 ----------
    // init() 内部会强制 AllocConsole 弹出独立窗口，无需再手动 ensureOwnConsole
    mCon.init("Tetris - Arrows/WASD to play, Q to quit");
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}

void Game::reset() {
    // ---------- 每局重新重置/创建游戏对象与状态 ----------
    mBoard.reset();
    mCurrent = Cube(Cube::randomType());
    mNext    = Cube(Cube::randomType());

    mScore   = 0;
    mLines   = 0;
    mLevel   = 1;
    mRunning = true;
    mPaused  = false;
    mQuit    = false;

    // 开局就卡住 -> 直接结束
    if (!mBoard.canPlace(mCurrent)) {
        mRunning = false;
    }

    playSfx(Sfx::Start);
    startMusic();

    mDropTimer    = 0;
    mDropInterval = dropIntervalFor(mLevel);
}

bool Game::lockAndAdvance() {
    // ---------- 锁定流程：固化 → 消行 → 计分 → 取下一块 ----------
    // 返回 true 表示出生点被堵 -> 游戏结束
    mBoard.lock(mCurrent);

    int cleared = mBoard.clearLines();
    // 按消除行数播放不同音效；0 行（没消除）就是落地撞击
    switch (cleared) {
        case 1: playSfx(Sfx::Clear1); break;
        case 2: playSfx(Sfx::Clear2); break;
        case 3: playSfx(Sfx::Clear3); break;
        case 4: playSfx(Sfx::Clear4); break;   // Tetris！爽感拉满
        default: playSfx(Sfx::Drop); break;    // 落地"咚"
    }

    if (cleared > 0) {
        mScore += lineScore(cleared, mLevel);
        mLines += cleared;

        int newLevel = 1 + mLines / 10;   // 每 10 行升一级
        if (newLevel > mLevel) {
            mLevel = newLevel;
            mDropInterval = dropIntervalFor(mLevel);
            playSfx(Sfx::SpeedUp);
        }
    }

    mCurrent = mNext;
    mNext    = Cube(Cube::randomType());
    return !mBoard.canPlace(mCurrent);   // 新方块放不下 = 顶到天花板
}

void Game::processInput() {
    // ===== 处理输入 =====
    while (mCon.hasKey()) {
        Key key = mCon.readKey();
        if (mPaused) {
            if (key == Key::P || key == Key::Escape) mPaused = false;
            else if (key == Key::Q) { mQuit = true; mRunning = false; }
            continue;
        }

        switch (key) {
            case Key::Escape:
            case Key::Q:
                mQuit = true;
                mRunning = false;
                break;

            case Key::P:
                mPaused = true;
                break;

            case Key::A:
            case Key::Left: {
                Cube t = mCurrent;
                t.move(-1, 0);
                if (mBoard.canPlace(t)) mCurrent = t;
                break;
            }

            case Key::D:
            case Key::Right: {
                Cube t = mCurrent;
                t.move(1, 0);
                if (mBoard.canPlace(t)) mCurrent = t;
                break;
            }

            case Key::W:
            case Key::Up:
            case Key::J: {
                Cube t = mCurrent;
                t.rotate();
                if (mBoard.canPlace(t)) {
                    mCurrent = t;
                    playSfx(Sfx::Rotate);   // 旋转成功才响
                }
                break;   // 不能转就不转（简单墙踢省略）
            }

            case Key::S:
            case Key::Down: {
                Cube t = mCurrent;
                t.move(0, 1);
                if (mBoard.canPlace(t)) {
                    mCurrent = t;
                    mScore += 1;   // 软降奖励
                }
                break;
            }

            case Key::Space: {
                // 硬降：一路下落到底并立即锁定
                int dist = 0;
                while (true) {
                    Cube t = mCurrent;
                    t.move(0, 1);
                    if (!mBoard.canPlace(t)) break;
                    mCurrent = t;
                    dist++;
                }
                mScore += dist * 2;
                mDropTimer = 0;
                if (lockAndAdvance()) mRunning = false;
                break;
            }

            default:
                break;
        }
    }
}

void Game::update() {
    // ===== 重力：计时到了自动下落一格 =====
    mDropTimer += FRAME_MS;
    if (mDropTimer >= mDropInterval) {
        mDropTimer = 0;
        Cube t = mCurrent;
        t.move(0, 1);
        if (mBoard.canPlace(t)) {
            mCurrent = t;
        } else if (lockAndAdvance()) {
            // 落到底：锁定；出生点被堵则游戏结束
            mRunning = false;
        }
    }
}

void Game::render() {
    // ===== 渲染游戏画面 =====
    mCon.frameClear();
    mBoard.draw(mCon, BOARD_PX, BOARD_PY);
    mBoard.drawGhost(mCon, mCurrent, BOARD_PX, BOARD_PY);
    mCurrent.draw(mCon, BOARD_PX, BOARD_PY);
    mHud.draw(mCon, mNext, mScore, mLevel, mLines);
    mCon.printfAt(BOARD_PX + 1, 1, Color::BrightCyan,
                 " Level %d   Speed %dms ", mLevel, mDropInterval);
    mCon.framePresent();
}

void Game::renderPaused() {
    // ===== 渲染暂停画面 =====
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

bool Game::showGameOver() {
    // ---------- 死亡：闪烁 + 询问是否再来一局 ----------
    playSfx(Sfx::GameOver, true);   // 强制播放，别被落地音效吞掉
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

    mCon.frameClear();
    mBoard.draw(mCon, BOARD_PX, BOARD_PY);
    mHud.draw(mCon, mNext, mScore, mLevel, mLines);
    mCon.printfAt(BOARD_PX + 1, BOARD_PY + 9,  Color::BrightRed,   "  GAME  OVER!  ");
    mCon.printfAt(BOARD_PX + 1, BOARD_PY + 11, Color::BrightYellow,
                 " Score: %d", mScore);
    mCon.printfAt(6, 5, Color::BrightYellow, " Play again? (Y/N) ");
    mCon.framePresent();

    // 丢弃闪烁期间残留的按键
    while (mCon.hasKey()) mCon.readKey();

    // 等待 Y / N
    while (true) {
        if (mCon.hasKey()) {
            Key key = mCon.readKey();
            char c = static_cast<char>(key);
            if (c == 'y' || c == 'Y') {
                return true;
            } else if (c == 'n' || c == 'N' || c == 'q' || c == 'Q') {
                return false;
            }
        }
        mCon.sleep(50);
    }
}

void Game::gameLoop() {
    // ---------- 单局游戏主循环 ----------
    while (mRunning) {
        processInput();
        if (!mRunning) break;

        if (mPaused) {
            renderPaused();
            mCon.sleep(100);
            continue;
        }

        update();
        if (!mRunning) break;

        render();
        mCon.sleep(FRAME_MS);
    }
}

void Game::run() {
    init();

    bool playAgain = true;
    while (playAgain) {
        reset();
        gameLoop();

        stopMusic();
        if (mQuit) {
            break;
        }

        playAgain = showGameOver();
    }

    stopMusic();
}
