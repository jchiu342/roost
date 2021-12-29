import torch
import numpy as np
from net import ConnectNet
from mcts_player import MCTSPlayer
from macros import *


# model = ConnectNet()
# model.load_state_dict(torch.load("test0.pth"))
# model.eval()


# returns numpy array for easier storage
def get_nn_input(game):
    board_size = game.board_size
    ret = np.zeros((5, board_size, board_size), dtype=np.float32)
    if game.turn == BLACK:
        for i in range(board_size):
            for j in range(board_size):
                if game.board[i][j] == BLACK:
                    ret[0][i][j] = 1
                elif game.board[i][j] == WHITE:
                    ret[1][i][j] = 1
                if game.prev_game_state.board[i][j] == BLACK:
                    ret[2][i][j] = 1
                elif game.prev_game_state.board[i][j] == WHITE:
                    ret[3][i][j] = 1
                ret[4][i][j] = 1
    elif game.turn == WHITE:
        for i in range(board_size):
            for j in range(board_size):
                if game.board[i][j] == BLACK:
                    ret[1][i][j] = 1
                elif game.board[i][j] == WHITE:
                    ret[0][i][j] = 1
                if game.prev_game_state.board[i][j] == BLACK:
                    ret[3][i][j] = 1
                elif game.prev_game_state.board[i][j] == WHITE:
                    ret[2][i][j] = 1
    return ret


def evaluate(game, model):
    with torch.no_grad():
        p, v = model(torch.from_numpy(get_nn_input(game)))
    return p[0].numpy(), v[0][0].numpy()


class NNPlayer(MCTSPlayer):
    def __init__(self, board_size, color, playouts=50):
        # super().__init__(board_size, color, evaluate, playouts)
        self.model = ConnectNet(9, 64, 5)
        # model.load_state_dict(torch.load("test0.pth"))
        self.model.eval()
        super().__init__(board_size, color, lambda x: evaluate(x, self.model), playouts)
