//
// Created by Jeremy on 11/24/2021.
//

#include <gtest/gtest.h>
#include "game/Action.h"
#include "game/GameState.h"

using namespace game;

// Demonstrate ko logic
TEST(GameTest, DISABLED_GameStateLogicTest) {
  GameState state(7.5);
  state.move(Action(Color::BLACK, ActionType::PLAY, 0, 2));
  state.move(Action(Color::WHITE, ActionType::PLAY, 0, 1));
  state.move(Action(Color::BLACK, ActionType::PLAY, 0, 0));
  state.move(Action(Color::WHITE, ActionType::PLAY, 1, 0));
  state.move(Action(Color::BLACK, ActionType::PLAY, 1, 1));
  state.move(Action(Color::WHITE, ActionType::PLAY, 8, 8));
  state.move(Action(Color::BLACK, ActionType::PLAY, 0, 0));
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