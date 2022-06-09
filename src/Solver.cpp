#include <iomanip>
#include <iostream>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "SearchState.h"
#include "Solver.h"

enum class FreezeStatus { UNKNOWN, FROZEN, FREE };

void NormalizeBoardAndFindPushes(
    Board &board,
    const SimpleDeadlockDetector &simpleDeadlockDetector,
    std::vector<bool> &normVisited,
    std::vector<Position> &normStack,
    std::vector<Push> &normPushes) {
  // Initialize input data structures.
  std::fill(normVisited.begin(), normVisited.end(), false);
  normStack.clear();
  normPushes.clear();

  // Perform DFS.
  Position normPlayer = board.Player();
  normStack.push_back(board.Player());
  while (!normStack.empty()) {
    Position p = normStack.back();
    normStack.pop_back();
    if (normVisited[p]) {
      continue;
    }
    normVisited[p] = true;
    for (Direction d : ALL_DIRECTIONS) {
      Position p2 = board.MovePosition(p, d);
      if (normVisited[p2]) {
        continue;
      }
      if (board.HasWall(p2)) {
        continue;
      }
      if (board.HasBox(p2)) {
        Position p3 = board.MovePosition(p2, d);
        if (!board.HasBox(p3) && !board.HasWall(p3) &&
            !simpleDeadlockDetector.IsDeadlock(p3)) {
          normPushes.emplace_back(p2, d);
        }
        continue;
      }
      if (p2 < normPlayer) {
        normPlayer = p2;
      }
      normStack.push_back(p2);
    }
  }

  // Normalize player position.
  board.MovePlayer(normPlayer);
}

Solver::Solver(Board &board, int maxStates)
    : board(board),
      simpleDeadlockDetector(board),
      freezeDeadlockDetector(board),
      distanceTable(board),
      maxStates(maxStates) {}

static void OutputDebugHash(std::ostream &debugFile, uint64_t hash) {
  std::ios oldState(nullptr);
  oldState.copyfmt(debugFile);
  debugFile << std::hex << std::setw(16) << std::setfill('0') << hash;
  debugFile.copyfmt(oldState);
}

static void OutputDebugState(std::ostream &debugFile,
                             const Board &board,
                             int stateIndex,
                             int gValue,
                             int hValue) {
  debugFile << "state " << stateIndex << ": ";
  OutputDebugHash(debugFile, board.Hash());
  debugFile << std::endl;
  board.DumpToText(debugFile);
  debugFile << "g-value: " << gValue << std::endl;
  debugFile << "h-value: " << hValue << std::endl;
  debugFile << "f-value: " << (gValue + hValue) << std::endl;
  debugFile << "goals: " << board.GoalsCompleted() << std::endl;
}

static void OutputDebugPush(std::ostream &debugFile,
                            const Push &push,
                            const Board &board,
                            bool isDeadlocked) {
  debugFile << "push: (" << board.PositionX(push.Box()) << ", "
            << board.PositionY(push.Box()) << ") ";
  switch (push.Direction()) {
  case Direction::UP:
    debugFile << "UP   ";
    break;
  case Direction::DOWN:
    debugFile << "DOWN ";
    break;
  case Direction::LEFT:
    debugFile << "LEFT ";
    break;
  case Direction::RIGHT:
    debugFile << "RIGHT";
    break;
  }
  debugFile << " -> ";
  OutputDebugHash(debugFile, board.Hash());
  debugFile << std::endl;
  if (isDeadlocked) {
    debugFile << "deadlock:" << std::endl;
    board.DumpToText(debugFile);
  }
}

SolveResult Solver::Solve(std::ostream *debugFile) {
  if (board.Done()) {
    return SolveResult(true, 0, 0);
  }

  std::vector<bool> normVisited(board.Width() * board.Height());
  std::vector<Position> normStack;
  std::vector<Push> normPushes;
  int statesVisited = 0;
  int solutionPushes = -1;

  auto compare = [](std::shared_ptr<SearchState> s1,
                    std::shared_ptr<SearchState> s2) {
    return s1->AStarFValue() >= s2->AStarFValue();
  };
  std::priority_queue<std::shared_ptr<SearchState>,
                      std::vector<std::shared_ptr<SearchState>>,
                      decltype(compare)>
      openStatesQueue(compare);
  std::unordered_map<uint64_t, std::shared_ptr<SearchState>> openStates;
  std::unordered_set<uint64_t> closedStates;

  NormalizeBoardAndFindPushes(board, simpleDeadlockDetector, normVisited,
                              normStack, normPushes);
  int initialHValue = distanceTable.EstimateDistance(board.Boxes());
  std::shared_ptr<SearchState> initialState =
      std::make_shared<SearchState>(board.Hash(), board.Player(), board.Boxes(),
                                    normPushes, 0, initialHValue);
  openStatesQueue.emplace(initialState);
  openStates[initialState->Id()] = initialState;

  while (!openStatesQueue.empty() && statesVisited < maxStates) {
    // Get current node, remove from open list, add to closed list.
    std::shared_ptr<SearchState> currState = openStatesQueue.top();
    openStatesQueue.pop();
    openStates.erase(currState->Id());
    closedStates.insert(currState->Id());
    statesVisited++;

    // Reset board state.
    board.ResetState(currState->Player(), currState->Boxes());

    // Check if done.
    if (board.Done()) {
      solutionPushes = currState->AStarGValue();
      break;
    }

    // Debug output.
    if (debugFile) {
      OutputDebugState(*debugFile, board, statesVisited,
                       currState->AStarGValue(), currState->AStarHValue());
    }

    // Generate children.
    for (const Push &p : currState->Pushes()) {
      // Mutate board.
      board.PerformPush(p);

      // Check for potential freeze deadlock.
      Position boxTo = board.MovePosition(p.Box(), p.Direction());
      if (freezeDeadlockDetector.IsDeadlock(boxTo)) {
        if (debugFile) {
          OutputDebugPush(*debugFile, p, board, true);
        }
        board.PerformUnpush(p);
        continue;
      }

      // Normalize and find children.
      NormalizeBoardAndFindPushes(board, simpleDeadlockDetector, normVisited,
                                  normStack, normPushes);

      // Check if child exists on closed list.
      if (closedStates.find(board.Hash()) != closedStates.end()) {
        board.PerformUnpush(p);
        continue;
      }

      // Check if we already have an open state (with lower-or-better g value).
      int childGValue = currState->AStarGValue() + 1;
      auto it = openStates.find(board.Hash());
      if (it != openStates.end()) {
        if (childGValue >= it->second->AStarGValue()) {
          board.PerformUnpush(p);
          continue;
        }
      }

      // Compute heuristic.
      int childHValue = distanceTable.EstimateDistance(board.Boxes());

      // Debug push.
      if (debugFile) {
        OutputDebugPush(*debugFile, p, board, false);
      }

      // Update open state.
      // N.B., note on duplicate states
      std::shared_ptr<SearchState> childState = std::make_shared<SearchState>(
          board.Hash(), board.Player(), board.Boxes(), normPushes, childGValue,
          childHValue);
      openStatesQueue.emplace(childState);
      openStates[childState->Id()] = childState;
      board.PerformUnpush(p);
    }

    if (debugFile) {
      *debugFile << std::endl;
    }
  }
end_of_search:

  return SolveResult(solutionPushes != -1, statesVisited, solutionPushes);
}
