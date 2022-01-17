//
// Created by Jeremy on 1/17/2022.
//

#include "play/GTP.h"
#include <iostream>

GTP::GTP(std::shared_ptr<AbstractPlayer> engine) : engine_(std::move(engine)) {}

void GTP::run() {
  try {
    game::GameState s(7.5);
  } catch (std::exception &e) {
    std::cerr << "Invalid GTP output\n";
  }
}
