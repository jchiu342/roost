//
// Created by Jeremy on 12/24/2021.
//

#include "utils/Zobrist.h"
#include <random>

Zobrist::Zobrist(int features) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<size_t> distribution(1, SIZE_MAX);
  for (int i = 0; i < features; ++i) {
    values_.push_back(distribution(gen));
  }
}

size_t Zobrist::size() const { return values_.size(); }

size_t Zobrist::get_value(size_t index) const { return values_[index]; }

std::vector<size_t> Zobrist::get_values() const { return values_; }
