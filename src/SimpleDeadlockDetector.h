#pragma once

#include "Board.h"

class SimpleDeadlockDetector {
public:
  SimpleDeadlockDetector(const Board &board);

  bool IsDeadlock(Position box) const { return deadlockArray[box]; }

private:
  std::vector<bool> deadlockArray;
};