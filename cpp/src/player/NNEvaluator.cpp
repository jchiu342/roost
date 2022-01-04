//
// Created by Jeremy on 12/15/2021.
//

#include "player/NNEvaluator.h"
#include <torch/script.h>
#include <torch/torch.h>
#include <memory>

using namespace torch;

NNEvaluator::NNEvaluator(const std::string &input_file, const int batch_size) : batch_size_(batch_size),
global_counter_(0) {
  try {
    module_ = std::make_shared<torch::jit::script::Module>();
    // Deserialize the ScriptModule from a file using torch::jit::load().
    // module_ = torch::jit::load(input_file);
    *module_ = torch::jit::load(input_file, torch::kCUDA);
    module_->eval();
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
  mtx_.lock();
  // each request is assigned a unique counter
  const int counter = global_counter_;
  ++global_counter_;
  const int batch_idx = counter / batch_size_;
  // if previous batches are full, start a new one
  if (counter % batch_size_ == 0) {
    counters_[batch_idx] = 0;
    batch_map_[batch_idx] = std::make_shared<Batch>(module_, batch_size_);
  }
  std::shared_ptr<Batch> batch = batch_map_[batch_idx];
  mtx_.unlock();
  Evaluation return_eval = batch->Evaluate(state, counter % batch_size_);
  mtx_.lock();
  if (++counters_[batch_idx] < batch_size_) {
    // not the last thread; we can leave
    mtx_.unlock();
    return return_eval;
  }
  // last thread is responsible for deallocation + erase
  // delete batch;
  batch_map_.erase(batch_idx);
  counters_.erase(batch_idx);
  mtx_.unlock();
  // delete batch;
  return return_eval;
  /* torch::NoGradGuard no_grad;
  Tensor input = torch::zeros({16, 5, BOARD_SIZE, BOARD_SIZE});
  const game::Color *index_0 = state.get_board(0);
  for (int z = 0; z < 16; ++z) {
    if (state.get_turn() == game::BLACK) {
      for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; ++i) {
        if (*(index_0 + i) == game::BLACK) {
          input[z][0][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
        } else if (*(index_0 + i) == game::WHITE) {
          input[z][1][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
        }
        if (*(index_0 + i + BOARD_SIZE * BOARD_SIZE) == game::BLACK) {
          input[z][2][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
        } else if (*(index_0 + i + BOARD_SIZE * BOARD_SIZE) == game::WHITE) {
          input[z][3][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
        }
        input[z][4][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
      }
    } else {
      for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; ++i) {
        if (*(index_0 + i) == game::BLACK) {
          input[z][1][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
        } else if (*(index_0 + i) == game::WHITE) {
          input[z][0][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
        }
        if (*(index_0 + i + BOARD_SIZE * BOARD_SIZE) == game::BLACK) {
          input[z][3][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
        } else if (*(index_0 + i + BOARD_SIZE * BOARD_SIZE) == game::WHITE) {
          input[z][2][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
        }
      }
    }
  }
  std::vector<torch::jit::IValue> inputs;
  // inputs.emplace_back(input);
  inputs.emplace_back(input.to(at::kCUDA));

  auto output = module_.forward(inputs).toTuple()->elements();
  assert(output.size() == 2);
  // auto policy_tensor = output[0].toTensor();
  auto value_tensor = output[1].toTensor();
  auto policy_tensor = output[0].toTensor().to(torch::kCPU);
  // auto value_tensor = output[1].toTensor().to(torch::kCPU);
  std::vector<float> policy(policy_tensor.data_ptr<float>(), policy_tensor.data_ptr<float>() + (BOARD_SIZE * BOARD_SIZE + 1));
  // policy.reserve(BOARD_SIZE * BOARD_SIZE + 1);
  // std::cout << policy_tensor.dim() << std::endl;
  // std::cout << policy_tensor.size(0) << ' ' << policy_tensor.size(1) << std::endl;
  // std::cout << value_tensor.dim() << std::endl;
  // std::cout << value_tensor.size(0) << ' ' << value_tensor.size(1) << std::endl;
  // std::cout << state.to_string() << std::endl;
  // std::memcpy(&policy[0], policy_tensor.data_ptr(), sizeof(float) * (BOARD_SIZE * BOARD_SIZE + 1));
  for (int i = 0; i < BOARD_SIZE * BOARD_SIZE + 1; ++i) {
    if (policy[i] != policy_tensor[0][i].item<float>()) {
      std::cout << "incorrect at position" << i << std::endl;
    }
  }
  // std::cout << "value: " << value_tensor.item<float>();
  return {policy, value_tensor[0].item<float>()};*/
}
