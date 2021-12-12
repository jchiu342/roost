//
// Created by Jeremy on 12/11/2021.
//

#ifndef ROOST_EVALUATOR_H
#define ROOST_EVALUATOR_H

#include <utility>
#include <vector>
#include "game/GameState.h"

// TODO: replace with abstract evaluator when we implement NN
class Evaluator {
public:
  class Evaluation {
  public:
    Evaluation(std::vector<float> policy, float value) : policy_(std::move(policy)), value_(value) {}
    // we always return a vector with length equal to the # of possible indices
    // the MCTS is responsible for filtering out illegal actions
    std::vector<float> policy_;
    // value ranges from 1 (black win) to -1 (white win)
    float value_;
  };
  Evaluation evaluate(const game::GameState &state);
};

#endif //ROOST_EVALUATOR_H
