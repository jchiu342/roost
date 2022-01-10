//
// Created by Jeremy on 11/24/2021.
//

#ifndef ROOST_GAMESTATE_H
#define ROOST_GAMESTATE_H

#include "Action.h"
#include "game_defs.h"
#include "utils/Zobrist.h"
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace game {

class GameState {
public:
  explicit GameState(float komi = 7.5,
                     const std::shared_ptr<Zobrist> &zobrist = nullptr);
  // int operator==(const GameState &other);

  // gets the turn; undefined behavior if game is done
  [[nodiscard]] Color get_turn() const;
  [[nodiscard]] int get_num_turns() const;
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
  [[nodiscard]] const std::vector<int> *get_legal_action_indexes() const;
  void move(Action action);
  [[nodiscard]] size_t hash() const;
  [[nodiscard]] std::string to_string() const;

private:
  // boards[0] is the most recent board, then boards[1], etc.
  Color boards_[GAME_HISTORY_LEN][BOARD_SIZE][BOARD_SIZE];
  Color turn_;
  Color winner_;
  float komi_;
  size_t hash_;
  unsigned turns_;
  unsigned passes_;
  bool done_;
  std::pair<int, int> uf_chains_[BOARD_SIZE][BOARD_SIZE];
  std::set<std::pair<int, int>> chain_lists_[BOARD_SIZE][BOARD_SIZE];
  int liberties_[BOARD_SIZE][BOARD_SIZE];
  // this is valid UNLESS done_ = true
  std::vector<int> legal_action_idxes_;
  std::shared_ptr<Zobrist> zobrist_;
  bool is_legal_play_(int x, int y, Color c);
  // returns true if some neighbors were removed
  bool remove_dead_neighbors_(int x, int y, Color opposite_color,
                              bool permanent = true);
  void dfs_remove_chain_(int x, int y, Color c);
  void dfs_liberties_(int x, int y, Color c, bool visited[][BOARD_SIZE],
                      std::set<std::pair<int, int>> *chain,
                      int *liberties) const;
  void dfs_score_(int x, int y, Color opposite_color,
                  bool reachable[][BOARD_SIZE]) const;
  void update_liberties_at_head_(int x, int y);
  void uf_make_(int x, int y);
  std::pair<int, int> uf_find_(int x, int y);
  void uf_union_(int x1, int y1, int x2, int y2);
  void uf_delete_(int x1, int y1);
};

} // namespace game

// Implement std::hash on GameState
// perhaps this could be improved
template <> struct std::hash<game::GameState> {
  std::size_t operator()(const game::GameState &g) const { return g.hash(); }
};

// TODO: figure out why I couldn't just overload operator== here and fix into a
// real equal_to
template <> struct std::equal_to<game::GameState> {
  bool operator()(const game::GameState &lhs,
                  const game::GameState &rhs) const {
    return lhs.hash() == rhs.hash() &&
           *(lhs.get_legal_action_indexes()) == *(rhs.get_legal_action_indexes());
  }
};

#endif // ROOST_GAMESTATE_H
