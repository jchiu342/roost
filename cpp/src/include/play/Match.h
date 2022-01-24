//
// Created by Jeremy on 11/23/2021.
//

#ifndef ROOST_MATCH_H
#define ROOST_MATCH_H

#define RESIGN_THRESHOLD 0.05
#define NORESIGN_PCT 0.25
#define RESIGN_CONSECUTIVE_MOVES 8

#include "player/AbstractPlayer.h"
#include <atomic>
#include <memory>
#include <random>
#include <string>

class Match {
public:
  Match(std::shared_ptr<AbstractPlayer> black,
        std::shared_ptr<AbstractPlayer> white, int num_games, int num_threads,
        int tid, std::shared_ptr<std::atomic<int>> wins,
        std::shared_ptr<std::atomic<int>> games);
  // returns number of games won by black
  int run();

private:
  // TODO: investigate unique_ptr memory leak
  std::shared_ptr<AbstractPlayer> black_;
  std::shared_ptr<AbstractPlayer> white_;
  std::shared_ptr<std::atomic<int>> wins_;
  std::shared_ptr<std::atomic<int>> games_;
  std::random_device rd_;
  std::mt19937 gen_;
  int tid_;
  int num_games_;
  int num_threads_;
  // std::string save_dir_;
};

#endif // ROOST_MATCH_H
