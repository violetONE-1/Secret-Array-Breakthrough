/**
 * ScoreRecord.hpp — 答题记录类 (ScoreRecord)
 *
 * 存储单次答题的完整成绩信息，支持：
 *   - 自动计算综合得分
 *   - CSV 格式序列化/反序列化
 *
 * 得分公式：
 *   score = accuracy × 1000 + max(0, 500 − timeSeconds × 2) + steps × 10
 *
 * 所属架构层：Model（数据持久化子层）
 * 所属模块：数据持久化与存储模块
 */

#ifndef SCORERECORD_HPP
#define SCORERECORD_HPP

#include <string>

class ScoreRecord {
public:
    ScoreRecord();

    /**
     * 构造完整记录。
     * @param playerName  玩家名称
     * @param puzzleId    题面 ID
     * @param timeSeconds 答题用时（秒）
     * @param steps       完成步数
     * @param accuracy    正确率 (0.0 ~ 1.0)
     * @param timestamp   完成时间戳（字符串）
     */
    ScoreRecord(const std::string& playerName,
                const std::string& puzzleId,
                double timeSeconds,
                int    steps,
                double accuracy,
                const std::string& timestamp);

    // ---- Getters ----

    const std::string& playerName()  const;
    const std::string& puzzleId()    const;
    double             timeSeconds() const;
    int                steps()       const;
    double             accuracy()    const;
    int                score()       const;
    const std::string& timestamp()   const;

    // ---- 序列化 ----

    /**
     * 序列化为 CSV 风格一行：
     *   "Alice, builtin_01, 45.23, 12, 1.00, 980, 2026-05-15 14:23:10"
     */
    std::string serialize() const;

    /** 从 CSV 行反序列化，成功返回 true */
    static bool deserialize(const std::string& line, ScoreRecord& outRecord);

private:
    /** 计算综合得分 */
    static int calculateScore(double timeSeconds, int steps, double accuracy);

    std::string _playerName;
    std::string _puzzleId;
    double      _timeSeconds;
    int         _steps;
    double      _accuracy;
    int         _score;
    std::string _timestamp;
};

#endif // SCORERECORD_HPP
