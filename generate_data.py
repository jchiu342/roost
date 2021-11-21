from match import Match
from nn_player import NNPlayer
from macros import *
import numpy as np
from fire import Fire
from tqdm import tqdm
import random


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
