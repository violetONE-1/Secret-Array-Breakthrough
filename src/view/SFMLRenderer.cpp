/**
 * SFMLRenderer.cpp — SFML 图形渲染器实现
 */

#ifdef HAS_GUI

#include "view/SFMLRenderer.hpp"
#include "core/MergeRule.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

// ================================================================
//  构造 / 析构
// ================================================================

SFMLRenderer::SFMLRenderer()
    : _window(sf::VideoMode({WINDOW_W, WINDOW_H}),
              "Matrix Breakthrough",
              sf::Style::Default,
              sf::State::Windowed)
    , _cellSize(26.0f)
    , _cursorRow(0), _cursorCol(0)
    , _gridRows(25), _gridCols(25)
    , _selectedRow(-1), _selectedCol(-1)
    , _hasSelection(false)
    , _animRow(-1), _animCol(-1)
    , _animating(false)
    , _pendingAction(UserAction::NONE)
    , _screen(UIScreen::Menu)
    , _puzzleCount(0)
    , _fullscreen(false)
{
    _window.setVerticalSyncEnabled(true);

    // 加载字体：优先使用项目自带字体，否则回退到系统字体
    std::string fontPaths[] = {
        "assets/fonts/NotoSansSC-Regular.ttf",
        "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };
    bool fontLoaded = false;
    for (const auto& path : fontPaths) {
        if (_font.openFromFile(path)) {
            fontLoaded = true;
            break;
        }
    }
    if (!fontLoaded) {
        std::cerr << "[SFMLRenderer] 警告：无法加载字体文件，文字可能无法正确显示"
                  << std::endl;
    }

    _text = std::make_unique<sf::Text>(_font, "", 14);
    _text->setFillColor(sf::Color(20, 20, 40));

    _arrowCursor = sf::Cursor::createFromSystem(sf::Cursor::Type::Arrow);
    _handCursor  = sf::Cursor::createFromSystem(sf::Cursor::Type::Hand);
    if (_arrowCursor) _window.setMouseCursor(*_arrowCursor);

    _rect.setSize(sf::Vector2f(_cellSize - 2, _cellSize - 2));
}

SFMLRenderer::~SFMLRenderer()
{
    if (_window.isOpen()) {
        _window.close();
    }
}

float SFMLRenderer::panelX() const
{
    auto size = _window.getSize();
    return static_cast<float>(size.x) - PANEL_W - 10.0f;
}

void SFMLRenderer::toggleFullscreen()
{
    _fullscreen = !_fullscreen;
    _window.close();
    if (_fullscreen) {
        _window.create(sf::VideoMode({WINDOW_W, WINDOW_H}),
                       "Matrix Breakthrough",
                       sf::Style::Default,
                       sf::State::Fullscreen);
    } else {
        _window.create(sf::VideoMode({WINDOW_W, WINDOW_H}),
                       "Matrix Breakthrough",
                       sf::Style::Default,
                       sf::State::Windowed);
    }
    _window.setVerticalSyncEnabled(true);
    _arrowCursor = sf::Cursor::createFromSystem(sf::Cursor::Type::Arrow);
    _handCursor  = sf::Cursor::createFromSystem(sf::Cursor::Type::Hand);
    if (_arrowCursor) _window.setMouseCursor(*_arrowCursor);
}

// ================================================================
//  主渲染：游戏中界面
// ================================================================

void SFMLRenderer::render(const GameState& state)
{
    _screen = UIScreen::Gameplay;
    _lastState = &state;
    const Grid& grid = state.grid();
    int rows = grid.rows();
    int cols = grid.cols();

    _gridRows = rows;
    _gridCols = cols;

    // 动态计算格子大小（基于当前实际窗口尺寸）
    auto winSize = _window.getSize();
    float px = panelX();
    float availW = px - GRID_OFFSET_X - 10.0f;
    float availH = static_cast<float>(winSize.y) - GRID_OFFSET_Y - 20.0f;
    _cellSize = std::min(availW / cols, availH / rows);
    if (_cellSize < 14.0f) _cellSize = 14.0f;
    if (_cellSize > 50.0f) _cellSize = 50.0f;

    _rect.setSize(sf::Vector2f(_cellSize - 2, _cellSize - 2));

    _window.clear(sf::Color(215, 228, 245));  // 淡蓝色背景

    drawGrid(grid, &state.activeCells());
    drawInfoPanel(state);

    // 合并动画
    if (_animating && _animClock.getElapsedTime().asMilliseconds() < 300) {
        float t = _animClock.getElapsedTime().asMilliseconds() / 300.0f;
        int alpha = static_cast<int>(255 * (1.0f - t));
        sf::Vector2f pos = gridToPixel(_animRow, _animCol);
        _rect.setPosition(pos);
        _rect.setFillColor(sf::Color(255, 200, 50, alpha));
        _rect.setOutlineThickness(0);
        _window.draw(_rect);
    } else {
        _animating = false;
    }

    _window.display();
}

// ================================================================
//  网格绘制
// ================================================================

void SFMLRenderer::drawGrid(const Grid& grid,
                            const std::set<std::pair<int, int>>* activeCells)
{
    int rows = grid.rows();
    int cols = grid.cols();

    // 列标题
    _text->setCharacterSize(11);
    _text->setFillColor(sf::Color(80, 80, 100));
    for (int c = 0; c < cols; ++c) {
        sf::Vector2f pos = gridToPixel(0, c);
        _text->setString(std::to_string(c));
        centerText(*_text, pos.x, GRID_OFFSET_Y - 16.0f, _cellSize, 14.0f);
        _window.draw(*_text);
    }

    // 行标题
    for (int r = 0; r < rows; ++r) {
        sf::Vector2f pos = gridToPixel(r, 0);
        _text->setString(std::to_string(r));
        centerText(*_text, GRID_OFFSET_X - 30.0f, pos.y, 26.0f, _cellSize);
        _window.draw(*_text);
    }

    // 收集有效邻居（如果选中了格子）
    std::vector<std::pair<int, int>> validNeighbors;
    if (_hasSelection) {
        validNeighbors = getValidNeighbors(grid, _selectedRow, _selectedCol);
    }

    // 绘制每个格子
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const Cell& cell = grid.at(r, c);
            sf::Vector2f pos = gridToPixel(r, c);

            bool isCursor = (r == _cursorRow && c == _cursorCol);
            bool isSelected = (r == _selectedRow && c == _selectedCol);
            bool isActive = activeCells &&
                            activeCells->find({r, c}) != activeCells->end();
            bool isValidTarget = false;
            for (const auto& n : validNeighbors) {
                if (n.first == r && n.second == c) {
                    isValidTarget = true;
                    break;
                }
            }

            // 背景色：白底为主
            sf::Color bgColor;
            if (cell.isEmpty()) {
                bgColor = sf::Color(225, 228, 235);  // 空格：浅灰
            } else if (isSelected) {
                bgColor = sf::Color(255, 255, 210);  // 选中：浅黄
            } else if (isCursor) {
                bgColor = sf::Color(190, 210, 240);  // 光标：浅蓝
            } else {
                bgColor = sf::Color::White;           // 普通格：白底
            }

            drawCellBg(r, c, bgColor);

            // 有效棋子高亮（金黄色粗边框，最优先）
            if (isActive) {
                _rect.setPosition(pos);
                _rect.setSize(sf::Vector2f(_cellSize - 2, _cellSize - 2));
                _rect.setFillColor(sf::Color::Transparent);
                _rect.setOutlineColor(sf::Color(255, 180, 30));
                _rect.setOutlineThickness(3.0f);
                _window.draw(_rect);
            }

            // 选中格边框（浅蓝边框）
            if (isSelected) {
                _rect.setPosition(pos);
                _rect.setSize(sf::Vector2f(_cellSize - 2, _cellSize - 2));
                _rect.setFillColor(sf::Color::Transparent);
                _rect.setOutlineColor(sf::Color(60, 130, 230));
                _rect.setOutlineThickness(2.0f);
                _window.draw(_rect);
            } else if (isCursor && !isActive) {
                _rect.setPosition(pos);
                _rect.setSize(sf::Vector2f(_cellSize - 2, _cellSize - 2));
                _rect.setFillColor(sf::Color::Transparent);
                _rect.setOutlineColor(sf::Color(80, 80, 100));
                _rect.setOutlineThickness(1.5f);
                _window.draw(_rect);
            }

            // 有效目标高亮（绿色边框）
            if (isValidTarget) {
                _rect.setPosition(pos);
                _rect.setSize(sf::Vector2f(_cellSize - 2, _cellSize - 2));
                _rect.setFillColor(sf::Color::Transparent);
                _rect.setOutlineColor(sf::Color(40, 180, 40));
                _rect.setOutlineThickness(2.5f);
                _window.draw(_rect);
            }

            // 文字：黑字
            if (cell.isEmpty()) {
                drawCellText(r, c, ".", sf::Color(170, 170, 180));
            } else {
                std::string str;
                str += cell.getLetter();
                str += std::to_string(cell.getNumber());
                drawCellText(r, c, str, sf::Color(20, 20, 30));
            }
        }
    }

    _text->setCharacterSize(14);
}

