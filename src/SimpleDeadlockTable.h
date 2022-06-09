#pragma once

#include "Board.h"

class SimpleDeadlockTable {
public:
  SimpleDeadlockTable(const Board &board);

  bool IsDeadlock(Position box) const { return deadlockArray[box]; }

private:
  std::vector<bool> deadlockArray;
};