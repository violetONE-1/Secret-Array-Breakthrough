/**
 * SFMLRenderer.cpp — SFML 图形渲染器实现
 */

#ifdef HAS_GUI

#include "view/SFMLRenderer.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

// ================================================================
//  构造 / 析构
// ================================================================

SFMLRenderer::SFMLRenderer()
    : _window(sf::VideoMode(WINDOW_W, WINDOW_H),
              "Mi Zhen Tu Wei - Matrix Breakthrough",
              sf::Style::Titlebar | sf::Style::Close)
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
        if (_font.loadFromFile(path)) {
            fontLoaded = true;
            break;
        }
    }
    if (!fontLoaded) {
        // 最终回退：尝试加载任意字体
        // SFML 不自带默认字体，这里写入错误提示
        std::cerr << "[SFMLRenderer] 警告：无法加载字体文件，文字可能无法正确显示"
                  << std::endl;
    }

    _text.setFont(_font);
    _text.setCharacterSize(14);
    _text.setFillColor(sf::Color::White);

    // 加载光标
    _arrowCursor.loadFromSystem(sf::Cursor::Arrow);
    _handCursor.loadFromSystem(sf::Cursor::Hand);
    _window.setMouseCursor(_arrowCursor);

    // 初始化复用矩形
    _rect.setSize(sf::Vector2f(_cellSize - 2, _cellSize - 2));
}

SFMLRenderer::~SFMLRenderer()
{
    if (_window.isOpen()) {
        _window.close();
    }
}

// ================================================================
//  主渲染：游戏中界面
// ================================================================

