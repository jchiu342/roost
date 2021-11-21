from player import Player
from game import Game
from copy import deepcopy
from macros import *
import random


# vanilla minimax player; requires an evaluator function that evaluates game objects
# really, REALLY slow because we make a copy of a game object for each node expanded
class MinimaxPlayer(Player):
    def __init__(self, board_size, color, evaluator, depth=2):
        super().__init__(board_size, color)
        self.evaluator = evaluator
        self.depth = depth

    def _max(self, game, depth, goal, alpha, beta):
        if depth == goal or game.done:
            return self.evaluator(game), None
        legal_moves = game.get_legal_moves()
        random.shuffle(legal_moves)
        if not legal_moves:
            return self.evaluator(game), None
        ret_value = NEGINF - 1
        ret_move = None
        for candidate in legal_moves:
            game_copy = Game.from_other(game)
            game_copy.move(candidate)
            (val, move) = self._min(game_copy, depth + 1, goal, alpha, beta)
            if val > ret_value:
                ret_value = val
                ret_move = candidate
            if ret_value >= beta:
                return ret_value, ret_move
            if ret_value > alpha:
                alpha = ret_value
        return ret_value, ret_move

    def _min(self, game, depth, goal, alpha, beta):
        if depth == goal or game.done:
            return self.evaluator(game), None
        legal_moves = game.get_legal_moves()
        random.shuffle(legal_moves)
        if not legal_moves:
            return self.evaluator(game), None
        ret_value = INF + 1
        ret_move = None
        for candidate in legal_moves:
            game_copy = Game.from_other(game)
            game_copy.move(candidate)
            (val, move) = self._max(game_copy, depth + 1, goal, alpha, beta)
            if val < ret_value:
                ret_value = val
                ret_move = candidate
            if ret_value <= alpha:
                return ret_value, ret_move
            if ret_value < beta:
                beta = ret_value
        return ret_value, ret_move

    def _minimax(self, game, depth=1):
        if self.color == BLACK:
            (val, move) = self._max(game, 0, depth, NEGINF, INF)
        else:
            (val, move) = self._min(game, 0, depth, NEGINF, INF)
        return move

    def get_move(self, game):
        return self._minimax(game, self.depth)


def simple_evaluation(game):
    if game.done:
        if game.winner == BLACK:
            return INF
        elif game.winner == WHITE:
            return NEGINF
        return 0
    return game.score()
