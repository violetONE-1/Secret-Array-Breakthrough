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
#include <thread>
#include <chrono>

// ---- 构造 / 析构 ----

GameController::GameController()
    : _phase(GamePhase::MENU)
    , _running(true)
    , _cursorRow(0)
    , _cursorCol(0)
    , _gridRows(25)
    , _gridCols(25)
    , _currentLevel(0)
    , _vsAI(false)
    , _aiDelayMs(300)
    , _aiStrategy(AIStrategy::RANDOM)
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

    // 关卡进度
    _progress = std::make_unique<Progress>(*_fileManager, 3);
    _progress->load();

    // 回放
    _replayBuffer = std::make_unique<ReplayBuffer>(*_fileManager);

    // 渲染器
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

            case GamePhase::VS_AI_MENU:
                handleVSAIMenu();
                break;

            case GamePhase::VS_AI_WATCH:
                handleVSAIWatch();
                break;

            case GamePhase::VS_AI_RESULT:
                handleVSResult();
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
            _vsAI = false;
            handleLevelSelect();
            break;

        case UserAction::SELECT_PUZZLE_2:
        case UserAction::VIEW_LEADERBOARD:
            _phase = GamePhase::LEADERBOARD;
            break;

        case UserAction::SELECT_PUZZLE_3:
        case UserAction::VS_AI_NORMAL:
            _vsAI = true;
            _phase = GamePhase::VS_AI_MENU;
            break;

        case UserAction::SELECT_PUZZLE_4:
        case UserAction::SELECT_PUZZLE_5:
        case UserAction::QUIT:
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
    _currentLevel = 0;  // VS AI 模式不属于闯关
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
        _player.setName(_renderer->promptPlayerName());
        startNewGame(puzzles[selectedIndex]);
    }
}

// ================================================================
//  开始新游戏
// ================================================================

void GameController::startNewGame(const Puzzle& puzzle)
{
    const Grid& initialGrid = puzzle.initialGrid();
    _gridRows = initialGrid.rows();
    _gridCols = initialGrid.cols();
    _cursorRow = _gridRows / 2;
    _cursorCol = _gridCols / 2;

    // 玩家选择 5 个起始格
    std::vector<std::pair<int, int>> playerStarts =
        _renderer->promptStartingCells(initialGrid, 5);

    // AI 自动选 5 个起始格（避开玩家已选的位置）
    std::vector<std::pair<int, int>> aiStarts;
    if (_vsAI) {
        aiStarts = _aiPlayer.pickStartingCells(initialGrid, 5);
        // 剔除玩家已选的单元格
        for (auto it = aiStarts.begin(); it != aiStarts.end(); ) {
            if (std::find(playerStarts.begin(), playerStarts.end(), *it)
                    != playerStarts.end()) {
                it = aiStarts.erase(it);
            } else {
                ++it;
            }
        }
        // 若剔除后不足 5 个，从剩余候选中补齐（仅按位置填充，不重新评分）
        while (static_cast<int>(aiStarts.size()) < 5) {
            bool found = false;
            for (int r = 0; r < _gridRows && !found; ++r) {
                for (int c = 0; c < _gridCols && !found; ++c) {
                    if (initialGrid.at(r, c).isEmpty()) continue;
                    auto pos = std::make_pair(r, c);
                    bool taken = std::find(playerStarts.begin(), playerStarts.end(), pos)
                                  != playerStarts.end();
                    bool aiTaken = std::find(aiStarts.begin(), aiStarts.end(), pos)
                                   != aiStarts.end();
                    if (!taken && !aiTaken) {
                        aiStarts.emplace_back(r, c);
                        found = true;
                    }
                }
            }
            if (!found) break;  // 棋盘都满了，放弃
        }
    }

    if (_vsAI) {
        // VS AI 模式：创建带双方归属的 GameState
        _state = std::make_unique<GameState>(initialGrid, playerStarts, aiStarts, puzzle.id());
        _phase = GamePhase::VS_AI_WATCH;
    } else {
        // 单人模式：传递空 AI 起始列表
        _state = std::make_unique<GameState>(initialGrid, playerStarts,
                                              std::vector<std::pair<int, int>>{}, puzzle.id());
        _phase = GamePhase::GAMEPLAY;
    }

    // 开始录制回放
    std::ostringstream startsStr;
    for (size_t i = 0; i < playerStarts.size(); ++i) {
        if (i > 0) startsStr << ' ';
        startsStr << playerStarts[i].first << ',' << playerStarts[i].second;
    }
    _replayBuffer->startRecording(puzzle.id(),
                                  _gridRows, _gridCols,
                                  _player.name(),
                                  startsStr.str());
}

