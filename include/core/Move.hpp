/**
 * Move.hpp — 操作记录类 (Move) + 方向枚举 (Direction)
 *
 * 记录单步合并操作的完整信息，用于：
 *   - 回放缓冲区的序列化/反序列化
 *   - 操作历史（GameState::_moveHistory）
 *   - 正确率计算
 *
 * Move 为轻量级值对象（Value Object），通过 std::vector<Move> 存储。
 *
 * 所属架构层：Model（核心数据层）
 * 所属模块：游戏核心逻辑模块
 */

#ifndef MOVE_HPP
#define MOVE_HPP

#include <string>

/** 合并移动方向 */
enum class Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    NONE    // 无方向（初始值）
};

/**
 * 将 Direction 枚举转为字符串表示：
 *   UP→"UP", DOWN→"DOWN", LEFT→"LEFT", RIGHT→"RIGHT"
 */
std::string directionToString(Direction dir);

/**
 * 从字符串解析 Direction。
 * 不区分大小写；无法识别时返回 Direction::NONE。
 */
Direction stringToDirection(const std::string& s);

/**
 * Move 表示一次合并操作。
 * 字段含义：
 *   - (_srcRow, _srcCol)：被移动格子（邻格）坐标，合并后此格置空
 *   - (_dstRow, _dstCol)：中心格子坐标，合并后此格更新为结果
 *   - _direction：移动方向（由邻格→中心的方向）
 *   - _resultLetter / _resultNumber：合并后中心格的内容
 */
struct Move {
    int       srcRow, srcCol;
    int       dstRow, dstCol;
    Direction direction;
    char      resultLetter;
    int       resultNumber;

    /** 构造全零 Move（NONE 方向，空格结果） */
    Move();

    /** 构造完整 Move */
    Move(int sr, int sc, int dr, int dc,
         Direction dir, char rl, int rn);

    // ---- 序列化 ----

    /**
     * 序列化为回放文件中的一行：
     *   "[Move] 001 3,4 -> 3,5 RIGHT A5"
     * seqNum 为步骤序号（外部传入，不回存在 Move 中）。
     */
    std::string serialize(int seqNum) const;

    /**
     * 从回放文件中的 [Move] 行反序列化。
     * 格式："[Move] 001 3,4 -> 3,5 RIGHT A5"
     * 成功返回 true，失败返回 false 且 outMove 不变。
     */
    static bool deserialize(const std::string& line, Move& outMove, int& outSeqNum);
};

#endif // MOVE_HPP
