from net import ConnectNet, AlphaLoss
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
MODEL_SAVE_FILE = "model_state_dict.pth"
EPOCH = 20
BATCH_SIZE = 256
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
        for state, action, result in tqdm(valset):
            s, a, r = state.to(DEVICE), action.to(DEVICE), result.to(DEVICE)
            pred_policy, pred_value = model(s)
            loss = loss_fn(pred_policy, a, pred_value, r)
            total_loss += loss.item()
            it += 1
    avg_loss = round(total_loss / it, 5)
    print(" val loss: ", avg_loss)
    LOGGER.add_scalar("Val loss", avg_loss, log_iter)
    save_trace(model, valset, save_name + str(log_iter) + ".pt")


def train(trainset, valset, model, loss_fn, optimizer, save_name):
    for i in range(EPOCH):
        val(valset, model, loss_fn, save_name, i)
        model.train()
        total_loss = 0
        iterator = iter(trainset)
        for i in tqdm(range(SAMPLES_PER_EPOCH)):
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
        torch.save(model.state_dict(), MODEL_SAVE_FILE)


def start_train(train_dir, val_dir, save_name):
    trainset = torch.utils.data.DataLoader(
        GameDataset(train_dir),
        shuffle=True,
        batch_size=BATCH_SIZE
    )
    valset = torch.utils.data.DataLoader(
        GameDataset(val_dir),
        batch_size=BATCH_SIZE
    )
    # board size, # filters, # blocks
    model = ConnectNet(9, 32, 4).to(DEVICE)
    loss_fn = AlphaLoss()
    optimizer = torch.optim.SGD(model.parameters(), momentum=0.9, lr=0.0001, weight_decay=1e-4)
    if not exists(MODEL_SAVE_FILE):
        temp = open(MODEL_SAVE_FILE, "w")
        temp.close()
    train(trainset, valset, model, loss_fn, optimizer, save_name)


def save_trace(model, valset, trace_file):
    model = model.to(torch.device("cpu"))
    scripted_model = torch.jit.script(model)
    scripted_model.save(trace_file)
    print("saved " + trace_file)
    model = model.to(DEVICE)
    # for state, action, result in tqdm(valset):
    #    traced_script_module = torch.jit.trace(model, state.to(DEVICE))
    #    traced_script_module.save(trace_file)
    #    print("saved " + trace_file)
    #    break


# def trace(val_file, trace_file):
#     valset = torch.utils.data.DataLoader(
#         GameDataset(val_file),
#         batch_size=BATCH_SIZE
#     )
#     model = ConnectNet()
#     for state, action, result in tqdm(valset):
#         traced_script_module = torch.jit.trace(model, state)
#         traced_script_module.save(trace_file)
#         break


if __name__ == "__main__":
    Fire()
