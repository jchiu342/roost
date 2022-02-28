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
  Match(std::unique_ptr<AbstractPlayer> &&black,
        std::unique_ptr<AbstractPlayer> &&white);
  // returns number of games won by black
  float run(int gameId);

private:
  // TODO: investigate unique_ptr memory leak
  std::unique_ptr<AbstractPlayer> players[2];
  std::random_device rd_;
  std::mt19937 gen_;
  // std::string save_dir_;
};

#endif // ROOST_MATCH_H