sf::Color SFMLRenderer::getCellColor(const Cell& cell) const
{
    if (cell.isEmpty()) return sf::Color(225, 228, 235);
    return sf::Color::White;
}

void SFMLRenderer::drawCellBg(int row, int col, const sf::Color& color)
{
    sf::Vector2f pos = gridToPixel(row, col);
    _rect.setPosition(pos);
    _rect.setSize(sf::Vector2f(_cellSize - 2, _cellSize - 2));
    _rect.setFillColor(color);
    _rect.setOutlineThickness(0);
    _window.draw(_rect);
}

void SFMLRenderer::drawCellText(int row, int col,
                                 const std::string& str,
                                 const sf::Color& color)
{
    sf::Vector2f pos = gridToPixel(row, col);

    // 动态字体大小
    unsigned fontSize = static_cast<unsigned>(_cellSize * 0.5f);
    if (fontSize < 8) fontSize = 8;
    if (fontSize > 20) fontSize = 20;
    _text->setCharacterSize(fontSize);
    _text->setString(str);
    _text->setFillColor(color);
    centerText(*_text, pos.x, pos.y, _cellSize - 2, _cellSize - 2);
    _window.draw(*_text);
    _text->setCharacterSize(14);  // 恢复默认
}

// ================================================================
//  信息面板
// ================================================================

void SFMLRenderer::drawInfoPanel(const GameState& state)
{
    float px = panelX();
    auto winSize = _window.getSize();

    // 面板背景
    _rect.setPosition(sf::Vector2f(px, 0));
    _rect.setSize(sf::Vector2f(PANEL_W, static_cast<float>(winSize.y)));
    _rect.setFillColor(sf::Color(225, 236, 250));
    _rect.setOutlineThickness(0);
    _window.draw(_rect);

    _text->setCharacterSize(20);
    _text->setFillColor(sf::Color(30, 70, 170));
    _text->setString("Matrix Breakthrough");
    _text->setPosition(sf::Vector2f(px + 20, 20));
    _window.draw(*_text);

    _text->setCharacterSize(14);
    _text->setFillColor(sf::Color(80, 80, 100));

    float y = 60.0f;
    auto drawLine = [&](const std::string& label, const std::string& value) {
        _text->setString(label);
        _text->setPosition(sf::Vector2f(px + 15, y));
        _window.draw(*_text);
        _text->setFillColor(sf::Color(20, 20, 40));
        _text->setString(value);
        _text->setPosition(sf::Vector2f(px + 120, y));
        _window.draw(*_text);
        _text->setFillColor(sf::Color(80, 80, 100));
        y += 28.0f;
    };

    drawLine("Puzzle:", state.puzzleId());
    drawLine("Steps:", std::to_string(state.stepsTaken()));

    std::ostringstream timeStr;
    timeStr << std::fixed << std::setprecision(1) << state.elapsedSeconds() << " s";
    drawLine("Time:", timeStr.str());

    drawLine("Grid:", std::to_string(state.grid().rows()) + "x" +
             std::to_string(state.grid().cols()));

    // 有效棋子数
    _text->setFillColor(sf::Color(200, 130, 20));
    _text->setString("Pieces:");
    _text->setPosition(sf::Vector2f(px + 15, y));
    _window.draw(*_text);
    _text->setFillColor(sf::Color(20, 20, 40));
    _text->setString(std::to_string(state.activeCells().size()));
    _text->setPosition(sf::Vector2f(px + 120, y));
    _window.draw(*_text);
    _text->setFillColor(sf::Color(80, 80, 100));
    y += 28.0f;

    _text->setCharacterSize(14);

    // 选中信息
    y += 20.0f;
    if (_hasSelection) {
        _text->setFillColor(sf::Color(30, 70, 170));
        _text->setString("Selected:");
        _text->setPosition(sf::Vector2f(px + 15, y));
        _window.draw(*_text);

        const Cell& selCell = state.grid().at(_selectedRow, _selectedCol);
        std::string selStr;
        selStr += selCell.getLetter();
        selStr += std::to_string(selCell.getNumber());
        _text->setString("(" + std::to_string(_selectedRow) + "," +
                        std::to_string(_selectedCol) + ") " + selStr);
        _text->setPosition(sf::Vector2f(px + 90, y));
        _window.draw(*_text);
        y += 24.0f;

        // 显示合法邻居数
        auto neighbors = getValidNeighbors(state.grid(), _selectedRow, _selectedCol);
        _text->setFillColor(sf::Color(30, 150, 30));
        _text->setString("Valid targets: " + std::to_string(neighbors.size()));
        _text->setPosition(sf::Vector2f(px + 15, y));
        _window.draw(*_text);
        y += 24.0f;
    }

    // 操作提示
    y = static_cast<float>(winSize.y) - 200.0f;
    _text->setFillColor(sf::Color(140, 140, 160));
    _text->setCharacterSize(12);

    auto drawHint = [&](const std::string& hint) {
        _text->setString(hint);
        _text->setPosition(sf::Vector2f(px + 15, y));
        _window.draw(*_text);
        y += 20.0f;
    };

    drawHint("Controls:");
    drawHint("  Arrow keys: Move cursor");
    drawHint("  Click: Select cell");
    drawHint("  Click neighbor: Merge");
    drawHint("  Enter/Space: Merge 1st valid");
    drawHint("  S: Submit answer");
    drawHint("  E: AI move (one step)");
    drawHint("  Q/Esc: Back to menu");
    drawHint("  F11: Toggle fullscreen");

    _text->setCharacterSize(14);
}

