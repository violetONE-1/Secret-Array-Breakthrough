/**
 * GameState.hpp — 游戏状态类 (GameState)
 *
 * 聚合当前一局游戏的全部运行时数据，是 Controller 查询/修改 Model 的统一入口。
 *
 * 职责：
 *   - 持有当前盘面（Grid）
 *   - 记录玩家选择的 5 个起始格子
 *   - 维护步数计数器与操作历史
 *   - 管理游戏计时（首次操作 → 提交/结束）
 *   - 计算正确率（正确步数 / 总步数）
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
#include <string>
#include <vector>
#include <utility>

class GameState {
public:
    /** 构造一局新游戏的初始状态 */
    GameState(const Grid& initialGrid,
              const std::vector<std::pair<int, int>>& playerStarts,
              const std::string& puzzleId);

    // ---- 盘面访问 ----

    const Grid& grid() const;   // 只读
    Grid&       grid();         // 可写（仅 Controller 使用）

    // ---- 基本信息 ----

    const std::string& puzzleId() const;

    const std::vector<std::pair<int, int>>& playerStarts() const;

    // ---- 有效棋子集合（active cells） ----

    /** 检查某格是否为当前有效的棋子起点 */
    bool isActiveCell(int row, int col) const;

    /** 获取全部有效棋子坐标（只读） */
    const std::set<std::pair<int, int>>& activeCells() const;

    /** 合并后更新有效棋子：移除源位置，加入目标位置 */
    void updateActiveCells(int fromRow, int fromCol, int toRow, int toCol);

    // ---- 操作记录 ----

    /** 记录一步有效操作，步数计数 +1 */
    void recordMove(const Move& move);

    /** 返回全部操作历史（只读） */
    const std::vector<Move>& moveHistory() const;

    // ---- 计时 ----

    /** 首次调用时记录起始时间；后续调用返回已过秒数 */
    double elapsedSeconds() const;

    // ---- 准确率 ----

    /**
     * 计算正确率 = 有效合并次数 / 总操作次数（含无效尝试）。
     * 此处“有效合并”即 moveHistory 中的每一条记录。
     * 返回值范围 [0.0, 1.0]。
     */
    double accuracy() const;

    // ---- 生命期 ----

    /** 是否有任何合法操作存在 */
    bool isOver() const;

    /** 检测死局：5个活跃棋子是否全部无合法邻居可合并 */
    bool isDeadEnd() const;

    /** 标记游戏结束 */
    void endGame();

    /** 返回本局总步数（合并 + 滑行） */
    int stepsTaken() const;

    /** 返回滑行步数 */
    int slideCount() const;

private:
    Grid                                _grid;
    std::string                         _puzzleId;
    std::vector<std::pair<int, int>>    _playerStarts;
    std::set<std::pair<int, int>>       _activeCells;
    int                                 _stepsTaken;
    int                                 _slideCount;
    std::vector<Move>                   _moveHistory;
    std::chrono::steady_clock::time_point _startTime;
    bool                                _timerStarted;
    bool                                _gameOver;
};

#endif // GAMESTATE_HPP
