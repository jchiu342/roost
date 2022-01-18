#include "game/GameState.h"
#include "play/GTP.h"
#include "play/Match.h"
#include "player/Evaluator.h"
#include "player/MCTSPlayer.h"
#include "player/NNEvaluator.h"
#include "player/RandomPlayer.h"
#include <atomic>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

using namespace game;
namespace fs = std::filesystem;

void generate_data(int num_threads, int games, int playouts,
                   const std::string &model_file, const std::string &save_dir) {
  std::shared_ptr<Evaluator> eval =
      std::make_shared<NNEvaluator<32>>(model_file);
  std::shared_ptr<std::atomic<int>> win_counter =
      std::make_shared<std::atomic<int>>(0);
  std::shared_ptr<std::atomic<int>> game_counter =
      std::make_shared<std::atomic<int>>(0);
  auto task = [&, eval, num_threads, playouts, win_counter,
               game_counter](int tid, int games) {
    std::shared_ptr<AbstractPlayer> black =
        std::make_shared<MCTSPlayer>(eval, playouts);
    std::shared_ptr<AbstractPlayer> white =
        std::make_shared<MCTSPlayer>(eval, playouts);
    Match m(black, white, games, num_threads, tid, win_counter, game_counter);
    m.run();
  };
  auto starting_path = fs::current_path();
  fs::create_directory(save_dir);
  fs::current_path(save_dir);
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

int test_strength(const std::string &black_model_file,
                  const std::string &white_model_file, int num_threads,
                  int games, int playouts, const std::string &save_dir) {
  int black_wins = 0;
  std::mutex mtx;
  std::shared_ptr<Evaluator> b_eval =
      std::make_shared<NNEvaluator<16>>(black_model_file);
  std::shared_ptr<Evaluator> w_eval =
      std::make_shared<NNEvaluator<16>>(white_model_file);
  std::shared_ptr<std::atomic<int>> win_counter =
      std::make_shared<std::atomic<int>>(0);
  std::shared_ptr<std::atomic<int>> game_counter =
      std::make_shared<std::atomic<int>>(0);
  auto task = [b_eval, w_eval, num_threads, &black_wins, &mtx, win_counter,
               game_counter](int tid, int games, int playouts) {
    std::shared_ptr<AbstractPlayer> black =
        std::make_shared<MCTSPlayer>(b_eval, playouts, true);
    std::shared_ptr<AbstractPlayer> white =
        std::make_shared<MCTSPlayer>(w_eval, playouts, true);
    Match m(black, white, games, num_threads, tid, win_counter, game_counter);
    int b_wins = m.run();
    mtx.lock();
    black_wins += b_wins;
    mtx.unlock();
  };
  auto starting_path = fs::current_path();
  fs::create_directory(save_dir);
  fs::current_path(save_dir);
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

void gtp(const std::string &model_file, int playouts) {
  std::shared_ptr<Evaluator> eval =
      std::make_shared<NNEvaluator<1>>(model_file);
  std::shared_ptr<AbstractPlayer> engine =
      std::make_shared<MCTSPlayer>(eval, playouts, true);
  GTP gtp_runner(engine);
  gtp_runner.run();
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "Incorrect usage\n";
    return -1;
  }
  std::string command = argv[1];
  if (command == "generate_data") {
    if (argc < 7) {
      std::cout << "generate_data usage: ./roost generate_data <model_file> "
                   "<num_games> "
                   "<num_threads> <playouts> <save_directory>\n";
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
      std::cout << "test_strength usage: ./roost test_strength <model_1_file> "
                   "<model_2_file> "
                   "<num_games> <num_threads> <num_playouts>\n";
      return -1;
    }
    std::string model_1 = argv[2];
    std::string model_2 = argv[3];
    int num_games = stoi(argv[4]) / 2;
    int num_threads = stoi(argv[5]);
    int num_playouts = stoi(argv[6]);
    int b_wins = test_strength(model_1, model_2, num_threads, num_games,
                               num_playouts, "test_strength_black");
    std::cout << b_wins +
                     (num_games - test_strength(model_2, model_1, num_threads,
                                                num_games, num_playouts,
                                                "test_strength_white"))
              << std::endl;
    return 0;
  } else if (command == "gtp") {
    if (argc < 4) {
      std::cout << "gtp usage: ./roost gtp <model_file> <playouts>\n";
    }
    int num_playouts = stoi(argv[3]);
    gtp(argv[2], num_playouts);
  }
  return -1;
}
