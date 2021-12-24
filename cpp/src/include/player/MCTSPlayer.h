//
// Created by Jeremy on 12/11/2021.
//

#ifndef ROOST_MCTSPLAYER_H
#define ROOST_MCTSPLAYER_H
#include "AbstractPlayer.h"
#include "Evaluator.h"

#include <memory>
#include <random>
#include <unordered_map>
#include <vector>

class MCTSPlayer : public AbstractPlayer {
  class MCTSNode {
  public:
    // these represent counts for each child
    // each has size (# of indices); this can be optimized later
    std::vector<int> N;
    std::vector<float> Q;
    std::vector<float> P;
  };

public:
  MCTSPlayer(game::Color c, std::unique_ptr<Evaluator> &&evaluator,
             float cpuct = 1.0f, int playouts = 250);
  game::Action get_move(game::GameState state) override;
  void reset() override;

private:
  float visit(const game::GameState &state);
  void apply_dirichlet_noise_(const game::GameState &state);
  std::unique_ptr<Evaluator> evaluator_;
  // std::unordered_set<game::GameState> visited_;
  std::unordered_map<game::GameState, MCTSNode> map_;
  std::random_device rd_;
  std::mt19937 gen_;
  // for dirichlet noise
  float alpha_;
  float epsilon_;
  // hyperparameter for MCTS exploration
  float cpuct_;

  int playouts_;
  bool use_t1_;
};

#endif // ROOST_MCTSPLAYER_H
