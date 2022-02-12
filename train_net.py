from net import Net, AlphaLoss
import numpy as np
from fire import Fire
from tqdm import tqdm
import torch
import torchvision
from torch.utils.tensorboard import SummaryWriter
import os
from random import random
from os.path import exists


DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")
LOGGER = SummaryWriter("runs/testrun")
EPOCH = 75
BATCH_SIZE = 256
TRAIN_TEST_SPLIT = 0.85
SAMPLE_INCLUDE_PROB = 1.0
SAMPLES_PER_EPOCH = 500


class GameDataset(torch.utils.data.Dataset):
    def __init__(self, examples):
        self.examples = examples

    def __len__(self):
        return len(self.examples)

    def __getitem__(self, idx):
        return self.examples[idx]


def make_datasets(dataset_dir):
    train_examples = []
    val_examples = []
    for root, dirs, files in os.walk(dataset_dir, topdown=False):
        for name in tqdm(files):
            with open(os.path.join(root, name), 'rb') as fin:
                while True:
                    try:
                        s = torch.from_numpy(np.load(fin))
                        a = torch.from_numpy(np.load(fin)).type(torch.float32)
                        w = torch.from_numpy(np.load(fin)).type(torch.float32)
                        if s.dim() == 0:
                            continue
                        assert (len(s) == len(a))
                        if random() < TRAIN_TEST_SPLIT:
                            for i in range(len(s)):
                                if random() < SAMPLE_INCLUDE_PROB:
                                    train_examples.append((s[i], a[i], w))
                        else:
                            for i in range(len(s)):
                                if random() < SAMPLE_INCLUDE_PROB:
                                    val_examples.append((s[i], a[i], w))
                    except ValueError:
                        break
    trainset = torch.utils.data.DataLoader(
    	GameDataset(train_examples),
    	shuffle=True, 
    	batch_size=BATCH_SIZE
    )
    valset = torch.utils.data.DataLoader(
    	GameDataset(val_examples),
    	shuffle=True,
    	batch_size=BATCH_SIZE
    )
    return trainset, valset


def val(valset, model, loss_fn, save_name, log_iter=0):
    total_loss = 0
    it = 0
    model.eval()
    with torch.no_grad():
        iterator = iter(valset)
        for _ in tqdm(range(SAMPLES_PER_EPOCH)):
            state, action, result = iterator.next()
            s, a, r = state.to(DEVICE), action.to(DEVICE), result.to(DEVICE)
            pred_policy, pred_value = model(s)
            loss = loss_fn(pred_policy, a, pred_value, r)
            total_loss += loss.item()
            it += 1
    avg_loss = round(total_loss / it, 5)
    print(" val loss: ", avg_loss)
    LOGGER.add_scalar("Val loss", avg_loss, log_iter)
    save_trace(model, save_name, log_iter)


def train(trainset, valset, model, save_name):
    loss_fn = AlphaLoss()
    optimizer = torch.optim.SGD(model.parameters(), momentum=0.9, lr=0.01, weight_decay=1e-4)
    scaler = torch.cuda.amp.GradScaler()
    for i in range(EPOCH):
        val(valset, model, loss_fn, save_name, i)
        model.train()
        total_loss = 0
        iterator = iter(trainset)
        for _ in tqdm(range(SAMPLES_PER_EPOCH)):
            state, action, result = iterator.next()
            s, a, r = state.to(DEVICE), action.to(DEVICE), result.to(DEVICE)
            with torch.autocast(device_type="cuda"):
                pred_policy, pred_value = model(s)
                loss = loss_fn(pred_policy, a, pred_value, r)
            total_loss += loss.item()
            optimizer.zero_grad()
            scaler.scale(loss).backward()
            scaler.step(optimizer)
            scaler.update()
        avg_loss = round(total_loss / SAMPLES_PER_EPOCH, 5)
        print(" train loss: ", avg_loss)
        LOGGER.add_scalar("Train loss", avg_loss, i)


def start_train(data_dir, save_name, load_name=None):
    assert torch.cuda.is_available()
    trainset, valset = make_datasets(data_dir)
    # board size, # filters, # blocks
    model = Net(9, 96, 6)
    if load_name is not None:
        model.load_state_dict(torch.load(load_name))
    model = model.to(DEVICE)
    train(trainset, valset, model, save_name)


def save_trace(model, trace_file_name, log_iter):
    model = model.to(torch.device("cpu"))
    torch.save(model.state_dict(), trace_file_name + str(log_iter) + ".pth")
    scripted_model = torch.jit.script(model)
    scripted_model.save(trace_file_name + str(log_iter) + ".pt")
    print("saved " + trace_file_name + str(log_iter) + ".pt/.pth")
    model = model.to(DEVICE)


if __name__ == "__main__":
    Fire()
