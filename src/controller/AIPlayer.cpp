/**
 * AIPlayer.cpp — AIPlayer 类的实现
 */

#include "controller/AIPlayer.hpp"
#include "core/MergeRule.hpp"
#include <random>
#include <algorithm>
#include <set>
#include <cmath>

AIPlayer::AIPlayer(AIStrategy strategy)
    : _strategy(strategy)
{
}

void AIPlayer::setStrategy(AIStrategy strategy) { _strategy = strategy; }
AIStrategy AIPlayer::strategy() const { return _strategy; }

std::string AIPlayer::strategyName(AIStrategy s)
{
    switch (s) {
        case AIStrategy::RANDOM: return "Random";
        case AIStrategy::GREEDY: return "Greedy";
    }
    return "Unknown";
}

// ---- 查找所有合法操作（合并 + 滑行） ----

std::vector<AIMove> AIPlayer::findAllValidMoves(const GameState& state) const
{
    std::vector<AIMove> result;
    const Grid& grid = state.grid();
    const auto& activeCells = state.activeCells();

    static const int dirs[4][2] = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1}
    };

    for (const auto& [srcRow, srcCol] : activeCells) {
        const Cell& src = grid.at(srcRow, srcCol);
        if (src.isEmpty()) continue;

        for (int d = 0; d < 4; ++d) {
            int dstRow = srcRow + dirs[d][0];
            int dstCol = srcCol + dirs[d][1];

            if (dstRow < 0 || dstRow >= grid.rows() ||
                dstCol < 0 || dstCol >= grid.cols()) {
                continue;
            }

            const Cell& dst = grid.at(dstRow, dstCol);

            Direction dir;
            if (d == 0)      dir = Direction::UP;
            else if (d == 1) dir = Direction::DOWN;
            else if (d == 2) dir = Direction::LEFT;
            else             dir = Direction::RIGHT;

            if (dst.isEmpty()) {
                // 滑行
                Move move(MoveType::SLIDE, srcRow, srcCol, dstRow, dstCol, dir,
                          src.getLetter(), src.getNumber());
                result.push_back({srcRow, srcCol, dstRow, dstCol, move});
            } else if (MergeRule::canMerge(src, dst)) {
                // 合并
                Cell merged = MergeRule::getMergedCell(src, dst);
                Move move(MoveType::MERGE, srcRow, srcCol, dstRow, dstCol, dir,
                          merged.getLetter(), merged.getNumber());
                result.push_back({srcRow, srcCol, dstRow, dstCol, move});
            }
        }
    }

    return result;
}

// ---- 查找最佳操作 ----

std::optional<AIMove> AIPlayer::findBestMove(const GameState& state) const
{
    std::vector<AIMove> moves = findAllValidMoves(state);
    if (moves.empty()) {
        return std::nullopt;
    }

    switch (_strategy) {
        case AIStrategy::RANDOM:
            return randomPick(moves);
        case AIStrategy::GREEDY:
            return greedyPick(moves);
    }

    return randomPick(moves);
}

// ---- 策略实现 ----

std::optional<AIMove> AIPlayer::randomPick(const std::vector<AIMove>& moves) const
{
    // 分离合并和滑行
    std::vector<const AIMove*> merges;
    std::vector<const AIMove*> slides;
    for (const auto& m : moves) {
        if (m.move.moveType == MoveType::MERGE) {
            merges.push_back(&m);
        } else {
            slides.push_back(&m);
        }
    }

    static std::mt19937 rng(std::random_device{}());

    // 70% 概率选合并（若存在），30% 选滑行
    if (!merges.empty() && !slides.empty()) {
        std::uniform_int_distribution<int> dist(1, 100);
        if (dist(rng) <= 70) {
            std::uniform_int_distribution<size_t> md(0, merges.size() - 1);
            return *merges[md(rng)];
        } else {
            std::uniform_int_distribution<size_t> sd(0, slides.size() - 1);
            return *slides[sd(rng)];
        }
    }

    // 只有一种类型
    if (!merges.empty()) {
        std::uniform_int_distribution<size_t> md(0, merges.size() - 1);
        return *merges[md(rng)];
    }
    if (!slides.empty()) {
        std::uniform_int_distribution<size_t> sd(0, slides.size() - 1);
        return *slides[sd(rng)];
    }

    return std::nullopt;
}

std::optional<AIMove> AIPlayer::greedyPick(const std::vector<AIMove>& moves) const
{
    // 分离合并和滑行
    std::vector<const AIMove*> merges;
    std::vector<const AIMove*> slides;
    for (const auto& m : moves) {
        if (m.move.moveType == MoveType::MERGE) {
            merges.push_back(&m);
        } else {
            slides.push_back(&m);
        }
    }

    if (!merges.empty()) {
        // 贪心：选合并后字母+数字总值最大的
        auto score = [](const AIMove* m) -> int {
            int letterVal = (m->move.resultLetter - 'A') + 1;
            return letterVal + m->move.resultNumber;
        };
        auto best = std::max_element(merges.begin(), merges.end(),
            [&](const AIMove* a, const AIMove* b) {
                return score(a) < score(b);
            });
        return **best;
    }

    // 无合并时随机滑行
    if (!slides.empty()) {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<size_t> sd(0, slides.size() - 1);
        return *slides[sd(rng)];
    }

    return std::nullopt;
}

// ---- AI 自选起始格 ----

std::vector<std::pair<int, int>> AIPlayer::pickStartingCells(
    const Grid& grid, int count) const
{
    struct Candidate {
        int row, col;
        int score;  // 合并潜力评分
    };

    std::vector<Candidate> candidates;
    int rows = grid.rows();
    int cols = grid.cols();

    static const int dirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const Cell& cell = grid.at(r, c);
            if (cell.isEmpty()) continue;

            // 评分 = 可合并邻居数 × 10 + 非空格邻居数
            int mergeCount = 0;
            int nonEmptyCount = 0;
            for (int d = 0; d < 4; ++d) {
                int nr = r + dirs[d][0];
                int nc = c + dirs[d][1];
                if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;
                const Cell& nb = grid.at(nr, nc);
                if (!nb.isEmpty()) {
                    nonEmptyCount++;
                    if (MergeRule::canMerge(cell, nb)) {
                        mergeCount++;
                    }
                }
            }
            candidates.push_back({r, c, mergeCount * 10 + nonEmptyCount});
        }
    }

    if (candidates.empty()) return {};

    // 按评分降序
    std::sort(candidates.begin(), candidates.end(),
        [](const Candidate& a, const Candidate& b) {
            return a.score > b.score;
        });

    // 选 top N，确保最小间距
    std::vector<std::pair<int, int>> result;
    for (const auto& cand : candidates) {
        if (static_cast<int>(result.size()) >= count) break;

        // 间距检查：与已选格子至少相距 3 格
        bool tooClose = false;
        for (const auto& sel : result) {
            int dr = std::abs(cand.row - sel.first);
            int dc = std::abs(cand.col - sel.second);
            if (dr + dc < 3) {
                tooClose = true;
                break;
            }
        }
        if (!tooClose) {
            result.emplace_back(cand.row, cand.col);
        }
    }

    // 如果间距限制导致不足 count 个，放宽限制补齐
    if (static_cast<int>(result.size()) < count) {
        for (const auto& cand : candidates) {
            if (static_cast<int>(result.size()) >= count) break;
            auto it = std::find(result.begin(), result.end(),
                                std::make_pair(cand.row, cand.col));
            if (it == result.end()) {
                result.emplace_back(cand.row, cand.col);
            }
        }
    }

    return result;
}
