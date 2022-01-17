//
// Created by Jeremy on 11/25/2021.
//

#include <cassert>
#include <random>
#include <vector>

#include "player/RandomPlayer.h"

RandomPlayer::RandomPlayer() : AbstractPlayer(), gen_(rd_()) {}

game::Action RandomPlayer::get_move(game::GameState state) {
  std::vector<int> legal_move_indexes = *state.get_legal_action_indexes();
  std::uniform_int_distribution<> dist(
      0, static_cast<int>(legal_move_indexes.size() - 1));
  return {state.get_turn(), legal_move_indexes[dist(gen_)]};
}
