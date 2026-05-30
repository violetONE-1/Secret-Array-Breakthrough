#include "model/Progress.hpp"
#include <sstream>
#include <algorithm>

Progress::Progress(FileManager& fileManager, int totalLevels)
    : _fileManager(fileManager)
    , _totalLevels(totalLevels)
    , _maxUnlocked(1)
    , _bestScores(totalLevels, 0)
{
}

int Progress::maxUnlocked() const { return _maxUnlocked; }

int Progress::bestScore(int level) const
{
    if (level < 1 || level > _totalLevels) return 0;
    return _bestScores[level - 1];
}

bool Progress::isCleared(int level) const
{
    return bestScore(level) > 0;
}

bool Progress::isUnlocked(int level) const
{
    return level >= 1 && level <= _maxUnlocked;
}

void Progress::updateLevel(int level, int score)
{
    if (level < 1 || level > _totalLevels) return;

    // 更新最佳成绩
    if (score > _bestScores[level - 1]) {
        _bestScores[level - 1] = score;
    }

    // 若通关当前最高解锁关且得分 > 0，解锁下一关
    if (level == _maxUnlocked && score > 0 && _maxUnlocked < _totalLevels) {
        _maxUnlocked++;
    }
}

void Progress::load()
{
    std::string content = _fileManager.readAll("progress.txt");
    if (content.empty()) return;

    std::istringstream iss(content);
    std::string line;

    // 第一行: max_unlocked = N
    if (std::getline(iss, line)) {
        auto pos = line.find('=');
        if (pos != std::string::npos) {
            int val = std::stoi(line.substr(pos + 1));
            if (val >= 1 && val <= _totalLevels)
                _maxUnlocked = val;
        }
    }

    // 后续行: level_N_best = SCORE
    while (std::getline(iss, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        // 提取 level 序号
        std::string key = line.substr(0, pos);
        int score = std::stoi(line.substr(pos + 1));

        // key 格式 "level_N_best"
        if (key.substr(0, 6) == "level_") {
            auto underscore = key.find('_', 6);
            if (underscore != std::string::npos) {
                int lvl = std::stoi(key.substr(6, underscore - 6));
                if (lvl >= 1 && lvl <= _totalLevels) {
                    _bestScores[lvl - 1] = score;
                }
            }
        }
    }
}

void Progress::save()
{
    std::ostringstream oss;
    oss << "max_unlocked = " << _maxUnlocked << "\n";
    for (int i = 0; i < _totalLevels; ++i) {
        oss << "level_" << (i + 1) << "_best = " << _bestScores[i] << "\n";
    }
    _fileManager.writeAll("progress.txt", oss.str());
}
