//
// Created by Jeremy on 12/15/2021.
//

#ifndef ROOST_NNEVALUATOR_H
#define ROOST_NNEVALUATOR_H
#include "Evaluator.h"
#include <memory>
#include <torch/script.h>
#include <torch/torch.h>
#include <vector>

#define NUM_BLOCKS 5
#define NUM_FILTERS 64

using namespace torch;

class NNEvaluator : public Evaluator {
public:
  explicit NNEvaluator(const std::string &input_file = "");
  Evaluation Evaluate(const game::GameState &state) override;

private:
  torch::jit::script::Module module_;
  std::vector<torch::jit::IValue> inputs_;
  // protects input vector
  std::mutex mtx_;
  std::condition_variable cv_;
  // Net nn_;
};

#endif // ROOST_NNEVALUATOR_H
