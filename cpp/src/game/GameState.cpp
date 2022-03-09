//
// Created by Jeremy on 11/24/2021.
//

#include "game/GameState.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace game {

GameState::GameState(float komi, const std::shared_ptr<Zobrist> &&zobrist)
    : turn_(BLACK), winner_(EMPTY), komi_(komi), turns_(0), passes_(0),
      done_(false) {
  if (zobrist == nullptr) {
    zobrist_ = std::make_shared<Zobrist>(BOARD_SIZE * BOARD_SIZE * 2 + 1);
  } else {
    zobrist_ = zobrist;
    if (zobrist_->size() < BOARD_SIZE * BOARD_SIZE * 2 + 1) {
      throw std::logic_error("zobrist vector too small");
    }
    // assert(zobrist_->size() >= BOARD_SIZE * BOARD_SIZE * 2 + 1);
  }
  std::memset(boards_, 0, sizeof(boards_));
  std::memset(liberties_, 0, sizeof(liberties_));
  for (int i = 0; i < BOARD_SIZE * BOARD_SIZE + 1; ++i) {
    legal_action_idxes_.push_back(i);
  }
  for (auto &uf_chain : uf_chains_) {
    for (auto &y : uf_chain) {
      y = std::make_pair(-1, -1);
    }
  }
  // black moves first; all other features are off on the empty board
  hash_ = zobrist_->get_value(BOARD_SIZE * BOARD_SIZE * 2);
}

Color GameState::get_turn() const {
  if (done_) {
    throw std::logic_error("get_turn called on finished game");
  }
  return turn_;
}

int GameState::get_num_turns() const { return turns_; }

const Color *GameState::get_board(int i) const { return &boards_[i][0][0]; }

float GameState::get_komi() const { return komi_; }

bool GameState::done() const { return done_; }

Color GameState::winner() const { return winner_; }

float GameState::score() const {
  bool white_reachable[BOARD_SIZE][BOARD_SIZE];
  bool black_reachable[BOARD_SIZE][BOARD_SIZE];
  memset(white_reachable, false, sizeof(white_reachable));
  memset(black_reachable, false, sizeof(black_reachable));
  for (int x = 0; x < BOARD_SIZE; ++x) {
    for (int y = 0; y < BOARD_SIZE; ++y) {
      if (boards_[0][x][y] == BLACK) {
        dfs_score_(x, y, WHITE, black_reachable);
      } else if (boards_[0][x][y] == WHITE) {
        dfs_score_(x, y, BLACK, white_reachable);
      }
    }
  }
  float count = -komi_;
  for (int x = 0; x < BOARD_SIZE; ++x) {
    for (int y = 0; y < BOARD_SIZE; ++y) {
      if (white_reachable[x][y] && !black_reachable[x][y])
        --count;
      else if (black_reachable[x][y] && !white_reachable[x][y])
        ++count;
    }
  }
  return count;
}

bool GameState::is_legal_action(Action action) const {
  // return (turn_ != action.get_color() || done_)
  if (turn_ != action.get_color() || done_) {
    throw std::logic_error("is_legal_action called on wrong color or finished game");
  }
  if (action.get_type() == RESIGN) {
    return true;
  }
  return std::find(legal_action_idxes_.begin(), legal_action_idxes_.end(),
                   action.get_index()) != legal_action_idxes_.end();
}

const std::vector<int> *GameState::get_legal_action_indexes() const {
  // action indexes go from 0 to BOARD_SIZE * BOARD_SIZE + 1
  // does NOT include resign, as stated in the header
  if (done_) {
    throw std::logic_error("get_legal_action called when game finished\n");
  }
  return &legal_action_idxes_;
}

