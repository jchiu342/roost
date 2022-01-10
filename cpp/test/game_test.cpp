//
// Created by Jeremy on 11/24/2021.
//

#include <gtest/gtest.h>
#include "game/Action.h"
#include "game/GameState.h"
#include <iostream>

using namespace game;

// Demonstrate ko logic
TEST(GameTest, GameStateLogicTest) {
  GameState state(7.5);
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 0, 2));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PLAY, 0, 1));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 0, 0));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PLAY, 1, 0));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 1, 1));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PLAY, 8, 8));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 0, 0));
  std::cout << state.to_string() << std::endl;
  EXPECT_EQ(false, state.is_legal_action(Action(Color::WHITE, ActionType::PLAY, 1, 0)));
  state.move(Action(Color::WHITE, ActionType::PASS));
  state.move(Action(Color::BLACK, ActionType::PASS));
  state.move(Action(Color::WHITE, ActionType::PLAY, 5, 5));
  state.move(Action(Color::BLACK, ActionType::PLAY, 8, 0));
  state.move(Action(Color::WHITE, ActionType::PLAY, 0, 1));
  state.move(Action(Color::BLACK, ActionType::PASS));
  state.move(Action(Color::WHITE, ActionType::PLAY, 0, 0));
  state.move(Action(Color::BLACK, ActionType::PLAY, 2, 0));
  EXPECT_EQ(state.score(), -2.5);
}

TEST(GameTest, GameStateLogicTest2) {
GameState state(7.5);
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 0, 0));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PLAY, 0, 1));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 1, 1));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PLAY, 0, 2));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 1, 2));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PLAY, 1, 0));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 0, 3));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PASS));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 0, 0));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PLAY, 0, 1));
  std::cout << state.to_string() << std::endl;
  EXPECT_EQ(state.score(), -7.5);
}

TEST(GameTest, GameStateLogicTest3) {
  GameState state(7.5);
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 6, 2));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PASS));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 5, 2));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PASS));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 8, 0));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PASS));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 8, 1));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PASS));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 7, 1));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PASS));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::BLACK, ActionType::PLAY, 6, 1));
  std::cout << state.to_string() << std::endl;
  state.move(Action(Color::WHITE, ActionType::PLAY, 0, 1));
  std::cout << state.to_string() << std::endl;
  // EXPECT_EQ(state.score(), -7.5);
}