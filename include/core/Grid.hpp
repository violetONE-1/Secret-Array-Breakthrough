/**
 * Grid.hpp — 网格类 (Grid)
 *
 * 管理 rows × cols 的二维 Cell 数组，提供网格级操作接口。
 * 默认尺寸为 25×25，由题面（Puzzle）在加载时指定实际尺寸。
 *
 * 职责：
 *   - 创建/重置指定尺寸的网格
 *   - 合并两格（中心格保留合并结果，邻格清空）
 *   - 扫描全盘，查找所有合法的合并操作
 *   - 判断盘面是否已死（无合法操作）
 *   - 序列化/反序列化为文本格式，供题面文件读写
 *
 * 内存策略：
 *   - 使用 std::vector<std::vector<Cell>> 值语义存储全部格子
 *   - Cell 为轻量 POD 对象（~12 bytes），连续内存，缓存友好
 *   - 不使用任何裸指针或动态分配
 *
 * 所属架构层：Model（核心数据层）
 * 所属模块：游戏核心逻辑模块
 */

#ifndef GRID_HPP
#define GRID_HPP

#include "Cell.hpp"
#include "Move.hpp"
#include <vector>
#include <string>

class Grid {
public:
    /** 构造 rows×cols 的全空格网格 */
    Grid(int rows, int cols);

    /** 构造空网格（0×0），通常用于后续 deserialize 填充 */
    Grid();

    // ---- 尺寸查询 ----

    int rows() const;
    int cols() const;

    // ---- 格子访问 ----

    /**
     * 获取指定坐标格子的只读引用。
     * 行列超出范围时抛出 std::out_of_range。
     */
    const Cell& at(int r, int c) const;

    /**
     * 获取指定坐标格子的可写引用。
     * 行列超出范围时抛出 std::out_of_range。
     */
    Cell& at(int r, int c);

    /** 设置指定坐标的格子内容 */
    void setAt(int r, int c, const Cell& cell);

    // ---- 核心合并操作 ----

    /**
     * 合并两格：
     *   - (sr, sc) 为被移动的邻格，合并后置为空
     *   - (dr, dc) 为中心格，合并后更新为合并结果
     * 本函数不校验合法性——调用方应先用 MergeRule::canMerge 判定。
     */
    void mergeCells(int sr, int sc, int dr, int dc, const Cell& mergedResult);

    // ---- 合法操作扫描 ----

    /**
     * 遍历全盘，返回所有合法的合并操作列表（Move 对象向量）。
     * 每个合法操作包含：源格坐标、目标格坐标、方向、合并后结果。
     * 若返回空 vector，表示盘面已死。
     */
    std::vector<Move> findAllValidMoves() const;

    /** 判断是否存在至少一个合法合并操作 */
    bool hasAnyValidMove() const;

    // ---- 序列化 ----

    /**
     * 序列化格式：
     *   rows cols
     *   每行 cols 个格子（每个格式 "L?"，L 为字母或 '.'，? 为数字）
     *   空格分隔，'\n' 换行
     */
    std::string serialize() const;

    /**
     * 从序列化字符串重建网格。
     * 若字符串格式错误，返回 false 且 grid 不变。
     */
    static bool deserialize(const std::string& data, Grid& outGrid);

    /**
     * 滑行操作：将源格内容移至相邻空格子。
     * 调用方应确保源格非空且目标格为空。
     */
    void slideCell(int sr, int sc, int dr, int dc);

    /** 将全部格子重置为空状态 */
    void clear();

private:
    int _rows;
    int _cols;
    std::vector<std::vector<Cell>> _cells;
};

#endif // GRID_HPP
