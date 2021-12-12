//
// Created by Jeremy on 11/25/2021.
//

#include <gtest/gtest.h>
#include "game/GameState.h"
#include "player/RandomPlayer.h"
#include "player/Evaluator.h"
#include "player/MCTSPlayer.h"
#include "play/Match.h"

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

TEST(PlayerTest, DISABLED_MCTSTest) {
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

TEST(PlayerTest, MatchTest) {
  // game::GameState state(7.5);
  std::unique_ptr<Evaluator> eval = std::make_unique<Evaluator>();
  std::unique_ptr<AbstractPlayer> black = std::make_unique<MCTSPlayer>(game::Color::BLACK, std::move(eval));
  std::unique_ptr<AbstractPlayer> white = std::make_unique<RandomPlayer>(game::Color::WHITE);
  Match m(std::move(black), std::move(white), 10);
  m.run();
  // MCTSPlayer black_player(game::Color::BLACK, std::move(eval));
  // RandomPlayer white_player(game::Color::WHITE);
}