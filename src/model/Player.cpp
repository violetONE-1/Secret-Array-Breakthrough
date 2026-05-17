/**
 * Player.cpp — Player 类的实现
 */

#include "model/Player.hpp"

Player::Player() = default;

Player::Player(const std::string& name)
    : _name(name)
{
}

const std::string& Player::name() const { return _name; }

void Player::setName(const std::string& name) { _name = name; }

const std::vector<std::pair<int, int>>& Player::startingCells() const
{
    return _startingCells;
}

void Player::setStartingCells(const std::vector<std::pair<int, int>>& cells)
{
    _startingCells = cells;
}
