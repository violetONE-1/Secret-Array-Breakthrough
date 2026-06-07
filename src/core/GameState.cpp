/**
 * GameState.cpp — GameState 类的实现
 *
 * 见 include/core/GameState.hpp 的详细文档注释。
 */

#include "core/GameState.hpp"
#include "core/MergeRule.hpp"
#include <queue>
#include <algorithm>

GameState::GameState(const Grid& initialGrid,
                     const std::vector<std::pair<int, int>>& playerStarts,
                     const std::vector<std::pair<int, int>>& aiStarts,
                     const std::string& puzzleId)
    : _grid(initialGrid)
    , _puzzleId(puzzleId)
    , _playerStarts(playerStarts)
    , _stepsTaken(0)
    , _playerSteps(0)
    , _aiSteps(0)
    , _timerStarted(false)
    , _gameOver(false)
{
    for (const auto& s : playerStarts) {
        _playerCells.insert(s);
    }
    for (const auto& s : aiStarts) {
        _aiCells.insert(s);
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

// ---- 棋子归属 ----

CellOwner GameState::cellOwner(int row, int col) const
{
    auto pos = std::make_pair(row, col);
    if (_playerCells.find(pos) != _playerCells.end()) return CellOwner::Player;
    if (_aiCells.find(pos) != _aiCells.end()) return CellOwner::AI;
    return CellOwner::None;
}

const std::set<std::pair<int, int>>& GameState::playerCells() const
{
    return _playerCells;
}

const std::set<std::pair<int, int>>& GameState::aiCells() const
{
    return _aiCells;
}

std::set<std::pair<int, int>> GameState::allActiveCells() const
{
    std::set<std::pair<int, int>> result = _playerCells;
    result.insert(_aiCells.begin(), _aiCells.end());
    return result;
}

bool GameState::isOwnCell(CellOwner owner, int row, int col) const
{
    auto pos = std::make_pair(row, col);
    if (owner == CellOwner::Player) return _playerCells.find(pos) != _playerCells.end();
    if (owner == CellOwner::AI)     return _aiCells.find(pos) != _aiCells.end();
    return false;
}

// ---- 棋子操作 ----

void GameState::moveOwnCell(CellOwner owner, int fromRow, int fromCol, int toRow, int toCol)
{
    if (owner == CellOwner::Player) {
        _playerCells.erase({fromRow, fromCol});
        _playerCells.insert({toRow, toCol});
    } else if (owner == CellOwner::AI) {
        _aiCells.erase({fromRow, fromCol});
        _aiCells.insert({toRow, toCol});
    }
}

void GameState::conquerCell(CellOwner attacker, int srcRow, int srcCol, int dstRow, int dstCol)
{
    auto dst = std::make_pair(dstRow, dstCol);
    if (attacker == CellOwner::Player) {
        _playerCells.erase({srcRow, srcCol});
        _playerCells.insert(dst);
        _aiCells.erase(dst);  // dst was AI's, now conquered
    } else {
        _aiCells.erase({srcRow, srcCol});
        _aiCells.insert(dst);
        _playerCells.erase(dst);  // dst was player's, now conquered
    }
}

void GameState::selfMerge(CellOwner owner, int srcRow, int srcCol, int dstRow, int dstCol)
{
    auto src = std::make_pair(srcRow, srcCol);
    auto dst = std::make_pair(dstRow, dstCol);
    if (owner == CellOwner::Player) {
        _playerCells.erase(src);
        // dst stays in _playerCells
    } else {
        _aiCells.erase(src);
        // dst stays in _aiCells
    }
}

// ---- 操作记录 ----

void GameState::recordMove(const Move& move, CellOwner owner)
{
    if (!_timerStarted) {
        _startTime = std::chrono::steady_clock::now();
        _timerStarted = true;
    }

    _moveHistory.push_back(move);
    _moveOwners.push_back(owner);
    ++_stepsTaken;
    if (owner == CellOwner::Player) ++_playerSteps;
    else ++_aiSteps;
}

const std::vector<Move>& GameState::moveHistory() const
{
    return _moveHistory;
}

int GameState::stepsTakenBy(CellOwner owner) const
{
    if (owner == CellOwner::Player) return _playerSteps;
    if (owner == CellOwner::AI)     return _aiSteps;
    return _stepsTaken;
}

int GameState::stepsTaken() const
{
    return _stepsTaken;
}

// ---- 计时 ----

double GameState::elapsedSeconds() const
{
    if (!_timerStarted) return 0.0;
    auto now = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration<double>(now - _startTime);
    return diff.count();
}

// ---- 生命期 ----

bool GameState::hasValidMoves(CellOwner owner) const
{
    const std::set<std::pair<int, int>>* cells = nullptr;
    if (owner == CellOwner::Player) cells = &_playerCells;
    else if (owner == CellOwner::AI) cells = &_aiCells;
    else return false;

    if (!cells) return false;

    const int rows = _grid.rows();
    const int cols = _grid.cols();
    const int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    for (const auto& [r, c] : *cells) {
        const Cell& piece = _grid.at(r, c);
        if (piece.isEmpty()) continue;

        // BFS：从当前棋子位置出发，沿空格滑行，搜索所有可达位置
        std::queue<std::pair<int, int>> q;
        std::set<std::pair<int, int>> visited;
        q.push({r, c});
        visited.insert({r, c});

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
                    // 跳过起始格——滑走之后它已变成空格，不能作为合并目标
                    if (nr == r && nc == c) continue;
                    // 非空格：检查棋子内容是否可与之合并
                    if (MergeRule::canMerge(piece, neighbor)) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool GameState::isOver() const
{
    return _gameOver;
}

void GameState::endGame()
{
    _gameOver = true;
}

bool GameState::gameOver() const
{
    return _gameOver;
}
