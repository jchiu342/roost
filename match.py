import player
from minimax_player import *
from mcts_player import *
from nn_player import get_nn_input, NNPlayer
from game import Game
from macros import *
import time


class Match:
    def __init__(self, player1, player2, board_size=BOARD_SIZE, handi=0, komi=7.5, gui=True, save_data=False):
        # player 1 is black, player 2 is white
        self.players = [player1, player2]
        self.game = Game(board_size, handi, komi, gui)
        self.save_data = save_data
        self.states = []
        self.actions = []

    def play(self):
        while not self.game.done:
            if self.game.turn == BLACK:
                move = self.players[0].get_move(self.game)
            else:
                move = self.players[1].get_move(self.game)
            if self.save_data:
                self.states.append(get_nn_input(self.game))
                self.actions.append(from_action(move))
            self.game.move(move)
            # time.sleep(0.3)
        return self.game.winner

    def get_train_data(self):
        assert self.save_data
        return self.states, self.actions, self.game.winner


if __name__ == "__main__":
    black_wins = 0
    white_wins = 0
    for i in range(10):
        # match = Match(MinimaxPlayer(9, BLACK, simple_evaluation, depth=2), MCTSPlayer(9, WHITE, simple_mcts_evaluation, rollouts=50))
        # match = Match(player.InputPlayer(9, BLACK), player.InputPlayer(9, WHITE))
        # match = Match(player.InputPlayer(9, BLACK), player.InputPlayer(9, WHITE), board_size=3)
        match = Match(NNPlayer(9, BLACK), NNPlayer(9, WHITE))
        winner = match.play()
        if winner == BLACK:
            black_wins += 1
        elif winner == WHITE:
            white_wins += 1
    print("black wins: {}; white wins: {}".format(black_wins, white_wins))