void SFMLRenderer::render(const GameState& state)
{
    _screen = UIScreen::Gameplay;
    const Grid& grid = state.grid();
    int rows = grid.rows();
    int cols = grid.cols();

    // 同步内部光标边界
    _gridRows = rows;
    _gridCols = cols;

    // 动态计算格子大小
    float availW = PANEL_X - GRID_OFFSET_X - 10.0f;
    float availH = WINDOW_H - GRID_OFFSET_Y - 20.0f;
    _cellSize = std::min(availW / cols, availH / rows);
    if (_cellSize < 18.0f) _cellSize = 18.0f;
    if (_cellSize > 40.0f) _cellSize = 40.0f;

    _rect.setSize(sf::Vector2f(_cellSize - 2, _cellSize - 2));

    _window.clear(sf::Color(20, 20, 30));  // 深蓝黑背景

    drawGrid(grid);
    drawInfoPanel(state);

    // 合并动画
    if (_animating && _animClock.getElapsedTime().asMilliseconds() < 300) {
        float t = _animClock.getElapsedTime().asMilliseconds() / 300.0f;
        int alpha = static_cast<int>(255 * (1.0f - t));
        sf::Vector2f pos = gridToPixel(_animRow, _animCol);
        _rect.setPosition(pos);
        _rect.setFillColor(sf::Color(255, 255, 100, alpha));
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

void SFMLRenderer::drawGrid(const Grid& grid)
{
    int rows = grid.rows();
    int cols = grid.cols();

    // 列标题
    _text.setCharacterSize(11);
    _text.setFillColor(sf::Color(150, 150, 150));
    for (int c = 0; c < cols; ++c) {
        sf::Vector2f pos = gridToPixel(0, c);
        _text.setString(std::to_string(c));
        centerText(_text, pos.x, GRID_OFFSET_Y - 16.0f, _cellSize, 14.0f);
        _window.draw(_text);
    }

    // 行标题
    for (int r = 0; r < rows; ++r) {
        sf::Vector2f pos = gridToPixel(r, 0);
        _text.setString(std::to_string(r));
        centerText(_text, GRID_OFFSET_X - 30.0f, pos.y, 26.0f, _cellSize);
        _window.draw(_text);
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
            bool isValidTarget = false;
            for (const auto& n : validNeighbors) {
                if (n.first == r && n.second == c) {
                    isValidTarget = true;
                    break;
                }
            }

            // 背景色
            sf::Color bgColor;
            if (cell.isEmpty()) {
                bgColor = sf::Color(40, 40, 50);  // 空格：深灰
            } else if (isSelected && isCursor) {
                bgColor = sf::Color(100, 80, 20);  // 选中+光标：深金
            } else if (isSelected) {
                bgColor = sf::Color(80, 60, 20);   // 选中：暗金
            } else if (isCursor) {
                bgColor = sf::Color(60, 60, 90);   // 光标：亮灰蓝
            } else {
                bgColor = getCellColor(cell);       // 正常颜色
            }

            drawCellBg(r, c, bgColor);

            // 边框
            if (isSelected) {
                _rect.setPosition(pos);
                _rect.setSize(sf::Vector2f(_cellSize - 2, _cellSize - 2));
                _rect.setFillColor(sf::Color::Transparent);
                _rect.setOutlineColor(sf::Color(255, 220, 50));
                _rect.setOutlineThickness(2.5f);
                _window.draw(_rect);
            } else if (isCursor) {
                _rect.setPosition(pos);
                _rect.setSize(sf::Vector2f(_cellSize - 2, _cellSize - 2));
                _rect.setFillColor(sf::Color::Transparent);
                _rect.setOutlineColor(sf::Color::White);
                _rect.setOutlineThickness(1.5f);
                _window.draw(_rect);
            }

            // 有效目标高亮
            if (isValidTarget) {
                _rect.setPosition(pos);
                _rect.setSize(sf::Vector2f(_cellSize - 2, _cellSize - 2));
                _rect.setFillColor(sf::Color::Transparent);
                _rect.setOutlineColor(sf::Color(100, 255, 100));
                _rect.setOutlineThickness(2.0f);
                _window.draw(_rect);
            }

            // 文字
            if (cell.isEmpty()) {
                drawCellText(r, c, ".", sf::Color(80, 80, 90));
            } else {
                std::string str;
                str += cell.getLetter();
                str += std::to_string(cell.getNumber());
                drawCellText(r, c, str, sf::Color::White);
            }
        }
    }

    _text.setCharacterSize(14);
}

sf::Color SFMLRenderer::getCellColor(const Cell& cell) const
{
    if (cell.isEmpty()) return sf::Color(40, 40, 50);

    // 根据字母映射颜色（A..Z 对应色相环）
    int idx = cell.getLetter() - 'A';
    float hue = idx * (360.0f / 26.0f);

    // HSL -> RGB 简化转换（6 段色相）
    auto hslToRgb = [](float h, float s, float l) -> sf::Color {
        float c = (1.0f - std::abs(2.0f * l - 1.0f)) * s;
        float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
        float m = l - c / 2.0f;
        float r, g, b;
        if (h < 60)       { r = c; g = x; b = 0; }
        else if (h < 120) { r = x; g = c; b = 0; }
        else if (h < 180) { r = 0; g = c; b = x; }
        else if (h < 240) { r = 0; g = x; b = c; }
        else if (h < 300) { r = x; g = 0; b = c; }
        else              { r = c; g = 0; b = x; }
        return sf::Color(
            static_cast<sf::Uint8>((r + m) * 255),
            static_cast<sf::Uint8>((g + m) * 255),
            static_cast<sf::Uint8>((b + m) * 255)
        );
    };

    return hslToRgb(hue, 0.7f, 0.25f);  // 低亮度、中饱和度
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
    _text.setCharacterSize(fontSize);
    _text.setString(str);
    _text.setFillColor(color);
    centerText(_text, pos.x, pos.y, _cellSize - 2, _cellSize - 2);
    _window.draw(_text);
    _text.setCharacterSize(14);  // 恢复默认
}

// ================================================================
//  信息面板
// ================================================================

void SFMLRenderer::drawInfoPanel(const GameState& state)
{
    // 面板背景
    _rect.setPosition(sf::Vector2f(PANEL_X, 0));
    _rect.setSize(sf::Vector2f(PANEL_W, static_cast<float>(WINDOW_H)));
    _rect.setFillColor(sf::Color(30, 30, 42));
    _rect.setOutlineThickness(0);
    _window.draw(_rect);

    _text.setCharacterSize(20);
    _text.setFillColor(sf::Color(255, 200, 50));
    _text.setString("Mi Zhen Tu Wei");
    _text.setPosition(sf::Vector2f(PANEL_X + 20, 20));
    _window.draw(_text);

    _text.setCharacterSize(14);
    _text.setFillColor(sf::Color(200, 200, 200));

    float y = 60.0f;
    auto drawLine = [&](const std::string& label, const std::string& value) {
        _text.setString(label);
        _text.setPosition(sf::Vector2f(PANEL_X + 15, y));
        _window.draw(_text);
        _text.setFillColor(sf::Color::White);
        _text.setString(value);
        _text.setPosition(sf::Vector2f(PANEL_X + 120, y));
        _window.draw(_text);
        _text.setFillColor(sf::Color(200, 200, 200));
        y += 28.0f;
    };

    drawLine("Puzzle:", state.puzzleId());
    drawLine("Steps:", std::to_string(state.stepsTaken()));

    std::ostringstream timeStr;
    timeStr << std::fixed << std::setprecision(1) << state.elapsedSeconds() << " s";
    drawLine("Time:", timeStr.str());

    drawLine("Grid:", std::to_string(state.grid().rows()) + "x" +
             std::to_string(state.grid().cols()));

    _text.setCharacterSize(14);

    // 选中信息
    y += 20.0f;
    if (_hasSelection) {
        _text.setFillColor(sf::Color(255, 220, 50));
        _text.setString("Selected:");
        _text.setPosition(sf::Vector2f(PANEL_X + 15, y));
        _window.draw(_text);

        const Cell& selCell = state.grid().at(_selectedRow, _selectedCol);
        std::string selStr;
        selStr += selCell.getLetter();
        selStr += std::to_string(selCell.getNumber());
        _text.setString("(" + std::to_string(_selectedRow) + "," +
                        std::to_string(_selectedCol) + ") " + selStr);
        _text.setPosition(sf::Vector2f(PANEL_X + 90, y));
        _window.draw(_text);
        y += 24.0f;

        // 显示合法邻居数
        auto neighbors = getValidNeighbors(state.grid(), _selectedRow, _selectedCol);
        _text.setFillColor(sf::Color(100, 255, 100));
        _text.setString("Valid targets: " + std::to_string(neighbors.size()));
        _text.setPosition(sf::Vector2f(PANEL_X + 15, y));
        _window.draw(_text);
        y += 24.0f;
    }

    // 操作提示
    y = static_cast<float>(WINDOW_H) - 200.0f;
    _text.setFillColor(sf::Color(150, 150, 150));
    _text.setCharacterSize(12);

    auto drawHint = [&](const std::string& hint) {
        _text.setString(hint);
        _text.setPosition(sf::Vector2f(PANEL_X + 15, y));
        _window.draw(_text);
        y += 20.0f;
    };

    drawHint("Controls:");
    drawHint("  Arrow keys: Move cursor");
    drawHint("  Click: Select cell");
    drawHint("  Click neighbor: Merge");
    drawHint("  Enter/Space: Merge 1st valid");
    drawHint("  S: Submit answer");
    drawHint("  Q/Esc: Back to menu");

    _text.setCharacterSize(14);
}

// ================================================================
//  菜单界面
// ================================================================

void SFMLRenderer::showMenu()
{
    _screen = UIScreen::Menu;
    _window.clear(sf::Color(20, 20, 30));

    _text.setCharacterSize(36);
    _text.setFillColor(sf::Color(255, 200, 50));
    _text.setString("Mi Zhen Tu Wei");
    centerText(_text, 0, 80, static_cast<float>(WINDOW_W), 50);
    _window.draw(_text);

    _text.setCharacterSize(20);
    _text.setFillColor(sf::Color(200, 200, 200));
    _text.setString("Matrix Breakthrough");
    centerText(_text, 0, 120, static_cast<float>(WINDOW_W), 30);
    _window.draw(_text);

    // 菜单按钮
    struct Btn { std::string text; float y; UserAction action; };
    Btn buttons[] = {
        {"1. Start Game (Select Puzzle)", 220, UserAction::SELECT_PUZZLE_1},
        {"2. View Leaderboard",          280, UserAction::VIEW_LEADERBOARD},
        {"3. Exit",                      340, UserAction::QUIT}
    };

    for (const auto& btn : buttons) {
        float bx = WINDOW_W / 2.0f - 180.0f;
        sf::FloatRect btnRect(bx, btn.y, 360, 44);

        _rect.setPosition(sf::Vector2f(bx, btn.y));
        _rect.setSize(sf::Vector2f(360, 44));
        _rect.setFillColor(sf::Color(50, 50, 70));
        _rect.setOutlineColor(sf::Color(100, 100, 140));
        _rect.setOutlineThickness(1.0f);
        _window.draw(_rect);

        _text.setCharacterSize(18);
        _text.setFillColor(sf::Color::White);
        _text.setString(btn.text);
        centerText(_text, bx, btn.y, 360, 44);
        _window.draw(_text);
    }

    _text.setCharacterSize(14);
    _text.setFillColor(sf::Color(150, 150, 150));
    _text.setString("Press 1-3 or click buttons");
    centerText(_text, 0, 420, static_cast<float>(WINDOW_W), 30);
    _window.draw(_text);

    _window.display();
}

// ================================================================
//  题面选择界面
// ================================================================

void SFMLRenderer::showPuzzleList(const std::vector<Puzzle>& puzzles)
{
    _screen = UIScreen::PuzzleList;
    _puzzleCount = static_cast<int>(puzzles.size());
    _window.clear(sf::Color(20, 20, 30));

    _text.setCharacterSize(28);
    _text.setFillColor(sf::Color(255, 200, 50));
    _text.setString("Select Puzzle");
    centerText(_text, 0, 50, static_cast<float>(WINDOW_W), 40);
    _window.draw(_text);

    float startY = 130.0f;
    for (size_t i = 0; i < puzzles.size(); ++i) {
        float y = startY + i * 70.0f;
        float bx = WINDOW_W / 2.0f - 250.0f;

        _rect.setPosition(sf::Vector2f(bx, y));
        _rect.setSize(sf::Vector2f(500, 56));
        _rect.setFillColor(sf::Color(45, 45, 65));
        _rect.setOutlineColor(sf::Color(90, 90, 130));
        _rect.setOutlineThickness(1.0f);
        _window.draw(_rect);

        _text.setCharacterSize(18);
        _text.setFillColor(sf::Color::White);
        _text.setString(std::to_string(i + 1) + ". " + puzzles[i].name());
        _text.setPosition(sf::Vector2f(bx + 20, y + 6));
        _window.draw(_text);

        _text.setCharacterSize(13);
        _text.setFillColor(sf::Color(160, 160, 160));
        _text.setString(puzzles[i].id() + "  |  " +
                        std::to_string(puzzles[i].gridSize()) + "x" +
                        std::to_string(puzzles[i].gridSize()));
        _text.setPosition(sf::Vector2f(bx + 20, y + 30));
        _window.draw(_text);
    }

    // 返回按钮
    float backY = startY + puzzles.size() * 70.0f + 30.0f;
    float bx = WINDOW_W / 2.0f - 100.0f;
    _rect.setPosition(sf::Vector2f(bx, backY));
    _rect.setSize(sf::Vector2f(200, 40));
    _rect.setFillColor(sf::Color(50, 50, 70));
    _rect.setOutlineColor(sf::Color(100, 100, 140));
    _rect.setOutlineThickness(1.0f);
    _window.draw(_rect);

    _text.setCharacterSize(16);
    _text.setFillColor(sf::Color::White);
    _text.setString("B. Back to Menu");
    centerText(_text, bx, backY, 200, 40);
    _window.draw(_text);

    _text.setCharacterSize(14);
    _text.setFillColor(sf::Color(150, 150, 150));
    _text.setString("Press 1-" + std::to_string(puzzles.size()) +
                    " to select, B to go back");
    centerText(_text, 0, backY + 60, static_cast<float>(WINDOW_W), 30);
    _window.draw(_text);

    _window.display();
}

// ================================================================
//  排行榜界面
// ================================================================

void SFMLRenderer::showLeaderboard(const std::vector<ScoreRecord>& records)
{
    _screen = UIScreen::Leaderboard;
    _window.clear(sf::Color(20, 20, 30));

    _text.setCharacterSize(28);
    _text.setFillColor(sf::Color(255, 200, 50));
    _text.setString("Leaderboard");
    centerText(_text, 0, 30, static_cast<float>(WINDOW_W), 40);
    _window.draw(_text);

    // 表头
    float headerY = 80.0f;
    float colX[] = {40.0f, 80.0f, 340.0f, 470.0f, 560.0f, 650.0f, 750.0f};
    const char* headers[] = {"#", "Player", "Puzzle", "Time(s)", "Steps", "Acc%", "Score"};
    _text.setCharacterSize(14);
    _text.setFillColor(sf::Color(200, 200, 200));
    for (int i = 0; i < 7; ++i) {
        _text.setString(headers[i]);
        _text.setPosition(sf::Vector2f(colX[i], headerY));
        _window.draw(_text);
    }

    // 分隔线
    _rect.setPosition(sf::Vector2f(30, headerY + 22));
    _rect.setSize(sf::Vector2f(WINDOW_W - 60.0f, 1));
    _rect.setFillColor(sf::Color(100, 100, 130));
    _rect.setOutlineThickness(0);
    _window.draw(_rect);

    // 记录行
    float rowY = headerY + 32.0f;
    for (size_t i = 0; i < records.size() && i < 20; ++i) {
        const auto& r = records[i];

        sf::Color rowColor = (i < 3) ? sf::Color(255, 220, 100)
                                     : sf::Color(200, 200, 200);
        _text.setFillColor(rowColor);
        _text.setCharacterSize(13);

        _text.setString(std::to_string(i + 1));
        _text.setPosition(sf::Vector2f(colX[0], rowY));
        _window.draw(_text);

        _text.setString(r.playerName());
        _text.setPosition(sf::Vector2f(colX[1], rowY));
        _window.draw(_text);

        _text.setString(r.puzzleId());
        _text.setPosition(sf::Vector2f(colX[2], rowY));
        _window.draw(_text);

        std::ostringstream ts;
        ts << std::fixed << std::setprecision(2) << r.timeSeconds();
        _text.setString(ts.str());
        _text.setPosition(sf::Vector2f(colX[3], rowY));
        _window.draw(_text);

        _text.setString(std::to_string(r.steps()));
        _text.setPosition(sf::Vector2f(colX[4], rowY));
        _window.draw(_text);

        std::ostringstream as;
        as << std::fixed << std::setprecision(0) << (r.accuracy() * 100);
        _text.setString(as.str());
        _text.setPosition(sf::Vector2f(colX[5], rowY));
        _window.draw(_text);

        _text.setFillColor(sf::Color(255, 200, 50));
        _text.setString(std::to_string(r.score()));
        _text.setPosition(sf::Vector2f(colX[6], rowY));
        _window.draw(_text);

        rowY += 24.0f;
    }

    if (records.empty()) {
        _text.setFillColor(sf::Color(150, 150, 150));
        _text.setString("No records yet.");
        _text.setPosition(sf::Vector2f(WINDOW_W / 2.0f - 80, rowY));
        _window.draw(_text);
    }

    // 底部提示
    _text.setCharacterSize(14);
    _text.setFillColor(sf::Color(150, 150, 150));
    _text.setString("Press any key to return");
    centerText(_text, 0, WINDOW_H - 50.0f, static_cast<float>(WINDOW_W), 30);
    _window.draw(_text);

    _window.display();
}

// ================================================================
//  结果展示
// ================================================================

void SFMLRenderer::showResult(const ScoreRecord& record)
{
    _screen = UIScreen::Result;
    _window.clear(sf::Color(20, 20, 30));

    _text.setCharacterSize(32);
    _text.setFillColor(sf::Color(255, 200, 50));
    _text.setString("Result");
    centerText(_text, 0, 60, static_cast<float>(WINDOW_W), 50);
    _window.draw(_text);

    float y = 160.0f;
    auto drawRow = [&](const std::string& label, const std::string& value,
                       const sf::Color& vColor = sf::Color::White) {
        _text.setCharacterSize(20);
        _text.setFillColor(sf::Color(180, 180, 180));
        _text.setString(label);
        _text.setPosition(sf::Vector2f(WINDOW_W / 2.0f - 200.0f, y));
        _window.draw(_text);

        _text.setFillColor(vColor);
        _text.setString(value);
        _text.setPosition(sf::Vector2f(WINDOW_W / 2.0f + 20.0f, y));
        _window.draw(_text);
        y += 40.0f;
    };

    drawRow("Player:", record.playerName());
    drawRow("Puzzle:", record.puzzleId());

    std::ostringstream ts;
    ts << static_cast<int>(record.timeSeconds()) << " seconds";
    drawRow("Time:", ts.str());

    drawRow("Steps:", std::to_string(record.steps()));

    std::ostringstream as;
    as << std::fixed << std::setprecision(0) << (record.accuracy() * 100) << "%";
    drawRow("Accuracy:", as.str());

    drawRow("Score:", std::to_string(record.score()), sf::Color(255, 220, 50));

    _text.setCharacterSize(14);
    _text.setFillColor(sf::Color(150, 150, 150));
    _text.setString("Press any key to continue");
    centerText(_text, 0, WINDOW_H - 60.0f, static_cast<float>(WINDOW_W), 30);
    _window.draw(_text);

    _window.display();
}

// ================================================================
//  消息 / 输入提示
// ================================================================

void SFMLRenderer::showMessage(const std::string& msg)
{
    _screen = UIScreen::Message;
    _window.clear(sf::Color(20, 20, 30));

    _text.setCharacterSize(20);
    _text.setFillColor(sf::Color::White);
    _text.setString(msg);
    centerText(_text, 0, WINDOW_H / 2.0f - 30, static_cast<float>(WINDOW_W), 40);
    _window.draw(_text);

    _text.setCharacterSize(14);
    _text.setFillColor(sf::Color(150, 150, 150));
    _text.setString("Press any key to continue");
    centerText(_text, 0, WINDOW_H / 2.0f + 20, static_cast<float>(WINDOW_W), 30);
    _window.draw(_text);

    _window.display();

    // 阻塞等待用户按键或关闭窗口
    while (_window.isOpen()) {
        sf::Event event;
        while (_window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                _window.close();
                return;
            }
            if (event.type == sf::Event::KeyPressed) {
                return;
            }
        }
        sf::sleep(sf::milliseconds(10));
    }
}

