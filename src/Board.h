#pragma once

#include <cstdlib>
#include <iostream>
#include <vector>

#include "Push.h"

class Board {
public:
  static Board ParseFromText(std::istream &is);
  void DumpToText(std::ostream &os, bool quoteNewlines) const;

  void PerformPush(const Push &p);
  void PerformUnpush(const Push &p);

  uint64_t ExtractSearchHash() const;
  void ResetState(Position player, const std::vector<Position> &boxes);

  bool Done() const { return GoalsCompleted() == GoalsRequired(); }

  int GoalsCompleted() const { return goalsCompleted; }
  int GoalsRequired() const { return boxes.size(); }

  int Width() const { return width; }
  int Height() const { return height; }

  bool HasGoal(Position p) const { return goalArray[p] != -1; }
  bool HasWall(Position p) const { return wallArray[p]; }
  bool HasBox(Position p) const { return boxArray[p] != -1; }

  Position Player() const { return player; }
  void MovePlayer(Position p) { player = p; }
  const std::vector<Position> &Boxes() const { return boxes; }
  const std::vector<Position> &Goals() const { return goals; }

  Position MovePosition(Position p, Direction d) const {
    switch (d) {
    case Direction::UP:
      return p - width;
    case Direction::DOWN:
      return p + width;
    case Direction::LEFT:
      return p - 1;
    case Direction::RIGHT:
      return p + 1;
    }
    std::abort();
  }

  Position UnmovePosition(Position p, Direction d) const {
    switch (d) {
    case Direction::UP:
      return p + width;
    case Direction::DOWN:
      return p - width;
    case Direction::LEFT:
      return p + 1;
    case Direction::RIGHT:
      return p - 1;
    }
    std::abort();
  }

private:
  Board(int width, int height, Position player,
        const std::vector<bool> &wallArray, const std::vector<Position> &boxes,
        const std::vector<Position> &goalArray);

  void Normalize();

  int width, height;
  Position player;
  std::vector<Position> boxes;
  std::vector<Position> goals;
  std::vector<int> boxArray;
  std::vector<int> goalArray;
  std::vector<bool> wallArray;
  std::vector<uint64_t> boxArrayHashTable;
  std::vector<uint64_t> playerArrayHashTable;
  int goalsCompleted;
};
