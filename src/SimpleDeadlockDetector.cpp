#include "SimpleDeadlockDetector.h"

static void SearchForSimpleDeadlocks(const Board &board,
                                     Position p,
                                     std::vector<bool> &deadlockArray,
                                     std::vector<bool> &deadlockVisited);

static void SearchForSimpleDeadlocks(const Board &board,
                                     Position p,
                                     std::vector<bool> &deadlockArray,
                                     std::vector<bool> &deadlockVisited) {
  if (deadlockVisited[p]) {
    return;
  }
  deadlockArray[p] = false;
  deadlockVisited[p] = true;
  for (auto d : ALL_DIRECTIONS) {
    Position p2 = board.MovePosition(p, d);
    if (!board.HasWall(p2)) {
      Position p3 = board.MovePosition(p2, d);
      if (!board.HasWall(p3)) {
        SearchForSimpleDeadlocks(board, p2, deadlockArray, deadlockVisited);
      }
    }
  }
}

SimpleDeadlockDetector::SimpleDeadlockDetector(const Board &board)
    : deadlockArray(board.Width() * board.Height(), true) {
  std::vector<bool> visited(board.Width() * board.Height(), false);
  for (Position p = 0; p < board.Width() * board.Height(); p++) {
    if (board.HasGoal(p)) {
      SearchForSimpleDeadlocks(board, p, deadlockArray, visited);
    }
  }
}
