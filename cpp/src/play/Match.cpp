//
// Created by Jeremy on 11/23/2021.
//

#include "play/Match.h"
#include "game/GameState.h"
// #include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <memory>

// namespace fs = std::filesystem;

Match::Match(std::shared_ptr<AbstractPlayer> black,
             std::shared_ptr<AbstractPlayer> white, int num_games, int num_threads,
             int tid)
    : black_(std::move(black)), white_(std::move(white)), tid_(tid),
      num_games_(num_games), num_threads_(num_threads) {}

int Match::run() {
  // auto starting_path = fs::current_path();
  // fs::create_directory(save_dir_);
  // fs::current_path(save_dir_);
  int black_wins = 0;
  for (int i = tid_; i < num_games_; i += num_threads_) {
    std::string sgf_string = "(;GM[1]FF[4]CA[UTF-8]AP[CGoban:3]ST[2]\nRU[AGA]"
                             "SZ[9]KM[7.50]\nPW[White]PB[Black]\n";
    game::GameState state;
    while (!state.done()) {
      if (state.get_turn() == game::Color::BLACK) {
        game::Action move = black_->get_move(state);
        state.move(move);
        sgf_string += move.to_sgf_string();
      } else {
        game::Action move = white_->get_move(state);
        state.move(move);
        sgf_string += move.to_sgf_string();
      }
      // std::cout << state.to_string() << std::endl;
    }
    if (state.winner() == game::BLACK) {
      ++black_wins;
      sgf_string += "RE[B+R])";
    } else {
      sgf_string += "RE[W+R])";
    }
    black_->reset();
    white_->reset();
    // dump sgf to file
    std::ofstream outfile(std::to_string(i) + ".sgf");
    outfile << sgf_string;
    outfile.close();
    // black_wins += (state.winner() == game::BLACK ? 1 : 0);

    std::cout << state.score() << std::endl;
  }
  // fs::current_path(starting_path);
  return black_wins;
}