// ================================================================
//  菜单界面
// ================================================================

void SFMLRenderer::showMenu()
{
    _screen = UIScreen::Menu;
    _window.clear(sf::Color(215, 228, 245));

    auto winSize = _window.getSize();
    float ww = static_cast<float>(winSize.x);

    _text->setCharacterSize(42);
    _text->setFillColor(sf::Color(30, 70, 170));
    _text->setString("Matrix Breakthrough");
    centerText(*_text, 0, 120, ww, 60);
    _window.draw(*_text);

    // 菜单按钮
    struct Btn { std::string text; float y; UserAction action; };
    Btn buttons[] = {
        {"1. Start Game (Select Puzzle)", 240, UserAction::SELECT_PUZZLE_1},
        {"2. View Leaderboard",          310, UserAction::VIEW_LEADERBOARD},
        {"3. Exit",                      380, UserAction::QUIT}
    };

    for (const auto& btn : buttons) {
        float bx = ww / 2.0f - 200.0f;

        _rect.setPosition(sf::Vector2f(bx, btn.y));
        _rect.setSize(sf::Vector2f(400, 50));
        _rect.setFillColor(sf::Color::White);
        _rect.setOutlineColor(sf::Color(40, 120, 220));
        _rect.setOutlineThickness(2.0f);
        _window.draw(_rect);

        _text->setCharacterSize(18);
        _text->setFillColor(sf::Color(20, 20, 40));
        _text->setString(btn.text);
        centerText(*_text, bx, btn.y, 400, 50);
        _window.draw(*_text);
    }

    _text->setCharacterSize(14);
    _text->setFillColor(sf::Color(140, 140, 160));
    _text->setString("Press 1-3 or click buttons  |  F11: Toggle fullscreen");
    centerText(*_text, 0, 470, ww, 30);
    _window.draw(*_text);

    _window.display();
}

// ================================================================
//  题面选择界面
// ================================================================

void SFMLRenderer::showPuzzleList(const std::vector<Puzzle>& puzzles)
{
    _screen = UIScreen::PuzzleList;
    _puzzleCount = static_cast<int>(puzzles.size());
    _window.clear(sf::Color(215, 228, 245));

    auto winSize = _window.getSize();
    float ww = static_cast<float>(winSize.x);

    _text->setCharacterSize(28);
    _text->setFillColor(sf::Color(30, 70, 170));
    _text->setString("Select Puzzle");
    centerText(*_text, 0, 50, ww, 40);
    _window.draw(*_text);

    float startY = 130.0f;
    for (size_t i = 0; i < puzzles.size(); ++i) {
        float y = startY + i * 70.0f;
        float bx = ww / 2.0f - 280.0f;

        _rect.setPosition(sf::Vector2f(bx, y));
        _rect.setSize(sf::Vector2f(560, 56));
        _rect.setFillColor(sf::Color::White);
        _rect.setOutlineColor(sf::Color(40, 120, 220));
        _rect.setOutlineThickness(1.5f);
        _window.draw(_rect);

        _text->setCharacterSize(18);
        _text->setFillColor(sf::Color(20, 20, 40));
        _text->setString(std::to_string(i + 1) + ". " + puzzles[i].name());
        _text->setPosition(sf::Vector2f(bx + 20, y + 6));
        _window.draw(*_text);

        _text->setCharacterSize(13);
        _text->setFillColor(sf::Color(120, 120, 140));
        _text->setString(puzzles[i].id() + "  |  " +
                        std::to_string(puzzles[i].gridSize()) + "x" +
                        std::to_string(puzzles[i].gridSize()));
        _text->setPosition(sf::Vector2f(bx + 20, y + 30));
        _window.draw(*_text);
    }

    // 返回按钮
    float backY = startY + puzzles.size() * 70.0f + 30.0f;
    float bx = ww / 2.0f - 120.0f;
    _rect.setPosition(sf::Vector2f(bx, backY));
    _rect.setSize(sf::Vector2f(240, 40));
    _rect.setFillColor(sf::Color::White);
    _rect.setOutlineColor(sf::Color(40, 120, 220));
    _rect.setOutlineThickness(1.5f);
    _window.draw(_rect);

    _text->setCharacterSize(16);
    _text->setFillColor(sf::Color(20, 20, 40));
    _text->setString("B. Back to Menu");
    centerText(*_text, bx, backY, 240, 40);
    _window.draw(*_text);

    _text->setCharacterSize(14);
    _text->setFillColor(sf::Color(140, 140, 160));
    _text->setString("Press 1-" + std::to_string(puzzles.size()) +
                    " to select, B to go back");
    centerText(*_text, 0, backY + 60, ww, 30);
    _window.draw(*_text);

    _window.display();
}

// ================================================================
//  排行榜界面
// ================================================================

