#pragma once

#include <vector>

#include "Position.h"
#include "Push.h"

class SearchState {
public:
  SearchState(uint64_t id,
              Position player,
              const std::vector<Position> &boxes,
              const std::vector<Push> &pushes,
              int aStarGValue,
              int aStarHValue)
      : id(id),
        player(player),
        boxes(boxes),
        pushes(pushes),
        aStarGValue(aStarGValue),
        aStarHValue(aStarHValue) {}

  uint64_t Id() const { return id; }
  Position Player() const { return player; }
  const std::vector<Position> &Boxes() const { return boxes; }
  const std::vector<Push> &Pushes() const { return pushes; }
  int AStarGValue() const { return aStarGValue; }
  int AStarHValue() const { return aStarHValue; }
  int AStarFValue() const { return aStarHValue + aStarGValue; }
  void AStarGValue(int newValue) { aStarGValue = newValue; }

private:
  uint64_t id;
  Position player;
  std::vector<Position> boxes;
  std::vector<Push> pushes;
  int aStarGValue;
  int aStarHValue;
};
