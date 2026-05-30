/**
 * GameState.hpp — 游戏状态类 (GameState)
 *
 * 聚合当前一局游戏的全部运行时数据，是 Controller 查询/修改 Model 的统一入口。
 *
 * 职责：
 *   - 持有当前盘面（Grid）
 *   - 分别追踪玩家和 AI 的棋子集合与归属
 *   - 维护步数计数器与操作历史
 *   - 管理游戏计时（首次操作 → 提交/结束）
 *   - 判断游戏是否结束
 *
 * 内存策略：
 *   - Grid 和 std::vector<Move> 均为值语义
 *   - GameState 本身通过 std::unique_ptr<GameState> 由 GameController 持有
 *
 * 所属架构层：Model（核心数据层）
 * 所属模块：游戏核心逻辑模块
 */

#ifndef GAMESTATE_HPP
#define GAMESTATE_HPP

#include "Grid.hpp"
#include "Move.hpp"
#include <chrono>
#include <set>
#include <map>
#include <string>
#include <vector>
#include <utility>

/** 格子所属方 */
enum class CellOwner {
    None,   // 中立 / 空格
    Player,
    AI
};

class GameState {
public:
    /** 构造一局 VS AI 游戏的初始状态（玩家 + AI 各 5 个起始格） */
    GameState(const Grid& initialGrid,
              const std::vector<std::pair<int, int>>& playerStarts,
              const std::vector<std::pair<int, int>>& aiStarts,
              const std::string& puzzleId);

    // ---- 盘面访问 ----

    const Grid& grid() const;   // 只读
    Grid&       grid();         // 可写（仅 Controller 使用）

    // ---- 基本信息 ----

    const std::string& puzzleId() const;

    const std::vector<std::pair<int, int>>& playerStarts() const;

    // ---- 棋子归属 ----

    /** 查询某格归属于谁 */
    CellOwner cellOwner(int row, int col) const;

    /** 获取玩家全部棋子坐标（只读） */
    const std::set<std::pair<int, int>>& playerCells() const;

    /** 获取 AI 全部棋子坐标（只读） */
    const std::set<std::pair<int, int>>& aiCells() const;

    /** 获取全部棋子坐标（玩家 + AI 合并，只读） */
    std::set<std::pair<int, int>> allActiveCells() const;

    /** 检查某格是否为有效的己方棋子起点 */
    bool isOwnCell(CellOwner owner, int row, int col) const;

    // ---- 棋子操作 ----

    /**
     * 滑行：将己方棋子从 (fromRow, fromCol) 移动到相邻空格 (toRow, toCol)。
     * 调用方应确保源格为己方棋子且目标格为空。
     */
    void moveOwnCell(CellOwner owner, int fromRow, int fromCol, int toRow, int toCol);

    /**
     * 合并并征服：己方棋子从 src 合并到 dst。
     * dst 可能为对手棋子或中立格子，合并后 dst 结果格归属 attacker。
     * 调用方应确保 MergeRule::canMerge 校验通过。
     */
    void conquerCell(CellOwner attacker, int srcRow, int srcCol, int dstRow, int dstCol);

    /**
     * 自身合并：己方棋子 src 合并到己方另一棋子 dst。
     * src 消逝，dst 保留为合并后结果，仍归同一方。
     */
    void selfMerge(CellOwner owner, int srcRow, int srcCol, int dstRow, int dstCol);

    // ---- 操作记录 ----

    /** 记录一步有效操作，步数计数 +1，标记操作方 */
    void recordMove(const Move& move, CellOwner owner = CellOwner::Player);

    /** 返回全部操作历史（只读） */
    const std::vector<Move>& moveHistory() const;

    /** 返回某一方的总步数 */
    int stepsTakenBy(CellOwner owner) const;

    /** 返回某一方的正确率 */
    double accuracyBy(CellOwner owner) const;

    /** 返回本局总步数（合并 + 滑行） */
    int stepsTaken() const;

    // ---- 计时 ----

    /** 首次调用时记录起始时间；后续调用返回已过秒数 */
    double elapsedSeconds() const;

    // ---- 生命期 ----

    /** 判断某一方是否仍有合法操作 */
    bool hasValidMoves(CellOwner owner) const;

    /** 是否有任何合法操作存在（全局） */
    bool isOver() const;

    /** 标记游戏结束 */
    void endGame();

    /** 游戏是否已结束 */
    bool gameOver() const;

private:
    Grid                                _grid;
    std::string                         _puzzleId;
    std::vector<std::pair<int, int>>    _playerStarts;
    std::set<std::pair<int, int>>       _playerCells;
    std::set<std::pair<int, int>>       _aiCells;
    int                                 _stepsTaken;
    int                                 _slideCount;
    int                                 _playerSteps;
    int                                 _aiSteps;
    int                                 _playerSlideCount;
    int                                 _aiSlideCount;
    std::vector<Move>                   _moveHistory;
    std::vector<CellOwner>              _moveOwners;
    std::chrono::steady_clock::time_point _startTime;
    bool                                _timerStarted;
    bool                                _gameOver;
};

#endif // GAMESTATE_HPP
