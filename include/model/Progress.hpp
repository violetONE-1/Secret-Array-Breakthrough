#ifndef PROGRESS_HPP
#define PROGRESS_HPP

#include "FileManager.hpp"
#include <vector>
#include <string>

/**
 * Progress.hpp — 关卡进度类
 *
 * 追踪玩家在闯关模式中的进度：
 *   - 当前已解锁的最大关卡
 *   - 每一关的历史最高分
 *
 * 持久化存储在 data/progress.txt
 */
class Progress {
public:
    Progress(FileManager& fileManager, int totalLevels);

    /** 当前已解锁的最大关卡（1-indexed，至少为 1） */
    int maxUnlocked() const;

    /** 某一关的最高分（0 表示尚未玩过） */
    int bestScore(int level) const;

    /** 某关是否已通关（最高分 > 0） */
    bool isCleared(int level) const;

    /** 某关是否已解锁 */
    bool isUnlocked(int level) const;

    /**
     * 更新关卡成绩：
     *   - 若本次得分高于历史最佳则更新
     *   - 若该关是当前最高解锁关且得分 > 0，则解锁下一关
     */
    void updateLevel(int level, int score);

    void load();
    void save();

private:
    FileManager& _fileManager;
    int _totalLevels;
    int _maxUnlocked;           // 当前最大解锁关卡（1-indexed）
    std::vector<int> _bestScores; // [0] = 第 1 关，依此类推
};

#endif // PROGRESS_HPP
