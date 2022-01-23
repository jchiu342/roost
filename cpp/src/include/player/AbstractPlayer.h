//
// Created by Jeremy on 11/25/2021.
//

#ifndef ROOST_ABSTRACTPLAYER_H
#define ROOST_ABSTRACTPLAYER_H

#include <utility>

#include "../game/Action.h"
#include "../game/GameState.h"

class AbstractPlayer {
public:
  explicit AbstractPlayer() {}
  virtual float get_wr(game::GameState state) { return 0.0f; }
  virtual game::Action get_move(game::GameState state, std::string *log) {
    return get_move(std::move(state));
  }
  virtual game::Action get_move(game::GameState state) = 0;
  virtual void reset() {}
};

#endif // ROOST_ABSTRACTPLAYER_H
