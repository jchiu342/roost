//
// Created by Jeremy on 12/11/2021.
//

#include "player/MCTSPlayer.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>

MCTSPlayer::MCTSPlayer(game::Color c, std::unique_ptr<Evaluator> &&evaluator,
                       float cpuct, int playouts)
    : AbstractPlayer(c), evaluator_(std::move(evaluator)), cpuct_(cpuct),
      playouts_(playouts), use_t1_(true) {}

game::Action MCTSPlayer::get_move(game::GameState state) {
  assert(state.get_turn() == color_);
  visit(state);
  // apply dirichlet to root node
  apply_dirichlet_noise_(state);
  for (int i = 1; i < playouts_; ++i) {
    visit(state);
  }
  // for (int i = 0; i < map_[state].N.size(); ++i) {
  //   std::cout << i << ' ' << map_[state].N[i] << "; ";
  // }
  // std::cout << std::endl;
  // if temperature = 1, we simply choose move proportional to # of visits
  if (use_t1_) {
    std::uniform_int_distribution<> dist(1, playouts_);
    int vis_num = dist(gen_);
    int counter = 0;
    for (int legal_idx : state.get_legal_action_indexes()) {
      counter += map_[state].N[legal_idx];
      if (counter >= vis_num) {
        return {color_, legal_idx};
      }
    }
  }
  int best_action_idx = -1;
  int max_visits = -1;
  for (int legal_idx : state.get_legal_action_indexes()) {
    if (map_[state].N[legal_idx] > max_visits) {
      best_action_idx = legal_idx;
      max_visits = map_[state].N[legal_idx];
    }
  }
  return {color_, best_action_idx};
  // std::vector<int> legal_move_indexes = state.get_legal_action_indexes();
  // return {color_, legal_move_indexes[0]};
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
    Evaluator::Evaluation eval = evaluator_->evaluate(state);
    // cache our policy and value
    map_[state].P = eval.policy_;
    // we want to return negative value bc other player is trying to minimize
    // our score
    return eval.value_;
  }

  // get best child and visit it
  // equivalent to neginf
  float max_u = -100000000.0f;
  int best_action_idx = -1;
  for (int legal_idx : state.get_legal_action_indexes()) {
    // u = Q(s, a) + cpuct * P(s, a) * sqrt(sum_a N(s, a)) / (1 + N(s, a))
    float u = map_[state].Q[legal_idx] +
              cpuct_ * map_[state].P[legal_idx] *
                  sqrt(std::accumulate(map_[state].N.begin(),
                                       map_[state].N.end(), 0)) /
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
  const std::vector<int> legal_actions = state.get_legal_action_indexes();
  size_t num_values = legal_actions.size();
  // generate dirichlet-distributed vector
  std::gamma_distribution<float> d(alpha_, 1);
  std::vector<float> values;
  float sum = 0.0;
  for (size_t i = 0; i < num_values; ++i) {
    values.push_back(d(gen_));
    sum += values[i];
  }
  for (size_t i = 0; i < num_values; ++i) {
    values[i] /= sum;
  }
  // apply dirichlet to each P(s, a)
  for (size_t i = 0; i < num_values; ++i) {
    map_[state].P[legal_actions[i]] =
        (1 - epsilon_) * map_[state].P[legal_actions[i]] + epsilon_ * values[i];
  }
}
