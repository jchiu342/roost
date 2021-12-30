//
// Created by Jeremy on 12/15/2021.
//

#include "player/NNEvaluator.h"
#include <torch/script.h>
#include <torch/torch.h>

using namespace torch;

NNEvaluator::NNEvaluator(const std::string &input_file) {
  try {
    // Deserialize the ScriptModule from a file using torch::jit::load().
    module_ = torch::jit::load(input_file, torch::kCUDA);
    module_.eval();
    // module_.to(at::kCUDA);
  } catch (const c10::Error &e) {
    std::cerr << "error loading the model\n";
    std::cout << e.what() << std::endl;
    assert(false);
    // return -1;
  }
  // Net nn;
  /* if (input_file != "") {
    torch::load(nn_, input_file);
    nn_->eval();
    // nn_.load_state_dict(torch::load("model.pt"));
  }
  nn_.eval(); */
}

// TODO: refactor this so it doesn't break abstraction for gamestate
Evaluator::Evaluation NNEvaluator::Evaluate(const game::GameState &state) {
  if (state.done()) {
    return {{}, (state.winner() == game::BLACK ? 1.0f : -1.0f)};
  }
  torch::NoGradGuard no_grad;
  Tensor input = torch::zeros({5, BOARD_SIZE, BOARD_SIZE});
  const game::Color *index_0 = state.get_board(0);
  if (state.get_turn() == game::BLACK) {
    for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; ++i) {
      if (*(index_0 + i) == game::BLACK) {
        input[0][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
      } else if (*(index_0 + i) == game::WHITE) {
        input[1][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
      }
      if (*(index_0 + i + BOARD_SIZE * BOARD_SIZE) == game::BLACK) {
        input[2][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
      } else if (*(index_0 + i + BOARD_SIZE * BOARD_SIZE) == game::WHITE) {
        input[3][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
      }
      input[4][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
    }
  } else {
    for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; ++i) {
      if (*(index_0 + i) == game::BLACK) {
        input[1][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
      } else if (*(index_0 + i) == game::WHITE) {
        input[0][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
      }
      if (*(index_0 + i + BOARD_SIZE * BOARD_SIZE) == game::BLACK) {
        input[3][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
      } else if (*(index_0 + i + BOARD_SIZE * BOARD_SIZE) == game::WHITE) {
        input[2][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
      }
    }
  }
  std::vector<torch::jit::IValue> inputs;
  // inputs.emplace_back(input);
  inputs.emplace_back(input.to(at::kCUDA));

  auto output = module_.forward(inputs).toTuple()->elements();
  assert(output.size() == 2);
  auto policy_tensor = output[0].toTensor();
  auto value_tensor = output[1].toTensor();
  std::vector<float> policy;
  policy.reserve(BOARD_SIZE * BOARD_SIZE + 1);
  // std::cout << state.to_string() << std::endl;
  for (int i = 0; i < BOARD_SIZE * BOARD_SIZE + 1; ++i) {
    policy.push_back(policy_tensor[0][i].item<float>());
    // std::cout << i << ' ' << policy[i] << std::endl;
  }
  // std::cout << "value: " << value_tensor.item<float>();*/
  return {policy, value_tensor.item<float>()};
}
