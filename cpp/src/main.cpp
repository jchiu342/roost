#include "game/GameState.h"
#include "play/Match.h"
#include "player/Evaluator.h"
#include "player/MCTSPlayer.h"
#include "player/NNEvaluator.h"
#include "player/RandomPlayer.h"
#include <iostream>
#include <memory>
#include <thread>

using namespace game;

void generate_data(int num_threads, int games, int playouts, const std::string &model_file, const std::string &save_dir) {
  auto task = [&, playouts, model_file, save_dir](int tid, int games) {
      std::unique_ptr<Evaluator> b_eval =
              std::make_unique<NNEvaluator>(model_file);
      std::unique_ptr<AbstractPlayer> black =
              std::make_unique<MCTSPlayer>(game::Color::BLACK, std::move(b_eval), 1.5, playouts);
      std::unique_ptr<Evaluator> w_eval =
              std::make_unique<NNEvaluator>(model_file);
      std::unique_ptr<AbstractPlayer> white =
              std::make_unique<MCTSPlayer>(game::Color::WHITE, std::move(w_eval), 1.5, playouts);
      std::string personal_save_dir = save_dir + to_string(tid);
      Match m(std::move(black), std::move(white), games, personal_save_dir);
      m.run();
  };
  int num_games = games / num_threads;
  std::vector<std::thread> threads;
  threads.reserve(num_threads);
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(std::thread{task, i, num_games});
  }
  for (int i = 0; i < num_threads; ++i) {
    threads[i].join();
  }
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
