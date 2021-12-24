//
// Created by Jeremy on 12/24/2021.
//

#ifndef ROOST_ZOBRIST_H
#define ROOST_ZOBRIST_H
#include <vector>

/* Zobrist for GameState uses BOARD_SIZE * BOARD_SIZE * 2 + 1 features. These
 * correspond to black stones on the current board, white stones, and whether
 * black is to move */

class Zobrist {
public:
  explicit Zobrist(int features);
  size_t size() const;
  size_t get_value(size_t index) const;
  [[nodiscard]] std::vector<size_t> get_values() const;

private:
  std::vector<size_t> values_;
};

#endif // ROOST_ZOBRIST_H
