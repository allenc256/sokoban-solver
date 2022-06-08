#include <iostream>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "SearchState.h"
#include "Solver.h"

static const Direction ALL_DIRECTIONS[] = {Direction::UP, Direction::DOWN,
                                           Direction::LEFT, Direction::RIGHT};

static void SearchForSimpleDeadlocks(const Board &board,
                                     Position p,
                                     std::vector<bool> &simpleDeadlockArray,
                                     std::vector<bool> &simpleDeadlockVisited);

static void SearchForSimpleDeadlocks(const Board &board,
                                     Position p,
                                     std::vector<bool> &simpleDeadlockArray,
                                     std::vector<bool> &simpleDeadlockVisited) {
  if (simpleDeadlockVisited[p]) {
    return;
  }
  simpleDeadlockArray[p] = false;
  simpleDeadlockVisited[p] = true;
  for (auto d : ALL_DIRECTIONS) {
    Position p2 = board.MovePosition(p, d);
    if (!board.HasWall(p2)) {
      Position p3 = board.MovePosition(p2, d);
      if (!board.HasWall(p3)) {
        SearchForSimpleDeadlocks(board, p2, simpleDeadlockArray,
                                 simpleDeadlockVisited);
      }
    }
  }
}

void NormalizeBoardAndFindPushes(Board &board,
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
        if (!board.HasBox(p3) && !board.HasWall(p3)) {
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
      simpleDeadlockArray(board.Width() * board.Height(), true),
      distanceTable(board),
      maxStates(maxStates) {
  // Compute simple deadlocks.
  std::vector<bool> simpleDeadlockVisited(board.Width() * board.Height(),
                                          false);
  for (Position p = 0; p < board.Width() * board.Height(); p++) {
    if (board.HasGoal(p)) {
      SearchForSimpleDeadlocks(board, p, simpleDeadlockArray,
                               simpleDeadlockVisited);
    }
  }
}

static void OutputGraphHeader(std::ostream &graphOutput) {
  graphOutput << "digraph {" << std::endl;
  graphOutput << "  node [fontname=\"Courier New\" fontsize=10]" << std::endl;
  graphOutput << "  edge [fontname=\"Courier New\" fontsize=10]" << std::endl;
}

static void OutputGraphFooter(std::ostream &graphOutput) {
  graphOutput << "}" << std::endl;
}

static void OutputGraphNode(std::ostream &graphOutput,
                            const Board &board,
                            uint64_t hash,
                            int gValue,
                            int hValue) {
  graphOutput << "  " << hash << "[label=\"";
  board.DumpToText(graphOutput, true);
  graphOutput << "\\ng=" << gValue << " h=" << hValue
              << " c=" << board.GoalsCompleted() << "\"]" << std::endl;
}

static void OutputGraphEdge(std::ostream &graphOutput,
                            uint64_t hashFrom,
                            uint64_t hashTo) {
  graphOutput << "  " << hashFrom << " -> " << hashTo << std::endl;
}

SolveResult Solver::Solve(std::ostream *graphOutput) {
  if (board.Done()) {
    return SolveResult(true, 0, 0);
  }

  if (graphOutput) {
    OutputGraphHeader(*graphOutput);
  }

  std::vector<bool> normVisited(board.Width() * board.Height());
  std::vector<Position> normStack;
  std::vector<Push> normPushes;
  int statesVisited = 0;
  int solutionPushes = -1;
  uint64_t hashFrom;

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

  NormalizeBoardAndFindPushes(board, normVisited, normStack, normPushes);
  int initialHValue = distanceTable.EstimateDistance(board.Boxes());
  std::shared_ptr<SearchState> initialState = std::make_shared<SearchState>(
      board.ExtractSearchHash(), board.Player(), board.Boxes(), normPushes, 0,
      initialHValue);
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
    if (graphOutput) {
      hashFrom = board.ExtractSearchHash();
      OutputGraphNode(*graphOutput, board, hashFrom, currState->AStarGValue(),
                      currState->AStarHValue());
    }

    // Generate children.
    for (const Push &p : currState->Pushes()) {
      // Check for simple deadlock.
      Position box = board.MovePosition(p.Box(), p.Direction());
      if (simpleDeadlockArray[box]) {
        continue;
      }

      // Mutate board.
      board.PerformPush(p);
      NormalizeBoardAndFindPushes(board, normVisited, normStack, normPushes);

      // Check if child exists on closed list.
      auto childId = board.ExtractSearchHash();
      if (closedStates.find(childId) != closedStates.end()) {
        board.PerformUnpush(p);
        continue;
      }

      // Check if we already have an open state (with lower-or-better g value).
      int childGValue = currState->AStarGValue() + 1;
      auto it = openStates.find(childId);
      if (it != openStates.end()) {
        if (childGValue >= it->second->AStarGValue()) {
          board.PerformUnpush(p);
          continue;
        }
      }

      // Compute heuristic.
      int childHValue = distanceTable.EstimateDistance(board.Boxes());

      // Debug output to child.
      if (graphOutput) {
        uint64_t hashTo = board.ExtractSearchHash();
        OutputGraphNode(*graphOutput, board, hashTo, childGValue, childHValue);
        OutputGraphEdge(*graphOutput, hashFrom, hashTo);
      }

      // Update open state.
      // N.B., note on duplicate states
      std::shared_ptr<SearchState> childState = std::make_shared<SearchState>(
          board.ExtractSearchHash(), board.Player(), board.Boxes(), normPushes,
          childGValue, childHValue);
      openStatesQueue.emplace(childState);
      openStates[childState->Id()] = childState;
      board.PerformUnpush(p);
    }
  }
end_of_search:

  if (graphOutput) {
    OutputGraphFooter(*graphOutput);
  }

  return SolveResult(solutionPushes != -1, statesVisited, solutionPushes);
}