void SFMLRenderer::showLeaderboard(const std::vector<ScoreRecord>& records)
{
    _screen = UIScreen::Leaderboard;
    _window.clear(sf::Color(215, 228, 245));

    auto winSize = _window.getSize();
    float ww = static_cast<float>(winSize.x);

    _text->setCharacterSize(28);
    _text->setFillColor(sf::Color(30, 70, 170));
    _text->setString("Leaderboard");
    centerText(*_text, 0, 30, ww, 40);
    _window.draw(*_text);

    // 表头
    float headerY = 80.0f;
    float colX[] = {40.0f, 80.0f, 340.0f, 470.0f, 560.0f, 650.0f, 750.0f};
    const char* headers[] = {"#", "Player", "Puzzle", "Time(s)", "Steps", "Acc%", "Score"};
    _text->setCharacterSize(14);
    _text->setFillColor(sf::Color(60, 60, 80));
    for (int i = 0; i < 7; ++i) {
        _text->setString(headers[i]);
        _text->setPosition(sf::Vector2f(colX[i], headerY));
        _window.draw(*_text);
    }

    // 分隔线
    _rect.setPosition(sf::Vector2f(30, headerY + 22));
    _rect.setSize(sf::Vector2f(ww - 60.0f, 1));
    _rect.setFillColor(sf::Color(180, 180, 200));
    _rect.setOutlineThickness(0);
    _window.draw(_rect);

    // 记录行
    float rowY = headerY + 32.0f;
    for (size_t i = 0; i < records.size() && i < 20; ++i) {
        const auto& r = records[i];

        sf::Color rowColor = (i < 3) ? sf::Color(200, 130, 20)
                                     : sf::Color(60, 60, 80);
        _text->setFillColor(rowColor);
        _text->setCharacterSize(13);

        _text->setString(std::to_string(i + 1));
        _text->setPosition(sf::Vector2f(colX[0], rowY));
        _window.draw(*_text);

        _text->setString(r.playerName());
        _text->setPosition(sf::Vector2f(colX[1], rowY));
        _window.draw(*_text);

        _text->setString(r.puzzleId());
        _text->setPosition(sf::Vector2f(colX[2], rowY));
        _window.draw(*_text);

        std::ostringstream ts;
        ts << std::fixed << std::setprecision(2) << r.timeSeconds();
        _text->setString(ts.str());
        _text->setPosition(sf::Vector2f(colX[3], rowY));
        _window.draw(*_text);

        _text->setString(std::to_string(r.steps()));
        _text->setPosition(sf::Vector2f(colX[4], rowY));
        _window.draw(*_text);

        std::ostringstream as;
        as << std::fixed << std::setprecision(0) << (r.accuracy() * 100);
        _text->setString(as.str());
        _text->setPosition(sf::Vector2f(colX[5], rowY));
        _window.draw(*_text);

        _text->setFillColor(sf::Color(200, 130, 20));
        _text->setString(std::to_string(r.score()));
        _text->setPosition(sf::Vector2f(colX[6], rowY));
        _window.draw(*_text);

        rowY += 24.0f;
    }

    if (records.empty()) {
        _text->setFillColor(sf::Color(140, 140, 160));
        _text->setString("No records yet.");
        _text->setPosition(sf::Vector2f(ww / 2.0f - 80, rowY));
        _window.draw(*_text);
    }

    // 底部提示
    _text->setCharacterSize(14);
    _text->setFillColor(sf::Color(140, 140, 160));
    _text->setString("Press any key to return");
    centerText(*_text, 0, static_cast<float>(winSize.y) - 50.0f, ww, 30);
    _window.draw(*_text);

    _window.display();
}

// ================================================================
//  结果展示
// ================================================================

void SFMLRenderer::showResult(const ScoreRecord& record,
                               const std::vector<Move>& moveHistory)
{
    _screen = UIScreen::Result;
    auto winSize = _window.getSize();
    float ww = static_cast<float>(winSize.x);
    float wh = static_cast<float>(winSize.y);

    int historyScrollOffset = 0;
    sf::RectangleShape scrollTrack, scrollThumb;

    auto cellName = [](int row, int col) -> std::string {
        return std::string(1, static_cast<char>('A' + col))
               + std::to_string(row + 1);
    };

    while (_window.isOpen()) {
        while (const auto event = _window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                _window.close();
                return;
            }
            if (event->is<sf::Event::KeyPressed>()) {
                return;
            }
            if (const auto* wheelEvt =
                    event->getIf<sf::Event::MouseWheelScrolled>()) {
                historyScrollOffset -= static_cast<int>(wheelEvt->delta * 3);
            }
        }

        // 计算可见行数
        const float histListY = 160.0f;
        const float histListH = wh - 240.0f;
        const float lineHeight = 22.0f;
        int maxVisible = static_cast<int>(histListH / lineHeight);
        int totalMoves = static_cast<int>(moveHistory.size());
        int maxScroll = std::max(0, totalMoves - maxVisible);
        if (historyScrollOffset < 0) historyScrollOffset = 0;
        if (historyScrollOffset > maxScroll) historyScrollOffset = maxScroll;

        // ---- 绘制 ----
        _window.clear(sf::Color(215, 228, 245));

        // 标题
        _text->setCharacterSize(30);
        _text->setFillColor(sf::Color(30, 70, 170));
        _text->setString("Result");
        centerText(*_text, 0, 40, ww, 50);
        _window.draw(*_text);

        // ---- 左侧：得分数据面板 ----
        float leftX = 80.0f;
        float statY = 120.0f;
        auto drawStat = [&](const std::string& label, const std::string& value,
                            const sf::Color& vColor = sf::Color(20, 20, 40)) {
            _text->setCharacterSize(20);
            _text->setFillColor(sf::Color(80, 80, 100));
            _text->setString(label);
            _text->setPosition(sf::Vector2f(leftX, statY));
            _window.draw(*_text);

            _text->setFillColor(vColor);
            _text->setString(value);
            _text->setPosition(sf::Vector2f(leftX + 150.0f, statY));
            _window.draw(*_text);
            statY += 42.0f;
        };

        drawStat("Player:", record.playerName());
        drawStat("Puzzle:", record.puzzleId());
        {
            std::ostringstream ts;
            ts << static_cast<int>(record.timeSeconds()) << " seconds";
            drawStat("Time:", ts.str());
        }
        drawStat("Steps:", std::to_string(record.steps()));
        {
            std::ostringstream as;
            as << std::fixed << std::setprecision(0)
               << (record.accuracy() * 100) << "%";
            drawStat("Accuracy:", as.str());
        }
        drawStat("Score:", std::to_string(record.score()), sf::Color(200, 130, 20));

        // ---- 右侧：操作历史面板 ----
        float histX = 540.0f;
        float histW = ww - histX - 60.0f;

        _text->setCharacterSize(22);
        _text->setFillColor(sf::Color(60, 60, 80));
        _text->setString("Operation History");
        _text->setPosition(sf::Vector2f(histX, 100.0f));
        _window.draw(*_text);

        // 历史区域背景
        _rect.setPosition(sf::Vector2f(histX, histListY));
        _rect.setSize(sf::Vector2f(histW, histListH));
        _rect.setFillColor(sf::Color(225, 236, 250));
        _rect.setOutlineColor(sf::Color(180, 190, 210));
        _rect.setOutlineThickness(1.0f);
        _window.draw(_rect);

        // 绘制可见的每一步
        _text->setCharacterSize(16);
        for (int i = 0; i < maxVisible && (historyScrollOffset + i) < totalMoves; ++i) {
            int moveIdx = historyScrollOffset + i;
            const auto& m = moveHistory[moveIdx];

            std::ostringstream line;
            line << std::setw(2) << (moveIdx + 1) << ".  "
                 << cellName(m.srcRow, m.srcCol) << " -> "
                 << cellName(m.dstRow, m.dstCol) << "  ["
                 << directionToString(m.direction) << "]  = "
                 << m.resultLetter << m.resultNumber;

            _text->setString(line.str());
            _text->setFillColor(sf::Color(20, 20, 40));
            _text->setPosition(sf::Vector2f(histX + 14.0f,
                                             histListY + 8.0f + i * lineHeight));
            _window.draw(*_text);
        }

        if (totalMoves == 0) {
            _text->setString("(No moves recorded)");
            _text->setFillColor(sf::Color(140, 140, 160));
            _text->setPosition(sf::Vector2f(histX + 14.0f, histListY + 12.0f));
            _window.draw(*_text);
        }

        // 滚动条
        if (maxScroll > 0) {
            float trackX = histX + histW - 14.0f;
            float trackH = histListH - 4.0f;
            scrollTrack.setPosition(sf::Vector2f(trackX, histListY + 2.0f));
            scrollTrack.setSize(sf::Vector2f(8.0f, trackH));
            scrollTrack.setFillColor(sf::Color(200, 205, 215));
            _window.draw(scrollTrack);

            float thumbH = trackH * static_cast<float>(maxVisible) / totalMoves;
            if (thumbH < 20.0f) thumbH = 20.0f;
            float thumbY = histListY + 2.0f
                + (trackH - thumbH) * historyScrollOffset / maxScroll;
            scrollThumb.setPosition(sf::Vector2f(trackX, thumbY));
            scrollThumb.setSize(sf::Vector2f(8.0f, thumbH));
            scrollThumb.setFillColor(sf::Color(140, 150, 170));
            _window.draw(scrollThumb);
        }

        // 底部提示
        _text->setCharacterSize(14);
        _text->setFillColor(sf::Color(140, 140, 160));
        _text->setString("Press any key to return  |  Scroll to view history");
        centerText(*_text, 0, wh - 50.0f, ww, 30);
        _window.draw(*_text);

        _window.display();
    }
}

