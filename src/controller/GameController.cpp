/**
 * GameController.cpp — GameController 的实现
 *
 * 游戏主控制器，编排游戏生命周期。
 * 所有 Model 层对象通过 unique_ptr 独占管理，析构时自动级联释放。
 */

#include "controller/GameController.hpp"
#include "view/ConsoleRenderer.hpp"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

// ---- 构造 / 析构 ----

GameController::GameController()
    : _phase(GamePhase::MENU)
    , _running(true)
    , _cursorRow(0)
    , _cursorCol(0)
    , _gridRows(25)
    , _gridCols(25)
{
    // 初始化数据层（按依赖顺序）
    _fileManager = std::make_unique<FileManager>("data");

    // 确保目录结构存在
    _fileManager->createDir("puzzles/custom");
    _fileManager->createDir("replays");

    // 题库
    _puzzleManager = std::make_unique<PuzzleManager>(*_fileManager);
    _puzzleManager->loadAll();

    // 排行榜
    _leaderboard = std::make_unique<Leaderboard>(*_fileManager, "leaderboard.txt");
    _leaderboard->load();

    // 回放
    _replayBuffer = std::make_unique<ReplayBuffer>(*_fileManager);

    // 渲染器（当前使用终端渲染器）
    _renderer = std::make_unique<ConsoleRenderer>();

    // 输入处理器
    _inputHandler = std::make_unique<InputHandler>();
}

GameController::~GameController() = default;

// ---- 主循环 ----

void GameController::run()
{
    while (_running) {
        switch (_phase) {
            case GamePhase::MENU:
                handleMenu();
                break;

            case GamePhase::PUZZLE_SELECT:
                handlePuzzleSelect();
                break;

            case GamePhase::GAMEPLAY:
                handleGameplay();
                break;

            case GamePhase::RESULT:
                handleResult();
                break;

            case GamePhase::LEADERBOARD:
                handleLeaderboard();
                break;

            case GamePhase::QUIT:
                _running = false;
                break;

            default:
                _phase = GamePhase::MENU;
                break;
        }
    }
}

// ================================================================
//  菜单阶段
// ================================================================

void GameController::handleMenu()
{
    _renderer->showMenu();

    UserAction action = _renderer->waitForAction();

    switch (action) {
        case UserAction::SELECT_PUZZLE_1:
        case UserAction::CONFIRM:
            _phase = GamePhase::PUZZLE_SELECT;
            break;

        case UserAction::VIEW_LEADERBOARD:
            _phase = GamePhase::LEADERBOARD;
            break;

        case UserAction::QUIT:
        case UserAction::SELECT_PUZZLE_3:
            _phase = GamePhase::QUIT;
            break;

        default:
            break;
    }
}

// ================================================================
//  题面选择阶段
// ================================================================

void GameController::handlePuzzleSelect()
{
    const auto& puzzles = _puzzleManager->all();

    if (puzzles.empty()) {
        _renderer->showMessage("错误：没有可用的题面！请检查 data/puzzles/ 目录。");
        _phase = GamePhase::MENU;
        return;
    }

    _renderer->showPuzzleList(puzzles);

    UserAction action = _renderer->waitForAction();

    int selectedIndex = -1;
    switch (action) {
        case UserAction::SELECT_PUZZLE_1: selectedIndex = 0; break;
        case UserAction::SELECT_PUZZLE_2: selectedIndex = 1; break;
        case UserAction::SELECT_PUZZLE_3: selectedIndex = 2; break;
        case UserAction::SELECT_PUZZLE_4: selectedIndex = 3; break;
        case UserAction::SELECT_PUZZLE_5: selectedIndex = 4; break;
        case UserAction::BACK:
        case UserAction::QUIT:
            _phase = GamePhase::MENU;
            return;
        default:
            return;
    }

    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(puzzles.size())) {
        // 获取玩家名称
        _player.setName(_renderer->promptPlayerName());

        // 开始游戏
        startNewGame(puzzles[selectedIndex]);
    }
}

// ================================================================
//  游戏进行阶段
// ================================================================

void GameController::startNewGame(const Puzzle& puzzle)
{
    const Grid& initialGrid = puzzle.initialGrid();
    _gridRows = initialGrid.rows();
    _gridCols = initialGrid.cols();
    _cursorRow = _gridRows / 2;
    _cursorCol = _gridCols / 2;

    // 默认起点（5 个固定坐标）
    std::vector<std::pair<int, int>> starts = {
        {0, 0}, {_gridRows/2, _gridCols/2},
        {0, _gridCols-1}, {_gridRows-1, 0},
        {_gridRows-1, _gridCols-1}
    };

    _state = std::make_unique<GameState>(initialGrid, starts, puzzle.id());

    // 开始录制回放
    std::ostringstream startsStr;
    for (size_t i = 0; i < starts.size(); ++i) {
        if (i > 0) startsStr << ' ';
        startsStr << starts[i].first << ',' << starts[i].second;
    }
    _replayBuffer->startRecording(puzzle.id(),
                                  _gridRows, _gridCols,
                                  _player.name(),
                                  startsStr.str());

    _phase = GamePhase::GAMEPLAY;
}

