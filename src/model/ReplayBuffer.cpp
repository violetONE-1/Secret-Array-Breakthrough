/**
 * ReplayBuffer.cpp — ReplayBuffer 类的实现
 */

#include "model/ReplayBuffer.hpp"
#include <sstream>
#include <iomanip>
#include <ctime>

ReplayBuffer::ReplayBuffer(FileManager& fileManager)
    : _fileManager(fileManager)
    , _recording(false)
    , _gridRows(0)
    , _gridCols(0)
{
}

void ReplayBuffer::startRecording(const std::string& puzzleId,
                                  int gridRows, int gridCols,
                                  const std::string& playerName,
                                  const std::string& startsStr)
{
    _recording   = true;
    _puzzleId    = puzzleId;
    _gridRows    = gridRows;
    _gridCols    = gridCols;
    _playerName  = playerName;
    _startsStr   = startsStr;
    _moves.clear();

    // 生成开始时间戳
    auto now = std::time(nullptr);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
    _startTime = oss.str();
    _endTime.clear();
}

void ReplayBuffer::stopRecording()
{
    auto now = std::time(nullptr);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
    _endTime = oss.str();
    _recording = false;
}

void ReplayBuffer::record(const Move& move)
{
    if (_recording) {
        _moves.push_back(move);
    }
}

bool ReplayBuffer::isRecording() const { return _recording; }

bool ReplayBuffer::saveToFile(const std::string& relPath)
{
    std::ostringstream oss;
    oss << "# 密阵突围 回放记录\n";
    oss << "[Puzzle] " << _puzzleId << '\n';
    oss << "[Grid] " << _gridRows << ' ' << _gridCols << '\n';
    oss << "[Player] " << _playerName << '\n';
    oss << "[Starts] " << _startsStr << '\n';
    oss << "[Start] " << _startTime << '\n';

    std::string endTime = _endTime;
    if (endTime.empty()) {
        auto now = std::time(nullptr);
        std::ostringstream tss;
        tss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
        endTime = tss.str();
    }
    oss << "[End] " << endTime << '\n';
    oss << "[TotalSteps] " << _moves.size() << '\n';
    oss << "# 操作序列：序号 源坐标 -> 目标坐标 方向 结果\n";

    for (size_t i = 0; i < _moves.size(); ++i) {
        oss << _moves[i].serialize(static_cast<int>(i + 1)) << '\n';
    }

    return _fileManager.writeAll(relPath, oss.str());
}

bool ReplayBuffer::loadFromFile(const std::string& relPath,
                                std::string& outPuzzleId,
                                int& outRows, int& outCols,
                                std::string& outPlayerName,
                                std::string& outStartTime,
                                std::string& outEndTime,
                                int& outTotalSteps,
                                std::vector<Move>& outMoves)
{
    std::string content = _fileManager.readAll(relPath);
    if (content.empty()) return false;

    outMoves.clear();

    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty() || line[0] == '#') continue;

        if (line.rfind("[Puzzle]", 0) == 0) {
            outPuzzleId = line.substr(8);
            // trim
            while (!outPuzzleId.empty() && outPuzzleId[0] == ' ') outPuzzleId.erase(0, 1);
        } else if (line.rfind("[Grid]", 0) == 0) {
            std::istringstream gs(line.substr(6));
            gs >> outRows >> outCols;
        } else if (line.rfind("[Player]", 0) == 0) {
            outPlayerName = line.substr(8);
            while (!outPlayerName.empty() && outPlayerName[0] == ' ') outPlayerName.erase(0, 1);
        } else if (line.rfind("[Start]", 0) == 0) {
            outStartTime = line.substr(7);
            while (!outStartTime.empty() && outStartTime[0] == ' ') outStartTime.erase(0, 1);
        } else if (line.rfind("[End]", 0) == 0) {
            outEndTime = line.substr(5);
            while (!outEndTime.empty() && outEndTime[0] == ' ') outEndTime.erase(0, 1);
        } else if (line.rfind("[TotalSteps]", 0) == 0) {
            outTotalSteps = std::stoi(line.substr(12));
        } else if (line.rfind("[Move]", 0) == 0) {
            Move move;
            int seqNum;
            if (Move::deserialize(line, move, seqNum)) {
                outMoves.push_back(move);
            }
        }
    }

    return true;
}

const std::vector<Move>& ReplayBuffer::moves() const { return _moves; }
