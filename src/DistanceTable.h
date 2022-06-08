#pragma once

#include "Board.h"

class DistanceTable {
public:
  DistanceTable(const Board &board);

  int EstimateDistance(const std::vector<Position> &boxes) const;

private:
  const Board &board;
  std::vector<std::vector<int>> distances;
  mutable std::vector<int> buffer;
};
