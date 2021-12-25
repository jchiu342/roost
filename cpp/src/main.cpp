#include "game/GameState.h"
#include "play/Match.h"
#include "player/Evaluator.h"
#include "player/MCTSPlayer.h"
#include "player/NNEvaluator.h"
#include "player/RandomPlayer.h"
#include <iostream>
#include <memory>

using namespace game;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: ./roost <dir_name>\n";
    return -1;
  }
  std::unique_ptr<Evaluator> b_eval =
      std::make_unique<NNEvaluator>("gen_9.pt");
  std::unique_ptr<AbstractPlayer> black =
      std::make_unique<MCTSPlayer>(game::Color::WHITE, std::move(b_eval));
  std::unique_ptr<Evaluator> w_eval =
      std::make_unique<NNEvaluator>("traced_model.pt");
  std::unique_ptr<AbstractPlayer> white =
      std::make_unique<MCTSPlayer>(game::Color::BLACK, std::move(w_eval));
  Match m(std::move(white), std::move(black), 25, argv[1]);
  int new_wins = m.run();
  std::cout << new_wins << std::endl;
  return 0;
}
