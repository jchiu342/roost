#include "game/GameState.h"
#include "play/Match.h"
#include "player/Evaluator.h"
#include "player/MCTSPlayer.h"
#include "player/NNEvaluator.h"
#include "player/RandomPlayer.h"
#include <filesystem>
#include <string>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

using namespace game;
namespace fs = std::filesystem;

void generate_data(int num_threads, int games, int playouts, std::string model_file, const std::string &save_dir) {
  // std::mutex mtx;
  // model_file = "../" + model_file;
  std::shared_ptr<Evaluator> eval = std::make_shared<NNEvaluator>(model_file, num_threads);
  auto task = [&, eval, num_threads, playouts, save_dir](int tid, int games) {
      std::unique_ptr<AbstractPlayer> black =
              std::make_unique<MCTSPlayer>(game::Color::BLACK, eval, playouts);
      std::unique_ptr<AbstractPlayer> white =
              std::make_unique<MCTSPlayer>(game::Color::WHITE, eval, playouts);
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

int test_strength(std::string black_model_file, std::string white_model_file, int num_threads, int games, int playouts, const std::string& save_dir) {
  // black_model_file = "../" + black_model_file;
  // white_model_file = "../" + white_model_file;
  int black_wins = 0;
  std::mutex mtx;
  std::shared_ptr<Evaluator> b_eval = std::make_shared<NNEvaluator>(black_model_file);
  std::shared_ptr<Evaluator> w_eval = std::make_shared<NNEvaluator>(white_model_file);
  auto task = [b_eval, w_eval, num_threads, &black_wins, &mtx](int tid, int games, int playouts) {
      std::unique_ptr<AbstractPlayer> black = std::make_unique<MCTSPlayer>(game::Color::BLACK, b_eval, playouts, true);
      std::unique_ptr<AbstractPlayer> white = std::make_unique<MCTSPlayer>(game::Color::WHITE, w_eval, playouts, true);
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
    threads.emplace_back(std::thread{task, i, games, playouts});
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
  std::string command = argv[1];
  if (command == "generate_data") {
    if (argc < 7) {
      std::cout << "generate_data usage: ./roost <model_file> <num_games> <num_threads> <playouts> <save_directory>\n";
      return -1;
    }
    std::string model_file = argv[2];
    int num_games = stoi(argv[3]);
    int num_threads = stoi(argv[4]);
    int num_playouts = stoi(argv[5]);
    generate_data(num_threads, num_games, num_playouts, model_file, argv[6]);
    return 0;
  } else if (command == "test_strength") {
    if (argc < 7) {
      std::cout << "test_strength usage: ./roost <model_1_file> <model_2_file> <num_games> <num_threads> <num_playouts>\n";
      return -1;
    }
    std::string model_1 = argv[2];
    std::string model_2 = argv[3];
    int num_games = stoi(argv[4]) / 2;
    int num_threads = stoi(argv[5]);
    int num_playouts = stoi(argv[6]);
    int b_wins = test_strength(model_1, model_2, num_threads, num_games, num_playouts, "test_strength_black");
    std::cout << b_wins + (num_games - test_strength(model_2, model_1, num_threads, num_games, num_playouts, "test_strength_white")) << std::endl;
    return 0;
  }
  std::cout << "Incorrect usage\n";
  return -1;
  /* int num_games = stoi(argv[1]);
  int num_threads = stoi(argv[2]);

  // generate_data(num_threads, num_games, 1000, "traced_model.pt", "speedtest" + to_string(num_threads));*/
  // int x = test_strength(argv[1], argv[2], "test_1");
  // std::cout << x << std::endl;
  // std::cout <<  x + (128 - test_strength(argv[2], argv[1], "test_2")) << std::endl;
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