// ================================================================
//  消息 / 输入提示
// ================================================================

void SFMLRenderer::showMessage(const std::string& msg)
{
    _screen = UIScreen::Message;
    _window.clear(sf::Color(215, 228, 245));

    auto winSize = _window.getSize();
    float ww = static_cast<float>(winSize.x);
    float wh = static_cast<float>(winSize.y);

    _text->setCharacterSize(20);
    _text->setFillColor(sf::Color(20, 20, 40));
    _text->setString(msg);
    centerText(*_text, 0, wh / 2.0f - 30, ww, 40);
    _window.draw(*_text);

    _text->setCharacterSize(14);
    _text->setFillColor(sf::Color(140, 140, 160));
    _text->setString("Press any key to continue");
    centerText(*_text, 0, wh / 2.0f + 20, ww, 30);
    _window.draw(*_text);

    _window.display();

    while (_window.isOpen()) {
        while (const auto event = _window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                _window.close();
                return;
            }
            if (event->is<sf::Event::KeyPressed>()) {
                return;
            }
        }
        sf::sleep(sf::milliseconds(10));
    }
}

std::string SFMLRenderer::promptPlayerName()
{
    auto winSize = _window.getSize();
    float ww = static_cast<float>(winSize.x);

    std::string name;
    bool inputFocused = true;
    sf::Clock cursorClock;

    const float boxW = 420.0f, boxH = 46.0f;
    const float boxX = ww / 2.0f - boxW / 2.0f;
    const float boxY = 305.0f;
    const float boxRadius = 8.0f;

    sf::RectangleShape cursorLine;

    auto drawRoundedRect = [&](float x, float y, float w, float h, float r,
                                const sf::Color& fill, const sf::Color& outline,
                                float outlineThk) {
        const int pts = 8;
        sf::ConvexShape shape(4 * pts);
        struct Corner { float cx, cy, startAngle; };
        Corner corners[] = {
            {x + r,     y + r,     3.14159f},
            {x + w - r, y + r,     -3.14159f / 2.0f},
            {x + w - r, y + h - r, 0.0f},
            {x + r,     y + h - r, 3.14159f / 2.0f},
        };
        for (int c = 0; c < 4; ++c) {
            for (int i = 0; i < pts; ++i) {
                float angle = corners[c].startAngle
                            + (3.14159f / 2.0f) * i / (pts - 1);
                shape.setPoint(c * pts + i, {
                    corners[c].cx + r * std::cos(angle),
                    corners[c].cy + r * std::sin(angle)
                });
            }
        }
        shape.setFillColor(fill);
        shape.setOutlineColor(outline);
        shape.setOutlineThickness(outlineThk);
        _window.draw(shape);
    };

    while (_window.isOpen()) {
        while (const auto event = _window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                _window.close();
                return "Player";
            }

            if (const auto* mouseEvt = event->getIf<sf::Event::MouseButtonPressed>()) {
                float mx = static_cast<float>(mouseEvt->position.x);
                float my = static_cast<float>(mouseEvt->position.y);
                inputFocused = (mx >= boxX && mx <= boxX + boxW &&
                                my >= boxY && my <= boxY + boxH);
            }

            if (const auto* textEvt = event->getIf<sf::Event::TextEntered>()) {
                if (!inputFocused) continue;
                if (textEvt->unicode == '\r' || textEvt->unicode == '\n') {
                    if (name.empty()) name = "Player";
                    return name;
                }
                if (textEvt->unicode == '\b') {
                    if (!name.empty()) name.pop_back();
                } else if (textEvt->unicode >= 32 && textEvt->unicode < 127) {
                    if (name.size() < 30) {
                        name += static_cast<char>(textEvt->unicode);
                    }
                }
            }
        }

        // ---- 每帧重绘 ----
        _window.clear(sf::Color(215, 228, 245));

        _text->setCharacterSize(24);
        _text->setFillColor(sf::Color(30, 70, 170));
        _text->setString("Enter Your Name");
        centerText(*_text, 0, 200, ww, 40);
        _window.draw(*_text);

        _text->setCharacterSize(16);
        _text->setFillColor(sf::Color(80, 80, 100));
        _text->setString("Click the input box and type your name");
        centerText(*_text, 0, 255, ww, 30);
        _window.draw(*_text);

        // 圆角输入框
        sf::Color borderColor = inputFocused
            ? sf::Color(40, 120, 220) : sf::Color(170, 180, 195);
        float borderThk = inputFocused ? 2.5f : 1.5f;
        drawRoundedRect(boxX, boxY, boxW, boxH, boxRadius,
                        sf::Color::White, borderColor, borderThk);

        // 输入文字（左对齐 + 内边距）
        _text->setCharacterSize(22);
        _text->setFillColor(sf::Color(20, 20, 40));
        _text->setString(name);
        _text->setPosition(sf::Vector2f(boxX + 14.0f, boxY + boxH / 2.0f - 14.0f));
        _window.draw(*_text);

        // 闪烁竖线光标
        bool showCursor = inputFocused
            && (cursorClock.getElapsedTime().asMilliseconds() % 800 < 400);
        if (showCursor) {
            float textW = _text->getLocalBounds().size.x;
            float cursorX = boxX + 14.0f + textW + 2.0f;
            float cursorY = boxY + 10.0f;
            cursorLine.setPosition(sf::Vector2f(cursorX, cursorY));
            cursorLine.setSize(sf::Vector2f(2.0f, boxH - 20.0f));
            cursorLine.setFillColor(sf::Color(40, 120, 220));
            _window.draw(cursorLine);
        }

        _text->setCharacterSize(14);
        _text->setFillColor(sf::Color(140, 140, 160));
        _text->setString("Press Enter to confirm");
        centerText(*_text, 0, 390, ww, 30);
        _window.draw(*_text);

        _window.display();
    }
    return "Player";
}