std::string SFMLRenderer::promptPlayerName()
{
    _window.clear(sf::Color(20, 20, 30));

    _text.setCharacterSize(24);
    _text.setFillColor(sf::Color(255, 200, 50));
    _text.setString("Enter Your Name");
    centerText(_text, 0, 200, static_cast<float>(WINDOW_W), 40);
    _window.draw(_text);

    _text.setCharacterSize(16);
    _text.setFillColor(sf::Color(200, 200, 200));
    _text.setString("Type your name and press Enter");
    centerText(_text, 0, 260, static_cast<float>(WINDOW_W), 30);
    _window.draw(_text);

    _window.display();

    // 输入循环
    std::string name;
    while (_window.isOpen()) {
        sf::Event event;
        while (_window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                _window.close();
                return "Player";
            }
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\r' || event.text.unicode == '\n') {
                    if (name.empty()) name = "Player";
                    return name;
                }
                if (event.text.unicode == '\b') {  // 退格
                    if (!name.empty()) name.pop_back();
                } else if (event.text.unicode >= 32 && event.text.unicode < 127) {
                    if (name.size() < 30) {
                        name += static_cast<char>(event.text.unicode);
                    }
                }
            }

            // 每次输入后重绘
            _window.clear(sf::Color(20, 20, 30));
            _text.setCharacterSize(24);
            _text.setFillColor(sf::Color(255, 200, 50));
            _text.setString("Enter Your Name");
            centerText(_text, 0, 200, static_cast<float>(WINDOW_W), 40);
            _window.draw(_text);

            _text.setCharacterSize(28);
            _text.setFillColor(sf::Color::White);
            _text.setString(name + "_");
            centerText(_text, 0, 310, static_cast<float>(WINDOW_W), 40);
            _window.draw(_text);

            _text.setCharacterSize(14);
            _text.setFillColor(sf::Color(150, 150, 150));
            _text.setString("Press Enter to confirm");
            centerText(_text, 0, 370, static_cast<float>(WINDOW_W), 30);
            _window.draw(_text);

            _window.display();
        }
    }
    return "Player";
}

