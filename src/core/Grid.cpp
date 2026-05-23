/**
 * Grid.cpp — Grid 类的实现
 *
 * 见 include/core/Grid.hpp 的详细文档注释。
 */

#include "core/Grid.hpp"
#include "core/MergeRule.hpp"
#include <sstream>
#include <stdexcept>

// ---- 构造函数 ----

Grid::Grid(int rows, int cols)
    : _rows(rows)
    , _cols(cols)
{
    // 用全空格 Cell 填充二维 vector
    _cells.resize(rows, std::vector<Cell>(cols, Cell::makeEmpty()));
}

Grid::Grid()
    : _rows(0), _cols(0)
{
}

// ---- 尺寸查询 ----

int Grid::rows() const { return _rows; }
int Grid::cols() const { return _cols; }

// ---- 格子访问 ----

const Cell& Grid::at(int r, int c) const
{
    if (r < 0 || r >= _rows || c < 0 || c >= _cols) {
        throw std::out_of_range("Grid::at() 索引越界: (" +
            std::to_string(r) + "," + std::to_string(c) + ")");
    }
    return _cells[r][c];
}

Cell& Grid::at(int r, int c)
{
    if (r < 0 || r >= _rows || c < 0 || c >= _cols) {
        throw std::out_of_range("Grid::at() 索引越界: (" +
            std::to_string(r) + "," + std::to_string(c) + ")");
    }
    return _cells[r][c];
}

void Grid::setAt(int r, int c, const Cell& cell)
{
    if (r < 0 || r >= _rows || c < 0 || c >= _cols) {
        throw std::out_of_range("Grid::setAt() 索引越界");
    }
    _cells[r][c] = cell;
}

// ---- 核心合并操作 ----

void Grid::mergeCells(int sr, int sc, int dr, int dc, const Cell& mergedResult)
{
    // 将源格置空
    _cells[sr][sc] = Cell::makeEmpty();

    // 将中心格更新为合并结果
    _cells[dr][dc] = mergedResult;
}

// ---- 合法操作扫描 ----

std::vector<Move> Grid::findAllValidMoves() const
{
    std::vector<Move> moves;

    // 四个邻格方向：(dr, dc, Direction)
    static const int delta[4][3] = {
        {-1,  0, 0},  // UP
        { 1,  0, 1},  // DOWN
        { 0, -1, 2},  // LEFT
        { 0,  1, 3}   // RIGHT
    };
    static const Direction dirMap[4] = {
        Direction::UP, Direction::DOWN,
        Direction::LEFT, Direction::RIGHT
    };

    for (int r = 0; r < _rows; ++r) {
        for (int c = 0; c < _cols; ++c) {
            const Cell& center = _cells[r][c];
            if (center.isEmpty()) continue;  // 空格子不能作为中心

            for (int d = 0; d < 4; ++d) {
                int nr = r + delta[d][0];
                int nc = c + delta[d][1];

                // 边界检查
                if (nr < 0 || nr >= _rows || nc < 0 || nc >= _cols) continue;

                const Cell& neighbor = _cells[nr][nc];
                if (neighbor.isEmpty()) continue;

                // 合并规则校验
                if (MergeRule::canMerge(center, neighbor)) {
                    Cell result = MergeRule::getMergedCell(center, neighbor);
                    // 邻格 (nr, nc) → 中心 (r, c)，方向为邻格→中心的反方向
                    Direction revDir;
                    switch (dirMap[d]) {
                        case Direction::UP:    revDir = Direction::DOWN;  break;
                        case Direction::DOWN:  revDir = Direction::UP;    break;
                        case Direction::LEFT:  revDir = Direction::RIGHT; break;
                        case Direction::RIGHT: revDir = Direction::LEFT;  break;
                        default:               revDir = Direction::NONE;  break;
                    }
                    moves.emplace_back(MoveType::MERGE, nr, nc, r, c, revDir,
                                       result.getLetter(), result.getNumber());
                }
            }
        }
    }

    return moves;
}

bool Grid::hasAnyValidMove() const
{
    // 遍历全盘，只要找到一个合法操作就立即返回 true
    for (int r = 0; r < _rows; ++r) {
        for (int c = 0; c < _cols; ++c) {
            const Cell& center = _cells[r][c];
            if (center.isEmpty()) continue;

            // 检查上、下、左、右四个邻居
            if (r > 0 && MergeRule::canMerge(center, _cells[r-1][c])) return true;
            if (r < _rows-1 && MergeRule::canMerge(center, _cells[r+1][c])) return true;
            if (c > 0 && MergeRule::canMerge(center, _cells[r][c-1])) return true;
            if (c < _cols-1 && MergeRule::canMerge(center, _cells[r][c+1])) return true;
        }
    }
    return false;
}

// ---- 序列化 ----

std::string Grid::serialize() const
{
    // 格式：
    //   rows cols
    //   每行：letter0number0 letter1number1 ... letterNnumberN
    //   字母用 'A'~'Z'，空格用 '.' 表示，数字紧跟其后
    std::ostringstream oss;
    oss << _rows << ' ' << _cols << '\n';

    for (int r = 0; r < _rows; ++r) {
        for (int c = 0; c < _cols; ++c) {
            if (c > 0) oss << ' ';
            const Cell& cell = _cells[r][c];
            if (cell.isEmpty()) {
                oss << ".0";   // 空格子用 .0 表示
            } else {
                oss << cell.getLetter() << cell.getNumber();
            }
        }
        oss << '\n';
    }

    return oss.str();
}

bool Grid::deserialize(const std::string& data, Grid& outGrid)
{
    std::istringstream iss(data);

    int rows, cols;
    if (!(iss >> rows >> cols)) return false;
    if (rows <= 0 || cols <= 0) return false;

    // 跳过第一行剩余字符到换行
    iss.ignore(1024, '\n');

    Grid temp(rows, cols);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            std::string token;
            if (!(iss >> token)) return false;
            if (token.size() < 2) return false;

            if (token[0] == '.') {
                temp.setAt(r, c, Cell::makeEmpty());
            } else {
                char letter  = token[0];
                int  number  = token[1] - '0';
                temp.setAt(r, c, Cell(letter, number));
            }
        }
    }

    outGrid = std::move(temp);
    return true;
}

void Grid::slideCell(int sr, int sc, int dr, int dc)
{
    _cells[dr][dc] = _cells[sr][sc];
    _cells[sr][sc] = Cell::makeEmpty();
}

void Grid::clear()
{
    for (auto& row : _cells) {
        for (auto& cell : row) {
            cell = Cell::makeEmpty();
        }
    }
}
