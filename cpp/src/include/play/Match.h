//
// Created by Jeremy on 11/23/2021.
//

#ifndef ROOST_MATCH_H
#define ROOST_MATCH_H

#include "player/AbstractPlayer.h"
#include <memory>
#include <string>

class Match {
public:
  Match(std::unique_ptr<AbstractPlayer> &&black,
        std::unique_ptr<AbstractPlayer> &&white, int num_games,
        std::string save_dir);
  // returns number of games won by black
  int run();

private:
  std::unique_ptr<AbstractPlayer> black_;
  std::unique_ptr<AbstractPlayer> white_;
  int num_games_;
  std::string save_dir_;
};

#endif // ROOST_MATCH_H
