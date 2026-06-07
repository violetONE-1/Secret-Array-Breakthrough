/**
 * Leaderboard.cpp — Leaderboard 类的实现
 */

#include "model/Leaderboard.hpp"
#include <sstream>
#include <algorithm>

Leaderboard::Leaderboard(FileManager& fileManager, const std::string& fileRelPath)
    : _fileManager(fileManager)
    , _fileRelPath(fileRelPath)
{
}

void Leaderboard::load()
{
    _records.clear();

    std::string content = _fileManager.readAll(_fileRelPath);
    if (content.empty()) return;

    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        // 跳过注释行
        if (line.empty() || line[0] == '#') continue;

        ScoreRecord record;
        if (ScoreRecord::deserialize(line, record)) {
            _records.push_back(record);
        }
    }
}

bool Leaderboard::save() const
{
    std::ostringstream oss;
    oss << "# 密阵突围 排行榜" << '\n';
    oss << "# 格式: 玩家名, 题面ID, 用时(s), 步数, 正确率, 总分, 完成时间" << '\n';
    for (const auto& rec : _records) {
        oss << rec.serialize() << '\n';
    }
    return _fileManager.writeAll(_fileRelPath, oss.str());
}

void Leaderboard::add(const ScoreRecord& record)
{
    _records.push_back(record);
    // 即时持久化：追加一行到文件
    _fileManager.appendLine(_fileRelPath, record.serialize());
}

// ---- 排序查询 ----

std::vector<ScoreRecord> Leaderboard::topByScore(int n) const
{
    auto sorted = _records;
    std::sort(sorted.begin(), sorted.end(),
              [](const ScoreRecord& a, const ScoreRecord& b) {
                  return a.score() > b.score();  // 降序
              });
    if (n > 0 && n < static_cast<int>(sorted.size())) {
        sorted.resize(n);
    }
    return sorted;
}

std::vector<ScoreRecord> Leaderboard::topByTime(int n) const
{
    auto sorted = _records;
    std::sort(sorted.begin(), sorted.end(),
              [](const ScoreRecord& a, const ScoreRecord& b) {
                  return a.timeSeconds() < b.timeSeconds();  // 升序（快在前）
              });
    if (n > 0 && n < static_cast<int>(sorted.size())) {
        sorted.resize(n);
    }
    return sorted;
}

std::vector<ScoreRecord> Leaderboard::recordsForPuzzle(const std::string& puzzleId) const
{
    std::vector<ScoreRecord> result;
    for (const auto& rec : _records) {
        if (rec.puzzleId() == puzzleId) {
            result.push_back(rec);
        }
    }
    return result;
}

const std::vector<ScoreRecord>& Leaderboard::all() const
{
    return _records;
}
