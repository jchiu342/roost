#include "game/GameState.h"
#include "play/Match.h"
#include "player/Evaluator.h"
#include "player/MCTSPlayer.h"
#include "player/RandomPlayer.h"
#include <iostream>

using namespace game;

int main() {
  std::unique_ptr<Evaluator> eval = std::make_unique<Evaluator>();
  // std::unique_ptr<AbstractPlayer> black =
  // std::make_unique<MCTSPlayer>(game::Color::BLACK, std::move(eval));
  // std::unique_ptr<AbstractPlayer> white =
  // std::make_unique<RandomPlayer>(game::Color::WHITE);
  std::unique_ptr<AbstractPlayer> white =
      std::make_unique<MCTSPlayer>(game::Color::WHITE, std::move(eval));
  std::unique_ptr<AbstractPlayer> black =
      std::make_unique<RandomPlayer>(game::Color::BLACK);
  Match m(std::move(black), std::move(white), 2);
  std::cout << m.run();
  return 0;
}
