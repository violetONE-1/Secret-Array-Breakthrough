/**
 * GameController.hpp — 主控制器类 (GameController)
 *
 * 游戏的中枢神经系统，持有全部 Model 和 View 对象，编排游戏生命周期。
 *
 * 唯一所有关系（通过 unique_ptr）：
 *   - GameState：当前游戏状态
 *   - PuzzleManager：题库管理
 *   - Leaderboard：排行榜
 *   - ReplayBuffer：回放录制
 *   - IRenderer：渲染器（多态，运行时绑定 Console/SFML 实现）
 *   - InputHandler：输入处理器
 *
 * 游戏阶段状态机：
 *   MENU → PUZZLE_SELECT → GAMEPLAY → RESULT → LEADERBOARD → MENU
 *
 * 所属架构层：Controller
 * 所属模块：游戏流程控制模块
 */

#ifndef GAMECONTROLLER_HPP
#define GAMECONTROLLER_HPP

#include "core/GameState.hpp"
#include "core/Grid.hpp"
#include "core/MergeRule.hpp"
#include "core/Move.hpp"
#include "model/FileManager.hpp"
#include "model/PuzzleManager.hpp"
#include "model/Leaderboard.hpp"
#include "model/ReplayBuffer.hpp"
#include "model/Player.hpp"
#include "model/ScoreRecord.hpp"
#include "view/IRenderer.hpp"
#include "controller/InputHandler.hpp"
#include <memory>
#include <string>
#include <vector>

/** 游戏阶段枚举 */
enum class GamePhase {
    MENU,
    PUZZLE_SELECT,
    GAMEPLAY,
    RESULT,
    LEADERBOARD,
    REPLAY,
    QUIT
};

class GameController {
public:
    /**
     * 构造函数。
     * 初始化 FileManager → PuzzleManager → Leaderboard
     *           → ReplayBuffer → Renderer → InputHandler
     */
    GameController();

    /** 析构函数：unique_ptr 自动级联释放所有子对象 */
    ~GameController();

    /**
     * 启动游戏主循环。
     * 伪代码：
     *   while (_running) {
     *       switch (_phase) {
     *           case MENU:          handleMenu(); break;
     *           case PUZZLE_SELECT: handlePuzzleSelect(); break;
     *           case GAMEPLAY:      handleGameplay(); break;
     *           case RESULT:        handleResult(); break;
     *           case LEADERBOARD:   handleLeaderboard(); break;
     *           case REPLAY:        handleReplay(); break;
     *       }
     *   }
     */
    void run();

private:
    // ---- 各阶段处理函数 ----

    void handleMenu();
    void handlePuzzleSelect();
    void handleGameplay();
    void handleResult();
    void handleLeaderboard();

    /** 开始新游戏：初始化 GameState + ReplayBuffer */
    void startNewGame(const Puzzle& puzzle);

    /** 执行一步合并操作 */
    bool tryMerge(int srcRow, int srcCol, int dstRow, int dstCol);

    /** 提交答题结果 */
    void submitAnswer();

    // ---- 拥有 (unique_ptr) ----

    std::unique_ptr<FileManager>    _fileManager;
    std::unique_ptr<PuzzleManager>  _puzzleManager;
    std::unique_ptr<Leaderboard>    _leaderboard;
    std::unique_ptr<ReplayBuffer>   _replayBuffer;
    std::unique_ptr<IRenderer>      _renderer;
    std::unique_ptr<InputHandler>   _inputHandler;
    std::unique_ptr<GameState>      _state;

    // ---- 值语义 ----

    Player      _player;
    GamePhase   _phase;
    bool        _running;

    // ---- 光标状态（用于游戏中的格子导航） ----

    int _cursorRow;
    int _cursorCol;
    int _gridRows;
    int _gridCols;
};

#endif // GAMECONTROLLER_HPP