std::vector<std::pair<int, int>> SFMLRenderer::promptStartingCells(
    const Grid& grid, int count)
{
    std::vector<std::pair<int, int>> starts = {
        {0, 0},
        {grid.rows() / 2, grid.cols() / 2},
        {0, grid.cols() - 1},
        {grid.rows() - 1, 0},
        {grid.rows() - 1, grid.cols() - 1}
    };

    int rows = grid.rows();
    int cols = grid.cols();
    int curRow = rows / 2, curCol = cols / 2;

    auto winSize = _window.getSize();
    float px = panelX();
    float cellSz = std::min(
        (px - GRID_OFFSET_X - 10.0f) / cols,
        (static_cast<float>(winSize.y) - GRID_OFFSET_Y - 80.0f) / rows
    );
    if (cellSz < 14.0f) cellSz = 14.0f;
    if (cellSz > 50.0f) cellSz = 50.0f;

    _rect.setSize(sf::Vector2f(cellSz - 2, cellSz - 2));

    while (_window.isOpen()) {
        while (const auto event = _window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                _window.close();
                return starts;
            }
            if (const auto* keyEvt = event->getIf<sf::Event::KeyPressed>()) {
                switch (keyEvt->code) {
                    case sf::Keyboard::Key::Up:    if (curRow > 0) curRow--; break;
                    case sf::Keyboard::Key::Down:  if (curRow < rows - 1) curRow++; break;
                    case sf::Keyboard::Key::Left:  if (curCol > 0) curCol--; break;
                    case sf::Keyboard::Key::Right: if (curCol < cols - 1) curCol++; break;
                    case sf::Keyboard::Key::Space: {
                        auto it = std::find(starts.begin(), starts.end(),
                                           std::make_pair(curRow, curCol));
                        if (it != starts.end()) {
                            starts.erase(it);
                        } else if (static_cast<int>(starts.size()) < count) {
                            starts.emplace_back(curRow, curCol);
                        }
                        break;
                    }
                    case sf::Keyboard::Key::Enter:
                        if (static_cast<int>(starts.size()) == count) return starts;
                        break;
                    case sf::Keyboard::Key::Escape:
                        return starts;
                    default: break;
                }
            }
        }

        // 绘制
        _window.clear(sf::Color(215, 228, 245));

        _text->setCharacterSize(18);
        _text->setFillColor(sf::Color(30, 70, 170));
        _text->setString("Select " + std::to_string(count) +
                        " starting cells (Space=select, Enter=confirm)");
        centerText(*_text, 0, 10, static_cast<float>(winSize.x), 30);
        _window.draw(*_text);

        // 显示已选
        std::ostringstream selStr;
        selStr << "Selected: ";
        for (size_t i = 0; i < starts.size(); ++i) {
            if (i > 0) selStr << ", ";
            selStr << "(" << starts[i].first << "," << starts[i].second << ")";
        }
        _text->setCharacterSize(13);
        _text->setFillColor(sf::Color(80, 80, 100));
        _text->setString(selStr.str());
        centerText(*_text, 0, 32, static_cast<float>(winSize.x), 20);
        _window.draw(*_text);

        // 绘制网格
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                float x = GRID_OFFSET_X + c * cellSz;
                float y = GRID_OFFSET_Y + r * cellSz;

                const Cell& cell = grid.at(r, c);
                sf::Color bgColor;
                if (cell.isEmpty()) {
                    bgColor = sf::Color(225, 228, 235);
                } else {
                    bgColor = sf::Color::White;
                }

                auto isStart = [&](int r, int c) {
                    for (const auto& s : starts)
                        if (s.first == r && s.second == c) return true;
                    return false;
                };

                _rect.setPosition(sf::Vector2f(x, y));
                _rect.setSize(sf::Vector2f(cellSz - 2, cellSz - 2));
                _rect.setFillColor(bgColor);

                if (isStart(r, c)) {
                    _rect.setOutlineColor(sf::Color(255, 180, 30));
                    _rect.setOutlineThickness(3.0f);
                } else if (r == curRow && c == curCol) {
                    _rect.setOutlineColor(sf::Color(80, 80, 100));
                    _rect.setOutlineThickness(1.5f);
                } else {
                    _rect.setOutlineThickness(0);
                }
                _window.draw(_rect);

                if (!cell.isEmpty()) {
                    std::string str;
                    str += cell.getLetter();
                    str += std::to_string(cell.getNumber());
                    unsigned fs = static_cast<unsigned>(cellSz * 0.5f);
                    if (fs < 8) fs = 8;
                    _text->setCharacterSize(fs);
                    _text->setFillColor(sf::Color(20, 20, 30));
                    _text->setString(str);
                    centerText(*_text, x, y, cellSz - 2, cellSz - 2);
                    _window.draw(*_text);
                }
            }
        }

        _window.display();
    }
    return starts;
}

// ================================================================
//  事件处理
// ================================================================

UserAction SFMLRenderer::waitForAction()
{
    while (_window.isOpen()) {
        while (const auto event = _window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                _window.close();
                return UserAction::QUIT;
            }

            if (const auto* resizeEvt = event->getIf<sf::Event::Resized>()) {
                sf::View view(sf::FloatRect({0, 0},
                    {static_cast<float>(resizeEvt->size.x),
                     static_cast<float>(resizeEvt->size.y)}));
                _window.setView(view);
            }

            if (const auto* keyEvt = event->getIf<sf::Event::KeyPressed>()) {
                UserAction a = processKeyEvent(*keyEvt);
                if (a != UserAction::NONE) return a;
            }

            if (const auto* mouseEvt = event->getIf<sf::Event::MouseButtonPressed>()) {
                UserAction a = processMouseEvent(*mouseEvt);
                if (a != UserAction::NONE) return a;
            }
        }

        // 游戏界面持续重绘，使光标移动、选中状态即时可见
        if (_screen == UIScreen::Gameplay && _lastState) {
            render(*_lastState);
        }

        sf::sleep(sf::milliseconds(10));
    }
    return UserAction::QUIT;
}

