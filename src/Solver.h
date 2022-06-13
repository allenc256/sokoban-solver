#pragma once

#include <optional>
#include <ostream>
#include <vector>

#include "Board.h"
#include "DistanceTable.h"
#include "FreezeDeadlockDetector.h"
#include "PushSearcher.h"
#include "SimpleDeadlockDetector.h"
#include "SolveResult.h"

class Solver {
public:
  Solver(Board &board, int maxStates);

  SolveResult Solve(std::ostream *graphOutput);

private:
  Board &board;
  SimpleDeadlockDetector simpleDeadlockDetector;
  FreezeDeadlockDetector freezeDeadlockDetector;
  PushSearcher pushSearcher;
  DistanceTable distanceTable;
  int maxStates;
};
