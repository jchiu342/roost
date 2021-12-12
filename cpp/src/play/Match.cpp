//
// Created by Jeremy on 11/23/2021.
//

#include "play/Match.h"
#include "game/GameState.h"
#include <iostream>

Match::Match(std::unique_ptr<AbstractPlayer> &&black,
             std::unique_ptr<AbstractPlayer> &&white, int num_games)
    : black_(std::move(black)), white_(std::move(white)),
      num_games_(num_games) {}

int Match::run() {
  int black_wins = 0;
  for (int i = 0; i < num_games_; ++i) {
    game::GameState state;
    while (!state.done()) {
      if (state.get_turn() == game::Color::BLACK) {
        state.move(black_->get_move(state));
      } else {
        state.move(white_->get_move(state));
      }
      std::cout << state.to_string() << std::endl;
    }
    black_wins += (state.winner() == game::BLACK ? 1 : 0);
    std::cout << state.score() << std::endl;
  }
  return black_wins;
}
