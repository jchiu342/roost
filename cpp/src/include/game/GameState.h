//
// Created by Jeremy on 11/24/2021.
//

#ifndef ROOST_GAMESTATE_H
#define ROOST_GAMESTATE_H

#include "Action.h"
#include "game_defs.h"
#include <set>
#include <string>
#include <vector>

namespace game {

class GameState {
public:
  explicit GameState(float komi = 7.5);
  // int operator==(const GameState &other);

  // gets the turn; undefined behavior if game is done
  [[nodiscard]] Color get_turn() const;
  [[nodiscard]] const Color *get_board(int i) const;
  // gets the komi
  [[nodiscard]] float get_komi() const;
  // gets whether the game is finished
  [[nodiscard]] bool done() const;
  // gets the winner if game is done; undefined behavior if not
  [[nodiscard]] Color winner() const;
  // returns current score if game were to finish at moment - valid both
  // before/after game is finished
  [[nodiscard]] float score() const;
  // always returns false if the game is done
  [[nodiscard]] bool is_legal_action(Action action) const;
  // returns empty vector if the game is done; otherwise, there is always >=1
  // legal move (pass) does NOT include resign, which is always legal
  [[nodiscard]] std::vector<int> get_legal_action_indexes() const;
  void move(Action action);
  [[nodiscard]] std::string to_string() const;

private:
  // boards[0] is the most recent board, then boards[1], etc.
  Color boards_[GAME_HISTORY_LEN][BOARD_SIZE * BOARD_SIZE];
  Color turn_;
  Color winner_;
  float komi_;
  unsigned turns_;
  unsigned passes_;
  bool done_;
  // this is valid UNLESS done_ = true
  std::vector<int> legal_action_idxes_;
  bool is_legal_play_(int x, int y, Color c);
  void remove_dead_neighbors_(int x, int y, Color opposite_color);
  void dfs_liberties_(int x, int y, Color c, bool *visited,
                      std::set<int> *chain, int *liberties) const;
  void dfs_score_(int x, int y, Color opposite_color, bool *reachable) const;
};

} // namespace game

// Implement std::hash on GameState
// perhaps this could be improved
template <> struct std::hash<game::GameState> {
  std::size_t operator()(const game::GameState &g) const {
    return std::hash<std::string>{}(g.to_string());
  }
};

// TODO: figure out why I couldn't just overload operator== here and fix into a
// real equal_to
template <> struct std::equal_to<game::GameState> {
  bool operator()(const game::GameState &lhs,
                  const game::GameState &rhs) const {
    return lhs.to_string() == rhs.to_string() &&
           lhs.get_legal_action_indexes() == rhs.get_legal_action_indexes();
  }
};

/*int operator==(const game::GameState &lhs, const game::GameState &rhs) {
  return 0;
}*/

#endif // ROOST_GAMESTATE_H