from player import Player
from game import Game, Action
from math import sqrt
import numpy as np
from macros import *
from collections import defaultdict
import time
import random
import torch
from net import ConnectNet
# TODO: implement N(s, a), etc. as dictionaries to save on transpositions


def to_action(move, turn):
    if move == BOARD_SIZE * BOARD_SIZE:
        return Action(turn, PASS)
    return Action(turn, PLAY, move // BOARD_SIZE, move % BOARD_SIZE)


def from_action(action):
    if action.type == PASS:
        return BOARD_SIZE * BOARD_SIZE
    assert(action.type == PLAY)
    return action.x * BOARD_SIZE + action.y


# credit for general MCTS code goes to
# https://github.com/plkmo/AlphaZero_Connect4/blob/master/src/MCTS_c4.py
class UCTNode:
    def __init__(self, game, move, parent=None):
        self.game = game
        self.move = move
        self.is_expanded = False
        self.parent = parent
        self.children = {}
        self.child_priors = np.zeros([INPUT], dtype=np.float32)
        self.child_total_value = np.zeros([INPUT], dtype=np.float32)
        self.child_number_visits = np.zeros([INPUT], dtype=np.float32)
        self.action_idxes = self.game.action_idxs()

    @property
    def number_visits(self):
        return self.parent.child_number_visits[self.move]

    @number_visits.setter
    def number_visits(self, value):
        self.parent.child_number_visits[self.move] = value

    @property
    def total_value(self):
        return self.parent.child_total_value[self.move]

    @total_value.setter
    def total_value(self, value):
        self.parent.child_total_value[self.move] = value

    def child_Q(self):
        return self.child_total_value / (1 + self.child_number_visits)

    def child_U(self):
        return sqrt(self.number_visits) * (abs(self.child_priors) / (1 + self.child_number_visits))

    def best_child(self):
        # if no legal actions, we're in a terminal state and we should return
        if self.action_idxes:
            bestmove = self.child_Q() + self.child_U()
            bestmove = self.action_idxes[np.argmax(bestmove[self.action_idxes])]
        else:
            bestmove = None
        return bestmove

    def select_leaf(self):
        current = self
        while current.is_expanded:
            best_move = current.best_child()
            if best_move is None:
                return current
            current = current.maybe_add_child(best_move)
        return current

    def add_dirichlet_noise(self, action_idxs, child_priors):
        # # TODO: understand what is going on here
        valid_child_priors = child_priors[action_idxs]  # select only legal moves entries in child_priors array
        valid_child_priors = 0.75 * valid_child_priors + 0.25 * np.random.dirichlet(np.zeros([len(valid_child_priors)],
                                                                                             dtype=np.float32) + 192)
        child_priors[action_idxs] = valid_child_priors
        return child_priors

    def expand(self, child_priors):
        self.is_expanded = True
        action_idxs = self.game.action_idxs()
        c_p = child_priors
        if not action_idxs:
            self.is_expanded = False
        self.action_idxes = action_idxs
        for i in range(len(child_priors)):
            if i not in action_idxs:
                c_p[i] = 0.00000000
        # for some reason the below line doesn't work, probably different python version
        # c_p[[i for i in range(len(child_priors)) if i not in action_idxs]] = 0.000000000  # mask all illegal actions
        if self.parent.parent is None:  # add dirichlet noise to child_priors in root node
            c_p = self.add_dirichlet_noise(action_idxs, c_p)
        self.child_priors = c_p

    def maybe_add_child(self, move):
        if move not in self.children:
            copy_game = Game.from_other(self.game)
            copy_game.move(to_action(move, copy_game.turn))
            self.children[move] = UCTNode(
                copy_game, move, parent=self)
        return self.children[move]

    def backup(self, value_estimate: float):
        current = self
        while current.parent is not None:
            current.number_visits += 1
            if current.game.turn == BLACK:  # same as current.parent.game.player = 0
                current.total_value += (-1 * value_estimate)  # value estimate +1 = O wins
            elif current.game.turn == WHITE:  # same as current.parent.game.player = 1
                current.total_value += (1 * value_estimate)
            current = current.parent


class DummyNode:
    def __init__(self):
        self.parent = None
        self.child_total_value = defaultdict(float)
        self.child_number_visits = defaultdict(float)


class MCTSPlayer(Player):
    def __init__(self, board_size, color, evaluator, playouts=50):
        super().__init__(board_size, color)
        self.evaluator = evaluator
        self.playouts = playouts

    def _mcts(self, game):
        root = UCTNode(game, move=None, parent=DummyNode())
        for _ in range(self.playouts):
            leaf = root.select_leaf()
            child_priors, value_estimate = self.evaluator(leaf.game)
            if leaf.game.done:
                # terminal game state, no need to expand
                leaf.backup(value_estimate)
                continue
            leaf.expand(child_priors)
            leaf.backup(value_estimate)
        return root

    def get_move(self, game):
        node = self._mcts(game)
        # return most visited move
        # print(node.child_number_visits)
        move_idx = np.random.choice(np.flatnonzero(node.child_number_visits == node.child_number_visits.max()))
        # parse move_idx into an action
        return to_action(move_idx, self.color)
        # if move_idx == self.board_size * self.board_size:
        #     return Action(self.turn, PASS)
        # return Action(self.turn, PLAY, move_idx // 9, move_idx % 9)


def simple_mcts_evaluation(game):
    priors = np.full((INPUT), 1 / INPUT)
    # priors = [1 / len(game.action_idxs())] * len(game.action_idxs())
    if game.done:
        if game.winner == BLACK:
            return priors, 1
        elif game.winner == WHITE:
            return priors, -1
        return priors, 0
    return priors, game.score() / (game.board_size * game.board_size + game.komi)
