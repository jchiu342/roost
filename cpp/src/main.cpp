#include "game/Action.h"
#include <iostream>

int main() {
  game::Action a(game::Color::BLACK, game::ActionType::PLAY);
  std::cout << "Hello, World!" << (a.get_color() == game::BLACK) << std::endl;
  return 0;
}
