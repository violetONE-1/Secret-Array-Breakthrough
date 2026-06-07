/**
 * Leaderboard.hpp — 排行榜类 (Leaderboard)
 *
 * 维护一份得分记录列表，支持：
 *   - 从 CSV 文件加载
 *   - 追加新记录并即时持久化（追加一行到文件）
 *   - 多维度排序查询（按总分、用时、正确率）
 *
 * 内存策略：
 *   - std::vector<ScoreRecord> 值语义存储全部记录
 *
 * 所属架构层：Model（数据持久化子层）
 * 所属模块：数据持久化与存储模块
 */

#ifndef LEADERBOARD_HPP
#define LEADERBOARD_HPP

#include "ScoreRecord.hpp"
#include "FileManager.hpp"
#include <vector>
#include <string>

class Leaderboard {
public:
    /**
     * @param fileManager  文件管理器（非所有权）
     * @param fileRelPath  排行榜文件相对路径，如 "leaderboard.txt"
     */
    Leaderboard(FileManager& fileManager, const std::string& fileRelPath);

    /** 从文件加载全部记录 */
    void load();

    /** 将所有记录写入文件（覆盖） */
    bool save() const;

    /** 追加一条新记录（内存 + 文件追加一行） */
    void add(const ScoreRecord& record);

    // ---- 排序查询 ----

    /** 按总分降序，取前 n 名（n=0 表示取全部） */
    std::vector<ScoreRecord> topByScore(int n = 0) const;

    /** 按用时升序，取前 n 名 */
    std::vector<ScoreRecord> topByTime(int n = 0) const;

    /** 筛选特定题面的记录 */
    std::vector<ScoreRecord> recordsForPuzzle(const std::string& puzzleId) const;

    /** 全部记录 */
    const std::vector<ScoreRecord>& all() const;

private:
    FileManager&           _fileManager;
    std::string            _fileRelPath;
    std::vector<ScoreRecord> _records;
};

#endif // LEADERBOARD_HPP
