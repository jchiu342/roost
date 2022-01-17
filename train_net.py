from net import Net, AlphaLoss
import numpy as np
from fire import Fire
from tqdm import tqdm
import torch
import torchvision
from torch.utils.tensorboard import SummaryWriter
import os
from os.path import exists


DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")
LOGGER = SummaryWriter("runs/testrun")
EPOCH = 75
BATCH_SIZE = 256
TRAIN_TEST_SPLIT = 0.9
SAMPLES_PER_EPOCH = 1000


class GameDataset(torch.utils.data.Dataset):
    def __init__(self, dataset_dir):
        self.examples = []
        for root, dirs, files in os.walk(dataset_dir, topdown=False):
            for name in tqdm(files):
                with open(os.path.join(root, name), 'rb') as fin:
                    while True:
                        try:
                            s = torch.from_numpy(np.load(fin))
                            a = torch.from_numpy(np.load(fin)).type(torch.long)
                            w = torch.from_numpy(np.load(fin)).type(torch.float32)
                            assert(len(s) == len(a))
                            for i in range(len(s)):
                                self.examples.append((s[i], a[i], w))
                        except ValueError:
                            break
        print("Loaded " + dataset_dir + ", number of examples: " + str(len(self.examples)))

    def __len__(self):
        return len(self.examples)

    def __getitem__(self, idx):
        return self.examples[idx]


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


def train(trainset, valset, model, loss_fn, optimizer, save_name):
    for i in range(EPOCH):
        val(valset, model, loss_fn, save_name, i)
        model.train()
        total_loss = 0
        iterator = iter(trainset)
        for _ in tqdm(range(SAMPLES_PER_EPOCH)):
            state, action, result = iterator.next()
            s, a, r = state.to(DEVICE), action.to(DEVICE), result.to(DEVICE)
            pred_policy, pred_value = model(s)
            loss = loss_fn(pred_policy, a, pred_value, r)
            total_loss += loss.item()
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()
        avg_loss = round(total_loss / SAMPLES_PER_EPOCH, 5)
        print(" train loss: ", avg_loss)
        LOGGER.add_scalar("Train loss", avg_loss, i)


def start_train(data_dir, save_name):
    dataset = GameDataset(data_dir)
    train_set_size = int(len(dataset) * TRAIN_TEST_SPLIT)
    val_set_size = len(dataset) - train_set_size
    trainset, valset = torch.utils.data.random_split(dataset, [train_set_size, val_set_size])
    trainset = torch.utils.data.DataLoader(
        trainset,
        shuffle=True,
        batch_size=BATCH_SIZE
    )
    valset = torch.utils.data.DataLoader(
        valset,
        shuffle=True,
        batch_size=BATCH_SIZE
    )
    # board size, # filters, # blocks
    model = Net(9, 32, 4)
    # model.load_state_dict(torch.load("model_state_dict.pth19.pth"))
    model = model.to(DEVICE)
    loss_fn = AlphaLoss()
    optimizer = torch.optim.SGD(model.parameters(), momentum=0.9, lr=0.0001, weight_decay=1e-4)
    train(trainset, valset, model, loss_fn, optimizer, save_name)


def save_trace(model, trace_file_name, log_iter):
    model = model.to(torch.device("cpu"))
    torch.save(model.state_dict(), trace_file_name + str(log_iter) + ".pth")
    scripted_model = torch.jit.script(model)
    scripted_model.save(trace_file_name + str(log_iter) + ".pt")
    print("saved " + trace_file_name + str(log_iter) + ".pt/.pth")
    model = model.to(DEVICE)


if __name__ == "__main__":
    Fire()
