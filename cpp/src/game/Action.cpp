//
// Created by Jeremy on 11/23/2021.
//

#include "game/Action.h"
#include <cassert>
#include <cstdlib>
#include <stdexcept>

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

Color Action::get_color() const { return value_ > 0 ? BLACK : WHITE; }

ActionType Action::get_type() const {
  switch (value_ > 0 ? value_ : -value_) {
  case (BOARD_SIZE * BOARD_SIZE + 1):
    return PASS;
  case (BOARD_SIZE * BOARD_SIZE + 2):
    return RESIGN;
  default:
    return PLAY;
  }
}

int Action::get_x() const {
  assert((abs(value_) - 1) < BOARD_SIZE * BOARD_SIZE);
  return (abs(value_) - 1) / BOARD_SIZE;
}

int Action::get_y() const {
  assert((abs(value_) - 1) < BOARD_SIZE * BOARD_SIZE);
  return (abs(value_) - 1) % BOARD_SIZE;
}

int Action::get_index() const { return abs(value_) - 1; }

std::string Action::to_string() const {
  switch (get_type()) {
  case PASS:
    return "PASS";
  case RESIGN:
    return "RESIGN";
  case PLAY:
    return std::string("PLAY ") + (get_color() == BLACK ? "B " : "W ") +
           static_cast<char>('A' + get_x()) + static_cast<char>('1' + get_y());
  }
  return {};
}

std::string Action::to_gtp_string() const {
  switch (get_type()) {
  case PASS:
    return "= PASS\n";
  case RESIGN:
    return "= RESIGN\n";
  case PLAY:
    std::string ret = "= ";
    // why does gtp make I an illegal coordinate wtf
    ret += static_cast<char>('A' + get_y() + (get_y() >= 8));
    ret += static_cast<char>('9' - get_x());
    return ret + "\n";
  }
}

std::string Action::to_sgf_string() const {
  if (get_type() == RESIGN) {
    return {};
  }
  std::string return_string = (get_color() == BLACK ? ";B[" : ";W[");
  if (get_type() == PLAY) {
    return_string += static_cast<char>('a' + get_x());
    return_string += static_cast<char>('a' + get_y());
  }
  return_string += "]";
  return return_string;
}

Action Action::from_action(const std::string &s) {
  if (s.size() < 9) {
    throw std::invalid_argument("Invalid action: " + s);
  }
  Color c = (s[5] == 'b' ? BLACK : WHITE);
  // coordinate; must be a play
  if (s[7] < 'p') {
    return {c, PLAY, '9' - s[8], (s[7] < 'j') ? s[7] - 'a' : s[7] - 'b'};
  } else if (s.substr(7, 4) == "pass") {
    return {c, PASS};
  }
  return {c, RESIGN};
}

} // namespace game