#include <cassert>

#include "FreezeDeadlockDetector.h"

FreezeDeadlockDetector::FreezeDeadlockDetector(
    const Board &board,
    const SimpleDeadlockDetector &simpleDeadlockDetector)
    : board(board),
      visited(board.Width() * board.Height()),
      simpleDeadlockDetector(simpleDeadlockDetector) {}

bool IsDeadlockInternal(const Board &board,
                        const SimpleDeadlockDetector &simpleDeadlockDetector,
                        Position position,
                        std::vector<bool> &visited,
                        int &boxesVisited,
                        int &goalsVisited) {
  assert(board.HasBox(position));
  boxesVisited++;
  if (board.HasGoal(position)) {
    goalsVisited++;
  }
  visited[position] = true;
  for (Direction dir : ALL_DIRECTIONS) {
    Position posFront = board.MovePosition(position, dir);
    Position posBack = board.UnmovePosition(position, dir);
    if (!board.HasWall(posFront) &&
        !simpleDeadlockDetector.IsDeadlock(posFront) &&
        !board.HasWall(posBack) && !board.HasBox(posBack) &&
        (!board.HasBox(posFront) ||
         (!visited[posFront] &&
          !IsDeadlockInternal(board, simpleDeadlockDetector, posFront, visited,
                              boxesVisited, goalsVisited)))) {
      return false;
    }
  }
  return true;
}

bool FreezeDeadlockDetector::IsDeadlock(Position position) {
  std::fill(visited.begin(), visited.end(), false);
  int boxesVisited = 0;
  int goalsVisited = 0;
  return IsDeadlockInternal(board, simpleDeadlockDetector, position, visited,
                            boxesVisited, goalsVisited) &&
         (goalsVisited < boxesVisited);
}
