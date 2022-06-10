#pragma once

#include <vector>

#include "Board.h"
#include "SimpleDeadlockDetector.h"

class FreezeDeadlockDetector {
public:
  FreezeDeadlockDetector(const Board &board,
                         const SimpleDeadlockDetector &simpleDeadlockDetector);

  bool IsDeadlock(Position position);

private:
  const Board &board;
  const SimpleDeadlockDetector &simpleDeadlockDetector;
  std::vector<bool> visited;
};
