/**
 * InputHandler.cpp — 输入处理类的实现
 */

#include "controller/InputHandler.hpp"

InputHandler::InputHandler()
{
}

std::pair<int, int> InputHandler::screenToGrid(int screenRow, int screenCol,
                                               int /*gridRows*/, int /*gridCols*/) const
{
    // 控制台网格布局：
    //   列标题占 1 行，顶框占 1 行
    //   每行数据占 2 行（格子行 + 分隔行）
    //   第 0 行格子对应 _screenBaseRow = 头部行数
    //   第 0 列格子对应 _screenBaseCol = 行号宽度(4) + 边框(1) = 5
    int row = (screenRow - 3) / 2;  // 跳过标题行
    int col = (screenCol - 5) / 4;  // 每格宽度 3 + 分隔符 1

    if (row < 0) row = 0;
    if (col < 0) col = 0;

    return {row, col};
}

std::pair<int, int> InputHandler::gridToScreen(int row, int col) const
{
    int screenRow = 3 + row * 2;
    int screenCol = 5 + col * 4;
    return {screenRow, screenCol};
}

bool InputHandler::isInGrid(int screenRow, int screenCol,
                            int gridRows, int gridCols) const
{
    auto pos = screenToGrid(screenRow, screenCol, gridRows, gridCols);
    return pos.first >= 0 && pos.first < gridRows &&
           pos.second >= 0 && pos.second < gridCols;
}
