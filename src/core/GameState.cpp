/**
 * GameState.cpp — GameState 类的实现
 *
 * 见 include/core/GameState.hpp 的详细文档注释。
 */

#include "core/GameState.hpp"
#include "core/MergeRule.hpp"
#include <queue>

GameState::GameState(const Grid& initialGrid,
                     const std::vector<std::pair<int, int>>& playerStarts,
                     const std::string& puzzleId)
    : _grid(initialGrid)
    , _puzzleId(puzzleId)
    , _playerStarts(playerStarts)
    , _stepsTaken(0)
    , _slideCount(0)
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
    if (move.moveType == MoveType::SLIDE) {
        ++_slideCount;
    }
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
    // 方案A：准确率只计合并操作，滑行不影响
    int mergeCount = _stepsTaken - _slideCount;
    if (mergeCount <= 0) return 0.0;
    return 1.0;  // 所有合并均为有效操作
}

int GameState::slideCount() const
{
    return _slideCount;
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
        if (r < 0 || r >= rows || c < 0 || c >= cols) continue;

        const Cell& piece = _grid.at(r, c);
        if (piece.isEmpty()) continue;

        // BFS：从当前棋子位置出发，沿空格滑行，搜索所有可达位置
        // 若任一可达位置存在可合并邻居 → 该棋子有解 → 非死局
        std::queue<std::pair<int, int>> q;
        std::set<std::pair<int, int>> visited;
        q.push({r, c});
        visited.insert({r, c});

        bool pieceAlive = false;
        while (!q.empty()) {
            auto [cr, cc] = q.front();
            q.pop();

            for (int d = 0; d < 4; ++d) {
                int nr = cr + dirs[d][0];
                int nc = cc + dirs[d][1];

                if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;

                const Cell& neighbor = _grid.at(nr, nc);
                if (neighbor.isEmpty()) {
                    // 空格：可滑行至此，加入 BFS 前沿
                    auto next = std::make_pair(nr, nc);
                    if (visited.find(next) == visited.end()) {
                        visited.insert(next);
                        q.push(next);
                    }
                } else {
                    // 非空格：检查棋子内容是否可与之合并
                    if (MergeRule::canMerge(piece, neighbor)) {
                        pieceAlive = true;
                        break;
                    }
                }
            }
            if (pieceAlive) break;
        }

        if (!pieceAlive) {
            // 当前棋子通过滑动也找不到任何可合并邻居 → 全体死局
            return true;
        }
    }
    return false;  // 全部棋子均能找到合并目标
}
