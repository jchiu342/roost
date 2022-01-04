//
// Created by Jeremy on 12/15/2021.
//

#ifndef ROOST_NNEVALUATOR_H
#define ROOST_NNEVALUATOR_H
#include "Evaluator.h"
#include <torch/script.h>
#include <torch/torch.h>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#define NUM_BLOCKS 5
#define NUM_FILTERS 64

using namespace torch;

class NNEvaluator : public Evaluator {
public:
  class Batch {
  public:
    Batch(std::shared_ptr<torch::jit::script::Module> module, int threads) : module_(std::move(module)), loaded_threads_(0),
                                                                             threads_(threads), done_processing_(false) {
      input_ = torch::zeros({threads, 5, BOARD_SIZE, BOARD_SIZE});
    }
    // it is the caller's responsibility to ensure no duplicate slots
    Evaluation Evaluate(const game::GameState &state, int slot) {
      torch::NoGradGuard no_grad;
      const game::Color *index_0 = state.get_board(0);
      if (state.get_turn() == game::BLACK) {
        for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; ++i) {
          if (*(index_0 + i) == game::BLACK) {
            input_[slot][0][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
          } else if (*(index_0 + i) == game::WHITE) {
            input_[slot][1][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
          }
          if (*(index_0 + i + BOARD_SIZE * BOARD_SIZE) == game::BLACK) {
            input_[slot][2][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
          } else if (*(index_0 + i + BOARD_SIZE * BOARD_SIZE) == game::WHITE) {
            input_[slot][3][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
          }
          input_[slot][4][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
        }
      } else {
        for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; ++i) {
          if (*(index_0 + i) == game::BLACK) {
            input_[slot][1][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
          } else if (*(index_0 + i) == game::WHITE) {
            input_[slot][0][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
          }
          if (*(index_0 + i + BOARD_SIZE * BOARD_SIZE) == game::BLACK) {
            input_[slot][3][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
          } else if (*(index_0 + i + BOARD_SIZE * BOARD_SIZE) == game::WHITE) {
            input_[slot][2][i / BOARD_SIZE][i % BOARD_SIZE] = 1;
          }
        }
      }
      // if we are the last thread to finish, we do the evaluation
      if (loaded_threads_.fetch_add(1) == threads_ - 1) {
        std::vector<torch::jit::IValue> inputs;
        inputs.emplace_back(input_.to(at::kCUDA));
        auto output = module_->forward(inputs).toTuple()->elements();
        policy_output_ = output[0].toTensor().to(torch::kCPU);
        value_output_ = output[1].toTensor().to(torch::kCPU);
        done_processing_ = true;
        cv_.notify_all();
      } else {
        // otherwise, we block until finished
        std::unique_lock<std::mutex> ul(mtx_);
        // TODO: confirm that we evaluate predicate before locking for the first time. otherwise there is a bug here
        cv_.wait(ul, [this] { return done_processing_; });
      }
      std::vector<float> policy(policy_output_.data_ptr<float>() + (slot * (BOARD_SIZE * BOARD_SIZE + 1)),
              policy_output_.data_ptr<float>() + ((slot + 1) * BOARD_SIZE * BOARD_SIZE + 1));
      return {policy, value_output_[slot].item<float>()};
    }

    std::shared_ptr<torch::jit::script::Module> module_;
    Tensor input_;
    Tensor policy_output_;
    Tensor value_output_;
    std::atomic<int> loaded_threads_;
    int threads_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool done_processing_;
  };

  explicit NNEvaluator(const std::string &input_file = "", const int batch_size = 1);
  Evaluation Evaluate(const game::GameState &state) override;

private:
  std::shared_ptr<torch::jit::script::Module> module_;
  int batch_size_;
  int global_counter_;
  // TODO: find better data structures for these tasks; unordered_map overhead is high
  std::unordered_map<int, int> counters_;
  std::unordered_map<int, std::shared_ptr<Batch>> batch_map_;
  // protects maps and global counter
  std::mutex mtx_;
};

#endif // ROOST_NNEVALUATOR_H