// ================================================================
//  闯关模式
// ================================================================

void GameController::handleLevelSelect()
{
    const auto& puzzles = _puzzleManager->all();

    if (puzzles.empty()) {
        _renderer->showMessage("No levels available!");
        _phase = GamePhase::MENU;
        return;
    }

    // 收集最高分数据
    int totalLevels = static_cast<int>(puzzles.size());
    std::vector<int> bestScores(totalLevels, 0);
    for (int i = 0; i < totalLevels; ++i) {
        if (_progress->isUnlocked(i + 1)) {
            bestScores[i] = _progress->bestScore(i + 1);
        }
    }

    _renderer->showLevelList(puzzles, _progress->maxUnlocked(), bestScores);

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
            return;  // 不做任何事，留在关卡列表
    }

    if (selectedIndex >= 0 && selectedIndex < totalLevels) {
        int levelNum = selectedIndex + 1;

        // 检查是否已解锁
        if (!_progress->isUnlocked(levelNum)) {
            _renderer->showMessage("This level is locked! Clear the previous level first.");
            _phase = GamePhase::MENU;
            return;
        }

        _player.setName(_renderer->promptPlayerName());
        _currentLevel = levelNum;
        startNewGame(puzzles[selectedIndex]);
    }
}

// ================================================================
//  游戏进行阶段（单人模式）
// ================================================================

