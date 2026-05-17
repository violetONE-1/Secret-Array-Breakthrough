/**
 * MergeRule.hpp — 合并规则类 (MergeRule)
 *
 * 纯静态工具类，封装"密阵突围"的全部合并规则算法。
 * 不可实例化（构造函数 = delete）。
 *
 * 合并规则：
 *   条件：两格相邻（上下左右），且至少一项属性（字母/数字）相同、另一项不同。
 *     - 字母相同 & 数字不同 → 合并后字母不变，数字相加取个位。
 *       例：A1 + A2 → A3；A9 + A8 → A7（取个位）
 *     - 数字相同 & 字母不同 → 合并后数字不变，字母相加循环进位。
 *       例：A1 + B1 → C1；Z1 + A1 → A1（循环：Z→A→B）
 *     - 字母相同 & 数字相同 → 合并后无变化，视为非法操作。
 *     - 字母不同 & 数字不同 → 不可合并。
 *
 * 所属架构层：Model（核心数据层）
 * 所属模块：游戏核心逻辑模块
 */

#ifndef MERGERULE_HPP
#define MERGERULE_HPP

#include "Cell.hpp"

class MergeRule {
public:
    // ---- 禁止实例化 ----
    MergeRule() = delete;

    /**
     * 判断单元格 a 与单元格 b 是否可以合并。
     *
     * 前置条件：调用方已确保 a 与 b 在网格中相邻（上下左右）。
     *
     * 返回 true 的条件（满足任一即可）：
     *   1. a 与 b 字母相同 且 数字不同
     *   2. a 与 b 数字相同 且 字母不同
     *
     * 若条件满足但合并后结果与原来完全一样（字母数字均相同），也返回 false。
     */
    static bool canMerge(const Cell& a, const Cell& b);

    /**
     * 合并两个字母：计算 (a + b) 的循环加法。
     *
     * 算法：result = 'A' + ((a - 'A') + (b - 'A')) % 26
     * 例：'A' + 'B' = 'C'；'Z' + 'A' = 'A'；'Y' + 'B' = 'A'
     */
    static char mergeLetters(char a, char b);

    /**
     * 合并两个数字：计算 (a + b) 的个位加法。
     *
     * 算法：result = (a + b) % 10
     * 例：1 + 2 = 3；9 + 8 = 7
     */
    static int mergeNumbers(int a, int b);

    /**
     * 返回单元格 a 与 b 合并后产生的新 Cell。
     *
     * 规则：
     *   - 若字母不同，合并字母；否则保留 a 的字母
     *   - 若数字不同，合并数字；否则保留 a 的数字
     *
     * 调用方应在调用 canMerge() 返回 true 后使用此函数。
     */
    static Cell getMergedCell(const Cell& a, const Cell& b);
};

#endif // MERGERULE_HPP
