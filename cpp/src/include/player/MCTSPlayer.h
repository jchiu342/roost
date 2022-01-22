//
// Created by Jeremy on 12/11/2021.
//

#ifndef ROOST_MCTSPLAYER_H
#define ROOST_MCTSPLAYER_H
#include "AbstractPlayer.h"
#include "Evaluator.h"

#include <memory>
#include <random>
#include <string>
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
    // our own value of N and Q
    int Ns;
    float Qs;
  };

public:
  MCTSPlayer(std::shared_ptr<Evaluator> evaluator, int playouts = 250,
             bool eval_mode = false, bool use_pcr = false, int pcr_small = 0,
             int pcr_big = 0);
  game::Action get_move(game::GameState state, std::string *playout_log) override;
  game::Action get_move(game::GameState state) override;
  void reset() override;

private:
  float visit(const game::GameState &state);
  void apply_dirichlet_noise_(const game::GameState &state);
  std::shared_ptr<Evaluator> evaluator_;
  std::unordered_map<game::GameState, MCTSNode> map_;
  std::random_device rd_;
  std::mt19937 gen_;
  int playouts_;
  bool eval_mode_;
  bool use_pcr_;
  int pcr_small_;
  int pcr_big_;
};

#endif // ROOST_MCTSPLAYER_H
