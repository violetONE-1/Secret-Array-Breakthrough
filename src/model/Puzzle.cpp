/**
 * Puzzle.cpp — Puzzle 类的实现
 */

#include "model/Puzzle.hpp"

Puzzle::Puzzle()
    : _id(""), _name("")
{
}

Puzzle::Puzzle(const std::string& id, const std::string& name,
               const Grid& initialGrid)
    : _id(id), _name(name), _initialGrid(initialGrid)
{
}

const std::string& Puzzle::id()   const { return _id; }
const std::string& Puzzle::name() const { return _name; }

int Puzzle::gridSize() const
{
    return _initialGrid.rows();
}

const Grid& Puzzle::initialGrid() const { return _initialGrid; }

std::string Puzzle::serialize() const
{
    // Puzzle 的序列化即 Grid 的序列化（网格数据）
    // 如需存储题面元信息，可在第一行添加 "# id name" 注释
    std::string result;
    result += "# " + _id + " " + _name + "\n";
    result += _initialGrid.serialize();
    return result;
}

bool Puzzle::deserialize(const std::string& data, Puzzle& outPuzzle)
{
    // 跳过第一行的注释 "# id name"
    std::string gridData = data;
    std::string id = "unknown";
    std::string name = "Unnamed";

    if (data.size() > 0 && data[0] == '#') {
        // 提取第一行
        size_t newlinePos = data.find('\n');
        if (newlinePos != std::string::npos) {
            std::string header = data.substr(1, newlinePos - 1);
            size_t spacePos = header.find(' ');
            if (spacePos != std::string::npos) {
                id = header.substr(0, spacePos);
                name = header.substr(spacePos + 1);
            }
            gridData = data.substr(newlinePos + 1);
        }
    }

    Grid grid;
    if (!Grid::deserialize(gridData, grid)) return false;

    outPuzzle = Puzzle(id, name, grid);
    return true;
}
