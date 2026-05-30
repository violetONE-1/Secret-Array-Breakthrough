/**
 * ConsoleRenderer.cpp — ConsoleRenderer 的实现
 *
 * 核心功能：
 *   - 使用 SetConsoleCursorPosition 定位光标，逐格绘制 25×25 网格
 *   - 周围邻格高亮显示
 *   - 步数、计时、选手信息实时更新
 *   - 通过 _getch() 捕获方向键/回车/Esc
 *
 * 网格布局（以 5×5 为例）：
 *     0   1   2   3   4
 *   +---+---+---+---+---+
 * 0 |A1 |B2 |C3 | . |E5 |
 *   +---+---+---+---+---+
 * 1 |F6 | . |H8 |I9 |J0 |
 *   ...
 */

#include "view/ConsoleRenderer.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <conio.h>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <algorithm>

ConsoleRenderer::ConsoleRenderer()
    : _running(true)
{
    _hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    _hInput   = GetStdHandle(STD_INPUT_HANDLE);

    // 设置控制台为 UTF-8 代码页以支持中英文混排
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

ConsoleRenderer::~ConsoleRenderer()
{
    // 重置控制台颜色
    SetConsoleTextAttribute(_hConsole, 7);
}

// ---- 网格绘制 ----

void ConsoleRenderer::drawGrid(const GameState& state)
{
    const Grid& grid = state.grid();
    int rows = grid.rows();
    int cols = grid.cols();

    const auto& playerCells = state.playerCells();
    const auto& aiCells = state.aiCells();

    setColor(7);  // 白色

    // ---- 列标题 ----
    std::cout << "\n     ";
    for (int c = 0; c < cols; ++c) {
        std::cout << std::setw(3) << c;
    }
    std::cout << "\n    +";
    for (int c = 0; c < cols; ++c) {
        std::cout << "---+";
    }
    std::cout << "\n";

    // ---- 行数据 ----
    for (int r = 0; r < rows; ++r) {
        std::cout << std::setw(3) << r << " |";

        for (int c = 0; c < cols; ++c) {
            const Cell& cell = grid.at(r, c);
            bool isPlayer = playerCells.find({r, c}) != playerCells.end();
            bool isAI = aiCells.find({r, c}) != aiCells.end();

            if (cell.isEmpty()) {
                setColor(8);  // 灰色
                std::cout << " . ";
                setColor(7);
            } else if (cell.isSelected()) {
                setColor(14);  // 亮黄
                std::cout << "[" << cell.getLetter() << cell.getNumber() << "]";
                setColor(7);
            } else if (isPlayer) {
                setColor(14);  // 亮黄 — 玩家棋子
                std::cout << "[" << cell.getLetter() << cell.getNumber() << "]";
                setColor(7);
            } else if (isAI) {
                setColor(12);  // 亮红 — AI 棋子
                std::cout << "<" << cell.getLetter() << cell.getNumber() << ">";
                setColor(7);
            } else {
                std::cout << " " << cell.getLetter() << cell.getNumber();
            }
            std::cout << "|";
        }
        std::cout << " " << r;
        std::cout << "\n    +";
        for (int c = 0; c < cols; ++c) {
            std::cout << "---+";
        }
        std::cout << "\n";
    }

    // ---- 底部列号 ----
    std::cout << "     ";
    for (int c = 0; c < cols; ++c) {
        std::cout << std::setw(3) << c;
    }
    std::cout << "\n";
}

void ConsoleRenderer::drawGrid(const Grid& grid)
{
    int rows = grid.rows();
    int cols = grid.cols();

    setColor(7);

    std::cout << "\n     ";
    for (int c = 0; c < cols; ++c) {
        std::cout << std::setw(3) << c;
    }
    std::cout << "\n    +";
    for (int c = 0; c < cols; ++c) {
        std::cout << "---+";
    }
    std::cout << "\n";

    for (int r = 0; r < rows; ++r) {
        std::cout << std::setw(3) << r << " |";

        for (int c = 0; c < cols; ++c) {
            const Cell& cell = grid.at(r, c);

            if (cell.isSelected()) {
                setColor(14);
                std::cout << "[" << cell.getLetter() << cell.getNumber() << "]";
                setColor(7);
            } else if (cell.isEmpty()) {
                setColor(8);
                std::cout << " . ";
                setColor(7);
            } else {
                std::cout << " " << cell.getLetter() << cell.getNumber();
            }
            std::cout << "|";
        }
        std::cout << " " << r;
        std::cout << "\n    +";
        for (int c = 0; c < cols; ++c) {
            std::cout << "---+";
        }
        std::cout << "\n";
    }

    std::cout << "     ";
    for (int c = 0; c < cols; ++c) {
        std::cout << std::setw(3) << c;
    }
    std::cout << "\n";
}

// ---- IRenderer 实现 ----

void ConsoleRenderer::render(const GameState& state)
{
    clearScreen();
    const Grid& grid = state.grid();

    // 绘制游戏信息栏
    setColor(11);  // 亮青色
    std::cout << "============================================================\n";
    std::cout << "  Matrix Breakthrough — Puzzle: " << state.puzzleId() << "\n";
    std::cout << "  步数: " << state.stepsTaken()
              << "    用时: " << std::fixed << std::setprecision(1)
              << state.elapsedSeconds() << " 秒\n";
    if (!_turnMessage.empty()) {
        setColor(14);
        std::cout << "  >>> " << _turnMessage << " <<<\n";
        setColor(11);
    }
    std::cout << "  玩家棋子: " << state.playerCells().size()
              << "   AI棋子: " << state.aiCells().size() << "\n";
    std::cout << "============================================================\n";

    // 绘制网格
    drawGrid(state);

    // 操作提示
    setColor(8);  // 灰色
    std::cout << "\n  [操作] 方向键:移动光标 | 空格/回车:选择并合并\n";
    std::cout << "          S:提交 | Q:返回 | E:AI代走一步\n";
    setColor(7);
}

void ConsoleRenderer::showMenu()
{
    clearScreen();
    setColor(14);
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════╗\n";
    std::cout << "  ║       Matrix Breakthrough             ║\n";
    std::cout << "  ╠══════════════════════════════════════╣\n";
    std::cout << "  ║  1. 开始游戏 (选题)                    ║\n";
    std::cout << "  ║  2. 查看排行榜                         ║\n";
    std::cout << "  ║  3. VS AI 对战                        ║\n";
    std::cout << "  ║  4. 退出                               ║\n";
    std::cout << "  ╚══════════════════════════════════════╝\n";
    setColor(7);
    std::cout << "\n  请输入选项 (1-4): ";
}

void ConsoleRenderer::showPuzzleList(const std::vector<Puzzle>& puzzles)
{
    clearScreen();
    setColor(14);
    std::cout << "\n  ╔══════════════════════════════════════╗\n";
    std::cout << "  ║         选  择  题  面                ║\n";
    std::cout << "  ╠══════════════════════════════════════╣\n";
    setColor(7);

    for (size_t i = 0; i < puzzles.size(); ++i) {
        std::cout << "  ║  " << (i + 1) << ". " << std::setw(20) << std::left
                  << puzzles[i].name()
                  << " (" << puzzles[i].gridSize() << "x"
                  << puzzles[i].gridSize() << ")"
                  << std::setw(17) << std::right << "║\n";
    }

    std::cout << "  ║  " << (puzzles.size() + 1) << ". 返回主菜单"
              << std::setw(36) << std::right << "║\n";
    std::cout << "  ╚══════════════════════════════════════╝\n";
    std::cout << "\n  请选择题号：";
}

void ConsoleRenderer::showLeaderboard(const std::vector<ScoreRecord>& records)
{
    clearScreen();
    setColor(14);
    std::cout << "\n  ╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║                        排  行  榜                                ║\n";
    std::cout << "  ╠══════════════════════╦══════════╦════════╦══════╦══════╦══════╣\n";
    std::cout << "  ║ 玩家名               ║ 题面     ║ 用时(s)║ 步数 ║正确率║ 总分 ║\n";
    std::cout << "  ╠══════════════════════╬══════════╬════════╬══════╬══════╬══════╣\n";
    setColor(7);

    for (size_t i = 0; i < records.size() && i < 15; ++i) {
        const auto& r = records[i];
        std::cout << "  ║ " << std::setw(21) << std::left << r.playerName()
                  << "║ " << std::setw(9)  << r.puzzleId()
                  << "║ " << std::setw(7)  << std::fixed << std::setprecision(2) << r.timeSeconds()
                  << "║ " << std::setw(5)  << r.steps()
                  << "║ " << std::setw(5)  << std::fixed << std::setprecision(2) << r.accuracy()
                  << "║ " << std::setw(5)  << r.score()
                  << "║\n";
    }

    if (records.empty()) {
        std::cout << "  ║                        暂无记录                                  ║\n";
    }

    std::cout << "  ╚══════════════════════╩══════════╩════════╩══════╩══════╩══════╝\n";
    std::cout << "\n  按任意键返回...";
}

void ConsoleRenderer::showResult(const ScoreRecord& record,
                                  const std::vector<Move>& moveHistory)
{
    clearScreen();
    setColor(14);
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════╗\n";
    std::cout << "  ║         答  题  结  果                ║\n";
    std::cout << "  ╠══════════════════════════════════════╣\n";
    setColor(7);
    std::cout << "  ║  玩家名: " << std::setw(28) << std::left << record.playerName() << "║\n";
    std::cout << "  ║  题  面: " << std::setw(28) << record.puzzleId()   << "║\n";
    std::cout << "  ║  用  时: " << std::setw(24) << std::left
              << (std::to_string(static_cast<int>(record.timeSeconds())) + " 秒")  << "  ║\n";
    std::cout << "  ║  步  数: " << std::setw(28) << record.steps()      << "║\n";
    std::cout << "  ║  正确率: " << std::setw(26) << std::left
              << (std::to_string(static_cast<int>(record.accuracy() * 100)) + "%") << "  ║\n";
    std::cout << "  ║  总  分: " << std::setw(28) << record.score()      << "║\n";
    std::cout << "  ╚══════════════════════════════════════╝\n";

    // 操作历史
    if (!moveHistory.empty()) {
        std::cout << "\n  ╔══════════════════════════════════════╗\n";
        std::cout << "  ║         操  作  历  史                ║\n";
        std::cout << "  ╚══════════════════════════════════════╝\n";
        for (size_t i = 0; i < moveHistory.size(); ++i) {
            const auto& m = moveHistory[i];
            char srcCol = static_cast<char>('A' + m.srcCol);
            char dstCol = static_cast<char>('A' + m.dstCol);
            std::cout << "  " << std::setw(2) << (i + 1) << ". "
                      << srcCol << (m.srcRow + 1)
                      << " -> " << dstCol << (m.dstRow + 1)
                      << "  " << directionToString(m.direction)
                      << "  -> " << m.resultLetter << m.resultNumber << "\n";
        }
    }

    std::cout << "\n  按任意键继续...";
    _getch();
}

void ConsoleRenderer::showVSAIMenu()
{
    clearScreen();
    setColor(14);
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════╗\n";
    std::cout << "  ║        VS  AI  对  战                ║\n";
    std::cout << "  ╠══════════════════════════════════════╣\n";
    std::cout << "  ║  1. 普通模式 (AI 随机策略)             ║\n";
    std::cout << "  ║  2. 高级模式 (AI 贪心策略)             ║\n";
    std::cout << "  ║  3. 返回主菜单                         ║\n";
    std::cout << "  ╚══════════════════════════════════════╝\n";
    setColor(7);
    std::cout << "\n  请选择难度 (1-3): ";
}

void ConsoleRenderer::showVSResult(const ScoreRecord& playerRecord,
                                    const ScoreRecord& aiRecord,
                                    const std::string& winner)
{
    clearScreen();
    setColor(14);
    std::cout << "\n";
    std::cout << "  ╔════════════════════════════════════════════════════╗\n";
    std::cout << "  ║              VS  AI  对  战  结  果                ║\n";
    std::cout << "  ╠════════════════════════════════════════════════════╣\n";

    auto printLine = [](const std::string& label,
                         const std::string& pVal,
                         const std::string& aVal) {
        std::cout << "  ║  " << std::setw(14) << std::left << label
                  << std::setw(16) << pVal
                  << std::setw(16) << aVal << "║\n";
    };

    setColor(11);
    std::cout << "  ║  " << std::setw(14) << ""
              << std::setw(16) << "--- 玩家 ---"
              << std::setw(16) << "--- AI ---" << "  ║\n";
    setColor(7);

    printLine("玩家名:", playerRecord.playerName(), aiRecord.playerName());
    printLine("用时(秒):",
              std::to_string(static_cast<int>(playerRecord.timeSeconds())),
              std::to_string(static_cast<int>(aiRecord.timeSeconds())));
    printLine("步数:",
              std::to_string(playerRecord.steps()),
              std::to_string(aiRecord.steps()));
    {
        std::ostringstream pa, aa;
        pa << std::fixed << std::setprecision(0)
           << (playerRecord.accuracy() * 100) << "%";
        aa << std::fixed << std::setprecision(0)
           << (aiRecord.accuracy() * 100) << "%";
        printLine("正确率:", pa.str(), aa.str());
    }
    setColor(14);
    printLine("总分:",
              std::to_string(playerRecord.score()),
              std::to_string(aiRecord.score()));
    setColor(7);

    std::cout << "  ╠════════════════════════════════════════════════════╣\n";
    if (winner == "player") {
        setColor(10);
        std::cout << "  ║              >>>  你赢了!  <<<                       ║\n";
    } else if (winner == "ai") {
        setColor(12);
        std::cout << "  ║              >>>  AI 获胜!  <<<                       ║\n";
    } else {
        setColor(14);
        std::cout << "  ║              >>>  平  局!  <<<                        ║\n";
    }
    setColor(7);
    std::cout << "  ╚════════════════════════════════════════════════════╝\n";
    std::cout << "\n  按任意键返回主菜单...";
    _getch();
}

void ConsoleRenderer::showMessage(const std::string& msg)
{
    std::cout << "\n  " << msg << "\n";
    std::cout << "  按任意键继续...";
    _getch();
}

std::string ConsoleRenderer::promptPlayerName()
{
    clearScreen();
    std::cout << "\n  ╔══════════════════════════════════════╗\n";
    std::cout << "  ║         输 入 玩 家 名                ║\n";
    std::cout << "  ╚══════════════════════════════════════╝\n";
    std::cout << "\n  请输入您的名字: ";
    std::string name;
    std::getline(std::cin, name);
    if (name.empty()) name = "Player";
    return name;
}

std::vector<std::pair<int, int>> ConsoleRenderer::promptStartingCells(
    const Grid& grid, int count)
{
    std::vector<std::pair<int, int>> starts;
    clearScreen();
    setColor(14);
    std::cout << "\n  请选择 " << count << " 个起始格子\n";
    std::cout << "  使用方向键移动光标，空格选中/取消，确认后按回车\n\n";
    setColor(7);

    drawGrid(grid);

    int curRow = 0, curCol = 0;
    int rows = grid.rows(), cols = grid.cols();

    // 用于跟踪已选格子
    Grid tempGrid = grid;

    // 简单的光标导航循环
    while (true) {
        // 移动光标到当前位置
        gotoxy(5 + curCol * 4, 5 + curRow * 2);  // 位置根据 drawGrid 调整
        std::cout << "\r";  // 刷新行

        int ch = _getch();
        if (ch == 224) {  // 方向键前缀
            ch = _getch();
            switch (ch) {
                case 72: if (curRow > 0) curRow--; break;       // 上
                case 80: if (curRow < rows - 1) curRow++; break; // 下
                case 75: if (curCol > 0) curCol--; break;       // 左
                case 77: if (curCol < cols - 1) curCol++; break; // 右
            }
        } else if (ch == ' ') {
            // 空格：选中/取消选中
            auto it = std::find(starts.begin(), starts.end(),
                                std::make_pair(curRow, curCol));
            if (it != starts.end()) {
                starts.erase(it);
            } else if (static_cast<int>(starts.size()) < count) {
                starts.emplace_back(curRow, curCol);
            }
            // 重新绘制网格以反映选中状态
            clearScreen();
            drawGrid(tempGrid);
            std::cout << "\n  已选: ";
            for (const auto& s : starts) {
                std::cout << "(" << s.first << "," << s.second << ") ";
            }
        } else if (ch == 13) {
            // 回车确认
            if (static_cast<int>(starts.size()) == count) break;
        } else if (ch == 27) {
            // Esc 取消
            starts.clear();
            break;
        }
    }
    return starts;
}

// ---- 键盘输入处理 ----

UserAction ConsoleRenderer::waitForAction()
{
    int ch = _getch();
    return parseKey(ch);
}

UserAction ConsoleRenderer::parseKey(int ch)
{
    if (ch == 224) {  // 方向键前缀（Windows 扩展键）
        ch = _getch();
        switch (ch) {
            case 72: return UserAction::UP;
            case 80: return UserAction::DOWN;
            case 75: return UserAction::LEFT;
            case 77: return UserAction::RIGHT;
            default: return UserAction::NONE;
        }
    }

    switch (ch) {
        case 13:  return UserAction::CONFIRM;       // Enter
        case 27:  return UserAction::BACK;           // Esc
        case ' ': return UserAction::SELECT_CELL;    // 空格
        case 'w': case 'W': return UserAction::UP;
        case 's': case 'S': return UserAction::SUBMIT;
        case 'a': case 'A': return UserAction::LEFT;
        case 'd': case 'D': return UserAction::RIGHT;
        case 'q': case 'Q': return UserAction::QUIT;
        case '1': return UserAction::SELECT_PUZZLE_1;
        case '2': return UserAction::SELECT_PUZZLE_2;
        case '3': return UserAction::SELECT_PUZZLE_3;
        case '4': return UserAction::SELECT_PUZZLE_4;
        case '5': return UserAction::SELECT_PUZZLE_5;
        case 'l': case 'L': return UserAction::VIEW_LEADERBOARD;
        case 'e': case 'E': return UserAction::AI_EXECUTE;
        default:  return UserAction::NONE;
    }
}

bool ConsoleRenderer::isOpen() const { return _running; }

void ConsoleRenderer::setTurnMessage(const std::string& msg)
{
    _turnMessage = msg;
}

void ConsoleRenderer::clearScreen()
{
    system("cls");
}

// ---- 光标控制 ----

void ConsoleRenderer::gotoxy(int x, int y)
{
    COORD coord;
    coord.X = static_cast<SHORT>(x);
    coord.Y = static_cast<SHORT>(y);
    SetConsoleCursorPosition(_hConsole, coord);
}

void ConsoleRenderer::setColor(WORD color)
{
    SetConsoleTextAttribute(_hConsole, color);
}
