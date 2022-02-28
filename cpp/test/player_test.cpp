//
// Created by Jeremy on 11/25/2021.
//

#include <gtest/gtest.h>
#include <torch/script.h>
#include <chrono>
#include <string>
#include <stdexcept>
#include <mutex>
#include "game/GameState.h"
#include "player/RandomPlayer.h"
#include "player/Evaluator.h"
#include "player/MCTSPlayer.h"
#include "player/NNEvaluator.h"
#include "play/Match.h"
#include "play/GTP.h"

TEST(PlayerTest, DISABLED_GameSpeedTest) {
  constexpr size_t num_iters = 10;
  constexpr size_t num_games = 1000;
  double sum = 0.0;
  for (size_t j = 0; j < num_iters; ++j) {
    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < num_games; ++i) {
      game::GameState state(7.5);
      RandomPlayer black_player;
      RandomPlayer white_player;
      while (!state.done()) {
        if (state.get_turn() == game::Color::BLACK) {
          state.move(black_player.get_move(state));
        } else {
          state.move(white_player.get_move(state));
        }
        // std::cout << state.to_string() << std::endl;
      }
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;
    sum += diff.count();
    std::cout << "time taken: " << std::fixed << diff.count() << "s\n";
  }
  std::cout << "avg: " << std::fixed << sum / (num_iters) << std::endl;
}

TEST(PlayerTest, DISABLED_NNTest) {
  game::GameState state(7.5);
  std::unique_ptr<Evaluator> eval = std::make_unique<NNEvaluator<1>>("traced_model.pt");
  MCTSPlayer black_player(std::move(eval));
  RandomPlayer white_player;
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
TEST(PlayerTest, DISABLED_MultiThreadNNTest) {
  constexpr size_t num_threads = 16;
  std::shared_ptr<Evaluator> eval = std::make_shared<NNEvaluator<num_threads>>("net3.pt");
  std::vector<game::GameState> states;
  std::vector<float> evals;
  states.reserve(num_threads);
  evals.reserve(num_threads);
  RandomPlayer black_player;
  RandomPlayer white_player;
  for (size_t i = 0; i < num_threads; ++i) {
    states.emplace_back(7.5);
    states[i].move(black_player.get_move(states[i]));
    states[i].move(white_player.get_move(states[i]));
  }
  auto task = [&eval, &states, &evals](int tid) {
    Evaluator::Evaluation x = eval->Evaluate(states[tid]);
    evals[tid] = x.value_;
    ASSERT_TRUE(evals[tid] < 1.001 && evals[tid] > -1.001);
    float sum = std::accumulate(x.policy_.begin(), x.policy_.end(), 0.0);
    ASSERT_TRUE(sum > 1 - 1e-3 && sum < 1 + 1e-3);
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
  std::unique_ptr<Evaluator> st_eval = std::make_unique<NNEvaluator<1>>("net3.pt");
  for (size_t i = 0; i < num_threads; ++i) {
    Evaluator::Evaluation x = st_eval->Evaluate(states[i]);
    // account for some rounding errors
    ASSERT_TRUE(abs(x.value_ - evals[i]) < 1e-4);
    // std::cout << x.value_ << ' ' << evals[i] << std::endl;
  }
}

// TODO: perhaps integrate Google Benchmark or some other tool for more accurate measurement
TEST(PlayerTest, DISABLED_SpeedTest) {
  double sum = 0.0;
  size_t num_iters = 10;
  for (size_t j = 0; j < num_iters; ++j) {
    size_t num_threads = 16;
    std::shared_ptr<Evaluator> eval = std::make_shared<NNEvaluator<4>>("net3.pt");
    std::vector<game::GameState> states;
    states.reserve(num_threads);
    RandomPlayer black_player;
    RandomPlayer white_player;
    for (size_t i = 0; i < num_threads; ++i) {
      states.emplace_back(7.5);
      states[i].move(black_player.get_move(states[i]));
      states[i].move(white_player.get_move(states[i]));
    }
    auto task = [&eval, &states](int tid) {
        float ret = 0;
        for (size_t i = 0; i < 200; ++i) {
          Evaluator::Evaluation x = eval->Evaluate(states[tid]);
          // prevent evaluation from being optimized away
          ret += x.value_;
        }
        return ret;
    };

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < num_threads; ++i) {
      threads.emplace_back(std::thread{task, i});
    }
    for (size_t i = 0; i < num_threads; ++i) {
      threads[i].join();
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;
    // disregard the first run because jit is being optimized
    if (j != 0) {
      sum += diff.count();
    }
    std::cout << "time taken: " << std::fixed << diff.count() << "s\n";
  }
  std::cout << "avg: " << std::fixed << sum / (num_iters - 1) << std::endl;
}

TEST(PlayerTest, DISABLED_GTPTest) {
  std::shared_ptr<Evaluator> eval = std::make_shared<NNEvaluator<1>>("net3.pt");
  std::shared_ptr<AbstractPlayer> engine = std::make_shared<MCTSPlayer>(eval, 100, true);
  GTP gtp_runner(engine);
  gtp_runner.run();
}