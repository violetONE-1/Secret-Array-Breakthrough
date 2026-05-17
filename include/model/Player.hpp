/**
 * Player.hpp — 玩家类 (Player)
 *
 * 封装玩家的身份信息和在一局游戏中的选择。
 * 轻量级值对象，在 GameController 中按值存储。
 *
 * 所属架构层：Model / Controller 交界
 * 所属模块：游戏流程控制模块
 */

#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <string>
#include <vector>
#include <utility>

class Player {
public:
    Player();
    explicit Player(const std::string& name);

    const std::string& name() const;
    void setName(const std::string& name);

    const std::vector<std::pair<int, int>>& startingCells() const;
    void setStartingCells(const std::vector<std::pair<int, int>>& cells);

private:
    std::string _name;
    // 玩家选择的 5 个起始格子坐标 (row, col)
    std::vector<std::pair<int, int>> _startingCells;
};

#endif // PLAYER_HPP
