/**
 * ScoreRecord.cpp — ScoreRecord 类的实现
 */

#include "model/ScoreRecord.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

ScoreRecord::ScoreRecord()
    : _timeSeconds(0), _steps(0), _accuracy(0), _score(0)
{
}

ScoreRecord::ScoreRecord(const std::string& playerName,
                         const std::string& puzzleId,
                         double timeSeconds,
                         int    steps,
                         double accuracy,
                         const std::string& timestamp)
    : _playerName(playerName)
    , _puzzleId(puzzleId)
    , _timeSeconds(timeSeconds)
    , _steps(steps)
    , _accuracy(accuracy)
    , _timestamp(timestamp)
{
    _score = calculateScore(timeSeconds, steps, accuracy);
}

// ---- Getters ----

const std::string& ScoreRecord::playerName()  const { return _playerName; }
const std::string& ScoreRecord::puzzleId()    const { return _puzzleId; }
double             ScoreRecord::timeSeconds() const { return _timeSeconds; }
int                ScoreRecord::steps()       const { return _steps; }
double             ScoreRecord::accuracy()    const { return _accuracy; }
int                ScoreRecord::score()       const { return _score; }
const std::string& ScoreRecord::timestamp()   const { return _timestamp; }

void ScoreRecord::setScore(int score) { _score = score; }

// ---- 得分计算 ----

int ScoreRecord::calculateScore(double timeSeconds, int steps, double accuracy)
{
    // 公式：accuracy × 1000 + max(0, 500 − time × 2) + steps × 10
    int score = static_cast<int>(std::round(accuracy * 1000.0));
    int timeBonus = 500 - static_cast<int>(timeSeconds * 2.0);
    if (timeBonus < 0) timeBonus = 0;
    score += timeBonus;
    score += steps * 10;
    return score;
}

// ---- 序列化 ----

std::string ScoreRecord::serialize() const
{
    std::ostringstream oss;
    oss << _playerName << ", "
        << _puzzleId << ", "
        << std::fixed << std::setprecision(2) << _timeSeconds << ", "
        << _steps << ", "
        << std::fixed << std::setprecision(2) << _accuracy << ", "
        << _score << ", "
        << _timestamp;
    return oss.str();
}

bool ScoreRecord::deserialize(const std::string& line, ScoreRecord& outRecord)
{
    std::istringstream iss(line);

    std::string playerName, puzzleId, timestamp;
    double timeSeconds, accuracy;
    int steps;

    // 按逗号分隔读取
    if (!std::getline(iss, playerName, ',')) return false;
    if (!(iss >> puzzleId)) return false; iss.ignore(1); // skip ','

    if (!(iss >> timeSeconds)) return false; iss.ignore(1);
    if (!(iss >> steps)) return false; iss.ignore(1);
    if (!(iss >> accuracy)) return false; iss.ignore(1);

    int score;
    if (!(iss >> score)) return false; iss.ignore(1);

    std::getline(iss, timestamp); // 剩余部分为时间戳

    outRecord = ScoreRecord(playerName, puzzleId, timeSeconds,
                            steps, accuracy, timestamp);
    return true;
}
