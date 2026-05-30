/**
 * ConsoleRenderer.hpp — 终端渲染器
 *
 * 在 Windows 控制台中使用 ASCII 字符绘制 25×25 网格界面。
 * 使用 _getch() 捕获键盘方向键/回车等输入，转换为 UserAction。
 *
 * 网格绘制使用制表符/竖线/横线，格子用 "A1" 格式显示。
 * 空格子显示 " . "。
 * 选中格子加 [] 标记以突出显示。
 *
 * 所属架构层：View（公有继承 IRenderer）
 * 所属模块：用户交互与渲染模块
 */

#ifndef CONSOLERENDERER_HPP
#define CONSOLERENDERER_HPP

#include "view/IRenderer.hpp"
#include <windows.h>

class ConsoleRenderer : public IRenderer {
public:
    ConsoleRenderer();
    ~ConsoleRenderer() override;

    // ---- IRenderer 实现 ----

    void render(const GameState& state) override;
    void showMenu() override;
    void showPuzzleList(const std::vector<Puzzle>& puzzles) override;
    void showLevelList(const std::vector<Puzzle>& puzzles,
                       int maxUnlocked,
                       const std::vector<int>& bestScores) override;
    void showLeaderboard(const std::vector<ScoreRecord>& records) override;
    void showResult(const ScoreRecord& record,
                    const std::vector<Move>& moveHistory) override;
    void showMessage(const std::string& msg) override;
    void showVSAIMenu() override;
    void showVSResult(const ScoreRecord& playerRecord,
                      const ScoreRecord& aiRecord,
                      const std::string& winner) override;
    std::string promptPlayerName() override;
    std::vector<std::pair<int, int>> promptStartingCells(
        const Grid& grid, int count) override;

    UserAction waitForAction() override;
    bool isOpen() const override;
    void clearScreen() override;
    void setTurnMessage(const std::string& msg) override;

private:
    /** 绘制网格框架（边框 + 坐标轴）及每个 Cell 的内容 */
    void drawGrid(const GameState& state);
    void drawGrid(const Grid& grid);  // 用于 promptStartingCells（无归属信息）

    /** 设置控制台光标位置（列, 行） */
    void gotoxy(int x, int y);

    /** 设置文本颜色 */
    void setColor(WORD color);

    /** 解析单个键盘事件 */
    UserAction parseKey(int ch);

    HANDLE _hConsole;
    HANDLE _hInput;
    bool   _running;
    std::string _turnMessage;
};

#endif // CONSOLERENDERER_HPP
