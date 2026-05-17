/**
 * Cell.cpp — Cell 类的实现
 *
 * 见 include/core/Cell.hpp 的详细文档注释。
 */

#include "core/Cell.hpp"
#include <iomanip>
#include <sstream>

// ---- 构造函数 ----

Cell::Cell(char letter, int number)
    : _letter(letter)
    , _number(number)
    , _empty(false)
    , _selected(false)
{
}

Cell Cell::makeEmpty()
{
    Cell c('A', 0);   // 先构造有内容的格子...
    c._empty = true;  // ...再标记为空
    return c;
}

// ---- 属性访问器 ----

char Cell::getLetter()   const { return _letter; }
void Cell::setLetter(char letter) { _letter = letter; }

int  Cell::getNumber()   const { return _number; }
void Cell::setNumber(int number) { _number = number; }

bool Cell::isEmpty()     const { return _empty; }
void Cell::setEmpty(bool empty) { _empty = empty; }

bool Cell::isSelected()  const { return _selected; }
void Cell::setSelected(bool selected) { _selected = selected; }

// ---- 比较操作 ----

bool Cell::sameLetter(const Cell& other) const
{
    // 空格子不参与任何有效比较
    if (_empty || other._empty) return false;
    return _letter == other._letter;
}

bool Cell::sameNumber(const Cell& other) const
{
    if (_empty || other._empty) return false;
    return _number == other._number;
}

bool Cell::operator==(const Cell& other) const
{
    if (_empty && other._empty) return true;
    if (_empty || other._empty) return false;
    return _letter == other._letter && _number == other._number;
}

bool Cell::operator!=(const Cell& other) const
{
    return !(*this == other);
}

// ---- 格式化 ----

std::string Cell::toString() const
{
    // 空格子显示为 " . "（三个字符宽度，保持网格对齐）
    if (_empty) {
        return " . ";
    }
    // 正常格子：字母 + 数字，右侧补空格到 3 字符宽度
    std::ostringstream oss;
    oss << _letter << _number;
    // 统一 3 字符宽度：字母(1) + 数字(1) + 尾随空格(1) = 3
    oss << ' ';
    return oss.str();
}
