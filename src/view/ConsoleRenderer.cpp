/**
 * ConsoleRenderer.cpp -- Console renderer implementation
 *
 * Core features:
 *   - Uses SetConsoleCursorPosition for cursor positioning, draws 25x25 grid
 *   - Highlights neighboring cells
 *   - Real-time step counter, timer, piece count updates
 *   - Captures arrow keys / Enter / Esc via _getch()
 *
 * Grid layout (example with 5x5):
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
#include <clocale>
#include <io.h>
#include <fcntl.h>

ConsoleRenderer::ConsoleRenderer()
    : _running(true)
{
    _hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    _hInput   = GetStdHandle(STD_INPUT_HANDLE);

    // Set console to UTF-8 codepage
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Set C runtime locale to UTF-8
    std::setlocale(LC_ALL, ".UTF-8");

    // Set stdout to binary mode to prevent CRT codepage conversion of UTF-8
    _setmode(_fileno(stdout), _O_BINARY);
}

ConsoleRenderer::~ConsoleRenderer()
{
    // Reset console color
    SetConsoleTextAttribute(_hConsole, 7);
}

// ---- Grid rendering ----

void ConsoleRenderer::drawGrid(const GameState& state)
{
    const Grid& grid = state.grid();
    int rows = grid.rows();
    int cols = grid.cols();

    const auto& playerCells = state.playerCells();
    const auto& aiCells = state.aiCells();

    setColor(7);  // white

    // ---- Column headers ----
    std::cout << "\n     ";
    for (int c = 0; c < cols; ++c) {
        std::cout << std::setw(3) << c;
    }
    std::cout << "\n    +";
    for (int c = 0; c < cols; ++c) {
        std::cout << "---+";
    }
    std::cout << "\n";

    // ---- Row data ----
    for (int r = 0; r < rows; ++r) {
        std::cout << std::setw(3) << r << " |";

        for (int c = 0; c < cols; ++c) {
            const Cell& cell = grid.at(r, c);
            bool isPlayer = playerCells.find({r, c}) != playerCells.end();
            bool isAI = aiCells.find({r, c}) != aiCells.end();

            if (cell.isEmpty()) {
                setColor(8);  // grey
                std::cout << " . ";
                setColor(7);
            } else if (cell.isSelected()) {
                setColor(14);  // bright yellow
                std::cout << "[" << cell.getLetter() << cell.getNumber() << "]";
                setColor(7);
            } else if (isPlayer) {
                setColor(14);  // bright yellow -- player piece
                std::cout << "[" << cell.getLetter() << cell.getNumber() << "]";
                setColor(7);
            } else if (isAI) {
                setColor(12);  // bright red -- AI piece
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

    // ---- Bottom column labels ----
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

// ---- IRenderer implementation ----

void ConsoleRenderer::render(const GameState& state)
{
    clearScreen();
    const Grid& grid = state.grid();

    setColor(11);  // bright cyan
    std::cout << "============================================================\n";
    std::cout << "  Matrix Breakthrough -- Puzzle: " << state.puzzleId() << "\n";
    std::cout << "  Steps: " << state.stepsTaken()
              << "    Elapsed: " << std::fixed << std::setprecision(1)
              << state.elapsedSeconds() << " s\n";
    if (!_turnMessage.empty()) {
        setColor(14);
        std::cout << "  >>> " << _turnMessage << " <<<\n";
        setColor(11);
    }
    std::cout << "  Player pieces: " << state.playerCells().size()
              << "   AI pieces: " << state.aiCells().size() << "\n";
    std::cout << "============================================================\n";

    drawGrid(state);

    setColor(8);  // grey
    std::cout << "\n  [Controls] Arrows: move cursor | Space/Enter: merge\n";
    std::cout << "             S: Submit | Q: Quit | E: AI move\n";
    setColor(7);
}

void ConsoleRenderer::showMenu()
{
    clearScreen();
    setColor(14);
    std::cout << "\n";
    std::cout << "  +--------------------------------------+\n";
    std::cout << "  |       Matrix Breakthrough            |\n";
    std::cout << "  +--------------------------------------+\n";
    std::cout << "  |  1. Campaign Mode                    |\n";
    std::cout << "  |  2. Leaderboard                      |\n";
    std::cout << "  |  3. VS AI Battle                     |\n";
    std::cout << "  |  4. Exit                             |\n";
    std::cout << "  +--------------------------------------+\n";
    setColor(7);
    std::cout << "\n  Select option (1-4): ";
}

void ConsoleRenderer::showPuzzleList(const std::vector<Puzzle>& puzzles)
{
    clearScreen();
    setColor(14);
    std::cout << "\n";
    std::cout << "  +--------------------------------------+\n";
    std::cout << "  |         Select Puzzle                |\n";
    std::cout << "  +--------------------------------------+\n";
    setColor(7);

    for (size_t i = 0; i < puzzles.size(); ++i) {
        std::ostringstream line;
        line << (i + 1) << ". " << puzzles[i].name()
             << " (" << puzzles[i].gridSize() << "x"
             << puzzles[i].gridSize() << ")";
        std::cout << "  |  " << std::setw(34) << std::left << line.str() << "|\n";
    }

    std::cout << "  |  " << std::setw(34) << std::left
              << (std::to_string(puzzles.size() + 1) + ". Back to Menu") << "|\n";
    std::cout << "  +--------------------------------------+\n";
    std::cout << "\n  Select puzzle number: ";
}

void ConsoleRenderer::showLevelList(const std::vector<Puzzle>& puzzles,
                                     int maxUnlocked,
                                     const std::vector<int>& bestScores)
{
    clearScreen();
    setColor(14);
    std::cout << "\n";
    std::cout << "  +--------------------------------------+\n";
    std::cout << "  |          Campaign Mode               |\n";
    std::cout << "  +--------------------------------------+\n";
    setColor(7);

    for (int i = 0; i < static_cast<int>(puzzles.size()); ++i) {
        int levelNum = i + 1;
        bool unlocked = levelNum <= maxUnlocked;
        bool cleared = (bestScores[i] > 0);

        std::string status;
        int statusColor = 8;
        if (!unlocked) {
            status = "LOCKED";
            statusColor = 8;
        } else if (cleared) {
            status = "CLEAR";
            statusColor = 10;
        } else {
            status = "NEW";
            statusColor = 14;
        }

        std::ostringstream line;
        line << (i + 1) << ". " << puzzles[i].name();
        std::cout << "  |  " << std::setw(34) << std::left << line.str() << "|\n";

        std::cout << "  |      " << puzzles[i].gridSize() << "x" << puzzles[i].gridSize()
                  << "  [";
        setColor(statusColor);
        std::cout << status;
        setColor(7);
        std::cout << "]";
        std::ostringstream scoreStr;
        if (cleared)
            scoreStr << "  Best: " << bestScores[i];
        else
            scoreStr << "  Best: -";
        std::cout << std::setw(20) << std::left << scoreStr.str() << "|\n";
    }

    std::cout << "  +--------------------------------------+\n";
    std::cout << "  |  " << std::setw(34) << std::left
              << (std::to_string(puzzles.size() + 1) + ". Back to Menu") << "|\n";
    std::cout << "  +--------------------------------------+\n";
    std::cout << "\n  Select level (1-" << puzzles.size() << "): ";
}

void ConsoleRenderer::showLeaderboard(const std::vector<ScoreRecord>& records)
{
    clearScreen();
    setColor(14);
    std::cout << "\n";
    std::cout << "  +--------------------------------------------------+\n";
    std::cout << "  |               Leaderboard                        |\n";
    std::cout << "  +--------------------------------------------------+\n";
    std::cout << "  | #  Player             Time(s)  Steps  Acc   Score|\n";
    std::cout << "  +--------------------------------------------------+\n";
    setColor(7);

    for (size_t i = 0; i < records.size() && i < 15; ++i) {
        const auto& r = records[i];
        std::cout << "  | " << std::setw(2) << std::left << (i + 1)
                  << std::setw(20) << std::left << r.playerName().substr(0, 19)
                  << std::setw(9) << std::fixed << std::setprecision(2) << r.timeSeconds()
                  << std::setw(7) << r.steps()
                  << std::setw(6) << std::fixed << std::setprecision(2) << r.accuracy()
                  << std::setw(6) << r.score() << "|\n";
    }

    if (records.empty()) {
        std::cout << "  |                  No records yet                  |\n";
    }

    std::cout << "  +--------------------------------------------------+\n";
    std::cout << "\n  Press any key to return...";
}

void ConsoleRenderer::showResult(const ScoreRecord& record,
                                  const std::vector<Move>& moveHistory)
{
    clearScreen();
    setColor(14);
    std::cout << "\n";
    std::cout << "  +--------------------------------------+\n";
    std::cout << "  |             Results                  |\n";
    std::cout << "  +--------------------------------------+\n";
    setColor(7);
    std::cout << "  |  Player:   " << std::setw(26) << std::left << record.playerName() << "|\n";
    std::cout << "  |  Puzzle:   " << std::setw(26) << record.puzzleId()   << "|\n";
    std::cout << "  |  Time:     " << std::setw(26) << std::left
              << (std::to_string(static_cast<int>(record.timeSeconds())) + " s") << "|\n";
    std::cout << "  |  Steps:    " << std::setw(26) << record.steps()      << "|\n";
    std::cout << "  |  Accuracy: " << std::setw(26) << std::left
              << (std::to_string(static_cast<int>(record.accuracy() * 100)) + "%") << "|\n";
    std::cout << "  |  Score:    " << std::setw(26) << record.score()      << "|\n";
    std::cout << "  +--------------------------------------+\n";

    if (!moveHistory.empty()) {
        std::cout << "\n  +--------------------------------------+\n";
        std::cout << "  |          Move History                |\n";
        std::cout << "  +--------------------------------------+\n";
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

    std::cout << "\n  Press any key to continue...";
    _getch();
}

void ConsoleRenderer::showVSAIMenu()
{
    clearScreen();
    setColor(14);
    std::cout << "\n";
    std::cout << "  +--------------------------------------+\n";
    std::cout << "  |          VS AI Battle                |\n";
    std::cout << "  +--------------------------------------+\n";
    std::cout << "  |  1. Normal Mode (AI Random)          |\n";
    std::cout << "  |  2. Hard Mode (AI Greedy)            |\n";
    std::cout << "  |  3. Back to Menu                     |\n";
    std::cout << "  +--------------------------------------+\n";
    setColor(7);
    std::cout << "\n  Select difficulty (1-3): ";
}

void ConsoleRenderer::showVSResult(const ScoreRecord& playerRecord,
                                    const ScoreRecord& aiRecord,
                                    const std::string& winner)
{
    clearScreen();
    setColor(14);
    std::cout << "\n";
    std::cout << "  +--------------------------------------------------+\n";
    std::cout << "  |              VS AI Results                       |\n";
    std::cout << "  +--------------------------------------------------+\n";

    auto printLine = [](const std::string& label,
                         const std::string& pVal,
                         const std::string& aVal) {
        std::cout << "  |  " << std::setw(14) << std::left << label
                  << std::setw(16) << pVal
                  << std::setw(16) << aVal << "|\n";
    };

    setColor(11);
    std::cout << "  |  " << std::setw(14) << ""
              << std::setw(16) << "--- Player ---"
              << std::setw(16) << "--- AI ---" << "|\n";
    setColor(7);

    printLine("Player:", playerRecord.playerName(), aiRecord.playerName());
    printLine("Time(s):",
              std::to_string(static_cast<int>(playerRecord.timeSeconds())),
              std::to_string(static_cast<int>(aiRecord.timeSeconds())));
    printLine("Steps:",
              std::to_string(playerRecord.steps()),
              std::to_string(aiRecord.steps()));
    {
        std::ostringstream pa, aa;
        pa << std::fixed << std::setprecision(0)
           << (playerRecord.accuracy() * 100) << "%";
        aa << std::fixed << std::setprecision(0)
           << (aiRecord.accuracy() * 100) << "%";
        printLine("Accuracy:", pa.str(), aa.str());
    }
    setColor(14);
    printLine("Score:",
              std::to_string(playerRecord.score()),
              std::to_string(aiRecord.score()));
    setColor(7);

    std::cout << "  +--------------------------------------------------+\n";
    if (winner == "player") {
        setColor(10);
        std::cout << "  |              >>>  You Win!  <<<                  |\n";
    } else if (winner == "ai") {
        setColor(12);
        std::cout << "  |              >>>  AI Wins!  <<<                  |\n";
    } else {
        setColor(14);
        std::cout << "  |              >>>  Draw!  <<<                     |\n";
    }
    setColor(7);
    std::cout << "  +--------------------------------------------------+\n";
    std::cout << "\n  Press any key to return to menu...";
    _getch();
}

void ConsoleRenderer::showMessage(const std::string& msg)
{
    std::cout << "\n  " << msg << "\n";
    std::cout << "  Press any key to continue...";
    _getch();
}

std::string ConsoleRenderer::promptPlayerName()
{
    clearScreen();
    std::cout << "\n";
    std::cout << "  +--------------------------------------+\n";
    std::cout << "  |          Enter Name                  |\n";
    std::cout << "  +--------------------------------------+\n";
    std::cout << "\n  Enter your name: ";
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
    std::cout << "\n  Select " << count << " starting cells\n";
    std::cout << "  Arrows: move | Space: select/deselect | Enter: confirm\n\n";
    setColor(7);

    drawGrid(grid);

    int curRow = 0, curCol = 0;
    int rows = grid.rows(), cols = grid.cols();

    // Track selected cells
    Grid tempGrid = grid;

    // Cursor navigation loop
    while (true) {
        // Move cursor to current position
        gotoxy(5 + curCol * 4, 5 + curRow * 2);  // position matching drawGrid layout
        std::cout << "\r";  // refresh line

        int ch = _getch();
        if (ch == 224) {  // arrow key prefix
            ch = _getch();
            switch (ch) {
                case 72: if (curRow > 0) curRow--; break;       // up
                case 80: if (curRow < rows - 1) curRow++; break; // down
                case 75: if (curCol > 0) curCol--; break;       // left
                case 77: if (curCol < cols - 1) curCol++; break; // right
            }
        } else if (ch == ' ') {
            // Space: select/deselect
            auto it = std::find(starts.begin(), starts.end(),
                                std::make_pair(curRow, curCol));
            if (it != starts.end()) {
                starts.erase(it);
            } else if (static_cast<int>(starts.size()) < count) {
                starts.emplace_back(curRow, curCol);
            }
            // Redraw the grid to reflect selection
            clearScreen();
            drawGrid(tempGrid);
            std::cout << "\n  Selected: ";
            for (const auto& s : starts) {
                std::cout << "(" << s.first << "," << s.second << ") ";
            }
        } else if (ch == 13) {
            // Enter: confirm
            if (static_cast<int>(starts.size()) == count) break;
        } else if (ch == 27) {
            // Esc: cancel
            starts.clear();
            break;
        }
    }
    return starts;
}

// ---- Keyboard input handling ----

UserAction ConsoleRenderer::waitForAction()
{
    int ch = _getch();
    return parseKey(ch);
}

UserAction ConsoleRenderer::parseKey(int ch)
{
    if (ch == 224) {  // arrow key prefix (Windows extended key)
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
        case ' ': return UserAction::SELECT_CELL;    // Space
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

// ---- Cursor control ----

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
