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

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "Incorrect usage\n";
    return -1;
  }
  int num_games = stoi(argv[1]);
  int num_threads = stoi(argv[2]);
  generate_data(num_threads, num_games, 1000, "traced_model.pt", "speedtest" + to_string(num_threads));
  /*std::unique_ptr<Evaluator> b_eval =
      std::make_unique<NNEvaluator>("traced_model.pt");
  std::unique_ptr<AbstractPlayer> black =
      std::make_unique<MCTSPlayer>(game::Color::BLACK, std::move(b_eval));
  std::unique_ptr<Evaluator> w_eval =
      std::make_unique<NNEvaluator>("traced_model.pt");
  std::unique_ptr<AbstractPlayer> white =
      std::make_unique<MCTSPlayer>(game::Color::WHITE, std::move(w_eval));
  Match m(std::move(black), std::move(white), 2, argv[1]);
  int new_wins = m.run();
  std::cout << new_wins << std::endl; */
  return 0;
}
