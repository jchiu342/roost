#include "player/NNEvaluator.h"
#include <memory>
#include <torch/script.h>
#include <torch/torch.h>

using namespace torch;

NNEvaluator::NNEvaluator(const std::string &input_file, const int batch_size)
    : batch_size_(batch_size), global_counter_(0) {
  try {
    module_ = std::make_shared<torch::jit::script::Module>();
    // *module_ = torch::jit::load(input_file);
    *module_ = torch::jit::load(input_file, torch::kCUDA);
    module_->eval();
  } catch (const c10::Error &e) {
    std::cerr << "error loading the model\n";
    std::cout << e.what() << std::endl;
    assert(false);
  }
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
  batch_map_.erase(batch_idx);
  counters_.erase(batch_idx);
  mtx_.unlock();
  return return_eval;
}
