//
// Created by Jeremy on 1/17/2022.
//

#include "play/GTP.h"
#include "game/Action.h"
#include <algorithm>
#include <iostream>
#include <string>

GTP::GTP(std::shared_ptr<AbstractPlayer> engine) : engine_(std::move(engine)) {}

void GTP::run() {
  try {
    game::GameState s(7.5);
    bool done = false;
    std::string input_str;
    while (!done) {
      std::cerr << s.to_string() << "\n";
      getline(std::cin, input_str);
      std::transform(input_str.begin(), input_str.end(), input_str.begin(),
                     ::tolower);
      if (input_str == "genmove b") {
        if (s.get_turn() != game::BLACK) {
          throw std::logic_error("invalid genmove turn");
        }
        game::Action a = engine_->get_move(s);
        std::cout << a.to_gtp_string() << "\n";
        s.move(a);
      } else if (input_str == "genmove w") {
        if (s.get_turn() != game::WHITE) {
          throw std::logic_error("invalid genmove turn");
        }
        game::Action a = engine_->get_move(s);
        std::cout << a.to_gtp_string() << "\n";
        s.move(a);
      } else if (input_str.substr(0, 4) == "play") {
        s.move(game::Action::from_action(input_str));
        std::cout << "=\n";
      }
      if (s.done()) {
        done = true;
      }
    }
  } catch (std::exception &e) {
    std::cerr << "Game crashed; error: " << e.what() << "\n";
  }
}
