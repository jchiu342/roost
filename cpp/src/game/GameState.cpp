//
// Created by Jeremy on 11/24/2021.
//

#include "game/GameState.h"
#include <cassert>
#include <cstring>
#include <memory>

namespace game {

GameState::GameState(float komi, const std::shared_ptr<Zobrist> &zobrist)
    : turn_(BLACK), winner_(EMPTY), komi_(komi), turns_(0), passes_(0),
      done_(false) {
  if (zobrist == nullptr) {
    zobrist_ = std::make_shared<Zobrist>(BOARD_SIZE * BOARD_SIZE * 2 + 1);
  } else {
    zobrist_ = zobrist;
    assert(zobrist_->size() >= BOARD_SIZE * BOARD_SIZE * 2 + 1);
  }
  std::memset(boards_, 0, sizeof(boards_));
  for (int i = 0; i < BOARD_SIZE * BOARD_SIZE + 1; ++i) {
    legal_action_idxes_.push_back(i);
  }
  // black moves first; all other features are off on the empty board
  hash_ = zobrist_->get_value(BOARD_SIZE * BOARD_SIZE * 2);
}

Color GameState::get_turn() const {
  assert(!done_);
  return turn_;
}

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
      if (white_reachable[x][y] &&
          !black_reachable[x][y])
        --count;
      else if (black_reachable[x][y] &&
               !white_reachable[x][y])
        ++count;
    }
  }
  return count;
}

bool GameState::is_legal_action(Action action) const {
  // return (turn_ != action.get_color() || done_)
  if (turn_ != action.get_color() || done_)
    return false;
  return std::find(legal_action_idxes_.begin(), legal_action_idxes_.end(),
                   action.get_index()) != legal_action_idxes_.end();
  /* if (action.get_type() == PASS || action.get_type() == RESIGN)
    return true;
  return is_legal_play_(action.get_x(), action.get_y(), action.get_color()); */
}

std::vector<int> GameState::get_legal_action_indexes() const {
  // action indexes go from 0 to BOARD_SIZE * BOARD_SIZE + 1
  // does NOT include resign, as stated in the header
  if (done_) {
    return {};
  }
  return legal_action_idxes_;
  /* std::vector<int> ret_val;
  for (int i = 0; i < BOARD_SIZE * BOARD_SIZE + 1; ++i) {
    if (is_legal_action(Action(turn_, i))) {
      ret_val.push_back(i);
    }
  }
  return ret_val; */
}

void GameState::move(Action action) {
  assert(is_legal_action(action));
  ++turns_;
  turn_ = (turn_ == BLACK ? WHITE : BLACK);
  hash_ ^= zobrist_->get_value(BOARD_SIZE * BOARD_SIZE * 2);
  switch (action.get_type()) {
  case RESIGN: {
    done_ = true;
    winner_ = turn_;
    return;
  }
  case PASS: {
    ++passes_;

    if (passes_ >= 3) {
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
    // update current board
    boards_[0][action.get_x()][action.get_y()] =
        action.get_color();
    // turn_ is already updated to the opposite color
    hash_ ^= zobrist_->get_value(
        (action.get_color() == BLACK ? 0 : BOARD_SIZE * BOARD_SIZE) +
        (action.get_x() * BOARD_SIZE + action.get_y()));
    remove_dead_neighbors_(action.get_x(), action.get_y(), turn_, true);
    break;
  }
  }
  // calculate all legal actions
  legal_action_idxes_.clear();
  // get liberties of all groups on the board
  /*int liberties[BOARD_SIZE * BOARD_SIZE];
  for (int x = 0; x < BOARD_SIZE * BOARD_SIZE; ++i) {

  }*/
  for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; ++i) {
    Action a(turn_, i);
    if (is_legal_play_(a.get_x(), a.get_y(), turn_)) {
      legal_action_idxes_.push_back(i);
    }
  }
  legal_action_idxes_.push_back(BOARD_SIZE * BOARD_SIZE);
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

bool GameState::is_legal_play_(int x, int y, Color c) {
  assert(0 <= x && x < BOARD_SIZE && 0 <= y && y < BOARD_SIZE);
  if (boards_[0][x][y] != EMPTY)
    return false;
  Color original_board[BOARD_SIZE][BOARD_SIZE];
  memcpy(original_board, boards_[0], sizeof(original_board));
  boards_[0][x][y] = c;
  remove_dead_neighbors_(x, y, (c == BLACK ? WHITE : BLACK), false);
  bool visited[BOARD_SIZE][BOARD_SIZE];
  memset(visited, false, sizeof(visited));
  std::set<std::pair<int, int>> chain;
  int liberties = 0;
  dfs_liberties_(x, y, c, visited, &chain, &liberties);
  // remove chain size requirement for normal go.
  if (liberties == 0 || chain.size() == 4) {
    memcpy(boards_[0], original_board, sizeof(original_board));
    return false;
  }
  // check if new state identical to previous game states (ko rules)
  for (int i = 1; i < GAME_HISTORY_LEN; i += 2) {
    if (memcmp(boards_[0], boards_[i], sizeof(original_board)) == 0) {
      memcpy(boards_[0], original_board, sizeof(original_board));
      return false;
    }
  }
  memcpy(boards_[0], original_board, sizeof(original_board));
  return true;
}

void GameState::remove_dead_neighbors_(int x, int y, Color opposite_color,
                                       bool permanent) {
  for (const auto a : neighbors) {
    if (0 <= a[0] + x && a[0] + x < BOARD_SIZE && 0 <= a[1] + y &&
        a[1] + y < BOARD_SIZE) {
      int liberties = 0;
      bool visited[BOARD_SIZE][BOARD_SIZE];
      memset(visited, false, sizeof(visited));
      std::set<std::pair<int, int>> chain;
      dfs_liberties_(a[0] + x, a[1] + y, opposite_color, visited, &chain,
                     &liberties);
      if (liberties == 0) {
        for (const std::pair<int, int> &coord : chain) {
          if (permanent) {
            hash_ ^= zobrist_->get_value(
                (boards_[0][coord.first][coord.second] == BLACK ? 0 : BOARD_SIZE * BOARD_SIZE) +
                coord.first * BOARD_SIZE + coord.second);
          }
          boards_[0][coord.first][coord.second] = EMPTY;
        }
      }
    }
  }
}

void GameState::dfs_liberties_(int x, int y, Color c, bool visited[][BOARD_SIZE],
                               std::set<std::pair<int, int>> *chain, int *liberties) const {
  if (visited[x][y])
    return;
  visited[x][y] = true;
  if (boards_[0][x][y] == EMPTY) {
    *liberties = *liberties + 1;
    return;
  }
  if (boards_[0][x][y] == c) {
    chain->insert({x, y});
    for (const auto a : neighbors) {
      if (0 <= a[0] + x && a[0] + x < BOARD_SIZE && 0 <= a[1] + y &&
          a[1] + y < BOARD_SIZE) {
        dfs_liberties_(a[0] + x, a[1] + y, c, visited, chain, liberties);
      }
    }
  }
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

/*int GameState::operator==(const GameState &other) {
  return 0;
}*/

} // namespace game
