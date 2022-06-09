#include <cassert>

#include "FreezeDeadlockDetector.h"

FreezeDeadlockDetector::FreezeDeadlockDetector(const Board &board)
    : board(board), visited(board.Width() * board.Height()) {}

bool IsDeadlockInternal(const Board &board,
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
    if (!board.HasWall(posFront) && !board.HasWall(posBack) &&
        !board.HasBox(posBack) &&
        (!board.HasBox(posFront) ||
         (!visited[posFront] &&
          !IsDeadlockInternal(board, posFront, visited, boxesVisited,
                              goalsVisited)))) {
      return false;
    }
  }
  return true;
}

bool FreezeDeadlockDetector::IsDeadlock(Position position) {
  std::fill(visited.begin(), visited.end(), false);
  int boxesVisited = 0;
  int goalsVisited = 0;
  return IsDeadlockInternal(board, position, visited, boxesVisited,
                            goalsVisited) &&
         (goalsVisited < boxesVisited);
}
