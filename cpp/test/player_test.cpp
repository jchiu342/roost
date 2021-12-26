//
// Created by Jeremy on 11/25/2021.
//

#include <gtest/gtest.h>
#include <torch/script.h>
#include "game/GameState.h"
#include "player/RandomPlayer.h"
#include "player/Evaluator.h"
#include "player/MCTSPlayer.h"
#include "player/NNEvaluator.h"
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

TEST(PlayerTest, DISABLED_MatchTest) {
  // game::GameState state(7.5);
  std::unique_ptr<Evaluator> eval = std::make_unique<Evaluator>();
  // std::unique_ptr<AbstractPlayer> black = std::make_unique<MCTSPlayer>(game::Color::BLACK, std::move(eval));
  // std::unique_ptr<AbstractPlayer> white = std::make_unique<RandomPlayer>(game::Color::WHITE);
  std::unique_ptr<AbstractPlayer> white = std::make_unique<MCTSPlayer>(game::Color::WHITE, std::move(eval));
  std::unique_ptr<AbstractPlayer> black = std::make_unique<RandomPlayer>(game::Color::BLACK);
  Match m(std::move(black), std::move(white), 10, "dummy_dir");
  std::cout << m.run();
  // MCTSPlayer black_player(game::Color::BLACK, std::move(eval));
  // RandomPlayer white_player(game::Color::WHITE);
}

TEST(PlayerTest, DISABLED_NNTest) {
  game::GameState state(7.5);
  std::unique_ptr<Evaluator> eval = std::make_unique<NNEvaluator>("traced_model.pt");
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

/*TEST(PlayerTest, TorchLoadTest) {
  torch::jit::script::Module module;
  const std::string model_string = "traced_model.pt";
  try {
    // Deserialize the ScriptModule from a file using torch::jit::load().
    module = torch::jit::load(model_string);
  }
  catch (const c10::Error& e) {
    std::cerr << "error loading the model\n";
    // return -1;
  }

  std::cout << "ok\n";
}*/