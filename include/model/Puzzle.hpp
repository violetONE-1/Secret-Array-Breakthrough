/**
 * Puzzle.hpp — 题面类 (Puzzle)
 *
 * 封装一道题目的完整数据：唯一 ID、标题名称、网格尺寸、初始布局。
 * 支持与纯文本文件互转（序列化 / 反序列化）。
 *
 * 所属架构层：Model（数据持久化子层）
 * 所属模块：数据持久化与存储模块
 */

#ifndef PUZZLE_HPP
#define PUZZLE_HPP

#include "core/Grid.hpp"
#include <string>

class Puzzle {
public:
    Puzzle();
    Puzzle(const std::string& id, const std::string& name,
           const Grid& initialGrid);

    // ---- 元信息 ----

    const std::string& id()   const;
    const std::string& name() const;
    int                gridSize() const;  // 返回 initialGrid 的行数（假设正方形）

    // ---- 盘面 ----

    const Grid& initialGrid() const;

    // ---- 序列化 ----

    /**
     * 序列化格式（纯文本）：
     *   rows cols
     *   25×25 格子数据（每格 "字母+数字" 或 ".0" 表示空）
     */
    std::string serialize() const;

    /** 从文本反序列化，成功返回 true */
    static bool deserialize(const std::string& data, Puzzle& outPuzzle);

private:
    std::string _id;
    std::string _name;
    Grid        _initialGrid;
};

#endif // PUZZLE_HPP
