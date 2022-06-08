#pragma once

struct SolveResult {
  bool solved;
  int statesVisited;
  int pushesRequired;

  SolveResult(bool solved, int statesVisited, int pushesRequired)
      : solved(solved),
        statesVisited(statesVisited),
        pushesRequired(pushesRequired) {}
};
