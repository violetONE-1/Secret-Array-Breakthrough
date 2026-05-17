/**
 * PuzzleManager.hpp — 题库管理类 (PuzzleManager)
 *
 * 管理全部题面的加载、检索、添加与文件持久化。
 *
 * 数据来源：
 *   1. 预置题面（≥ 3 道）— 硬编码内置，首次运行时自动保存到 data/puzzles/
 *   2. 自定义题面 — 存放在 data/puzzles/custom/，由用户创建
 *
 * 所属架构层：Model（数据持久化子层）
 * 所属模块：数据持久化与存储模块
 */

#ifndef PUZZLEMANAGER_HPP
#define PUZZLEMANAGER_HPP

#include "Puzzle.hpp"
#include "FileManager.hpp"
#include <vector>
#include <string>
#include <memory>

class PuzzleManager {
public:
    /**
     * 构造函数。
     * @param fileManager  文件管理器（非所有权，仅引用）
     */
    explicit PuzzleManager(FileManager& fileManager);

    /**
     * 加载全部题面：
     *   1. 初始化内置题面（若 data/puzzles/*.txt 不存在则自动创建）
     *   2. 加载 data/puzzles/custom/ 下的用户自定义题面
     */
    void loadAll();

    // ---- 查询 ----

    /** 返回所有题面列表（只读） */
    const std::vector<Puzzle>& all() const;

    /** 按 ID 查找题面，未找到返回 nullptr */
    const Puzzle* findById(const std::string& id) const;

    // ---- 自定义题面管理 ----

    /** 添加新题面（内存中），同时保存为文件 */
    bool addPuzzle(const Puzzle& puzzle);

    /** 重新从磁盘加载所有题面 */
    void reload();

private:
    /** 初始化 3 道内置题面的硬编码数据 */
    void createBuiltinPuzzles();

    /** 读取指定目录下所有 .txt 文件作为题面加载 */
    void loadPuzzlesFromDir(const std::string& dirRelPath);

    FileManager&      _fileManager;
    std::vector<Puzzle> _puzzles;
};

#endif // PUZZLEMANAGER_HPP
