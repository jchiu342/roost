//
// Created by Jeremy on 12/11/2021.
//

#include "player/Evaluator.h"
#include <cassert>
#include <iostream>

/*virtual Evaluator::Evaluation Evaluator::evaluate(const game::GameState &state) {
  if (state.done()) {
    return {{}, (state.winner() == game::BLACK ? 1.0f : -1.0f)};
  }
  std::cout << "doing this eval" << std::endl;
  std::vector<float> policy;
  policy.assign(BOARD_SIZE * BOARD_SIZE + 1,
                static_cast<float>(1.0 / (BOARD_SIZE * BOARD_SIZE + 1)));
  float max_score = BOARD_SIZE * BOARD_SIZE + state.get_komi();
  assert(state.score() / max_score <= 1 && state.score() / max_score >= -1);
  return {policy, state.score() / max_score};
}*/
