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
  std::chrono::system_clock::time_point start =
      std::chrono::system_clock::now();
  auto task = [&, eval, num_threads, playouts, win_counter,
               game_counter](int tid, int games) {
    std::shared_ptr<AbstractPlayer> black =
        std::make_shared<MCTSPlayer>(eval, playouts);
    std::shared_ptr<AbstractPlayer> white =
        std::make_shared<MCTSPlayer>(eval, playouts);
    Match m(black, white);

    for (int i = tid; i < games; i += num_threads) {
      float res = m.run(i);
      game_counter->fetch_add(1);
      if (res > 0) {
        win_counter->fetch_add(1);
      }
      auto end = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsed_seconds = end - start;
      std::cout << black->get_eval_time() + white->get_eval_time();
      std::cout << "Game " << i << ": " << res << "; " << *win_counter << "/"
                << *game_counter << "; "
                << (elapsed_seconds.count() / *game_counter) << std::endl;
    }
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

void generate_data_pcr(int num_threads, int games, int small, int big,
                       const std::string &model_file,
                       const std::string &save_dir) {
  std::shared_ptr<Evaluator> eval =
      std::make_shared<NNEvaluator<96>>(model_file);
  std::shared_ptr<std::atomic<int>> win_counter =
      std::make_shared<std::atomic<int>>(0);
  std::shared_ptr<std::atomic<int>> game_counter =
      std::make_shared<std::atomic<int>>(0);
  std::chrono::system_clock::time_point start =
      std::chrono::system_clock::now();

  auto task = [&, eval, num_threads, small, big, win_counter,
               game_counter](int tid, int games) {
    std::shared_ptr<AbstractPlayer> black =
        std::make_shared<MCTSPlayer>(eval, -1, false, true, small, big);
    std::shared_ptr<AbstractPlayer> white =
        std::make_shared<MCTSPlayer>(eval, -1, false, true, small, big);
    Match m(black, white);

    for (int i = tid; i < games; i += num_threads) {

      float res = m.run(i);
      game_counter->fetch_add(1);
      if (res > 0) {
        win_counter->fetch_add(1);
      }
      auto end = std::chrono::system_clock::now();

      std::chrono::duration<double> elapsed_seconds = end - start;

      std::cout << black->get_eval_time() + white->get_eval_time() << " "
                << elapsed_seconds.count() << std::endl;
      std::cout << "Game " << i << ": " << res << "; " << *win_counter << "/"
                << *game_counter << "; "
                << (elapsed_seconds.count() / *game_counter) << std::endl;
    }
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

int test_strength(const std::string &model1_file,
                  const std::string &model2_file, int num_threads, int games,
                  int playouts, const std::string &save_dir) {

  std::shared_ptr<Evaluator> model1_eval =
      std::make_shared<NNEvaluator<48>>(model1_file);
  std::shared_ptr<Evaluator> model2_eval =
      std::make_shared<NNEvaluator<48>>(model2_file);
  std::shared_ptr<std::atomic<int>> win_counter =
      std::make_shared<std::atomic<int>>(0);
  std::shared_ptr<std::atomic<int>> game_counter =
      std::make_shared<std::atomic<int>>(0);
  std::chrono::system_clock::time_point start =
      std::chrono::system_clock::now();

  auto task = [&, model1_eval, model2_eval, num_threads, win_counter,
               game_counter](int tid, int games, int playouts) {
    std::shared_ptr<AbstractPlayer> player1 =
        std::make_shared<MCTSPlayer>(model1_eval, playouts, true);
    std::shared_ptr<AbstractPlayer> player2 =
        std::make_shared<MCTSPlayer>(model2_eval, playouts, true);
    Match m(player1, player2);
    Match m2(player2, player1);
    for (int i = tid; i < games; i += num_threads) {
      float res;
      if (i % 2 == 0) {
        res = m.run(i);
      } else {
        res = m2.run(i);
      }
      game_counter->fetch_add(1);
      if ((i % 2 == 0 && res > 0) || (i % 2 == 1 && res < 0)) {
        win_counter->fetch_add(1); // player 1 wins
      }
      auto end = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsed_seconds = end - start;
      std::cout << res << "; " << *win_counter << "/" << *game_counter << "; "
                << (elapsed_seconds.count() / *game_counter) << std::endl;
    }
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
  return *win_counter;
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
  } else if (command == "generate_data_pcr") {
    if (argc < 8) {
      std::cout
          << "generate_data_pcr usage: ./roost generate_data <model_file> "
             "<num_games> <num_threads> <n> <N> <save_directory>\n";
      return -1;
    }
    std::string model_file = argv[2];
    int num_games = stoi(argv[3]);
    int num_threads = stoi(argv[4]);
    int small = stoi(argv[5]);
    int big = stoi(argv[6]);
    generate_data_pcr(num_threads, num_games, small, big, model_file, argv[7]);
  } else if (command == "test_strength") {
    if (argc < 7) {
      std::cout << "test_strength usage: ./roost test_strength <model_1_file> "
                   "<model_2_file> "
                   "<num_games> <num_threads> <num_playouts>\n";
      return -1;
    }
    std::string model_1 = argv[2];
    std::string model_2 = argv[3];
    int num_games = stoi(argv[4]);
    int num_threads = stoi(argv[5]);
    int num_playouts = stoi(argv[6]);
    std::cout << test_strength(model_1, model_2, num_threads, num_games,
                               num_playouts, "test_strength")
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
