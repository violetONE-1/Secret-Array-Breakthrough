/**
 * PuzzleManager.cpp — PuzzleManager 类的实现
 *
 * 内置 3 道 25×25 题面，通过随机种子生成保证可玩性（确保全盘始终存在合法合并操作）。
 * 首次运行时将内置题面写入 data/puzzles/ 目录。
 */

#include "model/PuzzleManager.hpp"
#include "core/MergeRule.hpp"
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <filesystem>

PuzzleManager::PuzzleManager(FileManager& fileManager)
    : _fileManager(fileManager)
{
}

void PuzzleManager::loadAll()
{
    _puzzles.clear();

    // 1. 初始化内置题面
    createBuiltinPuzzles();

    // 2. 加载自定义题面
    loadPuzzlesFromDir("puzzles/custom");
}

const std::vector<Puzzle>& PuzzleManager::all() const { return _puzzles; }

const Puzzle* PuzzleManager::findById(const std::string& id) const
{
    for (const auto& p : _puzzles) {
        if (p.id() == id) return &p;
    }
    return nullptr;
}

bool PuzzleManager::addPuzzle(const Puzzle& puzzle)
{
    // 检查 ID 不重复
    if (findById(puzzle.id()) != nullptr) {
        std::cerr << "[PuzzleManager] 题面 ID '" << puzzle.id() << "' 已存在" << std::endl;
        return false;
    }

    // 保存到文件
    std::string filePath = "puzzles/custom/" + puzzle.id() + ".txt";
    if (!_fileManager.writeAll(filePath, puzzle.serialize())) {
        std::cerr << "[PuzzleManager] 无法保存题面到 " << filePath << std::endl;
        return false;
    }

    _puzzles.push_back(puzzle);
    return true;
}

void PuzzleManager::reload()
{
    loadAll();
}

// ---- 内部辅助 ----

void PuzzleManager::createBuiltinPuzzles()
{
    // 生成 3 道内置题面
    // 使用确定性随机种子保证每次运行生成相同题面
    const int N = 25;

    // 检查是否已有文件
    bool filesExist = _fileManager.exists("puzzles/builtin_01.txt") &&
                      _fileManager.exists("puzzles/builtin_02.txt") &&
                      _fileManager.exists("puzzles/builtin_03.txt");

    if (filesExist) {
        // 从文件加载
        loadPuzzlesFromDir("puzzles");
        return;
    }

    // 首次运行：生成并保存题面
    const char* names[] = {
        "Beginner Training",
        "Advanced Challenge",
        "Matrix Master"
    };
    const char* ids[] = { "builtin_01", "builtin_02", "builtin_03" };

    std::string allStartsStr = "0,0 3,3 6,10 12,15 20,20";  // 默认起点

    for (int pid = 0; pid < 3; ++pid) {
        std::srand(42 + pid * 1000);  // 固定种子，每道题不同

        Grid grid(N, N);

        // 随机填充字母 (A~Z) 和数字 (0~9)
        for (int r = 0; r < N; ++r) {
            for (int c = 0; c < N; ++c) {
                char letter = static_cast<char>('A' + (std::rand() % 26));
                int  number = std::rand() % 10;
                grid.setAt(r, c, Cell(letter, number));
            }
        }

        // Connectivity pass: ensure every cell has at least one mergeable neighbor,
        // so the board is playable from any starting position
        static const int dirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        for (int r = 0; r < N; ++r) {
            for (int c = 0; c < N; ++c) {
                // Check if any neighbor can already merge with this cell
                bool hasMergeable = false;
                for (int d = 0; d < 4; ++d) {
                    int nr = r + dirs[d][0];
                    int nc = c + dirs[d][1];
                    if (nr < 0 || nr >= N || nc < 0 || nc >= N) continue;
                    if (MergeRule::canMerge(grid.at(r, c), grid.at(nr, nc))) {
                        hasMergeable = true;
                        break;
                    }
                }
                if (hasMergeable) continue;

                // Pick a random neighbor and make this cell mergeable with it
                int candR[4], candC[4];
                int nCount = 0;
                for (int d = 0; d < 4; ++d) {
                    int nr = r + dirs[d][0];
                    int nc = c + dirs[d][1];
                    if (nr < 0 || nr >= N || nc < 0 || nc >= N) continue;
                    candR[nCount] = nr;
                    candC[nCount] = nc;
                    ++nCount;
                }
                if (nCount == 0) continue;

                int pick = std::rand() % nCount;
                const Cell& neighbor = grid.at(candR[pick], candC[pick]);

                if (std::rand() % 2 == 0) {
                    // Same letter, different number
                    int newNum = (neighbor.getNumber() + 1 + (std::rand() % 9)) % 10;
                    grid.setAt(r, c, Cell(neighbor.getLetter(), newNum));
                } else {
                    // Same number, different letter
                    char base = neighbor.getLetter();
                    char newLetter = static_cast<char>('A' + ((base - 'A' + 1 + (std::rand() % 25)) % 26));
                    grid.setAt(r, c, Cell(newLetter, neighbor.getNumber()));
                }
            }
        }

        // Safety guard: ensure at least one valid move exists
        if (!grid.hasAnyValidMove()) {
            grid.setAt(0, 0, Cell('A', 1));
            grid.setAt(0, 1, Cell('A', 2));
        }

        Puzzle puzzle(ids[pid], names[pid], grid);

        // 保存为文件
        std::string fullContent = puzzle.serialize();
        // 追加起点信息
        fullContent += "5\n" + allStartsStr + "\n";
        std::string filePath = std::string("puzzles/") + ids[pid] + ".txt";
        _fileManager.writeAll(filePath, fullContent);

        // 添加到内存
        _puzzles.push_back(puzzle);
    }
}

void PuzzleManager::loadPuzzlesFromDir(const std::string& dirRelPath)
{
    // 注意：C++17 filesystem 的 directory_iterator 跨平台可用
    std::string fullDir = _fileManager.dataRoot() + "/" + dirRelPath;

    // 尝试遍历目录
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(fullDir, ec)) return;

    for (const auto& entry : fs::directory_iterator(fullDir, ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();
        // 仅读取 .txt 文件
        if (ext != ".txt") continue;

        std::string content = _fileManager.readAll(
            dirRelPath + "/" + entry.path().filename().string()
        );
        if (content.empty()) continue;

        Puzzle puzzle;
        if (Puzzle::deserialize(content, puzzle)) {
            // 防止重复（按 ID 去重）
            bool dup = false;
            for (const auto& p : _puzzles) {
                if (p.id() == puzzle.id()) { dup = true; break; }
            }
            if (!dup) {
                _puzzles.push_back(puzzle);
            }
        }
    }
}
