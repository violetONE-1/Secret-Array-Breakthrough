/**
 * MergeRule.cpp — MergeRule 类的实现
 *
 * 见 include/core/MergeRule.hpp 的详细文档注释。
 */

#include "core/MergeRule.hpp"

bool MergeRule::canMerge(const Cell& a, const Cell& b)
{
    // 空格子不能与任何格子合并
    if (a.isEmpty() || b.isEmpty()) return false;

    bool sameL = a.sameLetter(b);
    bool sameN = a.sameNumber(b);

    // 条件 1：字母相同 & 数字不同 → 可合并
    if (sameL && !sameN) return true;

    // 条件 2：数字相同 & 字母不同 → 可合并
    if (sameN && !sameL) return true;

    // 条件 3：字母数字都相同 → 可合并（合并后结果不变，仍为原格子）
    if (sameL && sameN) return true;

    // 其余情况（字母数字全不同）不可合并
    return false;
}

char MergeRule::mergeLetters(char a, char b)
{
    // 将字母映射到 0~25，相加后取模 26，再映射回 'A'~'Z'
    int indexA = a - 'A';
    int indexB = b - 'A';
    int result = (indexA + indexB) % 26;
    return static_cast<char>('A' + result);
}

int MergeRule::mergeNumbers(int a, int b)
{
    // 数字相加取个位
    return (a + b) % 10;
}

Cell MergeRule::getMergedCell(const Cell& a, const Cell& b)
{
    char newLetter;
    int  newNumber;

    // 字母不同 → 执行字母合并；否则保留 a 的字母
    if (!a.sameLetter(b)) {
        newLetter = mergeLetters(a.getLetter(), b.getLetter());
    } else {
        newLetter = a.getLetter();
    }

    // 数字不同 → 执行数字合并；否则保留 a 的数字
    if (!a.sameNumber(b)) {
        newNumber = mergeNumbers(a.getNumber(), b.getNumber());
    } else {
        newNumber = a.getNumber();
    }

    return Cell(newLetter, newNumber);
}
