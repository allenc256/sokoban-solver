#include "DistanceTable.h"

#include <cassert>
#include <deque>

struct State {
  Position position;
  int distance;

  State(Position position, int distance)
      : position(position), distance(distance) {}
};

static const Direction ALL_DIRECTIONS[] = {Direction::UP, Direction::DOWN,
                                           Direction::LEFT, Direction::RIGHT};

DistanceTable::DistanceTable(const Board &board)
    : board(board),
      distances(board.Goals().size(),
                std::vector<int>(board.Width() * board.Height(), -1)),
      buffer(board.Goals().size()) {
  std::vector<bool> visited(board.Width() * board.Height());
  std::deque<State> queue;

  for (int i = 0; i < board.Goals().size(); i++) {
    // Initialize distance array for goal.
    std::vector<int> &d = distances[i];

    // Clear DFS state.
    std::fill(visited.begin(), visited.end(), false);
    queue.clear();

    // Perform DFS.
    Position initialPos = board.Goals()[i];
    queue.emplace_back(State(initialPos, 0));
    while (!queue.empty()) {
      State &s = queue.front();
      queue.pop_front();
      if (visited[s.position]) {
        continue;
      }
      visited[s.position] = true;
      d[s.position] = s.distance;
      for (Direction d : ALL_DIRECTIONS) {
        Position newPos = board.MovePosition(s.position, d);
        if (!board.HasWall(newPos)) {
          queue.emplace_back(State(newPos, s.distance + 1));
        }
      }
    }
  }
}

int DistanceTable::EstimateDistance(const std::vector<Position> &boxes) const {
  assert(boxes.size() == board.Goals().size());

  buffer.clear();

  // Initialize goal indices.
  for (int i = 0; i < board.Goals().size(); i++) {
    buffer.push_back(i);
  }

  // Loop through boxes and greedily pick nearest goal.
  int totalDistance = 0;
  for (int i = 0; i < boxes.size(); i++) {
    Position box = boxes[i];

    // Find nearest goal to the box.
    int bestDistance = -1;
    int bestBufferIndex = -1;
    for (int j = 0; j < buffer.size(); j++) {
      int goalIndex = buffer[j];
      if (bestBufferIndex == -1 || distances[goalIndex][box] < bestDistance) {
        bestDistance = distances[goalIndex][box];
        bestBufferIndex = j;
      }
    }
    assert(bestDistance >= 0);

    // Update total distance.
    totalDistance += bestDistance;

    // Remove the matched goal from further consideration.
    buffer[bestBufferIndex] = buffer.back();
    buffer.pop_back();
  }

  return totalDistance;
}
