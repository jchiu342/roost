import random
from game import Action
from macros import *
from abc import abstractmethod


class Player:
    def __init__(self, board_size, color):
        self.board_size = board_size
        self.color = color

    @abstractmethod
    def get_move(self, game):
        pass


class RandomPlayer(Player):
    def __init__(self, board_size, color):
        super().__init__(board_size, color)

    def get_move(self, game):
        return random.choice(game.get_legal_moves())


class InputPlayer(Player):
    def __init__(self, board_size, color):
        super().__init__(board_size, color)

    def get_move(self, game):
        while True:
            d = input().split(' ')
            if d[0] == "PASS":
                return Action(self.color, PASS)
            elif d[0] == "RESIGN":
                return Action(self.color, RESIGN)
            elif d[0] == "PLAY":
                if len(d) < 3:
                    print("bad format, try again")
                    continue
                try:
                    candidate_action = Action(self.color, PLAY, int(d[1]), int(d[2]))
                except ValueError:
                    print("bad format, try again")
                    continue
                if not game.is_legal_action(candidate_action):
                    print("illegal move, try again")
                    continue
                return candidate_action
            else:
                print("bad format, try again")
                continue
