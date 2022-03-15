//
// Created by Jeremy on 12/11/2021.
//

#include "player/MCTSPlayer.h"
#include "player/MCTS_defs.h"
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>

MCTSPlayer::MCTSPlayer(std::shared_ptr<Evaluator> evaluator, int playouts,
                       bool eval_mode, bool use_pcr, int pcr_small, int pcr_big)
    : AbstractPlayer(), evaluator_(std::move(evaluator)), gen_(rd_()),
      playouts_(playouts), eval_mode_(eval_mode), use_pcr_(use_pcr),
      pcr_small_(pcr_small), pcr_big_(pcr_big), eval_time_(0) {}

game::Action MCTSPlayer::get_move(game::GameState state,
                                  std::string *playout_log) {
  if (state.done()) {
    throw std::logic_error("get_move called on finished game\n");
  }
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
    apply_dirichlet_noise_(state);
    for (int i = 1; i < playouts_; ++i) {
      visit(state);
    }
  }
  size_t state_hash = state.hash();
  if (playout_log != nullptr) {
    *playout_log = "C[";
    for (int legal_idx : *(state.get_legal_action_indexes())) {
      if (!map_.contains(state_hash)) {
        throw std::logic_error("map_ does not contain state");
      }
      if (map_[state_hash].N[legal_idx] > 0) {
        *playout_log += std::to_string(legal_idx) + ' ' +
                        std::to_string(map_[state_hash].N[legal_idx]) + ",";
      }
    }
    *playout_log += "]\n";
  }

  // not sure how randommness is achieved in AGZ for eval games, so I keep
  // temp = 1 for first 10 moves in eval games and first 20 moves in self-play
  // games (30 in 19x19 AGZ)
  if ((!eval_mode_ || state.get_num_turns() < TEMP_0_MOVE_NUM_VAL) &&
      state.get_num_turns() < TEMP_0_MOVE_NUM_TRAIN) {
    std::uniform_int_distribution<> dist(
        1, std::accumulate(map_[state_hash].N.begin(), map_[state_hash].N.end(), 0));
    int vis_num = dist(gen_);
    int counter = 0;
    for (int legal_idx : *(state.get_legal_action_indexes())) {
      counter += map_[state_hash].N[legal_idx];
      if (counter >= vis_num) {
        assert(0 <= legal_idx && legal_idx <= BOARD_SIZE * BOARD_SIZE + 1);
        return {state.get_turn(), legal_idx};
      }
    }
  }
  int best_action_idx = -1;
  int max_visits = -1;
  for (int legal_idx : *(state.get_legal_action_indexes())) {
    if (map_[state_hash].N[legal_idx] > max_visits) {
      best_action_idx = legal_idx;
      max_visits = map_[state_hash].N[legal_idx];
    }
  }
  if (best_action_idx < 0 || best_action_idx > BOARD_SIZE * BOARD_SIZE) {
    throw std::logic_error("invalid best action chosen");
  }
  /*std::erase_if(map_, [&state](const auto &item) {
    auto const &[map_state, info] = item;
    return map_state.get_num_turns() <= state.get_num_turns();
  });*/
  return {state.get_turn(), best_action_idx};
}

game::Action MCTSPlayer::get_move(game::GameState state) {
  return get_move(std::move(state), nullptr);
}

float MCTSPlayer::get_wr(game::GameState state) {
  return (map_[state.hash()].Qs + 1) / 2;
}

void MCTSPlayer::reset() { map_.clear(); }

double MCTSPlayer::get_eval_time() { return eval_time_; }

