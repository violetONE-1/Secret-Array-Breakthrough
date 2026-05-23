/**
 * GameState.cpp — GameState 类的实现
 *
 * 见 include/core/GameState.hpp 的详细文档注释。
 */

#include "core/GameState.hpp"
#include "core/MergeRule.hpp"

GameState::GameState(const Grid& initialGrid,
                     const std::vector<std::pair<int, int>>& playerStarts,
                     const std::string& puzzleId)
    : _grid(initialGrid)
    , _puzzleId(puzzleId)
    , _playerStarts(playerStarts)
    , _stepsTaken(0)
    , _timerStarted(false)
    , _gameOver(false)
{
    for (const auto& s : playerStarts) {
        _activeCells.insert(s);
    }
}

// ---- 盘面访问 ----

const Grid& GameState::grid() const { return _grid; }
Grid&       GameState::grid()       { return _grid; }

// ---- 基本信息 ----

const std::string& GameState::puzzleId() const { return _puzzleId; }

const std::vector<std::pair<int, int>>& GameState::playerStarts() const
{
    return _playerStarts;
}

// ---- 操作记录 ----

void GameState::recordMove(const Move& move)
{
    // 首次操作时启动计时器
    if (!_timerStarted) {
        _startTime = std::chrono::steady_clock::now();
        _timerStarted = true;
    }

    _moveHistory.push_back(move);
    ++_stepsTaken;
}

const std::vector<Move>& GameState::moveHistory() const
{
    return _moveHistory;
}

// ---- 计时 ----

double GameState::elapsedSeconds() const
{
    // 玩家尚未做出任何操作时返回 0
    if (!_timerStarted) {
        return 0.0;
    }

    auto now = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration<double>(now - _startTime);
    return diff.count();
}

// ---- 准确率 ----

double GameState::accuracy() const
{
    // 准确率 = 1.0（无操作时）或 1.0（全有效操作时）
    // 此处假设 recordMove 只在有效操作时调用
    return _stepsTaken > 0 ? 1.0 : 0.0;
}

// ---- 生命期 ----

bool GameState::isOver() const
{
    return _gameOver;
}

void GameState::endGame()
{
    _gameOver = true;
}

int GameState::stepsTaken() const
{
    return _stepsTaken;
}

// ---- 有效棋子集合 ----

bool GameState::isActiveCell(int row, int col) const
{
    return _activeCells.find({row, col}) != _activeCells.end();
}

const std::set<std::pair<int, int>>& GameState::activeCells() const
{
    return _activeCells;
}

void GameState::updateActiveCells(int fromRow, int fromCol, int toRow, int toCol)
{
    _activeCells.erase({fromRow, fromCol});
    _activeCells.insert({toRow, toCol});
}

bool GameState::isDeadEnd() const
{
    const int rows = _grid.rows();
    const int cols = _grid.cols();
    const int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    for (const auto& [r, c] : _activeCells) {
        // 防御：跳过越界或空格的活跃棋子（正常流程不应出现）
        if (r < 0 || r >= rows || c < 0 || c >= cols) continue;

        const Cell& src = _grid.at(r, c);
        if (src.isEmpty()) continue;

        for (int d = 0; d < 4; ++d) {
            int nr = r + dirs[d][0];
            int nc = c + dirs[d][1];

            if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;

            const Cell& dst = _grid.at(nr, nc);
            if (!dst.isEmpty() && MergeRule::canMerge(src, dst)) {
                return false;  // 至少有一个棋子还能走
            }
        }
    }
    return true;  // 全部 5 个棋子都被围死
}
