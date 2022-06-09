#include "Board.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <deque>
#include <iomanip>
#include <locale>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std::string_literals;

static const int NO_BOX = -1;
static const int NO_GOAL = -1;

Board::Board(int width,
             int height,
             Position player,
             const std::vector<bool> &wallArray,
             const std::vector<Position> &boxes,
             const std::vector<Position> &goals)
    : width(width),
      height(height),
      player(player),
      boxes(boxes),
      goals(goals),
      boxArray(width * height, NO_BOX),
      wallArray(wallArray),
      goalArray(width * height, NO_GOAL),
      goalsCompleted(0),
      boxArrayHashTable(width * height),
      playerArrayHashTable(width * height) {
  if (boxes.size() != goals.size()) {
    throw std::invalid_argument("box/goal count mismatch: "s +
                                std::to_string(boxes.size()) + "/" +
                                std::to_string(goals.size()));
  }

  // Initialize box array.
  for (int i = 0; i < boxes.size(); ++i) {
    boxArray[boxes[i]] = i;
  }

  // Initialize goal array.
  for (int i = 0; i < goals.size(); ++i) {
    goalArray[goals[i]] = i;
  }

  // Initialize goal counts.
  for (int i = 0; i < goalArray.size(); i++) {
    if (goalArray[i] != NO_GOAL && boxArray[i] != NO_BOX) {
      goalsCompleted++;
    }
  }

  // Compute hash tables.
  std::mt19937_64 rnd(0xdeadbeef);
  for (int i = 0; i < boxArrayHashTable.size(); ++i) {
    boxArrayHashTable[i] = rnd();
    playerArrayHashTable[i] = rnd();
  }

  // Compute hash.
  hash = ComputeHash();
}

// N.B., from https://stackoverflow.com/questions/216823/how-to-trim-a-stdstring
static inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

Board Board::ParseFromText(std::istream &is) {
  // Read lines from stream.
  std::string line;
  std::vector<std::string> lines;
  while (std::getline(is, line)) {
    rtrim(line);
    if (line.empty()) {
      continue;
    }
    lines.push_back(line);
  }

  // Compute width and height.
  int height = lines.size();
  int width = 0;
  for (auto &line : lines) {
    if (line.size() > width) {
      width = line.size();
    }
  }

  // Parse the lines.
  Position player;
  std::vector<Position> boxes;
  std::vector<Position> goals;
  std::vector<bool> wallArray(width * height, false);
  for (int y = 0; y < lines.size(); y++) {
    for (int x = 0; x < lines[y].size(); x++) {
      char ch = lines[y][x];
      int position = y * width + x;
      switch (ch) {
      case '#':
        wallArray[position] = true;
        break;
      case '@':
        player = position;
        break;
      case '+':
        player = position;
        goals.emplace_back(position);
        break;
      case '$':
        boxes.emplace_back(position);
        break;
      case '*':
        boxes.emplace_back(position);
        goals.emplace_back(position);
        break;
      case '.':
        goals.emplace_back(position);
        break;
      case ' ':
        break;
      default:
        throw std::invalid_argument("unrecognized character: "s + ch);
      }
    }
  }

  // N.B., shrink vector to min size since it should never grow.
  boxes.shrink_to_fit();

  return Board(width, height, player, wallArray, boxes, goals);
}

void Board::DumpToText(std::ostream &os) const {
  int position = 0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      char ch;
      if (wallArray[position]) {
        ch = '#';
      } else if (player == position) {
        if (goalArray[position] != NO_GOAL) {
          ch = '+';
        } else {
          ch = '@';
        }
      } else if (boxArray[position] != NO_BOX) {
        if (goalArray[position] != NO_GOAL) {
          ch = '*';
        } else {
          ch = '$';
        }
      } else if (goalArray[position] != NO_GOAL) {
        ch = '.';
      } else {
        ch = ' ';
      }
      os << ch;
      position++;
    }
    os << std::endl;
  }
}

void Board::PerformPush(const Push &push) {
  Position boxFrom = push.Box();
  Position boxTo = MovePosition(boxFrom, push.Direction());
  MoveBox(boxFrom, boxTo);
  MovePlayer(boxFrom);
}

void Board::PerformUnpush(const Push &push) {
  Position boxTo = push.Box();
  Position boxFrom = MovePosition(boxTo, push.Direction());
  MoveBox(boxFrom, boxTo);
  MovePlayer(UnmovePosition(boxTo, push.Direction()));
}

void Board::ResetState(Position playerTo,
                       const std::vector<Position> &boxesTo) {
  assert(boxes.size() == boxesTo.size());

  // Reset player state.
  player = playerTo;

  // Clear box state.
  for (Position p : boxes) {
    boxArray[p] = NO_BOX;
  }
  goalsCompleted = 0;

  // Reset box state.
  boxes = boxesTo;
  for (int i = 0; i < boxes.size(); i++) {
    Position p = boxes[i];
    boxArray[p] = i;
    if (goalArray[p] != NO_GOAL) {
      goalsCompleted++;
    }
  }

  // Reset hash.
  hash = ComputeHash();
}

void Board::MoveBox(Position from, Position to) {
  if (from == to) {
    return;
  }
  assert(!wallArray[to] && boxArray[to] == NO_BOX && boxArray[from] != NO_BOX);

  // Push the box.
  int boxIndex = boxArray[from];
  boxArray[from] = NO_BOX;
  boxArray[to] = boxIndex;
  boxes[boxIndex] = to;

  // Update goals completed count.
  if (goalArray[from] != NO_GOAL) {
    goalsCompleted--;
  }
  if (goalArray[to] != NO_GOAL) {
    goalsCompleted++;
  }

  // Update hash.
  hash ^= boxArrayHashTable[from] ^ boxArrayHashTable[to];
  assert(hash == ComputeHash());
}

void Board::MovePlayer(Position to) {
  if (player == to) {
    return;
  }
  assert(!wallArray[to]);
  Position from = player;
  player = to;
  hash ^= playerArrayHashTable[from] ^ playerArrayHashTable[to];
  assert(hash == ComputeHash());
}

uint64_t Board::ComputeHash() const {
  uint64_t result = 0;
  result ^= playerArrayHashTable[player];
  for (Position box : boxes) {
    result ^= boxArrayHashTable[box];
  }
  return result;
}
