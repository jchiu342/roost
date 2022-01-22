//
// Created by Jeremy on 12/15/2021.
// TODO: refactor this so it's no longer in 1 file
//

#ifndef ROOST_NNEVALUATOR_H
#define ROOST_NNEVALUATOR_H
#include "Evaluator.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <memory>
#include <mutex>
#include <torch/script.h>
#include <torch/torch.h>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace torch;
template <int threads> class NNEvaluator : public Evaluator {
public:
  class Batch {
  public:
    Batch(std::shared_ptr<torch::jit::script::Module> module)
        : module_(std::move(module)), loaded_threads_(0), threads_(threads),
          done_processing_(false) {
      memset(input_, 0, sizeof(input_));
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
        Tensor input_tensor =
            torch::from_blob(input_, {threads_, 5, BOARD_SIZE, BOARD_SIZE},
                             TensorOptions().dtype(kFloat));
        std::vector<torch::jit::IValue> inputs;
        inputs.emplace_back(input_tensor.to(at::kCUDA));
        // inputs.emplace_back(input_tensor);
        auto output = module_->forward(inputs).toTuple()->elements();
        // policy_output_ = output[0].toTensor();
        // value_output_ = output[1].toTensor();
        policy_output_ = output[0].toTensor().to(torch::kCPU);
        policy_output_ = torch::nn::functional::softmax(policy_output_,
                                                        torch::nn::functional::SoftmaxFuncOptions(1));
        value_output_ = output[1].toTensor().to(torch::kCPU);
        done_processing_ = true;
        cv_.notify_all();
      } else {
        // otherwise, we block until finished or until some time passes
        auto now = std::chrono::steady_clock::now();
        std::unique_lock<std::mutex> ul(mtx_);
        // TODO: confirm that we evaluate predicate before locking for the first
        // time. otherwise there is a bug here
        using namespace std::chrono_literals;
        cv_.wait_until(ul, now + 750ms, [this] { return done_processing_; });
        if (!done_processing_) {
          std::cout << "program will hang; feeding input\n";
          Tensor input_tensor =
              torch::from_blob(input_, {threads_, 5, BOARD_SIZE, BOARD_SIZE});
          std::vector<torch::jit::IValue> inputs;
          inputs.emplace_back(input_tensor.to(at::kCUDA));
          // inputs.emplace_back(input_tensor);
          auto output = module_->forward(inputs).toTuple()->elements();
          // policy_output_ = output[0].toTensor();
          // value_output_ = output[1].toTensor();
          policy_output_ = output[0].toTensor().to(torch::kCPU);
          policy_output_ = torch::nn::functional::softmax(policy_output_,
                                                          torch::nn::functional::SoftmaxFuncOptions(1));
          value_output_ = output[1].toTensor().to(torch::kCPU);
          done_processing_ = true;
          cv_.notify_all();
        }
      }
      std::vector<float> policy(
          policy_output_.data_ptr<float>() +
              (slot * (BOARD_SIZE * BOARD_SIZE + 1)),
          policy_output_.data_ptr<float>() +
              ((slot + 1) * (BOARD_SIZE * BOARD_SIZE + 1)));
      return {policy, value_output_[slot].item<float>()};
    }

    std::shared_ptr<torch::jit::script::Module> module_;
    float input_[threads][5][BOARD_SIZE][BOARD_SIZE];
    Tensor policy_output_;
    Tensor value_output_;
    std::atomic<int> loaded_threads_;
    int threads_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool done_processing_;
  };

  explicit NNEvaluator(const std::string &input_file = "");
  Evaluation Evaluate(const game::GameState &state) override;

private:
  std::shared_ptr<torch::jit::script::Module> module_;
  int batch_size_;
  int global_counter_;
  // TODO: test better data structures; unordered_map overhead may be high
  std::unordered_map<int, int> counters_;
  std::unordered_map<int, std::shared_ptr<Batch>> batch_map_;
  // protects maps and global counter
  std::mutex mtx_;
};

template <int threads>
NNEvaluator<threads>::NNEvaluator(const std::string &input_file)
    : batch_size_(threads), global_counter_(0) {
  try {
    std::cerr << "loading model " + input_file + "\n";
    module_ = std::make_shared<torch::jit::script::Module>();
    // *module_ = torch::jit::load(input_file);
    *module_ = torch::jit::load(input_file, torch::kCUDA);
    module_->eval();
    // at::globalContext().setBenchmarkCuDNN(false);
    std::cerr << "model " + input_file + " loaded successfully\n";
  } catch (const c10::Error &e) {
    std::cerr << "error loading model " + input_file + "\n";
    std::cerr << e.what() << std::endl;
    assert(false);
  }
}

// TODO: refactor this so it doesn't break abstraction for gamestate
template <int threads>
Evaluator::Evaluation
NNEvaluator<threads>::Evaluate(const game::GameState &state) {
  if (state.done()) {
    return {{}, (state.winner() == game::BLACK ? 1.0f : -1.0f)};
  }
  mtx_.lock();
  // each request is assigned a unique counter
  const int counter = global_counter_;
  ++global_counter_;
  // prevent counter overflow
  if (global_counter_ == batch_size_ * 1e7) {
    global_counter_ = 0;
  }
  const int batch_idx = counter / batch_size_;
  // if previous batches are full, start a new one
  if (counter % batch_size_ == 0) {
    counters_[batch_idx] = 0;
    batch_map_[batch_idx] = std::make_shared<Batch>(module_);
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
  batch_map_.erase(batch_idx);
  counters_.erase(batch_idx);
  mtx_.unlock();
  return return_eval;
}

#endif // ROOST_NNEVALUATOR_H
