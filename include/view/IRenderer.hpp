/**
 * IRenderer.hpp — 渲染器抽象接口
 *
 * 定义所有渲染器必须实现的纯虚函数接口。
 * 支持控制台 ASCII 和 SFML 图形两种实现的多态切换。
 *
 * 所属架构层：View
 * 所属模块：用户交互与渲染模块
 */

#ifndef IRENDERER_HPP
#define IRENDERER_HPP

#include "core/GameState.hpp"
#include "model/Puzzle.hpp"
#include "model/ScoreRecord.hpp"
#include <utility>
#include <vector>
#include <string>

/** 用户操作枚举 — 从任何输入设备抽象出的操作类型 */
enum class UserAction {
    NONE,
    QUIT,
    CONFIRM,
    BACK,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    SELECT_CELL,       // 选中/取消某格
    MERGE_DIRECTION,   // 朝指定方向合并
    SUBMIT,
    VIEW_LEADERBOARD,
    SELECT_PUZZLE_1,
    SELECT_PUZZLE_2,
    SELECT_PUZZLE_3,
    SELECT_PUZZLE_4,
    SELECT_PUZZLE_5,
    CUSTOM_PUZZLE,
    RESIZE_BOARD,
    TOGGLE_REPLAY,
    NEXT_FRAME,        // 回放中跳到下一步
};

class IRenderer {
public:
    virtual ~IRenderer() = default;

    // ---- 各阶段渲染 ----

    /** 渲染游戏中界面（网格 + 步数 + 计时） */
    virtual void render(const GameState& state) = 0;

    /** 显示主菜单 */
    virtual void showMenu() = 0;

    /** 显示题面选择列表 */
    virtual void showPuzzleList(const std::vector<Puzzle>& puzzles) = 0;

    /** 显示排行榜 */
    virtual void showLeaderboard(const std::vector<ScoreRecord>& records) = 0;

    /** 显示单次答题结果 */
    virtual void showResult(const ScoreRecord& record) = 0;

    /** 显示临时消息并等待用户按键 */
    virtual void showMessage(const std::string& msg) = 0;

    /** 提示用户输入玩家名 */
    virtual std::string promptPlayerName() = 0;

    /** 提示用户选择 5 个起始格子 */
    virtual std::vector<std::pair<int, int>> promptStartingCells(
        const Grid& grid, int count) = 0;

    // ---- 事件处理 ----

    /** 阻塞等待用户操作并返回 */
    virtual UserAction waitForAction() = 0;

    /** 返回渲染器内部的当前光标位置 (row, col)。
     *  仅 GUI 渲染器需要实现；控制台渲染器返回 {-1, -1}，
     *  因为控制台模式下光标由 GameController 自行管理。 */
    virtual std::pair<int, int> getCursorPosition() const { return {-1, -1}; }

    /** 检查窗口/控制台是否仍然有效 */
    virtual bool isOpen() const = 0;

    // ---- 杂项 ----

    /** 清屏 */
    virtual void clearScreen() = 0;
};

#endif // IRENDERER_HPP
