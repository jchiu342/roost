//
// Created by Jeremy on 12/11/2021.
//

#ifndef ROOST_EVALUATOR_H
#define ROOST_EVALUATOR_H

#include "game/GameState.h"
#include <utility>
#include <vector>

// TODO: replace with abstract evaluator when we implement NN
class Evaluator {
public:
  class Evaluation {
  public:
    Evaluation(std::vector<float> policy, float value)
        : policy_(std::move(policy)), value_(value) {}
    // we always return a vector with length equal to the # of possible indices
    // the MCTS is responsible for filtering out illegal actions
    std::vector<float> policy_;
    // value ranges from 1 (black win) to -1 (white win)
    float value_;
  };
  virtual Evaluation Evaluate(const game::GameState &state) = 0;
};

#endif // ROOST_EVALUATOR_H