std::vector<std::pair<int, int>> SFMLRenderer::promptStartingCells(
    const Grid& grid, int count)
{
    // 简化版：使用默认 5 个起始点
    std::vector<std::pair<int, int>> starts = {
        {0, 0},
        {grid.rows() / 2, grid.cols() / 2},
        {0, grid.cols() - 1},
        {grid.rows() - 1, 0},
        {grid.rows() - 1, grid.cols() - 1}
    };

    // 交互式选择
    int rows = grid.rows();
    int cols = grid.cols();
    int curRow = rows / 2, curCol = cols / 2;
    float cellSz = std::min(
        (PANEL_X - GRID_OFFSET_X - 10.0f) / cols,
        (WINDOW_H - GRID_OFFSET_Y - 80.0f) / rows
    );
    if (cellSz < 18.0f) cellSz = 18.0f;
    if (cellSz > 40.0f) cellSz = 40.0f;

    _rect.setSize(sf::Vector2f(cellSz - 2, cellSz - 2));

    while (_window.isOpen()) {
        sf::Event event;
        while (_window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                _window.close();
                return starts;
            }
            if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                    case sf::Keyboard::Up:    if (curRow > 0) curRow--; break;
                    case sf::Keyboard::Down:  if (curRow < rows - 1) curRow++; break;
                    case sf::Keyboard::Left:  if (curCol > 0) curCol--; break;
                    case sf::Keyboard::Right: if (curCol < cols - 1) curCol++; break;
                    case sf::Keyboard::Space: {
                        auto it = std::find(starts.begin(), starts.end(),
                                           std::make_pair(curRow, curCol));
                        if (it != starts.end()) {
                            starts.erase(it);
                        } else if (static_cast<int>(starts.size()) < count) {
                            starts.emplace_back(curRow, curCol);
                        }
                        break;
                    }
                    case sf::Keyboard::Enter:
                        if (static_cast<int>(starts.size()) == count) return starts;
                        break;
                    case sf::Keyboard::Escape:
                        return starts;
                    default: break;
                }
            }
        }

        // 绘制
        _window.clear(sf::Color(20, 20, 30));

        _text.setCharacterSize(18);
        _text.setFillColor(sf::Color(255, 200, 50));
        _text.setString("Select " + std::to_string(count) +
                        " starting cells (Space=select, Enter=confirm)");
        centerText(_text, 0, 10, static_cast<float>(WINDOW_W), 30);
        _window.draw(_text);

        // 显示已选
        std::ostringstream selStr;
        selStr << "Selected: ";
        for (size_t i = 0; i < starts.size(); ++i) {
            if (i > 0) selStr << ", ";
            selStr << "(" << starts[i].first << "," << starts[i].second << ")";
        }
        _text.setCharacterSize(13);
        _text.setFillColor(sf::Color(200, 200, 200));
        _text.setString(selStr.str());
        centerText(_text, 0, 32, static_cast<float>(WINDOW_W), 20);
        _window.draw(_text);

        // 绘制网格
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                float x = GRID_OFFSET_X + c * cellSz;
                float y = GRID_OFFSET_Y + r * cellSz;

                const Cell& cell = grid.at(r, c);
                sf::Color bgColor;
                if (cell.isEmpty()) {
                    bgColor = sf::Color(40, 40, 50);
                } else {
                    bgColor = getCellColor(cell);
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
                    _rect.setOutlineColor(sf::Color(255, 220, 50));
                    _rect.setOutlineThickness(2.5f);
                } else if (r == curRow && c == curCol) {
                    _rect.setOutlineColor(sf::Color::White);
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
                    _text.setCharacterSize(fs);
                    _text.setFillColor(sf::Color::White);
                    _text.setString(str);
                    centerText(_text, x, y, cellSz - 2, cellSz - 2);
                    _window.draw(_text);
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
        sf::Event event;
        while (_window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                _window.close();
                return UserAction::QUIT;
            }

            if (event.type == sf::Event::KeyPressed) {
                UserAction a = processKeyEvent(event);
                if (a != UserAction::NONE) return a;
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                UserAction a = processMouseEvent(event);
                if (a != UserAction::NONE) return a;
            }
        }

        // 小睡防止 CPU 空转
        sf::sleep(sf::milliseconds(10));
    }
    return UserAction::QUIT;
}

