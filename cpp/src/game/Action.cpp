//
// Created by Jeremy on 11/23/2021.
//

#include "game/Action.h"
#include <cassert>
#include <cstdlib>

namespace game {

Action::Action(Color color, int index) {
  assert(color != EMPTY);
  assert(0 <= index && index <= BOARD_SIZE * BOARD_SIZE + 1);
  value_ = index + 1;
  value_ *= (color == BLACK ? 1 : -1);
}

Action::Action(Color color, ActionType action_type, int x, int y) {
  assert(color != EMPTY);
  assert(0 <= x && x < BOARD_SIZE && 0 <= y && y < BOARD_SIZE);
  switch (action_type) {
  case PLAY: {
    value_ = x * BOARD_SIZE + y + 1;
    break;
  }
  case PASS: {
    value_ = BOARD_SIZE * BOARD_SIZE + 1;
    break;
  }
  case RESIGN: {
    value_ = BOARD_SIZE * BOARD_SIZE + 2;
    break;
  }
  }
  value_ *= (color == BLACK ? 1 : -1);
}

Color Action::get_color() { return value_ > 0 ? BLACK : WHITE; }

ActionType Action::get_type() {
  switch (value_ > 0 ? value_ : -value_) {
  case (BOARD_SIZE * BOARD_SIZE + 1):
    return PASS;
  case (BOARD_SIZE * BOARD_SIZE + 2):
    return RESIGN;
  default:
    return PLAY;
  }
}

int Action::get_x() {
  assert((abs(value_) - 1) < BOARD_SIZE * BOARD_SIZE);
  return (abs(value_) - 1) / BOARD_SIZE;
}

int Action::get_y() {
  assert((abs(value_) - 1) < BOARD_SIZE * BOARD_SIZE);
  return (abs(value_) - 1) % BOARD_SIZE;
}

int Action::get_index() {
  return abs(value_) - 1;
}

std::string Action::to_string() {
  switch (get_type()) {
  case PASS:
    return "PASS";
  case RESIGN:
    return "RESIGN";
  case PLAY:
    return std::string("PLAY ") + (get_color() == BLACK ? "B " : "W ");
  }
  return std::string();
}

} // namespace game