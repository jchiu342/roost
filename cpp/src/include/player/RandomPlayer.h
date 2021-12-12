//
// Created by Jeremy on 11/25/2021.
//

#ifndef ROOST_RANDOMPLAYER_H
#define ROOST_RANDOMPLAYER_H

#include "AbstractPlayer.h"
#include <random>

class RandomPlayer : public AbstractPlayer {
public:
  explicit RandomPlayer(game::Color c);
  game::Action get_move(game::GameState state) override;

private:
  std::random_device rd_;
  std::mt19937 gen_;
};

#endif // ROOST_RANDOMPLAYER_H
