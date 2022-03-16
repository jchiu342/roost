//
// Created by Jeremy on 11/23/2021.
//

#include "play/Match.h"
#include "game/GameState.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

Match::Match(std::unique_ptr<AbstractPlayer> &&black,
             std::unique_ptr<AbstractPlayer> &&white)
    : players{
          std::move(white),
          std::move(black),
      } {}

float Match::run(int gameId) {
  int black_wins = 0;
  std::uniform_real_distribution<float> dist(0, 1);
  float random_pct = dist(gen_);
  std::string sgf_string = "(;GM[1]FF[4]CA[UTF-8]AP[CGoban:3]ST[2]\nRU[AGA]"
                           "SZ[9]KM[7.50]\nPW[White]PB[Black]\n";
  game::GameState state;
  std::string temp_string;
  int black_resign_moves = 0;
  int white_resign_moves = 0;
  // int resign_moves[2] = {0, 0}; // white is index 0, black is index 1
  while (!state.done()) {
    game::Color turn = state.get_turn(); // -1 for white, 1 for black
    int turn_index = (turn + 1) / 2;     // 0 for white, 1 for black

    if (random_pct > NORESIGN_PCT && turn == game::BLACK && black_resign_moves >= RESIGN_CONSECUTIVE_MOVES) {
      state.move(game::Action(turn, game::RESIGN));
    } else if (random_pct > NORESIGN_PCT && turn == game::WHITE && white_resign_moves >= RESIGN_CONSECUTIVE_MOVES) {
      state.move(game::Action(turn, game::RESIGN));
    } else {
      game::Action move = players[turn_index]->get_move(state, &temp_string);
      float winrate = players[turn_index]->get_wr(state);
      if (winrate > (1.0 - RESIGN_THRESHOLD)) {
        ++white_resign_moves;
      } else {
        white_resign_moves = 0;
      }
      if (winrate < RESIGN_THRESHOLD) {
        ++black_resign_moves;
      } else {
        black_resign_moves = 0;
      }
      state.move(move);
      sgf_string += move.to_sgf_string() + temp_string;
    }
  }
  if (state.winner() == game::BLACK) {
    ++black_wins;
    sgf_string += "RE[B+R])";
  } else {
    sgf_string += "RE[W+R])";
  }
  players[0]->reset();
  players[1]->reset();
  // dump sgf to file
  std::ofstream outfile(std::to_string(gameId) + ".sgf");
  outfile << sgf_string;
  outfile.close();

  return state.score();
}
