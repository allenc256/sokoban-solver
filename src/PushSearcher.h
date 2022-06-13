#pragma once

#include <vector>

#include "Board.h"
#include "SimpleDeadlockDetector.h"

struct PushSearchResult {
  Position normalizedPlayer;
  bool isPICorral;

  PushSearchResult(Position normalizedPlayer, bool isPICorral)
      : normalizedPlayer(normalizedPlayer), isPICorral(isPICorral) {}
};

class PushSearcher {
public:
  PushSearcher(const Board &board,
               const SimpleDeadlockDetector &simpleDeadlockDetector);

  PushSearchResult FindPushes(std::vector<Push> &pushes);

private:
  Position FindUnprunedPushes(std::vector<Push> &pushes);
  bool PruneCorrals(std::vector<Push> &pushes);
  void PruneSimpleDeadlocks(std::vector<Push> &pushes);

  const Board &board;
  const SimpleDeadlockDetector &simpleDeadlockDetector;
  std::vector<Position> stack;
  std::vector<bool> pushesVisited;
  std::vector<bool> playerVisited;
  std::vector<bool> corralVisited;
  std::vector<bool> corralPushVisited;
  std::vector<bool> corralEdgeBoxes;
};