void GameState::move(Action action) {
  if (!is_legal_action(action)) {
    throw std::invalid_argument("illegal action played");
  }
  ++turns_;
  turn_ = opposite(turn_);
  hash_ ^= zobrist_->get_value(BOARD_SIZE * BOARD_SIZE * 2);
  switch (action.get_type()) {
  case RESIGN: {
    done_ = true;
    winner_ = turn_;
    return;
  }
  case PASS: {
    ++passes_;
    if (passes_ >= 2) {
      done_ = true;
      float game_score = score();
      if (game_score > 1e-8)
        winner_ = BLACK;
      else if (game_score < -1e-8)
        winner_ = WHITE;
      return;
    }
    break;
  }
  case PLAY: {
    passes_ = 0;
    // shift all game history by 1
    for (int i = 7; i > 0; --i) {
      memcpy(boards_[i], boards_[i - 1], sizeof(boards_[i]));
    }
    // update current board:
    // 1. place current stone + update hash + current stone's UF chain
    // 2. remove dead neighbors (if any) and update hash
    // 3. if capture, recount all liberties of our color's groups
    // 4. if not capture, recount liberties of placed stone's group
    int x = action.get_x();
    int y = action.get_y();
    // 1
    boards_[0][x][y] = action.get_color();
    // turn_ is already updated to the opposite color
    hash_ ^= zobrist_->get_value(
        (action.get_color() == BLACK ? 0 : BOARD_SIZE * BOARD_SIZE) +
        (x * BOARD_SIZE + y));
    uf_make_(x, y);
    bool capture = false;
    // 2
    std::set<std::pair<int, int>> neighbor_set;
    for (const auto a : neighbors) {
      if (0 <= a[0] + x && a[0] + x < BOARD_SIZE && 0 <= a[1] + y &&
          a[1] + y < BOARD_SIZE) {
        if (boards_[0][a[0] + x][a[1] + y] == action.get_color()) {
          uf_union_(x, y, a[0] + x, a[1] + y);
        } else if (boards_[0][a[0] + x][a[1] + y] ==
                   opposite(action.get_color())) {
          std::pair<int, int> head = uf_find_(a[0] + x, a[1] + y);
          if (!neighbor_set.contains(head) &&
              --(liberties_[head.first][head.second]) == 0) {
            // remove dead neighbors
            for (const std::pair<int, int> &stone :
                 chain_lists_[head.first][head.second]) {
              hash_ ^= zobrist_->get_value(
                  (boards_[0][stone.first][stone.second] == BLACK
                       ? 0
                       : BOARD_SIZE * BOARD_SIZE) +
                  (x * BOARD_SIZE + y));
              boards_[0][stone.first][stone.second] = EMPTY;
            }
            capture = true;
            uf_delete_(head.first, head.second);
            // liberties are already at 0, no need to update them
          }
          // if neighbors are part of same group, we don't want to subtract more
          // than 1 liberty
          neighbor_set.insert(head);
        }
      }
    }
    // 3
    if (capture) {
      for (int x0 = 0; x0 < BOARD_SIZE; ++x0) {
        for (int y0 = 0; y0 < BOARD_SIZE; ++y0) {
          if (boards_[0][x0][y0] == action.get_color() &&
              !chain_lists_[x0][y0].empty()) {
            update_liberties_at_head_(x0, y0);
          }
        }
      }
    } else {
      // 4
      std::pair<int, int> head = uf_find_(x, y);
      update_liberties_at_head_(head.first, head.second);
    }
    break;
  }
  }

  // calculate all legal actions
  legal_action_idxes_.clear();
  Color original_board[BOARD_SIZE][BOARD_SIZE];
  memcpy(original_board, boards_[0], sizeof(original_board));
  bool dirty_board = false;
  for (int x = 0; x < BOARD_SIZE; ++x) {
    for (int y = 0; y < BOARD_SIZE; ++y) {
      // reset to original board
      if (dirty_board) {
        memcpy(boards_[0], original_board, sizeof(original_board));
        dirty_board = false;
      }
      // action is legal if:
      // 1. empty space
      // 2. no chain of 4 stones
      // 3. if a capture, it does not repeat previous game state
      // 4. if not a capture, it cannot be a suicide
      // 1
      if (boards_[0][x][y] != EMPTY) {
        continue;
      }
      // 2
      // ext_lib is for checking 4) later
      bool ext_lib = false;
      std::set<std::pair<int, int>> chain_heads;
      for (const auto a : neighbors) {
        if (0 <= a[0] + x && a[0] + x < BOARD_SIZE && 0 <= a[1] + y &&
            a[1] + y < BOARD_SIZE) {
          if (boards_[0][a[0] + x][a[1] + y] == turn_) {
            chain_heads.insert(uf_find_(a[0] + x, a[1] + y));
          } else if (boards_[0][a[0] + x][a[1] + y] == EMPTY) {
            ext_lib = true;
          }
        }
      }
      size_t stone_count = 0;
      for (const std::pair<int, int> &head : chain_heads) {
        stone_count += chain_lists_[head.first][head.second].size();
      }
      // adding a new stone to this would make a 4-chain
      if (stone_count == 3) {
        continue;
      }
      // 3
      bool capture = false;
      Color opposite_c = opposite(turn_);
      for (const auto a : neighbors) {
        if (0 <= a[0] + x && a[0] + x < BOARD_SIZE && 0 <= a[1] + y &&
            a[1] + y < BOARD_SIZE &&
            boards_[0][a[0] + x][a[1] + y] == opposite_c) {
          std::pair<int, int> head = uf_find_(a[0] + x, a[1] + y);
          if (liberties_[head.first][head.second] == 1) {
            capture = true;
            dirty_board = true;
            // only temporarily removing from the board
            for (const std::pair<int, int> &stone :
                 chain_lists_[head.first][head.second]) {
              boards_[0][stone.first][stone.second] = EMPTY;
            }
          }
        }
      }
      if (capture) {
        // place the actual stone
        boards_[0][x][y] = turn_;
        bool kill = false;
        for (int i = 1; i < GAME_HISTORY_LEN; i += 2) {
          if (memcmp(boards_[0], boards_[i], sizeof(boards_[0])) == 0) {
            kill = true;
            break;
          }
        }
        if (kill) {
          continue;
        }
      } else if (!ext_lib) {
        for (const std::pair<int, int> &head : chain_heads) {
          assert(liberties_[head.first][head.second] >= 1);
          if (liberties_[head.first][head.second] > 1) {
            legal_action_idxes_.push_back(x * BOARD_SIZE + y);
            break;
          }
        }
        continue;
      }
      legal_action_idxes_.push_back(x * BOARD_SIZE + y);
    }
  }
  legal_action_idxes_.push_back(BOARD_SIZE * BOARD_SIZE);
  // restore original board
  if (dirty_board) {
    memcpy(boards_[0], original_board, sizeof(original_board));
  }
  if (turns_ >= MAX_GAME_LENGTH) {
    done_ = true;
    float game_score = score();
    if (game_score > 1e-8)
      winner_ = BLACK;
    else if (game_score < -1e-8)
      winner_ = WHITE;
  }
}