float MCTSPlayer::visit(const game::GameState &state) {
  if (state.done()) {
    return (state.winner() == game::BLACK ? 1.0f : -1.0f);
  }
  size_t state_hash = state.hash();
  // if never visited, expand
  if (!map_.contains(state_hash)) {
    // initialize everything to 0
    MCTSNode d;
    map_[state_hash] = d;
    map_[state_hash].N.assign(BOARD_SIZE * BOARD_SIZE + 1, 0);
    map_[state_hash].Q.assign(BOARD_SIZE * BOARD_SIZE + 1, 0.0f);

    auto start = std::chrono::system_clock::now();
    Evaluator::Evaluation eval = evaluator_->Evaluate(state);
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    eval_time_ += elapsed_seconds.count();

    // cache our policy and value
    map_[state_hash].P = eval.policy_;
    map_[state_hash].Ns = 1;
    map_[state_hash].Qs = eval.value_;
    assert(map_[state].P.size() == BOARD_SIZE * BOARD_SIZE + 1);

    return eval.value_;
  }
  // TODO: rewrite MCTS to handle transpositions
  float max_u = -100000000.0f;
  int best_action_idx = -1;
  // precompute sqrt(sum_a N(s, a)) term for all items
  float sqrt_term = std::sqrt(map_[state_hash].Ns);
  // calculate P(explored) term for FPU
  float c_fpu_term = 0.0;
  for (int legal_idx : *(state.get_legal_action_indexes())) {
    if (map_[state_hash].N[legal_idx] > 0) {
      c_fpu_term += map_[state_hash].P[legal_idx];
    }
  }
  // if we are white, we take -Q for all Q's since we try to minimize
  c_fpu_term =
      (state.get_turn() == game::BLACK ? map_[state_hash].Qs : -map_[state_hash].Qs) -
      MCTS_CFPU * std::sqrt(c_fpu_term);
  for (int legal_idx : *(state.get_legal_action_indexes())) {
    // u = Q(s, a) + cpuct * P(s, a) * sqrt(sum_a N(s, a)) / (1 + N(s, a))
    float u =
        (map_[state_hash].N[legal_idx] == 0
             ? c_fpu_term
             : (state.get_turn() == game::BLACK ? map_[state_hash].Q[legal_idx]
                                                : -map_[state_hash].Q[legal_idx])) +
        MCTS_CPUCT * map_[state_hash].P[legal_idx] * sqrt_term /
            (1 + map_[state_hash].N[legal_idx]);
    if (u > max_u) {
      max_u = u;
      best_action_idx = legal_idx;
    }
  }
  if (best_action_idx < 0) {
    throw std::logic_error("invalid mcts action");
  }
  game::GameState state_copy = state;
  state_copy.move(game::Action(state_copy.get_turn(), best_action_idx));
  size_t state_copy_hash = state_copy.hash();
  float result = visit(state_copy);
  map_[state_hash].Qs =
      ((map_[state_hash].Qs * map_[state_hash].Ns) + result) / (map_[state_hash].Ns + 1);
  ++(map_[state_hash].Ns);
  if (!state_copy.done()) {
    map_[state_hash].Q[best_action_idx] = map_[state_copy_hash].Qs;
    map_[state_hash].N[best_action_idx] = map_[state_copy_hash].Ns;
  } else {
    // we can't hash finished positions, so we just update manually
    map_[state_hash].Q[best_action_idx] = result;
    ++(map_[state_hash].N[best_action_idx]);
  }
  return result;
}

void MCTSPlayer::apply_dirichlet_noise_(const game::GameState &state) {
  if (state.done()) {
    return;
  }
  size_t state_hash = state.hash();
  const std::vector<int> legal_actions = *state.get_legal_action_indexes();
  size_t num_values = legal_actions.size();
  const float alpha = DIRICHLET_UNSCALED_ALPHA / num_values;
  // generate dirichlet-distributed vector
  std::gamma_distribution<float> d(alpha, 1);
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
  const float DIRICHLET_EPSILON =
      (eval_mode_) ? DIRICHLET_EPSILON_VAL : DIRICHLET_EPSILON_TRAIN;
  for (size_t i = 0; i < num_values; ++i) {
    assert(!std::isnan(map_[state].P[legal_actions[i]]));
    map_[state_hash].P[legal_actions[i]] =
        (1 - DIRICHLET_EPSILON) * map_[state_hash].P[legal_actions[i]] +
        DIRICHLET_EPSILON * values[i];
    // assert(!std::isnan(map_[state].P[legal_actions[i]]));
    if (std::isnan(map_[state_hash].P[legal_actions[i]])) {
      std::cout << "Dirichlet noise generated isnan policy\n";
      throw std::logic_error("Dirichlet noise generated isnan policy\n");
    }
  }
}
