/**
 * Move.cpp — Move 与 Direction 相关函数的实现
 *
 * 见 include/core/Move.hpp 的详细文档注释。
 */

#include "core/Move.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

// ---- Direction 字符串转换 ----

std::string directionToString(Direction dir)
{
    switch (dir) {
        case Direction::UP:    return "UP";
        case Direction::DOWN:  return "DOWN";
        case Direction::LEFT:  return "LEFT";
        case Direction::RIGHT: return "RIGHT";
        default:               return "NONE";
    }
}

Direction stringToDirection(const std::string& s)
{
    std::string upper = s;
    // 转为大写以支持不区分大小写的匹配
    for (auto& ch : upper) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));

    if (upper == "UP")    return Direction::UP;
    if (upper == "DOWN")  return Direction::DOWN;
    if (upper == "LEFT")  return Direction::LEFT;
    if (upper == "RIGHT") return Direction::RIGHT;
    return Direction::NONE;
}

// ---- Move 构造 ----

Move::Move()
    : moveType(MoveType::MERGE)
    , srcRow(0), srcCol(0)
    , dstRow(0), dstCol(0)
    , direction(Direction::NONE)
    , resultLetter('A')
    , resultNumber(0)
{
}

Move::Move(MoveType type, int sr, int sc, int dr, int dc,
           Direction dir, char rl, int rn)
    : moveType(type)
    , srcRow(sr), srcCol(sc)
    , dstRow(dr), dstCol(dc)
    , direction(dir)
    , resultLetter(rl)
    , resultNumber(rn)
{
}

// ---- 序列化 ----

std::string Move::serialize(int seqNum) const
{
    const char* tag = (moveType == MoveType::SLIDE) ? "[Slide]" : "[Move]";
    std::ostringstream oss;
    oss << tag << ' '
        << std::setw(3) << std::setfill('0') << seqNum << ' '
        << srcRow << ',' << srcCol << " -> "
        << dstRow << ',' << dstCol << ' '
        << directionToString(direction) << ' '
        << resultLetter << resultNumber;
    return oss.str();
}

bool Move::deserialize(const std::string& line, Move& outMove, int& outSeqNum)
{
    // 期望格式："[Move] 001 3,4 -> 3,5 RIGHT A5"  或  "[Slide] ..."
    std::istringstream iss(line);
    std::string tag;
    if (!(iss >> tag)) return false;

    if (tag == "[Move]" || tag == "[Merge]") {
        outMove.moveType = MoveType::MERGE;
    } else if (tag == "[Slide]") {
        outMove.moveType = MoveType::SLIDE;
    } else {
        return false;
    }

    // 序号
    if (!(iss >> outSeqNum)) return false;

    // "3,4"
    std::string srcCoord;
    if (!(iss >> srcCoord)) return false;
    auto commaPos = srcCoord.find(',');
    if (commaPos == std::string::npos) return false;
    outMove.srcRow = std::stoi(srcCoord.substr(0, commaPos));
    outMove.srcCol = std::stoi(srcCoord.substr(commaPos + 1));

    // "->"
    std::string arrow;
    if (!(iss >> arrow) || arrow != "->") return false;

    // "3,5"
    std::string dstCoord;
    if (!(iss >> dstCoord)) return false;
    commaPos = dstCoord.find(',');
    if (commaPos == std::string::npos) return false;
    outMove.dstRow = std::stoi(dstCoord.substr(0, commaPos));
    outMove.dstCol = std::stoi(dstCoord.substr(commaPos + 1));

    // "RIGHT"
    std::string dirStr;
    if (!(iss >> dirStr)) return false;
    outMove.direction = stringToDirection(dirStr);

    // "A5"
    std::string result;
    if (!(iss >> result) || result.size() < 2) return false;
    outMove.resultLetter = result[0];
    outMove.resultNumber = result[1] - '0';

    return true;
}
