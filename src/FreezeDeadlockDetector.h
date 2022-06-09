#pragma once

#include <vector>

#include "Board.h"

class FreezeDeadlockDetector {
public:
  FreezeDeadlockDetector(const Board &board);

  bool IsDeadlock(Position position);

private:
  const Board &board;
  std::vector<bool> visited;
};
