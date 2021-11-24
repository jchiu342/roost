//
// Created by Jeremy on 11/24/2021.
//

#include "game/GameState.h"
#include <cassert>
#include <cstring>
#include <sstream>

namespace game {

GameState::GameState(float komi)
    : komi_(komi), turns_(0), turn_(BLACK), passes_(0), done_(false),
      winner_(EMPTY) {
  std::memset(boards_, 0, sizeof(boards_));
}

float GameState::score() {
  bool *white_reachable = new bool[BOARD_SIZE * BOARD_SIZE];
  bool *black_reachable = new bool[BOARD_SIZE * BOARD_SIZE];
  for (int x = 0; x < BOARD_SIZE; ++x) {
    for (int y = 0; y < BOARD_SIZE; ++y) {
      if (boards_[0][x * BOARD_SIZE + y] == BLACK) {
        dfs_score_(x, y, WHITE, black_reachable);
      } else if (boards_[0][x * BOARD_SIZE + y] == WHITE) {
        dfs_score_(x, y, BLACK, white_reachable);
      }
    }
  }
  float count = -komi_;
  for (int x = 0; x < BOARD_SIZE; ++x) {
    for (int y = 0; y < BOARD_SIZE; ++y) {
      if (white_reachable[x * BOARD_SIZE + y] &&
          !black_reachable[x * BOARD_SIZE + y])
        --count;
      else if (black_reachable[x * BOARD_SIZE + y] &&
               !white_reachable[x * BOARD_SIZE + y])
        ++count;
    }
  }
  return count;
}

bool GameState::is_legal_action(Action action) {
  if (turn_ != action.get_color() || done_) return false;
  if (action.get_type() == PASS || action.get_type() == RESIGN) return true;
  return is_legal_play_(action.get_x(), action.get_y(), action.get_color());
}

void GameState::move(Action action) {
  assert(is_legal_action(action));
  ++turns_;
  turn_ = (turn_ == BLACK ? WHITE : BLACK);
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
        if (game_score > 1e-8) winner_ = BLACK;
        else if (game_score < -1e-8) winner_ = WHITE;
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
      boards_[0][action.get_x() * BOARD_SIZE + action.get_y()] = action.get_color();
      // turn_ is already updated to the opposite color
      remove_dead_neighbors_(action.get_x(), action.get_y(), turn_);
      break;
    }
  }
  if (turns_ >= MAX_GAME_LENGTH) {
    done_ = true;
    float game_score = score();
    if (game_score > 1e-8) winner_ = BLACK;
    else if (game_score < -1e-8) winner_ = WHITE;
  }
}

// X = black, O = white, . = empty
std::string GameState::to_string() {
  std::stringstream stream;
  for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; ++i) {
    if (i % BOARD_SIZE == 0) stream << '\n';
    switch (boards_[0][i]) {
      case BLACK: {
        stream << 'X';
        break;
      }
      case WHITE: {
        stream << 'O';
        break;
      }
      case EMPTY: {
        stream << '.';
        break;
      }
    }
  }
  return stream.str();
}

bool GameState::is_legal_play_(int x, int y, Color c) {
  assert(0 <= x && x < BOARD_SIZE && 0 <= y && y < BOARD_SIZE);
  if (boards_[0][x * BOARD_SIZE + y] != EMPTY) return false;
  Color original_board[BOARD_SIZE * BOARD_SIZE];
  memcpy(original_board, boards_[0], sizeof(original_board));
  boards_[0][x * BOARD_SIZE + y] = c;
  remove_dead_neighbors_(x, y, (c == BLACK ? WHITE : BLACK));
  bool visited[BOARD_SIZE * BOARD_SIZE];
  memset(visited, false, sizeof(visited));
  std::set<int> chain;
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

void GameState::remove_dead_neighbors_(int x, int y, Color opposite_color) {
  for (const auto a : neighbors) {
    if (0 <= a[0] + x && a[0] + x < BOARD_SIZE && 0 <= a[1] + y &&
        a[1] + y < BOARD_SIZE) {
      int liberties = 0;
      bool visited[BOARD_SIZE * BOARD_SIZE];
      std::set<int> chain;
      dfs_liberties_(a[0] + x, a[1] + y, opposite_color, visited, &chain, &liberties);
      if (liberties == 0) {
        for (const int &coord : chain) {
          boards_[0][coord] = EMPTY;
        }
      }
    }
  }
}

void GameState::dfs_liberties_(int x, int y, Color c, bool *visited, std::set<int> *chain, int *liberties) {
  if (visited[x * BOARD_SIZE + y]) return;
  visited[x * BOARD_SIZE + y] = true;
  if (boards_[0][x * BOARD_SIZE + y] == EMPTY) {
    *liberties = *liberties + 1;
    return;
  }
  if (boards_[0][x * BOARD_SIZE + y] == c) {
    chain->insert(x * BOARD_SIZE + y);
    for (const auto a : neighbors) {
      if (0 <= a[0] + x && a[0] + x < BOARD_SIZE && 0 <= a[1] + y &&
          a[1] + y < BOARD_SIZE) {
        dfs_liberties_(a[0] + x, a[1] + y, c, visited, chain, liberties);
      }
    }
  }
}

void GameState::dfs_score_(int x, int y, Color opposite_color,
                           bool *reachable) {
  if (reachable[x * BOARD_SIZE + y]) {
    return;
  }
  reachable[x * BOARD_SIZE + y] = true;
  for (const auto a : neighbors) {
    if (0 <= a[0] + x && a[0] + x < BOARD_SIZE && 0 <= a[1] + y &&
        a[1] + y < BOARD_SIZE &&
        boards_[0][(a[0] + x) * BOARD_SIZE + a[1] + y] != opposite_color) {
      dfs_score_(a[0] + x, a[1] + y, opposite_color, reachable);
    }
  }
}

} // namespace game
