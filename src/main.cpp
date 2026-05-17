/**
 * main.cpp — 密阵突围 (Matrix Breakthrough) 程序入口
 *
 * C++ 实训项目，基于《最强大脑》第 13 季《密阵突围》赛题。
 * 在 25×25 的网格上进行字母+数字格子合并操作。
 *
 * 架构：Model-View-Controller (MVC)
 *   - Model：Cell, Grid, MergeRule, GameState, Move (核心)
 *            Puzzle, PuzzleManager, Leaderboard, ReplayBuffer, FileManager (存储)
 *   - View：IRenderer → ConsoleRenderer (当前), SFMLRenderer (Lv2+)
 *   - Controller：GameController, InputHandler, AIPlayer (Lv2+)
 *
 * 构建方式：
 *   cmake -B build -DENABLE_GUI=OFF   # 控制台版
 *   cmake --build build && ./build/matrix_breakthrough
 *
 * 参见同级目录下的 Architecture_Design.md 了解完整架构。
 */

#include "controller/GameController.hpp"
#include <iostream>
#include <conio.h>

int main()
{
    std::cout << "========================================\n";
    std::cout << "  密阵突围 — Matrix Breakthrough\n";
    std::cout << "  C++ 实训项目\n";
    std::cout << "  Version 1.0.0\n";
    std::cout << "========================================\n\n";

    try {
        GameController controller;
        controller.run();
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] 未捕获的异常: " << e.what() << std::endl;
        std::cerr << "按任意键退出..." << std::endl;
        _getch();
        return 1;
    }

    std::cout << "\n  感谢游玩！再见。\n";
    return 0;
}
