/**
 * Cell.hpp — 格子类 (Cell)
 *
 * 密阵突围游戏中最小的数据单元。
 * 每个 Cell 存储一个字母（'A'~'Z'）、一个数字（0~9），
 * 以及两个 UI 状态位：是否为空（已被合并清空）、是否被选中。
 *
 * 职责：
 *   - 封装格子的核心数据（字母+数字）
 *   - 提供属性比较接口，供 MergeRule 判定合并条件
 *   - 管理 UI 状态位（空 / 选中），供 Renderer 渲染高亮
 *
 * 所属架构层：Model（核心数据层）
 * 所属模块：游戏核心逻辑模块
 */

#ifndef CELL_HPP
#define CELL_HPP

#include <string>

class Cell {
public:
    // ---- 构造函数 ----

    /** 构造有内容的格子，指定字母与数字 */
    Cell(char letter, int number);

    /** 静态工厂：创建一个已被清空的空格子 */
    static Cell makeEmpty();

    // ---- 属性访问器 (Getters / Setters) ----

    char  getLetter()   const;
    void  setLetter(char letter);

    int   getNumber()   const;
    void  setNumber(int number);

    bool  isEmpty()     const;
    void  setEmpty(bool empty);

    bool  isSelected()  const;
    void  setSelected(bool selected);

    // ---- 比较操作 ----

    /** 判断另一个格子与本格字母是否相同（空格子永远返回 false） */
    bool sameLetter(const Cell& other) const;

    /** 判断另一个格子与本格数字是否相同（空格子永远返回 false） */
    bool sameNumber(const Cell& other) const;

    /** 完全相等判定：字母、数字、空状态三者全同 */
    bool operator==(const Cell& other) const;

    /** 不等判定 */
    bool operator!=(const Cell& other) const;

    // ---- 格式化 ----

    /**
     * 返回用于屏幕显示的字符串：
     *   - 空格子返回 " . "（三个字符宽度）
     *   - 有内容格子返回 "A1"、"B3" 等（对齐到固定宽度）
     */
    std::string toString() const;

private:
    char _letter;       // 字母：'A' 到 'Z'（空格子上此值无意义）
    int  _number;       // 数字：0 到 9
    bool _empty;        // 是否已被合并清空
    bool _selected;     // 是否被玩家当前选中
};

#endif // CELL_HPP
