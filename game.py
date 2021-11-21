from display import GameViewer
from macros import *
from random import choice
from copy import deepcopy
import time


class Action:
    def __init__(self, color, type, x=None, y=None):
        self.color = color
        self.type = type
        self.x = x
        self.y = y

    def get_info(self):
        return self.color, self.type, self.x, self.y

    def __str__(self):
        if self.type == PASS:
            return "PASS"
        elif self.type == RESIGN:
            return "RESIGN"
        return "PLAY ({}, {})".format(self.x, self.y)


class GameState:
    def __init__(self, board, turn, most_recent_action):
        self.board = board
        self.turn = turn
        self.most_recent_action = most_recent_action

    def __str__(self):
        return str(self.board) + str(self.turn)

    def __eq__(self, other):
        return str(self) == str(other)


class Game:
    # TODO: implement handicap
    def __init__(self, board_size=BOARD_SIZE, handi=0, komi=0, gui=False):
        self.board_size = board_size
        self.komi = komi
        self.handi = handi
        self.board = [[EMPTY for _ in range(board_size)] for _ in range(board_size)]
        self.done = False
        self.winner = EMPTY
        self.turn = BLACK
        self.turns = 0
        self.most_recent_action = None
        # TODO: fix the edge case where black and white both pass first -- this should be legal but isn't here
        self.prev_game_state = self.get_game_state()
        self.gui = gui
        if self.gui:
            self.viewer = GameViewer(self.board_size)
            self.viewer.start()
        # internal variables for dfs
        self._liberties = 0
        self._chain = set()
        self._visited = set()
        self._white_reachable = [[False for _ in range(board_size)] for _ in range(board_size)]
        self._black_reachable = [[False for _ in range(board_size)] for _ in range(board_size)]

    # TODO: make a nicer version of copying
    @classmethod
    def from_other(cls, other):
        return_copy = Game(other.board_size, other.handi, other.komi, False)
        for (k, v) in other.__dict__.items():
            if k not in ["gui", "viewer"]:
                setattr(return_copy, k, deepcopy(v))
        return return_copy

    # only called on reachable states
    def _dfs_score(self, x1, y1, color):
        if color == WHITE:
            if self._white_reachable[x1][y1]:
                return
            self._white_reachable[x1][y1] = True
            for (i, j) in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                if 0 <= x1 + i < self.board_size and 0 <= y1 + j < self.board_size and self.board[x1 + i][y1 + j] != BLACK:
                    self._dfs_score(x1 + i, y1 + j, WHITE)
            return
        assert(color == BLACK)
        if self._black_reachable[x1][y1]:
            return
        self._black_reachable[x1][y1] = True
        for (i, j) in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
            if 0 <= x1 + i < self.board_size and 0 <= y1 + j < self.board_size and self.board[x1 + i][y1 + j] != WHITE:
                self._dfs_score(x1 + i, y1 + j, BLACK)

    def _dfs(self, x1, y1, color):
        if (x1, y1) in self._visited:
            return
        self._visited.add((x1, y1))
        if self.board[x1][y1] == EMPTY:
            self._liberties += 1
            return
        elif self.board[x1][y1] != color:
            return
        self._chain.add((x1, y1))
        for (i, j) in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
            if 0 <= x1 + i < self.board_size and 0 <= y1 + j < self.board_size:
                self._dfs(x1 + i, y1 + j, color)
        return

    def _start_dfs(self, x1, y1, color):
        self._liberties = 0
        self._visited = set()
        self._chain = set()
        self._dfs(x1, y1, color)

    def _remove_neighbor_dead(self, x1, y1, color):
        for (i, j) in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
            if 0 <= x1 + i < self.board_size and 0 <= y1 + j < self.board_size and self.board[x1 + i][y1 + j] == -color:
                self._start_dfs(x1 + i, y1 + j, -color)
                if self._liberties == 0:
                    for (x_loc, y_loc) in self._chain:
                        self.board[x_loc][y_loc] = EMPTY

    def _is_legal_move(self, x1, y1, color=None):
        # empty at square
        if color is None:
            color = self.turn
        if self.board[x1][y1] != EMPTY:
            return False
        # TODO: optimize ko detection so that we don't need to make deep copies of game state
        # TODO: add superko detection
        original_board = deepcopy(self.board)
        self.board[x1][y1] = color
        self._remove_neighbor_dead(x1, y1, color)
        self._start_dfs(x1, y1, color)
        if self._liberties == 0 or len(self._chain) == 4:
            self.board = original_board
            return False
        self.turn = -self.turn
        if self.get_game_state() == self.prev_game_state:
            self.board = original_board
            self.turn = -self.turn
            return False
        self.turn = -self.turn
        self.board = original_board
        return True

    # scores the board according to TT rules.
    def score(self):
        for x in range(self.board_size):
            for y in range(self.board_size):
                if self.board[x][y] == WHITE:
                    self._dfs_score(x, y, WHITE)
                if self.board[x][y] == BLACK:
                    self._dfs_score(x, y, BLACK)
        white_score = 0
        black_score = 0
        for x in range(self.board_size):
            for y in range(self.board_size):
                if self._white_reachable[x][y]:
                    if not self._black_reachable[x][y]:
                        white_score += 1
                elif self._black_reachable[x][y]:
                    black_score += 1
        if white_score + self.komi > black_score:
            self.winner = WHITE
        elif white_score + self.komi < black_score:
            self.winner = BLACK
        return black_score - white_score - self.komi

    def is_legal_action(self, action):
        # no more legal actions
        if self.done:
            return False
        color, type, x, y = action.get_info()
        if type in [PASS, RESIGN]:
            return True
        assert(type == PLAY)
        return self._is_legal_move(x, y, color)

    def action_idxs(self):
        # returns a list of length (board_size * board_size + 1) of 0's and 1's corresponding to whether action is legal
        # last action corresponds to pass; we don't feed resign as an option
        # ret = [0] * (self.board_size * self.board_size)
        ret = []
        for x in range(self.board_size):
            for y in range(self.board_size):
                if self.is_legal_action(Action(self.turn, PLAY, x, y)):
                    # ret[x * self.board_size + y] = 1
                    ret.append(x * self.board_size + y)
        ret.append(self.board_size * self.board_size)
        return ret

    def get_game_state(self):
        return GameState(self.board, self.turn, self.most_recent_action)

    def get_legal_moves(self):
        legals = [Action(self.turn, PASS), Action(self.turn, RESIGN)]
        for x in range(self.board_size):
            for y in range(self.board_size):
                if self._is_legal_move(x, y):
                    legals.append(Action(self.turn, PLAY, x, y))
        return legals

    def move(self, action):
        assert(self.is_legal_action(action))
        self.prev_game_state = deepcopy(self.get_game_state())
        color, type, x1, y1 = action.get_info()
        if type == RESIGN:
            self.done = True
            self.winner = -color
            return
        if type == PASS:
            self.turn = -self.turn
            if self.most_recent_action is not None and self.most_recent_action.type == PASS:
                self.done = True
                self.score()
            self.most_recent_action = action
            return
        self.most_recent_action = action
        self.board[x1][y1] = color
        self.turn = -self.turn
        # remove dead stones
        self._remove_neighbor_dead(x1, y1, color)
        if self.gui:
            self.viewer.update_from_state(self.get_game_state())
        self.turns += 1
        if self.turns >= MAX_GAME_LENGTH:
            self.done = True
            self.score()


if __name__ == "__main__":
    game = Game(7, gui=True)
    while not game.done:
        d = game.get_legal_moves()
        random_move = choice(d)
        game.move(random_move)
        time.sleep(0.25)
