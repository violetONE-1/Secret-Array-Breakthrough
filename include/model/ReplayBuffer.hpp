/**
 * ReplayBuffer.hpp — 回放缓冲区类 (ReplayBuffer)
 *
 * 录制整局 Move 操作序列，支持：
 *   - 开始/停止录制
 *   - 逐步记录
 *   - 导出为带元信息的回放文本文件
 *   - 从回放文件加载操作序列
 *
 * 回放文件格式：
 *   [Puzzle] puzzle_id
 *   [Grid] rows cols
 *   [Player] player_name
 *   [Start] timestamp
 *   [End] timestamp
 *   [TotalSteps] n
 *   [Move] 001 src -> dst 方向 结果
 *   ...
 *
 * 所属架构层：Model（数据持久化子层）
 * 所属模块：数据持久化与存储模块
 */

#ifndef REPLAYBUFFER_HPP
#define REPLAYBUFFER_HPP

#include "core/Move.hpp"
#include "FileManager.hpp"
#include <string>
#include <vector>

class ReplayBuffer {
public:
    ReplayBuffer(FileManager& fileManager);

    // ---- 录制控制 ----

    /** 开始录制：指定题面 ID、网格尺寸、玩家名、起始坐标 */
    void startRecording(const std::string& puzzleId,
                        int gridRows, int gridCols,
                        const std::string& playerName,
                        const std::string& startsStr);

    /** 停止录制 */
    void stopRecording();

    /** 录制一步操作 */
    void record(const Move& move);

    /** 是否正在录制 */
    bool isRecording() const;

    // ---- 文件操作 ----

    /** 将录制内容保存为回放文件 */
    bool saveToFile(const std::string& relPath);

    /** 从回放文件加载操作序列，成功返回 true */
    bool loadFromFile(const std::string& relPath,
                      std::string& outPuzzleId,
                      int& outRows, int& outCols,
                      std::string& outPlayerName,
                      std::string& outStartTime,
                      std::string& outEndTime,
                      int& outTotalSteps,
                      std::vector<Move>& outMoves);

    /** 加载后返回的 moves 序列 */
    const std::vector<Move>& moves() const;

private:
    FileManager&    _fileManager;
    bool            _recording;
    std::string     _puzzleId;
    int             _gridRows;
    int             _gridCols;
    std::string     _playerName;
    std::string     _startsStr;
    std::string     _startTime;
    std::string     _endTime;
    std::vector<Move> _moves;
};

#endif // REPLAYBUFFER_HPP
