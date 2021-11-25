//
// Created by Jeremy on 11/24/2021.
//

#ifndef ROOST_GAMESTATE_H
#define ROOST_GAMESTATE_H

#include <string>
#include <set>
#include "game_defs.h"
#include "Action.h"

namespace game {

class GameState {
public:
  explicit GameState(float komi = 7.5);
  float score();
  bool is_legal_action(Action action);
  void move(Action action);
  std::string to_string() const;
private:
  // boards[0] is the most recent board, then boards[1], etc.
  Color boards_[GAME_HISTORY_LEN][BOARD_SIZE * BOARD_SIZE];
  Color turn_;
  Color winner_;
  float komi_;
  unsigned turns_;
  unsigned passes_;
  bool done_;
  bool is_legal_play_(int x, int y, Color c);
  void remove_dead_neighbors_(int x, int y, Color opposite_color);
  void dfs_liberties_(int x, int y, Color c, bool *visited, std::set<int> *chain, int *liberties);
  void dfs_score_(int x, int y, Color opposite_color, bool *reachable);
};

}

#endif //ROOST_GAMESTATE_H
