/**
 * GameController.cpp — GameController 的实现
 *
 * 游戏主控制器，编排游戏生命周期。
 * 所有 Model 层对象通过 unique_ptr 独占管理，析构时自动级联释放。
 */

#include "controller/GameController.hpp"
#include "view/ConsoleRenderer.hpp"
#ifdef HAS_GUI
#include "view/SFMLRenderer.hpp"
#endif
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

    // 渲染器（根据编译选项选择终端或 SFML 图形渲染器）
#ifdef HAS_GUI
    _renderer = std::make_unique<SFMLRenderer>();
#else
    _renderer = std::make_unique<ConsoleRenderer>();
#endif

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
        _renderer->showMessage("Error: No puzzles available! Check data/puzzles/ directory.");
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

    // 起始格子：GUI 模式交互选择，控制台模式使用默认 5 个固定坐标
#ifdef HAS_GUI
    std::vector<std::pair<int, int>> starts =
        _renderer->promptStartingCells(initialGrid, 5);
#else
    std::vector<std::pair<int, int>> starts = {
        {0, 0}, {_gridRows/2, _gridCols/2},
        {0, _gridCols-1}, {_gridRows-1, 0},
        {_gridRows-1, _gridCols-1}
    };
#endif

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
    // 控制台模式：用 cell.setSelected() 标记光标位置供 ConsoleRenderer 高亮
    // GUI 模式：SFMLRenderer 自行管理光标，无需修改 model 状态
#ifndef HAS_GUI
    _state->grid().at(_cursorRow, _cursorCol).setSelected(true);
#endif
    _renderer->render(*_state);
#ifndef HAS_GUI
    _state->grid().at(_cursorRow, _cursorCol).setSelected(false);
#endif

    // 检查是否有合法操作（全盘无解）
    if (!_state->grid().hasAnyValidMove() && _state->stepsTaken() > 0) {
        submitAnswer();
        return;
    }

    // 检查活跃棋子是否全部陷入死局（盘面可能有解但 5 个棋子都走不了）
    if (_state->isDeadEnd() && _state->stepsTaken() > 0) {
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
            // 两段式合并：渲染器已确定源格（玩家棋子）和目标格
            {
#ifdef HAS_GUI
                auto [curR, curC] = _renderer->getCursorPosition();
                auto [selR, selC] = _renderer->getSelectedCell();
                if (selR >= 0 && selC >= 0) {
                    // GUI 模式：源格 = 第一次选中, 目标格 = 第二次点击
                    if (tryMerge(selR, selC, curR, curC)) {
                        _renderer->clearSelection();
                    }
                }
#else
                // 控制台模式：光标所在格为源格（必须是有效棋子）
                if (_state->isActiveCell(_cursorRow, _cursorCol)) {
                    Grid& gr = _state->grid();
                    int check[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
                    for (int d = 0; d < 4; ++d) {
                        int nr = _cursorRow + check[d][0];
                        int nc = _cursorCol + check[d][1];
                        if (nr >= 0 && nr < _gridRows && nc >= 0 && nc < _gridCols) {
                            if (tryMerge(_cursorRow, _cursorCol, nr, nc)) break;
                        }
                    }
                }
#endif
            }
            break;

        case UserAction::CONFIRM:
            // Enter 键：光标所在格为源格（必须是有效棋子），向第一个合法邻居合并
            {
#ifdef HAS_GUI
                auto [curR, curC] = _renderer->getCursorPosition();
#else
                int curR = _cursorRow;
                int curC = _cursorCol;
#endif
                if (_state->isActiveCell(curR, curC)) {
                    Grid& gr = _state->grid();
                    int check[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
                    for (int d = 0; d < 4; ++d) {
                        int nr = curR + check[d][0];
                        int nc = curC + check[d][1];
                        if (nr >= 0 && nr < _gridRows && nc >= 0 && nc < _gridCols) {
                            if (tryMerge(curR, curC, nr, nc)) break;
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

    // 校验源格必须是有效棋子（玩家选取的 5 个初始格之一或合并后的新位置）
    if (!_state->isActiveCell(srcRow, srcCol)) return false;

    Grid& grid = _state->grid();
    const Cell& source = grid.at(srcRow, srcCol);
    const Cell& target = grid.at(dstRow, dstCol);

    // 目标格不能为空
    if (target.isEmpty()) return false;

    // 校验合并条件
    if (!MergeRule::canMerge(source, target)) return false;

    // 计算合并结果
    Cell result = MergeRule::getMergedCell(source, target);

    // 确定方向：源格 → 目标格
    Direction dir;
    if (dstRow < srcRow)      dir = Direction::UP;
    else if (dstRow > srcRow) dir = Direction::DOWN;
    else if (dstCol < srcCol) dir = Direction::LEFT;
    else                      dir = Direction::RIGHT;

    // 执行合并：源位置置空，目标位置更新为合并结果
    // （玩家棋子从源位置移动到目标位置）
    grid.mergeCells(srcRow, srcCol, dstRow, dstCol, result);

    // 记录操作
    Move move(srcRow, srcCol, dstRow, dstCol, dir,
              result.getLetter(), result.getNumber());
    _state->recordMove(move);
    _replayBuffer->record(move);

    // 更新有效棋子：移除源位置，加入目标位置
    _state->updateActiveCells(srcRow, srcCol, dstRow, dstCol);

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

    // 显示结果（showResult 内部处理等待按键）
    _renderer->showResult(record, _state->moveHistory());

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