void GameController::handleGameplay()
{
    if (!_state) {
        _phase = GamePhase::MENU;
        return;
    }

    // 渲染当前状态
    _state->grid().at(_cursorRow, _cursorCol).setSelected(true);
    _renderer->render(*_state);
    _state->grid().at(_cursorRow, _cursorCol).setSelected(false);

    // 检查是否有合法操作
    if (!_state->grid().hasAnyValidMove() && _state->stepsTaken() > 0) {
        // 死局，自动提交
        submitAnswer();
        return;
    }

    // 等待输入
    UserAction action = _renderer->waitForAction();

    switch (action) {
        case UserAction::UP:
            if (_cursorRow > 0) _cursorRow--;
            break;

        case UserAction::DOWN:
            if (_cursorRow < _gridRows - 1) _cursorRow++;
            break;

        case UserAction::LEFT:
            if (_cursorCol > 0) _cursorCol--;
            break;

        case UserAction::RIGHT:
            if (_cursorCol < _gridCols - 1) _cursorCol++;
            break;

        case UserAction::SELECT_CELL:
            // 空格：检查光标格与周围邻格是否可以合并
            {
                Grid& grid = _state->grid();
                Cell& center = grid.at(_cursorRow, _cursorCol);
                if (center.isEmpty()) break;  // 不能合并空格

                // 尝试四个方向找到第一个可合并的邻格并执行合并
                bool merged = false;
                int check[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
                for (int d = 0; d < 4 && !merged; ++d) {
                    int nr = _cursorRow + check[d][0];
                    int nc = _cursorCol + check[d][1];
                    if (nr >= 0 && nr < _gridRows && nc >= 0 && nc < _gridCols) {
                        if (tryMerge(nr, nc, _cursorRow, _cursorCol)) {
                            merged = true;
                        }
                    }
                }
            }
            break;

        case UserAction::CONFIRM:
            // Enter: 与光标格上的空格操作相同，尝试合并
            {
                Grid& gr = _state->grid();
                const Cell& center = gr.at(_cursorRow, _cursorCol);
                if (!center.isEmpty()) {
                    bool merged = false;
                    int check[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
                    for (int d = 0; d < 4 && !merged; ++d) {
                        int nr = _cursorRow + check[d][0];
                        int nc = _cursorCol + check[d][1];
                        if (nr >= 0 && nr < _gridRows && nc >= 0 && nc < _gridCols) {
                            if (tryMerge(nr, nc, _cursorRow, _cursorCol)) {
                                merged = true;
                            }
                        }
                    }
                }
            }
            break;

        case UserAction::SUBMIT:
            submitAnswer();
            return;

        case UserAction::QUIT:
            // 返回菜单
            _state.reset();
            _phase = GamePhase::MENU;
            return;

        default:
            break;
    }
}

bool GameController::tryMerge(int srcRow, int srcCol, int dstRow, int dstCol)
{
    if (!_state) return false;

    Grid& grid = _state->grid();
    const Cell& center   = grid.at(dstRow, dstCol);
    const Cell& neighbor = grid.at(srcRow, srcCol);

    // 校验合并条件
    if (!MergeRule::canMerge(center, neighbor)) return false;

    // 计算合并结果
    Cell result = MergeRule::getMergedCell(center, neighbor);

    // 确定方向
    Direction dir;
    if (srcRow < dstRow)      dir = Direction::DOWN;
    else if (srcRow > dstRow) dir = Direction::UP;
    else if (srcCol < dstCol) dir = Direction::RIGHT;
    else                      dir = Direction::LEFT;

    // 执行合并
    grid.mergeCells(srcRow, srcCol, dstRow, dstCol, result);

    // 记录操作
    Move move(srcRow, srcCol, dstRow, dstCol, dir,
              result.getLetter(), result.getNumber());
    _state->recordMove(move);
    _replayBuffer->record(move);
    _cursorRow = dstRow;
    _cursorCol = dstCol;

    return true;
}

// ================================================================
//  答题结果阶段
// ================================================================

void GameController::submitAnswer()
{
    if (!_state) return;

    // 停止录制回放
    _replayBuffer->stopRecording();

    // 生成得分记录
    auto now = std::time(nullptr);
    std::ostringstream timestamp;
    timestamp << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");

    ScoreRecord record(
        _player.name(),
        _state->puzzleId(),
        _state->elapsedSeconds(),
        _state->stepsTaken(),
        _state->accuracy(),
        timestamp.str()
    );

    // 更新排行榜
    _leaderboard->add(record);

    // 保存回放文件
    std::ostringstream replayPath;
    replayPath << "replays/replay_"
               << std::put_time(std::localtime(&now), "%Y%m%d_%H%M%S")
               << ".txt";
    _replayBuffer->saveToFile(replayPath.str());

    // 显示结果
    _renderer->showResult(record);
    _renderer->waitForAction();  // 等待用户按键

    _state.reset();
    _phase = GamePhase::MENU;
}

// ================================================================
//  结果展示阶段
// ================================================================

void GameController::handleResult()
{
    // 结果展示在 submitAnswer() 中已处理，切换回菜单
    _phase = GamePhase::MENU;
}

// ================================================================
//  排行榜阶段
// ================================================================

void GameController::handleLeaderboard()
{
    auto records = _leaderboard->topByScore(15);  // 取前 15 名

    _renderer->showLeaderboard(records);
    _renderer->waitForAction();  // 等待用户按键

    _phase = GamePhase::MENU;
}
