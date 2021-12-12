#include "game/Action.h"
#include "game/GameState.h"
#include <iostream>

using namespace game;

int main() {
  // game::Action a(game::Color::BLACK, game::ActionType::PLAY);
  GameState state(7.5);
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 0, 2));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PLAY, 0, 1));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 0, 0));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PLAY, 1, 0));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 1, 1));
  std::cout << state.to_string() << std::endl;
  // std::cout << "Hello, World!" << (a.get_color() == game::BLACK) <<
  // std::endl;
  return 0;
}
