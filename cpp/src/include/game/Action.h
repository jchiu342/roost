//
// Created by Jeremy on 11/23/2021.
//

#ifndef ROOST_ACTION_H
#define ROOST_ACTION_H
#include "game_defs.h"
#include <string>

namespace game {

enum ActionType { PLAY, PASS, RESIGN };

class Action {
public:
  // action indexes go from 0 to BOARD_SIZE * BOARD_SIZE + 1
  Action(Color color, int index);
  Action(Color color, ActionType action_type, int x = 0, int y = 0);
  [[nodiscard]] Color get_color() const;
  [[nodiscard]] ActionType get_type() const;
  [[nodiscard]] int get_x() const;
  [[nodiscard]] int get_y() const;
  [[nodiscard]] int get_index() const;
  [[maybe_unused]] [[nodiscard]] std::string to_string() const;
  [[nodiscard]] std::string to_sgf_string() const;

private:
  /* Each action is internally represented by a single integer from 1 to
   * BOARD_SIZE^2 + 2, plus positive for Black and negative for White moves.
   * Moves 1 to BOARD_SIZE^2 represent playing; BOARD_SIZE^2 + 1 is passing;
   * BOARD_SIZE^2 + 2 is resigning. */
  int value_;
};

} // namespace game

#endif // ROOST_ACTION_H
