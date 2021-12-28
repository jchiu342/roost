#include "game/GameState.h"
#include "play/Match.h"
#include "player/Evaluator.h"
#include "player/MCTSPlayer.h"
#include "player/NNEvaluator.h"
#include "player/RandomPlayer.h"
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

using namespace game;
namespace fs = std::filesystem;

void generate_data(int num_threads, int games, int playouts, std::string model_file, const std::string &save_dir) {
  // std::mutex mtx;
  model_file = "../" + model_file;
  auto task = [&, num_threads, playouts, model_file, save_dir](int tid, int games) {
      // mtx.lock();
      std::unique_ptr<Evaluator> b_eval =
              std::make_unique<NNEvaluator>(model_file);
      std::unique_ptr<AbstractPlayer> black =
              std::make_unique<MCTSPlayer>(game::Color::BLACK, std::move(b_eval), 1.5, playouts);
      std::unique_ptr<Evaluator> w_eval =
              std::make_unique<NNEvaluator>(model_file);
      std::unique_ptr<AbstractPlayer> white =
              std::make_unique<MCTSPlayer>(game::Color::WHITE, std::move(w_eval), 1.5, playouts);
      // mtx.unlock();
      Match m(std::move(black), std::move(white), games, num_threads, tid);
      m.run();
  };
  auto starting_path = fs::current_path();
  fs::create_directory(save_dir);
  fs::current_path(save_dir);
  // int num_games = games / num_threads;
  std::vector<std::thread> threads;
  threads.reserve(num_threads);
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(std::thread{task, i, games});
  }
  for (int i = 0; i < num_threads; ++i) {
    threads[i].join();
  }
  fs::current_path(starting_path);
}

int test_strength(std::string black_model_file, std::string white_model_file, std::string save_dir) {
  black_model_file = "../" + black_model_file;
  white_model_file = "../" + white_model_file;
  int num_threads = 16;
  int games = 64;
  int black_wins = 0;
  std::mutex mtx;
  auto task = [black_model_file, white_model_file, num_threads, &black_wins, &mtx](int tid, int games) {
      std::unique_ptr<Evaluator> b_eval = std::make_unique<NNEvaluator>(black_model_file);
      std::unique_ptr<Evaluator> w_eval = std::make_unique<NNEvaluator>(white_model_file);
      std::unique_ptr<AbstractPlayer> black = std::make_unique<MCTSPlayer>(game::Color::BLACK, std::move(b_eval), 1.5, 150, true);
      std::unique_ptr<AbstractPlayer> white = std::make_unique<MCTSPlayer>(game::Color::WHITE, std::move(w_eval), 1.5, 20, true);
      Match m(std::move(black), std::move(white), games, num_threads, tid);
      int b_wins = m.run();
      mtx.lock();
      black_wins += b_wins;
      mtx.unlock();
  };
  auto starting_path = fs::current_path();
  fs::create_directory(save_dir);
  fs::current_path(save_dir);
  // int num_games = games / num_threads;
  std::vector<std::thread> threads;
  threads.reserve(num_threads);
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(std::thread{task, i, games});
  }
  for (int i = 0; i < num_threads; ++i) {
    threads[i].join();
  }
  fs::current_path(starting_path);
  return black_wins;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "Incorrect usage\n";
    return -1;
  }
  /* int num_games = stoi(argv[1]);
  int num_threads = stoi(argv[2]);

  // generate_data(num_threads, num_games, 1000, "traced_model.pt", "speedtest" + to_string(num_threads));*/
  int x = test_strength(argv[1], argv[2], "test_1");
  std::cout << x << std::endl;
  // std::cout <<  x + (64 - test_strength(argv[2], argv[1], "test_2")) << std::endl;
  /*std::unique_ptr<Evaluator> b_eval = std::make_unique<NNEvaluator>("4x323.pt");
  std::unique_ptr<AbstractPlayer> black = std::make_unique<MCTSPlayer>(game::Color::BLACK, std::move(b_eval), 1.5, 2, true);
  std::unique_ptr<AbstractPlayer> white = std::make_unique<RandomPlayer>(game::Color::WHITE);
  Match m(std::move(black),std::move(white), 128, 1, 0);
  int net_wins = m.run();
  std::unique_ptr<Evaluator> w_eval = std::make_unique<NNEvaluator>("4x323.pt");
  std::unique_ptr<AbstractPlayer> w_mcts = std::make_unique<MCTSPlayer>(game::Color::WHITE, std::move(w_eval), 1.5, 2, true);
  std::unique_ptr<AbstractPlayer> b_random = std::make_unique<RandomPlayer>(game::Color::BLACK);
  Match m2(std::move(b_random),std::move(w_mcts), 128, 1, 0);
  net_wins += (128 - m2.run());
  std::cout << net_wins << std::endl;*/
  return 0;
}
