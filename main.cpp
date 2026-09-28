#include "Console.h"
#include "Cube.h"
#include "Board.h"
#include "Hud.h"
#include "Sound.h"
#include "GameConfig.h"
#include <cstdlib>
#include <ctime>

// ========== 帧切片 ==========
#define FRAME_MS      20            // 主循环每帧 20ms，保证按键响应

// ========== 下落间隔（随等级平滑加快） ==========
// 关键：让"下落速度（格/秒）"线性增长，而不是"间隔毫秒"线性递减。
// 若直接线性减间隔，速度 = 1000/间隔 会越加越快，最后一级还断崖式飙升。
// 这里：level 1 起 2 格/秒，每级 +0.4 格/秒，封顶 8 格/秒（125ms/格）。
static int dropIntervalFor(int level) {
    double speed = 2.0 + (level - 1) * 0.4;
    if (speed > 8.0) speed = 8.0;
    return static_cast<int>(1000.0 / speed);
}

// ========== 计分 ==========
static int lineScore(int lines, int level) {
    static const int base[5] = {0, 100, 300, 500, 800};
    return base[lines] * level;
}

int main(int argc, char* argv[]) {
    // ---------- 初始化控制台 ----------
    Console con(SCREEN_W, SCREEN_H);
    // init() 内部会强制 AllocConsole 弹出独立窗口，无需再手动 ensureOwnConsole
    con.init("Tetris - Arrows/WASD to play, Q to quit");

    srand(static_cast<unsigned>(time(nullptr)));

    Hud hud;            // 界面外壳（信息面板 + Next 预览框）
    bool playAgain = true;

    // ========== 一局游戏（Y 重开则循环） ==========
    while (playAgain) {
        // ---------- 每局重新创建游戏对象 ----------
        Board board;
        Cube  current(Cube::randomType());
        Cube  next(Cube::randomType());

        int  score    = 0;
        int  lines    = 0;
        int  level    = 1;
        bool running  = true;
        bool paused   = false;
        bool quit     = false;

        // 开局就卡住 -> 直接结束
        if (!board.canPlace(current)) running = false;

        playSfx(Sfx::Start);
        startMusic();

        int  dropTimer = 0;         // 重力计时（毫秒）
        int  dropInterval = dropIntervalFor(level);

        // ---------- 锁定流程：固化 → 消行 → 计分 → 取下一块 ----------
        // 返回 true 表示出生点被堵 -> 游戏结束
        auto lockAndAdvance = [&]() -> bool {
            board.lock(current);

            int cleared = board.clearLines();
            // 按消除行数播放不同音效；0 行（没消除）就是落地撞击
            switch (cleared) {
                case 1: playSfx(Sfx::Clear1); break;
                case 2: playSfx(Sfx::Clear2); break;
                case 3: playSfx(Sfx::Clear3); break;
                case 4: playSfx(Sfx::Clear4); break;   // Tetris！爽感拉满
                default: playSfx(Sfx::Drop); break;    // 落地"咚"
            }

            if (cleared > 0) {
                score += lineScore(cleared, level);
                lines += cleared;

                int newLevel = 1 + lines / 10;   // 每 10 行升一级
                if (newLevel > level) {
                    level = newLevel;
                    dropInterval = dropIntervalFor(level);
                    playSfx(Sfx::SpeedUp);
                }
            }

            current = next;
            next    = Cube(Cube::randomType());
            return !board.canPlace(current);   // 新方块放不下 = 顶到天花板
        };

        // ---------- 游戏主循环 ----------
        while (running) {
            // ===== 1. 处理输入 =====
            while (con.hasKey()) {
                Key key = con.readKey();
                if (paused) {
                    if (key == Key::P || key == Key::Escape) paused = false;
                    else if (key == Key::Q) { quit = true; running = false; }
                    continue;
                }

                switch (key) {
                    case Key::Escape:
                    case Key::Q:
                        quit = true;
                        running = false;
                        break;

                    case Key::P:
                        paused = true;
                        break;

                    case Key::A:
                    case Key::Left: {
                        Cube t = current;
                        t.move(-1, 0);
                        if (board.canPlace(t)) current = t;
                        break;
                    }

                    case Key::D:
                    case Key::Right: {
                        Cube t = current;
                        t.move(1, 0);
                        if (board.canPlace(t)) current = t;
                        break;
                    }

                    case Key::W:
                    case Key::Up:
                    case Key::J: {
                        Cube t = current;
                        t.rotate();
                        if (board.canPlace(t)) {
                            current = t;
                            playSfx(Sfx::Rotate);   // 旋转成功才响
                        }
                        break;   // 不能转就不转（简单墙踢省略）
                    }

                    case Key::S:
                    case Key::Down: {
                        Cube t = current;
                        t.move(0, 1);
                        if (board.canPlace(t)) {
                            current = t;
                            score += 1;   // 软降奖励
                        }
                        break;
                    }

                    case Key::Space: {
                        // 硬降：一路下落到底并立即锁定
                        int dist = 0;
                        while (true) {
                            Cube t = current;
                            t.move(0, 1);
                            if (!board.canPlace(t)) break;
                            current = t;
                            dist++;
                        }
                        score += dist * 2;
                        dropTimer = 0;
                        if (lockAndAdvance()) running = false;
                        break;
                    }

                    default:
                        break;
                }
            }

            if (!running) break;

            // ===== 2. 暂停画面 =====
            if (paused) {
                con.frameClear();
                board.draw(con, BOARD_PX, BOARD_PY);
                current.draw(con, BOARD_PX, BOARD_PY);
                hud.draw(con, next, score, level, lines);
                con.printfAt(BOARD_PX + 2, BOARD_PY + 10, Color::BrightYellow,
                             "  PAUSED  ");
                con.printfAt(BOARD_PX + 2, BOARD_PY + 12, Color::White,
                             " P: resume");
                con.framePresent();
                con.sleep(100);
                continue;
            }

            // ===== 3. 重力：计时到了自动下落一格 =====
            dropTimer += FRAME_MS;
            if (dropTimer >= dropInterval) {
                dropTimer = 0;
                Cube t = current;
                t.move(0, 1);
                if (board.canPlace(t)) {
                    current = t;
                } else if (lockAndAdvance()) {
                    // 落到底：锁定；出生点被堵则游戏结束
                    running = false;
                    break;
                }
            }

            // ===== 4. 渲染帧 =====
            con.frameClear();
            board.draw(con, BOARD_PX, BOARD_PY);
            board.drawGhost(con, current, BOARD_PX, BOARD_PY);
            current.draw(con, BOARD_PX, BOARD_PY);
            hud.draw(con, next, score, level, lines);
            con.printfAt(BOARD_PX + 1, 1, Color::BrightCyan,
                         " Level %d   Speed %dms ", level, dropInterval);
            con.framePresent();

            // ===== 5. 帧率控制 =====
            con.sleep(FRAME_MS);
        }

        stopMusic();
        if (quit) break;

        // ---------- 死亡：闪烁 + 询问是否再来一局 ----------
        playSfx(Sfx::GameOver, true);   // 强制播放，别被落地音效吞掉
        for (int flash = 0; flash < 6; flash++) {
            con.frameClear();
            board.draw(con, BOARD_PX, BOARD_PY);
            hud.draw(con, next, score, level, lines);
            if (flash % 2 == 0) {
                con.printfAt(BOARD_PX + 1, BOARD_PY + 9, Color::BrightRed,
                             "  GAME  OVER!  ");
            }
            con.framePresent();
            con.sleep(200);
        }

        con.frameClear();
        board.draw(con, BOARD_PX, BOARD_PY);
        hud.draw(con, next, score, level, lines);
        con.printfAt(BOARD_PX + 1, BOARD_PY + 9,  Color::BrightRed,   "  GAME  OVER!  ");
        con.printfAt(BOARD_PX + 1, BOARD_PY + 11, Color::BrightYellow,
                     " Score: %d", score);
        con.printfAt(6, 5, Color::BrightYellow, " Play again? (Y/N) ");
        con.framePresent();

        // 丢弃闪烁期间残留的按键
        while (con.hasKey()) con.readKey();

        // 等待 Y / N
        bool answered = false;
        while (!answered) {
            if (con.hasKey()) {
                Key key = con.readKey();
                char c = (char)key;
                if (c == 'y' || c == 'Y') {
                    playAgain = true;
                    answered = true;
                } else if (c == 'n' || c == 'N' || c == 'q' || c == 'Q') {
                    playAgain = false;
                    answered = true;
                }
            }
            con.sleep(50);
        }
    }

    stopMusic();
    return 0;
}
