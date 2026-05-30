/**
 * SFMLRenderer.hpp — SFML 图形渲染器
 *
 * 使用 SFML 库绘制 25×25 网格的图形界面，支持：
 *   - 彩色矩形格子 + 白色文字
 *   - 鼠标点击选中格子（高亮边框）
 *   - 可合并邻格自动高亮提示
 *   - 方向键或点击邻格执行合并
 *   - 简易合并动画（颜色闪烁）
 *   - 排行榜面板、选题界面图形化
 *
 * 所属架构层：View（公有继承 IRenderer）
 * 所属模块：用户交互与渲染模块
 */

#ifndef SFMLRENDERER_HPP
#define SFMLRENDERER_HPP

#ifdef HAS_GUI

#include "view/IRenderer.hpp"
#include <SFML/Graphics.hpp>
#include <set>
#include <vector>
#include <string>
#include <utility>
#include <optional>
#include <memory>

/** SFMLRenderer 内部界面标识 */
enum class UIScreen { Menu, PuzzleList, Leaderboard, Result, Message,
                      Gameplay, NamePrompt, StartSelect, VSAIMenu, VSResult, LevelList };

class SFMLRenderer : public IRenderer {
public:
    SFMLRenderer();
    ~SFMLRenderer() override;

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
    void setTurnMessage(const std::string& msg) override;
    std::string promptPlayerName() override;
    std::vector<std::pair<int, int>> promptStartingCells(
        const Grid& grid, int count) override;

    UserAction waitForAction() override;
    bool isOpen() const override;
    void clearScreen() override;
    std::pair<int, int> getCursorPosition() const override;
    std::pair<int, int> getSelectedCell() const override;
    void clearSelection() override;

    // ---- 动画触发（由 Controller 在合并后调用） ----

    void triggerMergeAnim(int row, int col);

    // ---- 全屏切换 ----

    void toggleFullscreen();

private:
    // ---- 布局常量 ----
    static constexpr unsigned WINDOW_W = 1400;
    static constexpr unsigned WINDOW_H = 900;
    static constexpr float GRID_OFFSET_X = 30.0f;
    static constexpr float GRID_OFFSET_Y = 50.0f;
    static constexpr float PANEL_W = 320.0f;

    /** 动态计算右侧面板 X 坐标 */
    float panelX() const;

    // ---- 绘制辅助 ----

    void drawGrid(const GameState& state);
    void drawCellBg(int row, int col, const sf::Color& color);
    void drawCellText(int row, int col, const std::string& text, const sf::Color& color);
    void drawInfoPanel(const GameState& state);
    void drawRect(float x, float y, float w, float h,
                  const sf::Color& fill, const sf::Color& outline,
                  float outlineThickness = 1.0f);

    // ---- 坐标转换 ----

    std::pair<int, int> pixelToGrid(int px, int py) const;
    sf::Vector2f gridToPixel(int row, int col) const;

    // ---- 事件处理 ----

    UserAction processKeyEvent(const sf::Event::KeyPressed& keyEvent);
    UserAction processMouseEvent(const sf::Event::MouseButtonPressed& mouseEvent);

    // ---- 工具 ----

    sf::Color getCellColor(const Cell& cell) const;
    std::vector<std::pair<int, int>> getValidNeighbors(const Grid& grid,
                                                        int row, int col) const;
    std::vector<std::pair<int, int>> getEmptyNeighbors(const Grid& grid,
                                                        int row, int col) const;
    void centerText(sf::Text& text, float x, float y, float w, float h);

    // ---- 成员 ----

    sf::RenderWindow _window;
    sf::Font         _font;
    std::unique_ptr<sf::Text> _text;       // 复用的文本对象（延迟构造）
    sf::RectangleShape _rect;              // 复用的矩形对象
    std::optional<sf::Cursor> _arrowCursor;
    std::optional<sf::Cursor> _handCursor;

    // 布局
    float _cellSize;

    // 光标状态
    int _cursorRow, _cursorCol;
    int _gridRows, _gridCols;
    int _selectedRow, _selectedCol;
    bool _hasSelection;

    // 动画
    sf::Clock _animClock;
    int  _animRow, _animCol;
    bool _animating;

    // 杂项
    const GameState* _lastState = nullptr;  // 最近一次 render() 的状态（供 waitForAction 内重绘）
    UserAction _pendingAction;
    UIScreen   _screen;       // 当前显示的界面
    int        _puzzleCount;  // 题面列表项数（用于点击检测）
    int        _levelCount;   // 关卡列表项数
    bool       _fullscreen;   // 是否全屏模式
    std::string _turnMessage; // 当前回合提示消息
};

#endif // HAS_GUI
#endif // SFMLRENDERER_HPP
