#pragma once

#include "Direction.h"
#include "Position.h"

class Push {
public:
  Push(Position box, Direction direction) : box(box), direction(direction) {}

  Position Box() const { return box; }
  ::Direction Direction() const { return direction; }

private:
  Position box;
  ::Direction direction;
};
