//
// Created by Jeremy on 1/17/2022.
//

#include "play/GTP.h"
#include "game/Action.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

GTP::GTP(std::shared_ptr<AbstractPlayer> engine) : engine_(std::move(engine)) {}

void GTP::run() {
  try {
    game::GameState s(7.5);
    bool done = false;
    std::string input_str;
    std::string playout_log;
    while (!done) {
      std::cerr << s.to_string() << "\n";
      getline(std::cin, input_str);
      std::cerr << "received: " << input_str << std::endl;
      std::transform(input_str.begin(), input_str.end(), input_str.begin(),
                     ::tolower);
      if (input_str == "genmove b") {
        if (s.get_turn() != game::BLACK) {
          throw std::logic_error("invalid genmove turn");
        }
        game::Action a = engine_->get_move(s, &playout_log);
        std::cout << a.to_gtp_string() << "\n" << std::endl;
        std::cerr << playout_log;
        s.move(a);
      } else if (input_str == "genmove w") {
        if (s.get_turn() != game::WHITE) {
          throw std::logic_error("invalid genmove turn");
        }
        game::Action a = engine_->get_move(s, &playout_log);
        std::cout << a.to_gtp_string() << "\n" << std::endl;
        std::cerr << playout_log;
        s.move(a);
      } else if (input_str.substr(0, 8) == "kgs-chat") {

        std::istringstream ss(input_str);
        std::string word;
        std::vector<std::string> input_str_list;
        while (ss >> word) {
          input_str_list.push_back(word);
        }

        if (input_str_list[3] == "wr") {
          std::cout << "= winrate (for black) is " << engine_->get_wr(s) * 100
                    << "\n"
                    << std::endl;
        } else {
          std::cout << "= don't make illegal (tetris) moves -- i will "
                       "crash/bug out. please report bugs to jeremy!\n"
                    << std::endl;
        }
      } else if (input_str.substr(0, 4) == "play") {
        s.move(game::Action::from_action(input_str));
        std::cout << "=\n" << std::endl;
      } else if (input_str == "final_score") {
        std::cout << ((s.score() > 0) ? "= B+0.5\n" : "= W+0.5\n") << std::endl;
      } else if (input_str == "list_commands") {
        std::cout << "= genmove\nkomi\nplay\nclear_board\nkgs-chat\n"
                  << std::endl;
      } else if (input_str == "quit") {
        done = true;
        std::cout << "=\n" << std::endl;
      } else if (input_str == "clear_board") {
        s = game::GameState(7.5);
        std::cout << "=\n" << std::endl;
      } else {
        std::cout << "=\n" << std::endl;
      }
    }
  } catch (std::exception &e) {
    std::cerr << "Game crashed; error: " << e.what() << "\n";
  }
}
