//
// Created by Jeremy on 11/25/2021.
//

#include <gtest/gtest.h>
#include "game/GameState.h"
#include "player/RandomPlayer.h"
#include "player/Evaluator.h"
#include "player/MCTSPlayer.h"

TEST(PlayerTest, DISABLED_PlayerGameTest) {
  game::GameState state(7.5);
  RandomPlayer black_player(game::Color::BLACK);
  RandomPlayer white_player(game::Color::WHITE);
  while (!state.done()) {
    if (state.get_turn() == game::Color::BLACK) {
      state.move(black_player.get_move(state));
    } else {
      state.move(white_player.get_move(state));
    }
    std::cout << state.to_string() << std::endl;
  }
  std::cout << state.score() << std::endl;
}

TEST(PlayerTest, MCTSTest) {
  game::GameState state(7.5);
  std::unique_ptr<Evaluator> eval = std::make_unique<Evaluator>();
  MCTSPlayer black_player(game::Color::BLACK, std::move(eval));
  RandomPlayer white_player(game::Color::WHITE);
  while (!state.done()) {
    if (state.get_turn() == game::Color::BLACK) {
      state.move(black_player.get_move(state));
    } else {
      state.move(white_player.get_move(state));
    }
    std::cout << state.to_string() << std::endl;
  }
  std::cout << state.score() << std::endl;
}