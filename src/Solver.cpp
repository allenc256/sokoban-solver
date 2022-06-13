#include <cassert>
#include <iomanip>
#include <iostream>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "Solver.h"

struct SearchState {
  uint64_t id;
  Position player;
  std::vector<Position> boxes;
  std::vector<Push> pushes;
  int aStarGValue;
  int aStarHValue;
  bool isPICorral;

  int AStarFValue() const { return aStarGValue + aStarHValue; }

  SearchState(uint64_t id,
              Position player,
              const std::vector<Position> &boxes,
              const std::vector<Push> &pushes,
              int aStarGValue,
              int aStarHValue,
              bool isPICorral)
      : id(id),
        player(player),
        boxes(boxes),
        pushes(pushes),
        aStarGValue(aStarGValue),
        aStarHValue(aStarHValue),
        isPICorral(isPICorral) {
    assert(this->pushes.size() == this->pushes.capacity());
    assert(this->boxes.size() == this->boxes.capacity());
  }
};

Solver::Solver(Board &board, int maxStates)
    : board(board),
      simpleDeadlockDetector(board),
      freezeDeadlockDetector(board, simpleDeadlockDetector),
      pushSearcher(board, simpleDeadlockDetector),
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
                             int hValue,
                             bool isPICorral) {
  debugFile << "state " << stateIndex << ": ";
  OutputDebugHash(debugFile, board.Hash());
  debugFile << std::endl;
  board.DumpToText(debugFile);
  debugFile << "g-value: " << gValue << std::endl;
  debugFile << "h-value: " << hValue << std::endl;
  debugFile << "f-value: " << (gValue + hValue) << std::endl;
  debugFile << "goals: " << board.GoalsCompleted() << std::endl;
  if (isPICorral) {
    debugFile << "PI-corral: true" << std::endl;
  }
}

static void OutputDeadlock(std::ostream &debugFile, const Board &board) {
  debugFile << "deadlock:" << std::endl;
  board.DumpToText(debugFile);
}

enum class PushType {
  OPEN,
  OPEN_ALREADY,
  DEADLOCK,
  CLOSED,
};

static void OutputDebugPush(std::ostream &debugFile,
                            const Push &push,
                            const Board &board,
                            PushType type) {
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
  switch (type) {
  case PushType::OPEN_ALREADY:
    debugFile << " (pruned: open already)";
    break;
  case PushType::DEADLOCK:
    debugFile << " (pruned: deadlock)";
    break;
  case PushType::CLOSED:
    debugFile << " (pruned: closed)";
    break;
  }
  debugFile << std::endl;
}

SolveResult Solver::Solve(std::ostream *debugFile) {
  if (board.Done()) {
    return SolveResult(true, 0, 0);
  }

  std::vector<Push> pushes;
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

  // Find pushes and normalize the board.
  PushSearchResult pushSearchResult = pushSearcher.FindPushes(pushes);
  board.MovePlayer(pushSearchResult.normalizedPlayer);

  int initialHValue = distanceTable.EstimateDistance(board.Boxes());
  std::shared_ptr<SearchState> initialState = std::make_shared<SearchState>(
      board.Hash(), board.Player(), board.Boxes(), pushes, 0, initialHValue,
      pushSearchResult.isPICorral);
  openStatesQueue.emplace(initialState);
  openStates[initialState->id] = initialState;

  while (!openStatesQueue.empty() && statesVisited < maxStates) {
    // Get current node, remove from open list, add to closed list.
    std::shared_ptr<SearchState> currState = openStatesQueue.top();
    openStatesQueue.pop();
    openStates.erase(currState->id);
    closedStates.insert(currState->id);
    statesVisited++;

    // Reset board state.
    board.ResetState(currState->player, currState->boxes);

    // Check if done.
    if (board.Done()) {
      solutionPushes = currState->aStarGValue;
      break;
    }

    // Debug output.
    if (debugFile) {
      OutputDebugState(*debugFile, board, statesVisited, currState->aStarGValue,
                       currState->aStarHValue, currState->isPICorral);
    }

    // Generate children.
    for (const Push &p : currState->pushes) {
      // Mutate board.
      board.PerformPush(p);

      // Check for potential freeze deadlock.
      Position boxTo = board.MovePosition(p.Box(), p.Direction());
      if (freezeDeadlockDetector.IsDeadlock(boxTo)) {
        if (debugFile) {
          OutputDebugPush(*debugFile, p, board, PushType::DEADLOCK);
          OutputDeadlock(*debugFile, board);
        }
        board.PerformUnpush(p);
        continue;
      }

      // Find pushes and normalize the board.
      pushSearchResult = pushSearcher.FindPushes(pushes);
      board.MovePlayer(pushSearchResult.normalizedPlayer);

      // Check if child exists on closed list.
      if (closedStates.find(board.Hash()) != closedStates.end()) {
        if (debugFile) {
          OutputDebugPush(*debugFile, p, board, PushType::CLOSED);
        }
        board.PerformUnpush(p);
        continue;
      }

      // Check if we already have an open state (with lower-or-better g value).
      int childGValue = currState->aStarGValue + 1;
      auto it = openStates.find(board.Hash());
      if (it != openStates.end()) {
        if (childGValue >= it->second->aStarGValue) {
          if (debugFile) {
            OutputDebugPush(*debugFile, p, board, PushType::OPEN_ALREADY);
          }
          board.PerformUnpush(p);
          continue;
        }
      }

      // Compute heuristic.
      int childHValue = distanceTable.EstimateDistance(board.Boxes());

      // Debug push.
      if (debugFile) {
        OutputDebugPush(*debugFile, p, board, PushType::OPEN);
      }

      // Update open state.
      // N.B., note on duplicate states
      std::shared_ptr<SearchState> childState = std::make_shared<SearchState>(
          board.Hash(), board.Player(), board.Boxes(), pushes, childGValue,
          childHValue, pushSearchResult.isPICorral);
      openStatesQueue.emplace(childState);
      openStates[childState->id] = childState;
      board.PerformUnpush(p);
    }

    if (debugFile) {
      *debugFile << std::endl;
    }
  }
end_of_search:

  return SolveResult(solutionPushes != -1, statesVisited, solutionPushes);
}
