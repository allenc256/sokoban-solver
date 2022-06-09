#pragma once

#include <optional>
#include <ostream>
#include <vector>

#include "Board.h"
#include "DistanceTable.h"
#include "SimpleDeadlockTable.h"
#include "SolveResult.h"

class Solver {
public:
  Solver(Board &board, int maxStates);

  SolveResult Solve(std::ostream *graphOutput);

private:
  Board &board;
  SimpleDeadlockTable simpleDeadlockTable;
  DistanceTable distanceTable;
  int maxStates;
};