size_t GameState::hash() const { return hash_; }

// X = black, O = white, . = empty
std::string GameState::to_string() const {
  std::string str;
  for (int x = 0; x < BOARD_SIZE; ++x) {
    str += '\n';
    for (int y = 0; y < BOARD_SIZE; ++y) {
      switch (boards_[0][x][y]) {
      case BLACK: {
        str += 'X';
        break;
      }
      case WHITE: {
        str += 'O';
        break;
      }
      case EMPTY: {
        str += '.';
        break;
      }
      }
    }
  }
  return str;
}

void GameState::dfs_score_(int x, int y, Color opposite_color,
                           bool reachable[][BOARD_SIZE]) const {
  if (reachable[x][y]) {
    return;
  }
  reachable[x][y] = true;
  for (const auto a : neighbors) {
    if (0 <= a[0] + x && a[0] + x < BOARD_SIZE && 0 <= a[1] + y &&
        a[1] + y < BOARD_SIZE &&
        boards_[0][(a[0] + x)][a[1] + y] != opposite_color) {
      dfs_score_(a[0] + x, a[1] + y, opposite_color, reachable);
    }
  }
}

void GameState::update_liberties_at_head_(int x, int y) {
  assert(uf_find_(x, y) == std::make_pair(x, y));
  std::set<std::pair<int, int>> lib_set;
  for (const std::pair<int, int> &stone : chain_lists_[x][y]) {
    int x0 = stone.first;
    int y0 = stone.second;
    for (const auto a : neighbors) {
      if (0 <= a[0] + x0 && a[0] + x0 < BOARD_SIZE && 0 <= a[1] + y0 &&
          a[1] + y0 < BOARD_SIZE && boards_[0][a[0] + x0][a[1] + y0] == EMPTY) {
        lib_set.insert({a[0] + x0, a[1] + y0});
      }
    }
  }
  liberties_[x][y] = static_cast<int>(lib_set.size());
}

void GameState::uf_make_(int x, int y) {
  uf_chains_[x][y].first = x;
  uf_chains_[x][y].second = y;
  chain_lists_[x][y].emplace(std::make_pair(x, y));
}

std::pair<int, int> GameState::uf_find_(int x, int y) {
  int parent_x = uf_chains_[x][y].first;
  int parent_y = uf_chains_[x][y].second;
  // path halving
  while (parent_x != x || parent_y != y) {
    uf_chains_[x][y] = uf_chains_[parent_x][parent_y];
    x = parent_x;
    y = parent_y;
    parent_x = uf_chains_[x][y].first;
    parent_y = uf_chains_[x][y].second;
  }
  return {x, y};
}

void GameState::uf_union_(int x1, int y1, int x2, int y2) {
  std::pair<int, int> head_1 = uf_find_(x1, y1);
  std::pair<int, int> head_2 = uf_find_(x2, y2);
  if (head_1 == head_2) {
    // no merging needed here
    return;
  }
  // larger chain retains its head
  if (chain_lists_[head_1.first][head_1.second].size() >=
      chain_lists_[head_2.first][head_2.second].size()) {
    uf_chains_[head_2.first][head_2.second] = head_1;
    chain_lists_[head_1.first][head_1.second].insert(
        chain_lists_[head_2.first][head_2.second].begin(),
        chain_lists_[head_2.first][head_2.second].end());
    chain_lists_[head_2.first][head_2.second].clear();
    liberties_[head_2.first][head_2.second] = 0;
  } else {
    uf_chains_[head_1.first][head_1.second] = head_2;
    chain_lists_[head_2.first][head_2.second].insert(
        chain_lists_[head_1.first][head_1.second].begin(),
        chain_lists_[head_1.first][head_1.second].end());
    chain_lists_[head_1.first][head_1.second].clear();
    liberties_[head_1.first][head_1.second] = 0;
  }
}

void GameState::uf_delete_(int x1, int y1) {
  std::pair<int, int> head = uf_find_(x1, y1);
  for (const std::pair<int, int> &x : chain_lists_[head.first][head.second]) {
    uf_chains_[x.first][x.second] = {-1, -1};
  }
  chain_lists_[head.first][head.second].clear();
}

} // namespace game
