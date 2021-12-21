//
// Created by Jeremy on 12/15/2021.
//

#ifndef ROOST_NNEVALUATOR_H
#define ROOST_NNEVALUATOR_H
#include "Evaluator.h"
#include <Torch/torch.h>
#include <vector>

#define NUM_BLOCKS 5
#define NUM_FILTERS 64

using namespace torch;

class NNEvaluator : public Evaluator {
  struct ConvBlock : nn::Module {
    ConvBlock() {
      conv1 = register_module(
          "conv1",
          nn::Conv2d(
              nn::Conv2dOptions(5, NUM_FILTERS, 3).stride(1).padding(1)));
      bn1 = register_module("bn1", nn::BatchNorm2d(NUM_FILTERS));
    }

    Tensor forward(Tensor s) {
      s = s.view({-1, 5, BOARD_SIZE, BOARD_SIZE});
      return nn::functional::relu(bn1->forward(conv1->forward(s)));
    }

    nn::Conv2d conv1{nullptr};
    nn::BatchNorm2d bn1{nullptr};
  };
  struct ResBlock : nn::Module {
    ResBlock() {
      conv1 = register_module(
          "conv1", nn::Conv2d(nn::Conv2dOptions(NUM_FILTERS, NUM_FILTERS, 3)
                                  .stride(1)
                                  .padding(1)
                                  .bias(false)));
      bn1 = register_module("bn1", nn::BatchNorm2d(NUM_FILTERS));
      conv2 = register_module(
          "conv2", nn::Conv2d(nn::Conv2dOptions(NUM_FILTERS, NUM_FILTERS, 3)
                                  .stride(1)
                                  .padding(1)
                                  .bias(false)));
      bn2 = register_module("bn2", nn::BatchNorm2d(NUM_FILTERS));
    }

    Tensor forward(Tensor s) {
      // auto residual = s;
      auto out = nn::functional::relu(bn1->forward(conv1->forward(s)));
      out = bn2->forward(conv2->forward(out));
      out += s;
      return nn::functional::relu(out);
    }
    nn::Conv2d conv1{nullptr}, conv2{nullptr};
    nn::BatchNorm2d bn1{nullptr}, bn2{nullptr};
  };

  struct OutBlock : nn::Module {
    OutBlock() {
      p_conv1 = register_module("p_conv1", nn::Conv2d(NUM_FILTERS, 2, 1));
      p_bn1 = register_module("p_bn1", nn::BatchNorm2d(2));
      p_logsoftmax1 = register_module("p_logsoftmax", nn::LogSoftmax(1));
      p_fc1 = register_module("p_fc1", nn::Linear(BOARD_SIZE * BOARD_SIZE * 2,
                                                  BOARD_SIZE * BOARD_SIZE + 1));

      v_conv1 = register_module("v_conv1", nn::Conv2d(NUM_FILTERS, 1, 1));
      v_bn1 = register_module("v_bn1", nn::BatchNorm2d(1));
      v_fc1 = register_module("v_fc1",
                              nn::Linear(BOARD_SIZE * BOARD_SIZE, NUM_FILTERS));
      v_fc2 = register_module("v_fc2", nn::Linear(NUM_FILTERS, 1));
    }

    // std::pair<Tensor, Tensor> forward(Tensor s) {
    Tensor forward(Tensor s) {
      // policy
      auto p = nn::functional::relu(p_bn1->forward(p_conv1->forward(s)));
      p = p.view({-1, BOARD_SIZE * BOARD_SIZE * 2});
      p = p_logsoftmax1->forward(p_fc1->forward(p)).exp();

      // value
      auto v = nn::functional::relu(v_bn1->forward(v_conv1->forward(s)));
      v = v.view({-1, BOARD_SIZE * BOARD_SIZE});
      v = nn::functional::relu(v_fc1->forward(v));
      v = tanh(v_fc2->forward(v));
      // TODO: fix this to return both policy/value
      return p;
      // return {p, v};
    }
    nn::Conv2d p_conv1{nullptr}, v_conv1{nullptr};
    nn::BatchNorm2d p_bn1{nullptr}, v_bn1{nullptr};
    nn::LogSoftmax p_logsoftmax1{nullptr};
    nn::Linear p_fc1{nullptr}, v_fc1{nullptr}, v_fc2{nullptr};
  };

  struct Net : nn::Module {
    // sequential is used to register each block, etc.; not sure if better way
    // to do this
    Net() {
      blocks->push_back(ConvBlock());
      // conv = ConvBlock();
      for (int i = 0; i < NUM_BLOCKS; ++i) {
        blocks->push_back(ResBlock());
      }
      blocks->push_back(OutBlock());
      register_module("blocks", blocks);
    }

    Tensor forward(Tensor s) { return blocks->forward(s); }
    // ConvBlock conv;
    nn::Sequential blocks;
    // OutBlock out;
  };

private:
  Net nn;
};

#endif // ROOST_NNEVALUATOR_H
