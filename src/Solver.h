#pragma once

#include <optional>
#include <ostream>
#include <vector>

#include "Board.h"
#include "DistanceTable.h"

class Solver {
 public:
  Solver(Board& board);

  int Solve(std::ostream* graphOutput);

 private:
  Board& board;
  std::vector<bool> simpleDeadlockArray;
  DistanceTable distanceTable;
};