void GameController::handleGameplay()
{
    if (!_state) {
        _phase = GamePhase::MENU;
        return;
    }

    // 渲染当前状态
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

    // 检查活跃棋子是否全部陷入死局
    if (!_state->hasValidMoves(CellOwner::Player) && _state->stepsTaken() > 0) {
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
            {
#ifdef HAS_GUI
                auto [curR, curC] = _renderer->getCursorPosition();
                auto [selR, selC] = _renderer->getSelectedCell();
                if (selR >= 0 && selC >= 0) {
                    if (_state->grid().at(curR, curC).isEmpty()) {
                        if (trySlide(selR, selC, curR, curC)) {
                            _renderer->clearSelection();
                        }
                    } else {
                        if (tryMerge(selR, selC, curR, curC)) {
                            _renderer->clearSelection();
                        }
                    }
                }
#else
                if (_state->isOwnCell(CellOwner::Player, _cursorRow, _cursorCol)) {
                    Grid& gr = _state->grid();
                    int check[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
                    bool acted = false;
                    for (int d = 0; d < 4; ++d) {
                        int nr = _cursorRow + check[d][0];
                        int nc = _cursorCol + check[d][1];
                        if (nr >= 0 && nr < _gridRows && nc >= 0 && nc < _gridCols) {
                            if (tryMerge(_cursorRow, _cursorCol, nr, nc)) {
                                acted = true; break;
                            }
                        }
                    }
                    if (!acted) {
                        for (int d = 0; d < 4; ++d) {
                            int nr = _cursorRow + check[d][0];
                            int nc = _cursorCol + check[d][1];
                            if (nr >= 0 && nr < _gridRows && nc >= 0 && nc < _gridCols) {
                                if (trySlide(_cursorRow, _cursorCol, nr, nc)) break;
                            }
                        }
                    }
                }
#endif
            }
            break;

        case UserAction::CONFIRM:
            {
#ifdef HAS_GUI
                auto [curR, curC] = _renderer->getCursorPosition();
#else
                int curR = _cursorRow;
                int curC = _cursorCol;
#endif
                if (_state->isOwnCell(CellOwner::Player, curR, curC)) {
                    Grid& gr = _state->grid();
                    int check[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
                    bool acted = false;
                    for (int d = 0; d < 4; ++d) {
                        int nr = curR + check[d][0];
                        int nc = curC + check[d][1];
                        if (nr >= 0 && nr < _gridRows && nc >= 0 && nc < _gridCols) {
                            if (tryMerge(curR, curC, nr, nc)) {
                                acted = true; break;
                            }
                        }
                    }
                    if (!acted) {
                        for (int d = 0; d < 4; ++d) {
                            int nr = curR + check[d][0];
                            int nc = curC + check[d][1];
                            if (nr >= 0 && nr < _gridRows && nc >= 0 && nc < _gridCols) {
                                if (trySlide(curR, curC, nr, nc)) break;
                            }
                        }
                    }
                }
            }
            break;

        case UserAction::AI_EXECUTE:
            {
                auto aiMove = _aiPlayer.findBestMove(*_state);
                if (aiMove.has_value()) {
                    tryMerge(aiMove->srcRow, aiMove->srcCol,
                             aiMove->dstRow, aiMove->dstCol);
                }
            }
            break;

        case UserAction::SUBMIT:
            submitAnswer();
            return;

        case UserAction::QUIT:
            _state.reset();
            _phase = GamePhase::MENU;
            return;

        default:
            break;
    }
}

// ================================================================
//  合并 / 滑行操作
// ================================================================

bool GameController::tryMerge(int srcRow, int srcCol, int dstRow, int dstCol)
{
    if (!_state) return false;

    // 校验源格必须是己方棋子
    if (!_state->isOwnCell(CellOwner::Player, srcRow, srcCol)) return false;

    Grid& grid = _state->grid();
    const Cell& source = grid.at(srcRow, srcCol);
    const Cell& target = grid.at(dstRow, dstCol);

    if (target.isEmpty()) return false;

    if (!MergeRule::canMerge(source, target)) return false;

    Cell result = MergeRule::getMergedCell(source, target);

    Direction dir;
    if (dstRow < srcRow)      dir = Direction::UP;
    else if (dstRow > srcRow) dir = Direction::DOWN;
    else if (dstCol < srcCol) dir = Direction::LEFT;
    else                      dir = Direction::RIGHT;

    // 执行合并
    grid.mergeCells(srcRow, srcCol, dstRow, dstCol, result);

    // 判断目标归属，更新棋子集合
    CellOwner targetOwner = _state->cellOwner(dstRow, dstCol);
    if (targetOwner == CellOwner::Player) {
        // 合并自己的两个格子
        _state->selfMerge(CellOwner::Player, srcRow, srcCol, dstRow, dstCol);
    } else {
        // 合并对手或中立格子
        _state->conquerCell(CellOwner::Player, srcRow, srcCol, dstRow, dstCol);
    }

    // 记录操作
    Move move(MoveType::MERGE, srcRow, srcCol, dstRow, dstCol, dir,
              result.getLetter(), result.getNumber());
    _state->recordMove(move, CellOwner::Player);
    _replayBuffer->record(move);

    _cursorRow = dstRow;
    _cursorCol = dstCol;

    // 触发合并动画
#ifdef HAS_GUI
    auto* sfml = dynamic_cast<SFMLRenderer*>(_renderer.get());
    if (sfml) sfml->triggerMergeAnim(dstRow, dstCol);
#endif

    return true;
}

bool GameController::trySlide(int srcRow, int srcCol, int dstRow, int dstCol)
{
    if (!_state) return false;

    if (!_state->isOwnCell(CellOwner::Player, srcRow, srcCol)) return false;

    Grid& grid = _state->grid();
    const Cell& target = grid.at(dstRow, dstCol);

    if (!target.isEmpty()) return false;

    Direction dir;
    if (dstRow < srcRow)      dir = Direction::UP;
    else if (dstRow > srcRow) dir = Direction::DOWN;
    else if (dstCol < srcCol) dir = Direction::LEFT;
    else                      dir = Direction::RIGHT;

    grid.slideCell(srcRow, srcCol, dstRow, dstCol);
    _state->moveOwnCell(CellOwner::Player, srcRow, srcCol, dstRow, dstCol);

    Move move(MoveType::SLIDE, srcRow, srcCol, dstRow, dstCol, dir,
              grid.at(dstRow, dstCol).getLetter(),
              grid.at(dstRow, dstCol).getNumber());
    _state->recordMove(move, CellOwner::Player);
    _replayBuffer->record(move);

    _cursorRow = dstRow;
    _cursorCol = dstCol;

    return true;
}

// ================================================================
//  提交答案（单人模式）
// ================================================================

void GameController::submitAnswer()
{
    if (!_state) return;

    _replayBuffer->stopRecording();

    auto now = std::time(nullptr);
    std::ostringstream timestamp;
    timestamp << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");

    ScoreRecord record(
        _player.name(),
        _state->puzzleId(),
        _state->elapsedSeconds(),
        _state->stepsTakenBy(CellOwner::Player),
        _state->accuracyBy(CellOwner::Player),
        timestamp.str()
    );

    // 保存回放
    std::ostringstream replayPath;
    replayPath << "replays/replay_"
               << std::put_time(std::localtime(&now), "%Y%m%d_%H%M%S")
               << ".txt";
    _replayBuffer->saveToFile(replayPath.str());

    _leaderboard->add(record);

    // 闯关模式：更新关卡进度
    if (_currentLevel > 0) {
        _progress->updateLevel(_currentLevel, record.score());
        _progress->save();
        _currentLevel = 0;
    }

    _renderer->showResult(record, _state->moveHistory());

    _state.reset();
    _phase = GamePhase::MENU;
}

// ================================================================
//  结果展示阶段
// ================================================================

void GameController::handleResult()
{
    _phase = GamePhase::MENU;
}

// ================================================================
//  排行榜阶段
// ================================================================

void GameController::handleLeaderboard()
{
    auto records = _leaderboard->topByScore(15);

    _renderer->showLeaderboard(records);
    _renderer->waitForAction();

    _phase = GamePhase::MENU;
}

// ================================================================
//  VS AI 难度选择阶段
// ================================================================

void GameController::handleVSAIMenu()
{
    _renderer->showVSAIMenu();

    UserAction action = _renderer->waitForAction();

    switch (action) {
        case UserAction::SELECT_PUZZLE_1:
            // 普通模式
            _aiStrategy = AIStrategy::RANDOM;
            _aiDelayMs = 300;
            _aiPlayer.setStrategy(_aiStrategy);
            _phase = GamePhase::PUZZLE_SELECT;
            break;

        case UserAction::SELECT_PUZZLE_2:
            // 高级模式
            _aiStrategy = AIStrategy::GREEDY;
            _aiDelayMs = 500;
            _aiPlayer.setStrategy(_aiStrategy);
            _phase = GamePhase::PUZZLE_SELECT;
            break;

        case UserAction::SELECT_PUZZLE_3:
        case UserAction::BACK:
        case UserAction::QUIT:
            _vsAI = false;
            _phase = GamePhase::MENU;
            break;

        default:
            break;
    }
}

// ================================================================
//  VS AI 同台对战（轮流操作）
// ================================================================

void GameController::handleVSAIWatch()
{
    if (!_state) {
        _phase = GamePhase::MENU;
        return;
    }

    bool playerOut = false;
    bool aiOut = false;

    while (_renderer->isOpen() && (!playerOut || !aiOut)) {
        // ---- 玩家回合 ----
        if (!playerOut) {
            _renderer->setTurnMessage("Your turn! Pick one of your pieces.");
            bool moved = processVSAIPlayerAction();

            if (_phase != GamePhase::VS_AI_WATCH) return;  // 退出

            if (!moved) {
                playerOut = true;
                _renderer->setTurnMessage("You're out! AI continues...");
                _renderer->render(*_state);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }

        if (playerOut && aiOut) break;
        if (!_renderer->isOpen()) return;

        // ---- AI 回合 ----
        if (!aiOut) {
            _renderer->setTurnMessage("AI thinking...");
            bool moved = executeVSAIAIAction();

            if (_phase != GamePhase::VS_AI_WATCH) return;

            if (!moved) {
                aiOut = true;
                _renderer->setTurnMessage("AI is out! Your turn if you can move.");
                _renderer->render(*_state);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
    }

    // 双方都出局 → 结算
    finishVSAI();
}

// ---- 玩家回合：等待一次有效操作 ----

bool GameController::processVSAIPlayerAction()
{
    if (!_state) return false;

    // 先检查玩家是否有棋可走
    if (!_state->hasValidMoves(CellOwner::Player)) {
        return false;
    }

    while (_renderer->isOpen()) {
        _renderer->setTurnMessage("Your turn! Pick one of your pieces.");
        _renderer->render(*_state);

        // 检查是否突然无棋可走（可能被 AI 上回合吃光）
        if (!_state->hasValidMoves(CellOwner::Player)) {
            return false;
        }

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
            {
#ifdef HAS_GUI
                auto [curR, curC] = _renderer->getCursorPosition();
                auto [selR, selC] = _renderer->getSelectedCell();
                if (selR >= 0 && selC >= 0) {
                    // 检查源格是否为玩家棋子
                    if (_state->isOwnCell(CellOwner::Player, selR, selC)) {
                        bool ok = false;
                        if (_state->grid().at(curR, curC).isEmpty()) {
                            // 尝试滑行
                            ok = doAISlide(CellOwner::Player, selR, selC, curR, curC);
                        } else {
                            // 尝试合并
                            ok = doAIMerge(CellOwner::Player, selR, selC, curR, curC);
                        }
                        if (ok) {
                            _renderer->clearSelection();
                            return true;  // 成功走了一步
                        }
                    }
                    // 源格不是自己的棋子或操作失败，重新选
                    _renderer->clearSelection();
                }
#else
                // 控制台：光标所在格必须为玩家棋子
                if (_state->isOwnCell(CellOwner::Player, _cursorRow, _cursorCol)) {
                    Grid& gr = _state->grid();
                    int check[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
                    bool acted = false;
                    for (int d = 0; d < 4; ++d) {
                        int nr = _cursorRow + check[d][0];
                        int nc = _cursorCol + check[d][1];
                        if (nr >= 0 && nr < _gridRows && nc >= 0 && nc < _gridCols) {
                            if (doAIMerge(CellOwner::Player, _cursorRow, _cursorCol, nr, nc)) {
                                acted = true; break;
                            }
                        }
                    }
                    if (!acted) {
                        for (int d = 0; d < 4; ++d) {
                            int nr = _cursorRow + check[d][0];
                            int nc = _cursorCol + check[d][1];
                            if (nr >= 0 && nr < _gridRows && nc >= 0 && nc < _gridCols) {
                                if (doAISlide(CellOwner::Player, _cursorRow, _cursorCol, nr, nc)) {
                                    acted = true; break;
                                }
                            }
                        }
                    }
                    if (acted) return true;
                }
#endif
            }
            break;

        case UserAction::CONFIRM:
            {
#ifdef HAS_GUI
                auto [curR, curC] = _renderer->getCursorPosition();
#else
                int curR = _cursorRow;
                int curC = _cursorCol;
#endif
                if (_state->isOwnCell(CellOwner::Player, curR, curC)) {
                    Grid& gr = _state->grid();
                    int check[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
                    bool acted = false;
                    for (int d = 0; d < 4; ++d) {
                        int nr = curR + check[d][0];
                        int nc = curC + check[d][1];
                        if (nr >= 0 && nr < _gridRows && nc >= 0 && nc < _gridCols) {
                            if (doAIMerge(CellOwner::Player, curR, curC, nr, nc)) {
                                acted = true; break;
                            }
                        }
                    }
                    if (!acted) {
                        for (int d = 0; d < 4; ++d) {
                            int nr = curR + check[d][0];
                            int nc = curC + check[d][1];
                            if (nr >= 0 && nr < _gridRows && nc >= 0 && nc < _gridCols) {
                                if (doAISlide(CellOwner::Player, curR, curC, nr, nc)) {
                                    acted = true; break;
                                }
                            }
                        }
                    }
                    if (acted) return true;
                }
            }
            break;

        case UserAction::SUBMIT:
            return false;  // 主动认输

        case UserAction::QUIT:
            _state.reset();
            _phase = GamePhase::MENU;
            return false;

        default:
            break;
        }
    }

    return false;
}

// ---- AI 回合：走一步 ----

bool GameController::executeVSAIAIAction()
{
    if (!_state) return false;

    if (!_state->hasValidMoves(CellOwner::AI)) {
        return false;
    }

    auto aiMove = _aiPlayer.findBestMove(*_state, _state->aiCells());
    if (!aiMove.has_value()) {
        return false;
    }

    int srcR = aiMove->srcRow, srcC = aiMove->srcCol;
    int dstR = aiMove->dstRow, dstC = aiMove->dstCol;

    bool ok = false;
    if (aiMove->move.moveType == MoveType::SLIDE) {
        ok = doAISlide(CellOwner::AI, srcR, srcC, dstR, dstC);
    } else {
        ok = doAIMerge(CellOwner::AI, srcR, srcC, dstR, dstC);
    }

    if (!ok) return false;

    _cursorRow = dstR;
    _cursorCol = dstC;

    // 延迟并持续渲染
    auto delayStart = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - delayStart).count() < _aiDelayMs) {
        if (_renderer->isOpen()) {
            _renderer->setTurnMessage("AI thinking...");
            _renderer->render(*_state);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return true;
}

// ---- VS AI 结算 ----

void GameController::finishVSAI()
{
    if (!_state) {
        _phase = GamePhase::MENU;
        return;
    }

    auto now = std::time(nullptr);
    std::ostringstream timestamp;
    timestamp << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");

    std::string aiName = "AI(" + AIPlayer::strategyName(_aiStrategy) + ")";

    // 玩家成绩
    ScoreRecord playerRecord(
        _player.name(),
        _state->puzzleId(),
        _state->elapsedSeconds(),
        _state->stepsTakenBy(CellOwner::Player),
        _state->accuracyBy(CellOwner::Player),
        timestamp.str()
    );

    // AI 成绩
    ScoreRecord aiRecord(
        aiName,
        _state->puzzleId(),
        _state->elapsedSeconds(),
        _state->stepsTakenBy(CellOwner::AI),
        _state->accuracyBy(CellOwner::AI),
        timestamp.str()
    );

    // VS AI 模式：总分 = 步数 × 10（不计时间 / 正确率）
    {
        int playerSteps = _state->stepsTakenBy(CellOwner::Player);
        int aiSteps = _state->stepsTakenBy(CellOwner::AI);
        playerRecord.setScore(playerSteps * 10);
        aiRecord.setScore(aiSteps * 10);
    }

    // 判定胜负
    std::string winner;
    if (playerRecord.score() > aiRecord.score()) {
        winner = "player";
    } else if (playerRecord.score() < aiRecord.score()) {
        winner = "ai";
    } else {
        if (playerRecord.steps() > aiRecord.steps()) {
            winner = "player";
        } else if (playerRecord.steps() < aiRecord.steps()) {
            winner = "ai";
        } else {
            winner = "draw";
        }
    }

    _renderer->showVSResult(playerRecord, aiRecord, winner);
    _renderer->waitForAction();

    _state.reset();
    _phase = GamePhase::MENU;
    _vsAI = false;
}

// ================================================================
//  VS AI 结果阶段
// ================================================================

void GameController::handleVSResult()
{
    _phase = GamePhase::MENU;
}

// ================================================================
//  通用操作辅助（供 doAIMerge / doAISlide 等使用）
// ================================================================

bool GameController::doAIMerge(CellOwner owner, int srcRow, int srcCol, int dstRow, int dstCol)
{
    if (!_state) return false;
    if (!_state->isOwnCell(owner, srcRow, srcCol)) return false;

    Grid& grid = _state->grid();
    const Cell& source = grid.at(srcRow, srcCol);
    const Cell& target = grid.at(dstRow, dstCol);

    if (target.isEmpty()) return false;
    if (!MergeRule::canMerge(source, target)) return false;

    Cell result = MergeRule::getMergedCell(source, target);

    Direction dir;
    if (dstRow < srcRow)      dir = Direction::UP;
    else if (dstRow > srcRow) dir = Direction::DOWN;
    else if (dstCol < srcCol) dir = Direction::LEFT;
    else                      dir = Direction::RIGHT;

    grid.mergeCells(srcRow, srcCol, dstRow, dstCol, result);

    CellOwner targetOwner = _state->cellOwner(dstRow, dstCol);
    if (targetOwner == owner) {
        _state->selfMerge(owner, srcRow, srcCol, dstRow, dstCol);
    } else {
        _state->conquerCell(owner, srcRow, srcCol, dstRow, dstCol);
    }

    Move move(MoveType::MERGE, srcRow, srcCol, dstRow, dstCol, dir,
              result.getLetter(), result.getNumber());
    _state->recordMove(move, owner);
    _replayBuffer->record(move);

    _cursorRow = dstRow;
    _cursorCol = dstCol;

#ifdef HAS_GUI
    auto* sfml = dynamic_cast<SFMLRenderer*>(_renderer.get());
    if (sfml) sfml->triggerMergeAnim(dstRow, dstCol);
#endif

    return true;
}

bool GameController::doAISlide(CellOwner owner, int srcRow, int srcCol, int dstRow, int dstCol)
{
    if (!_state) return false;
    if (!_state->isOwnCell(owner, srcRow, srcCol)) return false;

    Grid& grid = _state->grid();
    if (!grid.at(dstRow, dstCol).isEmpty()) return false;

    Direction dir;
    if (dstRow < srcRow)      dir = Direction::UP;
    else if (dstRow > srcRow) dir = Direction::DOWN;
    else if (dstCol < srcCol) dir = Direction::LEFT;
    else                      dir = Direction::RIGHT;

    grid.slideCell(srcRow, srcCol, dstRow, dstCol);
    _state->moveOwnCell(owner, srcRow, srcCol, dstRow, dstCol);

    Move move(MoveType::SLIDE, srcRow, srcCol, dstRow, dstCol, dir,
              grid.at(dstRow, dstCol).getLetter(),
              grid.at(dstRow, dstCol).getNumber());
    _state->recordMove(move, owner);
    _replayBuffer->record(move);

    _cursorRow = dstRow;
    _cursorCol = dstCol;

    return true;
}
