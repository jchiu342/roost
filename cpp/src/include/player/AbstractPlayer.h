//
// Created by Jeremy on 11/25/2021.
//

#ifndef ROOST_ABSTRACTPLAYER_H
#define ROOST_ABSTRACTPLAYER_H

#include "../game/Action.h"
#include "../game/GameState.h"

class AbstractPlayer {
public:
  explicit AbstractPlayer(game::Color color) : color_(color) {}
  virtual game::Action get_move(game::GameState state) = 0;

protected:
  game::Color color_;
};


#endif //ROOST_ABSTRACTPLAYER_H