UserAction SFMLRenderer::processKeyEvent(const sf::Event::KeyPressed& keyEvent)
{
    switch (keyEvent.code) {
        case sf::Keyboard::Key::Up:
            if (_cursorRow > 0) _cursorRow--;
            return UserAction::NONE;  // 内化：不干扰 Controller
        case sf::Keyboard::Key::Down:
            if (_cursorRow < _gridRows - 1) _cursorRow++;
            return UserAction::NONE;
        case sf::Keyboard::Key::Left:
            if (_cursorCol > 0) _cursorCol--;
            return UserAction::NONE;
        case sf::Keyboard::Key::Right:
            if (_cursorCol < _gridCols - 1) _cursorCol++;
            return UserAction::NONE;
        case sf::Keyboard::Key::Enter: return UserAction::CONFIRM;
        case sf::Keyboard::Key::Space:
            // 两段式：无选中→选中；同格再按→取消；邻格→合并；非邻格→重选
            if (!_hasSelection) {
                _selectedRow = _cursorRow;
                _selectedCol = _cursorCol;
                _hasSelection = true;
                return UserAction::NONE;
            }
            if (_cursorRow == _selectedRow && _cursorCol == _selectedCol) {
                _hasSelection = false;  // 同一格：取消选中
                return UserAction::NONE;
            }
            {
                int dr = std::abs(_cursorRow - _selectedRow);
                int dc = std::abs(_cursorCol - _selectedCol);
                if (dr + dc == 1) {
                    // 相邻格：执行合并
                    _hasSelection = false;
                    return UserAction::SELECT_CELL;
                }
                // 不相邻：重选到新位置
                _selectedRow = _cursorRow;
                _selectedCol = _cursorCol;
                return UserAction::NONE;
            }
        case sf::Keyboard::Key::Escape:
            // 先清除选中态，已无选中则返回 BACK
            if (_hasSelection) {
                _hasSelection = false;
                return UserAction::NONE;
            }
            return UserAction::BACK;
        case sf::Keyboard::Key::F11:
            toggleFullscreen();
            return UserAction::NONE;
        case sf::Keyboard::Key::S:     return UserAction::SUBMIT;
        case sf::Keyboard::Key::Q:     return UserAction::QUIT;
        case sf::Keyboard::Key::L:     return UserAction::VIEW_LEADERBOARD;
        case sf::Keyboard::Key::Num1: case sf::Keyboard::Key::Numpad1:
            return UserAction::SELECT_PUZZLE_1;
        case sf::Keyboard::Key::Num2: case sf::Keyboard::Key::Numpad2:
            return UserAction::SELECT_PUZZLE_2;
        case sf::Keyboard::Key::Num3: case sf::Keyboard::Key::Numpad3:
            return UserAction::SELECT_PUZZLE_3;
        case sf::Keyboard::Key::Num4: case sf::Keyboard::Key::Numpad4:
            return UserAction::SELECT_PUZZLE_4;
        case sf::Keyboard::Key::Num5: case sf::Keyboard::Key::Numpad5:
            return UserAction::SELECT_PUZZLE_5;
        case sf::Keyboard::Key::B:
            return UserAction::BACK;
        case sf::Keyboard::Key::E:
            return UserAction::AI_EXECUTE;
        default:
            return UserAction::NONE;
    }
}

UserAction SFMLRenderer::processMouseEvent(const sf::Event::MouseButtonPressed& mouseEvent)
{
    if (mouseEvent.button != sf::Mouse::Button::Left) return UserAction::NONE;

    int mx = mouseEvent.position.x;
    int my = mouseEvent.position.y;

    switch (_screen) {

    case UIScreen::Gameplay:
    case UIScreen::StartSelect:
        // ---- 游戏内网格点击 ----
        if (mx >= GRID_OFFSET_X && mx < panelX() - 10 &&
            my >= GRID_OFFSET_Y && my < _window.getSize().y - 10) {

            auto [row, col] = pixelToGrid(mx, my);
            if (row >= 0 && col >= 0 && row < _gridRows && col < _gridCols) {
                _cursorRow = row;
                _cursorCol = col;

                if (_hasSelection) {
                    int dr = row - _selectedRow;
                    int dc = col - _selectedCol;
                    if ((std::abs(dr) + std::abs(dc)) == 1) {
                        return UserAction::SELECT_CELL;
                    }
                    if (dr == 0 && dc == 0) {
                        _hasSelection = false;
                        return UserAction::NONE;
                    }
                }

                _selectedRow = row;
                _selectedCol = col;
                _hasSelection = true;
                return UserAction::NONE;
            }
        }
        break;

    case UIScreen::Menu: {
        // ---- 主菜单按钮点击 ----
        float ww = static_cast<float>(_window.getSize().x);
        float bx = ww / 2.0f - 200.0f;
        float bw = 400.0f, bh = 50.0f;

        auto inside = [&](float by) {
            return mx >= bx && mx <= bx + bw && my >= by && my <= by + bh;
        };

        if (inside(240.0f))      return UserAction::SELECT_PUZZLE_1;
        if (inside(310.0f))      return UserAction::VIEW_LEADERBOARD;
        if (inside(380.0f))      return UserAction::QUIT;
        break;
    }

    case UIScreen::PuzzleList: {
        // ---- 题面列表点击 ----
        float ww = static_cast<float>(_window.getSize().x);
        float startY = 130.0f;
        float listBx = ww / 2.0f - 280.0f;
        float listBw = 560.0f, listBh = 56.0f;

        for (int i = 0; i < _puzzleCount; ++i) {
            float by = startY + i * 70.0f;
            if (mx >= listBx && mx <= listBx + listBw &&
                my >= by && my <= by + listBh) {
                switch (i) {
                    case 0: return UserAction::SELECT_PUZZLE_1;
                    case 1: return UserAction::SELECT_PUZZLE_2;
                    case 2: return UserAction::SELECT_PUZZLE_3;
                    case 3: return UserAction::SELECT_PUZZLE_4;
                    case 4: return UserAction::SELECT_PUZZLE_5;
                    default: return UserAction::NONE;
                }
            }
        }

        // 返回按钮
        float backY = startY + _puzzleCount * 70.0f + 30.0f;
        float backBx = ww / 2.0f - 120.0f;
        if (mx >= backBx && mx <= backBx + 240.0f &&
            my >= backY && my <= backY + 40.0f) {
            return UserAction::BACK;
        }
        break;
    }

    default:
        // Leaderboard / Result / Message / NamePrompt：任意点击视为确认/继续
        return UserAction::CONFIRM;
    }

    return UserAction::NONE;
}

// ================================================================
//  动画
// ================================================================

void SFMLRenderer::triggerMergeAnim(int row, int col)
{
    _animRow = row;
    _animCol = col;
    _animating = true;
    _animClock.restart();
}

// ================================================================
//  坐标转换
// ================================================================

std::pair<int, int> SFMLRenderer::pixelToGrid(int px, int py) const
{
    int col = static_cast<int>((px - GRID_OFFSET_X) / _cellSize);
    int row = static_cast<int>((py - GRID_OFFSET_Y) / _cellSize);
    return {row, col};
}

sf::Vector2f SFMLRenderer::gridToPixel(int row, int col) const
{
    float x = GRID_OFFSET_X + col * _cellSize;
    float y = GRID_OFFSET_Y + row * _cellSize;
    return sf::Vector2f(x, y);
}

// ================================================================
//  有效邻居查找
// ================================================================

std::vector<std::pair<int, int>> SFMLRenderer::getValidNeighbors(
    const Grid& grid, int row, int col) const
{
    std::vector<std::pair<int, int>> result;
    if (row < 0 || col < 0) return result;
















































































    
    const Cell& center = grid.at(row, col);
    if (center.isEmpty()) return result;

    static const int dirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
    for (int d = 0; d < 4; ++d) {
        int nr = row + dirs[d][0];
        int nc = col + dirs[d][1];
        if (nr >= 0 && nr < grid.rows() && nc >= 0 && nc < grid.cols()) {
            const Cell& neighbor = grid.at(nr, nc);
            if (!neighbor.isEmpty() && MergeRule::canMerge(center, neighbor)) {
                result.emplace_back(nr, nc);
            }
        }
    }
    return result;
}

