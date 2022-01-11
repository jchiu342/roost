//
// Created by Jeremy on 12/11/2021.
//

#include "player/MCTSPlayer.h"
#include "player/MCTS_defs.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>

MCTSPlayer::MCTSPlayer(game::Color c, std::shared_ptr<Evaluator> evaluator,
                       int playouts, bool eval_mode, bool use_pcr,
                       int pcr_small, int pcr_big)
    : AbstractPlayer(c), evaluator_(std::move(evaluator)), gen_(rd_()),
      playouts_(playouts), eval_mode_(eval_mode), use_pcr_(use_pcr),
      pcr_small_(pcr_small), pcr_big_(pcr_big) {}

game::Action MCTSPlayer::get_move(game::GameState state) {
  assert(state.get_turn() == color_);
  visit(state);
  if (!eval_mode_ && use_pcr_) {
    std::uniform_real_distribution<float> dist(0.0, 1.0);
    if (dist(gen_) < PCR_P) {
      // perform a full search
      apply_dirichlet_noise_(state);
      for (int i = 1; i < pcr_big_; ++i) {
        visit(state);
      }
    } else {
      // quick search; no dirichlet noise
      for (int i = 1; i < pcr_small_; ++i) {
        visit(state);
      }
    }
  } else {
    if (!eval_mode_) {
      apply_dirichlet_noise_(state);
    }
    for (int i = 1; i < playouts_; ++i) {
      visit(state);
    }
  }
  // not sure how randommness is achieved in AGZ for eval games, so I keep
  // temp = 1 for first 10 moves in eval games and first 20 moves in self-play
  // games (30 in 19x19 AGZ)
  if ((!eval_mode_ || state.get_num_turns() < TEMP_0_MOVE_NUM_VAL) &&
      state.get_num_turns() < TEMP_0_MOVE_NUM_TRAIN) {
    std::uniform_int_distribution<> dist(1, playouts_);
    int vis_num = dist(gen_);
    int counter = 0;
    for (int legal_idx : *(state.get_legal_action_indexes())) {
      counter += map_[state].N[legal_idx];
      if (counter >= vis_num) {
        assert(0 <= legal_idx && legal_idx <= BOARD_SIZE * BOARD_SIZE + 1);
        return {color_, legal_idx};
      }
    }
  }
  int best_action_idx = -1;
  int max_visits = -1;
  for (int legal_idx : *(state.get_legal_action_indexes())) {
    if (map_[state].N[legal_idx] > max_visits) {
      best_action_idx = legal_idx;
      max_visits = map_[state].N[legal_idx];
    }
  }
  assert(0 <= best_action_idx &&
         best_action_idx <= BOARD_SIZE * BOARD_SIZE + 1);
  std::erase_if(map_, [&state](const auto &item) {
    auto const& [map_state, info] = item;
    return map_state.get_num_turns() <= state.get_num_turns();
  });
  return {color_, best_action_idx};
}

void MCTSPlayer::reset() { map_.clear(); }

float MCTSPlayer::visit(const game::GameState &state) {
  if (state.done()) {
    return (state.winner() == game::BLACK ? 1.0f : -1.0f);
  }
  // if never visited, expand
  if (!map_.contains(state)) {
    // initialize everything to 0
    map_[state].N.assign(BOARD_SIZE * BOARD_SIZE + 1, 0);
    map_[state].Q.assign(BOARD_SIZE * BOARD_SIZE + 1, 0.0f);
    Evaluator::Evaluation eval = evaluator_->Evaluate(state);
    // cache our policy and value
    map_[state].P = eval.policy_;
    assert(map_[state].P.size() == BOARD_SIZE * BOARD_SIZE + 1);
    return eval.value_;
  }
  // std::cout << "map contains state already" << std::endl;
  // get best child and visit it
  // equivalent to neginf
  float max_u = -100000000.0f;
  int best_action_idx = -1;
  for (int legal_idx : *(state.get_legal_action_indexes())) {
    // u = Q(s, a) + cpuct * P(s, a) * sqrt(sum_a N(s, a)) / (1 + N(s, a))
    // TODO: correct this math. currently this does sum_a N(s, a) + 1 to explore
    // the max-policy action on the first playout, but this doesn't seem to be
    // talked about anywhere
    float u = map_[state].Q[legal_idx] +
              MCTS_CPUCT * map_[state].P[legal_idx] *
                  sqrt(std::accumulate(map_[state].N.begin(),
                                       map_[state].N.end(), 1)) /
                  (1 + map_[state].N[legal_idx]);
    if (u > max_u) {
      max_u = u;
      best_action_idx = legal_idx;
    }
  }
  game::GameState state_copy = state;
  state_copy.move(game::Action(state.get_turn(), best_action_idx));
  float v = visit(state_copy);
  // if we're white, we want to store -v, since we're trying to minimize the
  // score Q(s, a) = (N(s, a)*Q(s, a) + v) / (N(s, a) + 1)
  map_[state].Q[best_action_idx] =
      (static_cast<float>(map_[state].N[best_action_idx]) *
           map_[state].Q[best_action_idx] +
       (state.get_turn() == game::BLACK ? v : -v)) /
      static_cast<float>(map_[state].N[best_action_idx] + 1);
  ++map_[state].N[best_action_idx];
  return v;
}

void MCTSPlayer::apply_dirichlet_noise_(const game::GameState &state) {
  if (state.done()) {
    return;
  }
  const std::vector<int> legal_actions = *state.get_legal_action_indexes();
  size_t num_values = legal_actions.size();
  // generate dirichlet-distributed vector
  std::gamma_distribution<float> d(DIRICHLET_ALPHA, 1);
  std::vector<float> values;
  // small number to prevent div by 0 errors
  float sum = 1e-8;
  for (size_t i = 0; i < num_values; ++i) {
    values.push_back(d(gen_));
    assert(!std::isnan(values[i]));
    sum += values[i];
  }
  for (size_t i = 0; i < num_values; ++i) {
    values[i] /= sum;
    assert(!std::isnan(values[i]));
  }
  // apply dirichlet to each P(s, a)
  for (size_t i = 0; i < num_values; ++i) {
    assert(!std::isnan(map_[state].P[legal_actions[i]]));
    map_[state].P[legal_actions[i]] =
        (1 - DIRICHLET_EPSILON) * map_[state].P[legal_actions[i]] +
        DIRICHLET_EPSILON * values[i];
    assert(!std::isnan(map_[state].P[legal_actions[i]]));
  }
}
