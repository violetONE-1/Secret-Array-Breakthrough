/**
 * InputHandler.hpp — 输入处理类
 *
 * 将原始坐标/按键转换为网格行列位置。
 *
 * 控制台模式下：
 *   - 用户通过方向键移动光标选择格子
 *   - 本类将光标位置映射到格子坐标
 *
 * GUI 模式下（Lv2+）：
 *   - 将鼠标像素坐标转换为网格行列
 *
 * 所属架构层：Controller
 * 所属模块：游戏流程控制模块
 */

#ifndef INPUTHANDLER_HPP
#define INPUTHANDLER_HPP

#include <utility>

class InputHandler {
public:
    InputHandler();

    /**
     * 将渲染坐标（列, 行）转换为网格坐标（row, col）。
     * 控制台模式下：renderedCol 从 2 开始（跳过左边框），每格宽度 3 字符。
     */
    std::pair<int, int> screenToGrid(int screenRow, int screenCol,
                                     int gridRows, int gridCols) const;

    /**
     * 将网格坐标转换为屏幕光标位置。
     */
    std::pair<int, int> gridToScreen(int row, int col) const;

    /** 判断屏幕坐标是否落在网格范围内 */
    bool isInGrid(int screenRow, int screenCol,
                  int gridRows, int gridCols) const;
};

#endif // INPUTHANDLER_HPP