UserAction SFMLRenderer::processKeyEvent(const sf::Event& event)
{
    switch (event.key.code) {
        case sf::Keyboard::Up:
            if (_cursorRow > 0) _cursorRow--;
            return UserAction::NONE;  // 内化：不干扰 Controller
        case sf::Keyboard::Down:
            if (_cursorRow < _gridRows - 1) _cursorRow++;
            return UserAction::NONE;
        case sf::Keyboard::Left:
            if (_cursorCol > 0) _cursorCol--;
            return UserAction::NONE;
        case sf::Keyboard::Right:
            if (_cursorCol < _gridCols - 1) _cursorCol++;
            return UserAction::NONE;
        case sf::Keyboard::Enter: return UserAction::CONFIRM;
        case sf::Keyboard::Space:
            // 两段式：无选中→选中；同格再按→取消；异格再按→合并
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
            _hasSelection = false;
            return UserAction::SELECT_CELL;
        case sf::Keyboard::Escape:
            // 先清除选中态，已无选中则返回 BACK
            if (_hasSelection) {
                _hasSelection = false;
                return UserAction::NONE;
            }
            return UserAction::BACK;
        case sf::Keyboard::S:     return UserAction::SUBMIT;
        case sf::Keyboard::Q:     return UserAction::QUIT;
        case sf::Keyboard::L:     return UserAction::VIEW_LEADERBOARD;
        case sf::Keyboard::Num1: case sf::Keyboard::Numpad1:
            return UserAction::SELECT_PUZZLE_1;
        case sf::Keyboard::Num2: case sf::Keyboard::Numpad2:
            return UserAction::SELECT_PUZZLE_2;
        case sf::Keyboard::Num3: case sf::Keyboard::Numpad3:
            return UserAction::SELECT_PUZZLE_3;
        case sf::Keyboard::Num4: case sf::Keyboard::Numpad4:
            return UserAction::SELECT_PUZZLE_4;
        case sf::Keyboard::Num5: case sf::Keyboard::Numpad5:
            return UserAction::SELECT_PUZZLE_5;
        case sf::Keyboard::B:
            return UserAction::BACK;
        default:
            return UserAction::NONE;
    }
}