// ================================================================
//  杂项
// ================================================================

bool SFMLRenderer::isOpen() const
{
    return _window.isOpen();
}

std::pair<int, int> SFMLRenderer::getCursorPosition() const
{
    return {_cursorRow, _cursorCol};
}

std::pair<int, int> SFMLRenderer::getSelectedCell() const
{
    if (_hasSelection) {
        return {_selectedRow, _selectedCol};
    }
    return {-1, -1};
}

void SFMLRenderer::clearSelection()
{
    _hasSelection = false;
}

void SFMLRenderer::clearScreen()
{
    _window.clear(sf::Color(20, 20, 30));
}

void SFMLRenderer::drawRect(float x, float y, float w, float h,
                             const sf::Color& fill, const sf::Color& outline,
                             float outlineThickness)
{
    _rect.setPosition(sf::Vector2f(x, y));
    _rect.setSize(sf::Vector2f(w, h));
    _rect.setFillColor(fill);
    _rect.setOutlineColor(outline);
    _rect.setOutlineThickness(outlineThickness);
    _window.draw(_rect);
}

void SFMLRenderer::centerText(sf::Text& text, float x, float y, float w, float h)
{
    sf::FloatRect bounds = text.getLocalBounds();
    float tx = x + (w - bounds.size.x) / 2.0f - bounds.position.x;
    float ty = y + (h - bounds.size.y) / 2.0f - bounds.position.y;
    text.setPosition(sf::Vector2f(tx, ty));
}

// ================================================================
//  VS AI 难度选择界面
// ================================================================

void SFMLRenderer::showVSAIMenu()
{
    _screen = UIScreen::VSAIMenu;
    _window.clear(sf::Color(215, 228, 245));

    auto winSize = _window.getSize();
    float ww = static_cast<float>(winSize.x);

    _text->setCharacterSize(32);
    _text->setFillColor(sf::Color(30, 70, 170));
    _text->setString("VS AI");
    centerText(*_text, 0, 100, ww, 50);
    _window.draw(*_text);

    struct Btn { std::string text; float y; };
    Btn buttons[] = {
        {"1. Normal Mode (AI Random)",   240},
        {"2. Advanced Mode (AI Greedy)", 310},
        {"3. Back to Menu",              380}
    };

    for (const auto& btn : buttons) {
        float bx = ww / 2.0f - 240.0f;

        _rect.setPosition(sf::Vector2f(bx, btn.y));
        _rect.setSize(sf::Vector2f(480, 50));
        _rect.setFillColor(sf::Color::White);
        _rect.setOutlineColor(sf::Color(40, 120, 220));
        _rect.setOutlineThickness(2.0f);
        _window.draw(_rect);

        _text->setCharacterSize(18);
        _text->setFillColor(sf::Color(20, 20, 40));
        _text->setString(btn.text);
        centerText(*_text, bx, btn.y, 480, 50);
        _window.draw(*_text);
    }

    _text->setCharacterSize(14);
    _text->setFillColor(sf::Color(140, 140, 160));
    _text->setString("Press 1-3 or click buttons to choose");
    centerText(*_text, 0, 470, ww, 30);
    _window.draw(*_text);

    _window.display();
}

// ================================================================
//  VS AI 对战结果界面
// ================================================================

void SFMLRenderer::showVSResult(const ScoreRecord& playerRecord,
                                 const ScoreRecord& aiRecord,
                                 const std::string& winner)
{
    _screen = UIScreen::VSResult;
    auto winSize = _window.getSize();
    float ww = static_cast<float>(winSize.x);
    float wh = static_cast<float>(winSize.y);

    while (_window.isOpen()) {
        while (const auto event = _window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                _window.close();
                return;
            }
            if (event->is<sf::Event::KeyPressed>()) return;
            if (event->is<sf::Event::MouseButtonPressed>()) return;
        }

        _window.clear(sf::Color(215, 228, 245));

        _text->setCharacterSize(28);
        _text->setFillColor(sf::Color(30, 70, 170));
        _text->setString("VS AI Result");
        centerText(*_text, 0, 40, ww, 50);
        _window.draw(*_text);

        _text->setCharacterSize(22);
        _text->setFillColor(sf::Color(200, 130, 20));
        _text->setString("Winner: " + winner);
        centerText(*_text, 0, 110, ww, 40);
        _window.draw(*_text);

        float colX[] = {160.0f, 350.0f, 540.0f};
        float headerY = 170.0f;

        const char* headers[] = {"Stat", "Player", "AI"};
        _text->setCharacterSize(16);
        _text->setFillColor(sf::Color(60, 60, 80));
        for (int i = 0; i < 3; ++i) {
            _text->setString(headers[i]);
            _text->setPosition(sf::Vector2f(colX[i], headerY));
            _window.draw(*_text);
        }

        _rect.setPosition(sf::Vector2f(colX[0], headerY + 22));
        _rect.setSize(sf::Vector2f(ww - 2 * colX[0], 1));
        _rect.setFillColor(sf::Color(180, 180, 200));
        _rect.setOutlineThickness(0);
        _window.draw(_rect);

        auto drawRow = [&](float y, const std::string& label,
                           const std::string& pVal, const std::string& aVal,
                           const sf::Color& color = sf::Color(20, 20, 40)) {
            _text->setCharacterSize(16);
            _text->setFillColor(sf::Color(80, 80, 100));
            _text->setString(label);
            _text->setPosition(sf::Vector2f(colX[0], y));
            _window.draw(*_text);
            _text->setFillColor(color);
            _text->setString(pVal);
            _text->setPosition(sf::Vector2f(colX[1], y));
            _window.draw(*_text);
            _text->setString(aVal);
            _text->setPosition(sf::Vector2f(colX[2], y));
            _window.draw(*_text);
        };

        float rowY = headerY + 30.0f;
        drawRow(rowY, "Name:", playerRecord.playerName(), aiRecord.playerName());
        rowY += 28.0f;
        {
            std::ostringstream pTime, aTime;
            pTime << static_cast<int>(playerRecord.timeSeconds()) << " s";
            aTime << static_cast<int>(aiRecord.timeSeconds()) << " s";
            drawRow(rowY, "Time:", pTime.str(), aTime.str());
        }
        rowY += 28.0f;
        drawRow(rowY, "Steps:",
                std::to_string(playerRecord.steps()),
                std::to_string(aiRecord.steps()));
        rowY += 28.0f;
        drawRow(rowY, "Score:",
                std::to_string(playerRecord.score()),
                std::to_string(aiRecord.score()),
                sf::Color(200, 130, 20));

        _text->setCharacterSize(14);
        _text->setFillColor(sf::Color(140, 140, 160));
        _text->setString("Press any key or click to return");
        centerText(*_text, 0, wh - 60.0f, ww, 30);
        _window.draw(*_text);

        _window.display();
    }
}

#endif // HAS_GUI
