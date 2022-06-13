#include <cassert>

#include "PushSearcher.h"

PushSearcher::PushSearcher(const Board &board,
                           const SimpleDeadlockDetector &simpleDeadlockDetector)
    : board(board),
      simpleDeadlockDetector(simpleDeadlockDetector),
      playerVisited(board.Size(), false),
      pushesVisited(board.Size() * 4, false),
      corralVisited(board.Size(), false),
      corralPushVisited(board.Size(), false),
      corralEdgeBoxes(board.Size(), false) {}

PushSearchResult PushSearcher::FindPushes(std::vector<Push> &pushes) {
  Position normPlayer = FindUnprunedPushes(pushes);
  bool isPICorral = PruneCorrals(pushes);
  PruneSimpleDeadlocks(pushes);
  return PushSearchResult(normPlayer, isPICorral);
}

Position PushSearcher::FindUnprunedPushes(std::vector<Push> &pushes) {
  // Initialize input data structures.
  pushes.clear();
  std::fill(playerVisited.begin(), playerVisited.end(), false);
  std::fill(pushesVisited.begin(), pushesVisited.end(), false);
  stack.clear();
  stack.push_back(board.Player());

  // Perform DFS.
  Position normPlayer = board.Player();
  while (!stack.empty()) {
    Position p = stack.back();
    stack.pop_back();
    if (playerVisited[p]) {
      continue;
    }
    playerVisited[p] = true;
    for (Direction d : ALL_DIRECTIONS) {
      Position p2 = board.MovePosition(p, d);
      if (playerVisited[p2]) {
        continue;
      }
      if (board.HasWall(p2)) {
        continue;
      }
      if (board.HasBox(p2)) {
        Position p3 = board.MovePosition(p2, d);
        if (!board.HasBox(p3) && !board.HasWall(p3)) {
          pushesVisited[(int)d * board.Size() + p2] = true;
          pushes.emplace_back(p2, d);
        }
        continue;
      }
      if (p2 < normPlayer) {
        normPlayer = p2;
      }
      stack.push_back(p2);
    }
  }

  return normPlayer;
}

bool PushSearcher::PruneCorrals(std::vector<Push> &pushes) {
  std::fill(corralVisited.begin(), corralVisited.end(), false);

  for (const Push &push : pushes) {
    Position pushTo = board.MovePosition(push.Box(), push.Direction());

    // If push into player visited region, this is not a corral.
    if (playerVisited[pushTo]) {
      continue;
    }

    // Otherwise this push leads to a corral.
    // If we've already processed this corral, then we can skip.
    if (corralVisited[pushTo]) {
      continue;
    }

    // Do a DFS to find the extent of the corral.
    // N.B., unlike "corralVisited", "corralEdgeBoxes" is cleared every time we
    // do a DFS since any given box may belong to multiple corrals.
    bool foundNonGoalCorralEdgeBox = false;
    std::fill(corralEdgeBoxes.begin(), corralEdgeBoxes.end(), false);
    stack.clear();
    stack.push_back(pushTo);
    while (!stack.empty()) {
      Position p = stack.back();
      stack.pop_back();
      if (corralVisited[p]) {
        continue;
      }
      corralVisited[p] = true;
      for (Direction d : ALL_DIRECTIONS) {
        Position p2 = board.MovePosition(p, d);
        if (corralVisited[p2]) {
          continue;
        }
        if (board.HasWall(p2)) {
          continue;
        }
        if (board.HasBox(p2)) {
          corralEdgeBoxes[p2] = true;
          if (!board.HasGoal(p2)) {
            foundNonGoalCorralEdgeBox = true;
          }
          continue;
        }
        stack.push_back(p2);
      }
    }

    // If all corral edge boxes sit on goal squares, pruning is not permitted,
    // as it is no longer guaranteed that the player must push a corral edge box
    // first to reach a solution.
    if (!foundNonGoalCorralEdgeBox) {
      continue;
    }

    // Do a secondary DFS to check if:
    // 1. All corral edge box pushes land inside the corral, and
    // 2. All corral edge box pushes can be made currently by the player.
    // If these conditions hold, we may use the corral for pruning.
    std::fill(corralPushVisited.begin(), corralPushVisited.end(), false);
    bool isPrunableCorral = true;
    stack.clear();
    stack.push_back(board.Player());
    while (!stack.empty()) {
      Position p = stack.back();
      stack.pop_back();
      if (corralPushVisited[p]) {
        continue;
      }
      corralPushVisited[p] = true;
      for (Direction d : ALL_DIRECTIONS) {
        Position p2 = board.MovePosition(p, d);
        if (corralPushVisited[p2]) {
          continue;
        }
        if (board.HasWall(p2)) {
          continue;
        }
        if (corralEdgeBoxes[p2]) {
          Position p3 = board.MovePosition(p2, d);
          if (!corralEdgeBoxes[p3] && !board.HasWall(p3)) {
            // This is a valid push --- we need to check two conditions:
            // 1. If the push lands outside the corral, or
            // 2. If the player cannot make the push.
            // In either case we know this corral cannot be used for pruning.
            if (!corralVisited[p3] ||
                !pushesVisited[(int)d * board.Size() + p2]) {
              isPrunableCorral = false;
              goto exit_dfs;
            }
          }
          continue;
        }
        stack.push_back(p2);
      }
    }
  exit_dfs:

    if (isPrunableCorral) {
      int i = 0;
      while (i < pushes.size()) {
        if (!corralEdgeBoxes[pushes[i].Box()]) {
          pushes[i] = pushes.back();
          pushes.pop_back();
        } else {
          i++;
        }
      }
      return true;
    }
  }

  return false;
}

void PushSearcher::PruneSimpleDeadlocks(std::vector<Push> &pushes) {
  int i = 0;
  while (i < pushes.size()) {
    Position pushTo =
        board.MovePosition(pushes[i].Box(), pushes[i].Direction());
    if (simpleDeadlockDetector.IsDeadlock(pushTo)) {
      pushes[i] = pushes.back();
      pushes.pop_back();
    } else {
      i++;
    }
  }
}