UserAction SFMLRenderer::processMouseEvent(const sf::Event& event)
{
    if (event.mouseButton.button != sf::Mouse::Left) return UserAction::NONE;

    int mx = event.mouseButton.x;
    int my = event.mouseButton.y;

    switch (_screen) {

    case UIScreen::Gameplay:
    case UIScreen::StartSelect:
        // ---- 游戏内网格点击 ----
        if (mx >= GRID_OFFSET_X && mx < PANEL_X - 10 &&
            my >= GRID_OFFSET_Y && my < WINDOW_H - 10) {

            auto [row, col] = pixelToGrid(mx, my);
            if (row >= 0 && col >= 0 && row < _gridRows && col < _gridCols) {
                _cursorRow = row;
                _cursorCol = col;

                if (_hasSelection) {
                    int dr = row - _selectedRow;
                    int dc = col - _selectedCol;
                    if ((std::abs(dr) + std::abs(dc)) == 1) {
                        _hasSelection = false;
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
        // 按钮区域：居中，宽 360，高 44
        float bx = WINDOW_W / 2.0f - 180.0f;
        float bw = 360.0f, bh = 44.0f;

        auto inside = [&](float by) {
            return mx >= bx && mx <= bx + bw && my >= by && my <= by + bh;
        };

        if (inside(220.0f))      return UserAction::SELECT_PUZZLE_1;
        if (inside(280.0f))      return UserAction::VIEW_LEADERBOARD;
        if (inside(340.0f))      return UserAction::QUIT;
        break;
    }

    case UIScreen::PuzzleList: {
        // ---- 题面列表点击 ----
        float startY = 130.0f;
        float listBx = WINDOW_W / 2.0f - 250.0f;
        float listBw = 500.0f, listBh = 56.0f;

        for (int i = 0; i < _puzzleCount; ++i) {
            float by = startY + i * 70.0f;
            if (mx >= listBx && mx <= listBx + listBw &&
                my >= by && my <= by + listBh) {
                // 映射到 SELECT_PUZZLE_1..5
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
        float backBx = WINDOW_W / 2.0f - 100.0f;
        if (mx >= backBx && mx <= backBx + 200.0f &&
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
    float tx = x + (w - bounds.width) / 2.0f - bounds.left;
    float ty = y + (h - bounds.height) / 2.0f - bounds.top;
    text.setPosition(sf::Vector2f(tx, ty));
}

#endif // HAS_GUI
