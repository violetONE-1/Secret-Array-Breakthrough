/**
 * AIPlayer.hpp — AI 对手类 (AIPlayer)
 *
 * 为 VS AI 对战模式提供 AI 对手，可自动执行合并和滑行操作。
 * 支持两种策略：
 *   - RANDOM：随机选择合法操作（合并 70% 权重，滑行 30% 权重）
 *   - GREEDY：优先合并，无合并时才滑行；选择合并后字母+数字总值最大的操作
 *
 * 所属架构层：Controller
 * 所属模块：游戏流程控制模块
 */

#ifndef AIPLAYER_HPP
#define AIPLAYER_HPP

#include "core/GameState.hpp"
#include "core/Move.hpp"
#include <vector>
#include <utility>
#include <optional>
#include <string>

/** AI 策略枚举 */
enum class AIStrategy {
    RANDOM,     // 随机选择合法操作
    GREEDY,     // 贪心：优先合并，选最优
};

/** AI 选择的一次操作 */
struct AIMove {
    int  srcRow, srcCol;
    int  dstRow, dstCol;
    Move move;
};

class AIPlayer {
public:
    explicit AIPlayer(AIStrategy strategy = AIStrategy::RANDOM);

    void setStrategy(AIStrategy strategy);
    AIStrategy strategy() const;

    static std::string strategyName(AIStrategy s);

    /**
     * 查找当前盘面下 AI 的最佳操作（含合并和滑行）。
     * 若无合法操作，返回 std::nullopt。
     */
    std::optional<AIMove> findBestMove(const GameState& state) const;

    /**
     * 从指定的己方棋子集合中查找最佳操作。
     * 用于 VS AI 同台对战模式：AI 只从自己的 5 个棋子中搜索。
     */
    std::optional<AIMove> findBestMove(const GameState& state,
                                       const std::set<std::pair<int, int>>& myCells) const;

    /**
     * 查找当前盘面下所有合法操作（合并 + 滑行，仅从活跃棋子出发）。
     */
    std::vector<AIMove> findAllValidMoves(const GameState& state) const;

    /**
     * 从指定棋子集合中查找所有合法操作。
     */
    std::vector<AIMove> findAllValidMoves(const GameState& state,
                                          const std::set<std::pair<int, int>>& cells) const;

    /**
     * AI 从初始网格中自选 count 个起始格子。
     * 策略：扫描所有非空格，按合并潜力评分，选 top N 且保持间距。
     */
    std::vector<std::pair<int, int>> pickStartingCells(
        const Grid& grid, int count) const;

private:
    AIStrategy _strategy;

    std::optional<AIMove> randomPick(const std::vector<AIMove>& moves) const;
    std::optional<AIMove> greedyPick(const std::vector<AIMove>& moves) const;
};

#endif // AIPLAYER_HPP
