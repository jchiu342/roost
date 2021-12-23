from game import Action, Game
from match import Match
from mcts_player import from_action
from nn_player import get_nn_input, NNPlayer
from macros import *
import numpy as np
from fire import Fire
from tqdm import tqdm
import random
import sys
import os


# TODO: make this less hacky
# ONLY WORKS FOR BOARD_SIZE <= 9
def read_from_sgf(game_dir, save_train_file, save_val_file, train_split=0.8):
    train = open(save_train_file, "ab+")
    val = open(save_val_file, "ab+")
    for root, dirs, files in os.walk(game_dir, topdown=False):
        for name in tqdm(files):
            with open(os.path.join(root, name), 'r') as file_object:
                game = Game(BOARD_SIZE, handi=0, komi=7.5, gui=False)
                states = []
                actions = []
                while True:
                    try:
                        line = file_object.readline()
                        if len(line) <= 0:
                            # end of file
                            break
                        if line[0] == ';':
                            color = BLACK if line[1] == 'B' else WHITE
                            if line[3] == ']':
                                action = Action(color, PASS)
                            else:
                                action = Action(color, PLAY, x=(ord(line[3]) - ord('a')), y=(ord(line[4]) - ord('a')))
                            states.append(get_nn_input(game))
                            actions.append(from_action(action))
                            game.move(action)
                        elif line.find("RE[") != -1:
                            winner = WHITE if line.find("RE[W") != -1 else BLACK
                    except EOFError:
                        break
                if random.random() < train_split:
                    np.save(train, states)
                    np.save(train, actions)
                    np.save(train, winner)
                else:
                    np.save(val, states)
                    np.save(val, actions)
                    np.save(val, winner)



def generate_training_data(num_games, save_train_file, save_val_file, train_split=0.8, playouts=50):
    black = NNPlayer(BOARD_SIZE, BLACK, playouts)
    white = NNPlayer(BOARD_SIZE, WHITE, playouts)
    train = open(save_train_file, "ab+")
    val = open(save_val_file, "ab+")
    for _ in tqdm(range(num_games)):
        match = Match(black, white, gui=False, save_data=True)
        match.play()
        # w = 1 for black win and -1 for white win
        s, a, w = match.get_train_data()
        assert(w in [-1, 1])
        if random.random() < train_split:
            np.save(train, s)
            np.save(train, a)
            np.save(train, w)
        else:
            np.save(val, s)
            np.save(val, a)
            np.save(val, w)
    train.close()
    val.close()


if __name__ == "__main__":
    Fire()
