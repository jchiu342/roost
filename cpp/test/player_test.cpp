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

/* TEST(PlayerTest, DISABLED_MatchTest) {
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
}*/

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

// test NNEvaluator correctness under multiple threads
TEST(PlayerTest, MultiThreadNNTest) {
  size_t num_threads = 10;
  std::shared_ptr<Evaluator> eval = std::make_shared<NNEvaluator>("4x32_net1.pt");
  std::vector<game::GameState> states;
  std::vector<float> evals;
  states.reserve(num_threads);
  evals.reserve(num_threads);
  RandomPlayer black_player(game::Color::BLACK);
  RandomPlayer white_player(game::Color::WHITE);
  for (size_t i = 0; i < num_threads; ++i) {
    states.emplace_back(7.5);
    states[i].move(black_player.get_move(states[i]));
    states[i].move(white_player.get_move(states[i]));
  }
  auto task = [&eval, &states, &evals](int tid) {
    Evaluator::Evaluation x = eval->Evaluate(states[tid]);
    evals[tid] = x.value_;
  };

  std::vector<std::thread> threads;
  threads.reserve(num_threads);
  for (size_t i = 0; i < num_threads; ++i) {
    threads.emplace_back(std::thread{task, i});
  }
  for (size_t i = 0; i < num_threads; ++i) {
    threads[i].join();
  }
  // check correctness against single-thread mode
  for (size_t i = 0; i < num_threads; ++i) {
    Evaluator::Evaluation x = eval->Evaluate(states[i]);
    // no arithmetic is done, so we shouldn't get any rounding errors
    ASSERT_EQ(x.value_, evals[i]);
  }
}