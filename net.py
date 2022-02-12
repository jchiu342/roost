###########################################################################
# CODE ADAPTED FROM SAME PLACE AS MCTS_PLAYER
###########################################################################


import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.utils.data import Dataset
import matplotlib
import numpy as np
from macros import *
matplotlib.use("Agg")


# we require 5 channels: 2-history and 1 for whose move it is
class ConvBlock(nn.Module):
    def __init__(self, num_filters, board_size):
        super().__init__()
        self.conv1 = nn.Conv2d(5, num_filters, 3, stride=1, padding=1)
        self.bn1 = nn.BatchNorm2d(num_filters)
        self.board_size = board_size

    def forward(self, s):
        s = s.view(-1, 5, self.board_size, self.board_size)  # batch_size x channels x board_x x board_y
        s = F.relu(self.bn1(self.conv1(s)))
        return s


# modified version of global pooling layers -- only 1 mean bc we are on a single board size
class PoolingStructure(nn.Module):
    def __init__(self, cX, cG, board_size):
        super().__init__()
        self.board_size = board_size
        self.bn1 = nn.BatchNorm2d(cG)
        self.avg_pool1 = nn.AvgPool2d(board_size)
        self.max_pool1 = nn.MaxPool2d(board_size)
        self.fc1 = nn.Linear(2 * cG, cX)
        self.cX = cX
        self.cG = cG

    def forward(self, s):
        X, G = torch.split(s, [self.cX, self.cG], dim=1)
        # print(G.size())
        G = F.relu(self.bn1(G))
        #avg_pool = self.avg_pool1(G)
        #max_pool = self.max_pool1(G)
        G = torch.cat((self.avg_pool1(G), self.max_pool1(G)), dim=1)
        #print(G.size())
        G = torch.squeeze(G)
        #print(G.size())
        G = self.fc1(G)
        #print(G.size())
        G = G.view(-1, self.cX, 1, 1)
        #print(X.size())
        return X + G


class PoolingBlock(nn.Module):
    def __init__(self, num_filters, cX, board_size):
        super().__init__()
        self.conv1 = nn.Conv2d(num_filters, num_filters, kernel_size=3, stride=1,
                               padding=1, bias=False)
        self.conv2 = nn.Conv2d(cX, num_filters, kernel_size=3, stride=1,
                               padding=1, bias=False)
        self.bn1 = nn.BatchNorm2d(cX)
        self.cX = cX
        self.num_filters = num_filters
        self.pool = PoolingStructure(cX, num_filters - cX, board_size)

    def forward(self, x):
        residual = x
        out = self.conv1(x)
        out = self.pool.forward(out)
        #out = self.pool.forward(torch.split(out, [self.cX, self.num_filters - self.cX], dim=1))
        out = F.relu(self.bn1(out))
        out = self.conv2(out)
        out += residual
        out = F.relu(out)
        return out


class ResBlock(nn.Module):
    def __init__(self, num_filters, stride=1):
        super().__init__()
        self.conv1 = nn.Conv2d(num_filters, num_filters, kernel_size=3, stride=stride,
                               padding=1, bias=False)
        self.bn1 = nn.BatchNorm2d(num_filters)
        self.conv2 = nn.Conv2d(num_filters, num_filters, kernel_size=3, stride=stride,
                               padding=1, bias=False)
        self.bn2 = nn.BatchNorm2d(num_filters)

    def forward(self, x):
        residual = x
        out = self.conv1(x)
        out = F.relu(self.bn1(out))
        out = self.conv2(out)
        out = self.bn2(out)
        out += residual
        out = F.relu(out)
        return out


class OutBlock(nn.Module):
    def __init__(self, board_size, num_filters):
        super().__init__()
        self.board_size = board_size
        self.v_conv1 = nn.Conv2d(num_filters, 1, kernel_size=1)  # value head
        self.v_bn1 = nn.BatchNorm2d(1)
        self.v_fc1 = nn.Linear(board_size * board_size, num_filters)
        self.v_fc2 = nn.Linear(num_filters, 1)

        self.p_conv1 = nn.Conv2d(num_filters, 2, kernel_size=1)  # policy head
        self.p_bn1 = nn.BatchNorm2d(2)
        self.p_fc1 = nn.Linear(board_size * board_size * 2, board_size * board_size + 1)

    def forward(self, s):
        v = F.relu(self.v_bn1(self.v_conv1(s)))  # value head
        v = v.view(-1, self.board_size * self.board_size)  # batch_size X channel X height X width
        v = F.relu(self.v_fc1(v))
        v = torch.tanh(self.v_fc2(v))

        p = F.relu(self.p_bn1(self.p_conv1(s)))  # policy head
        p = p.view(-1, self.board_size * self.board_size * 2)
        p = self.p_fc1(p)
        return p, v


class Net(nn.Module):
    def __init__(self, board_size, num_filters, num_blocks, cX=32):
        super(Net, self).__init__()
        self.blocks = [ConvBlock(num_filters, board_size)]
        for i in range(num_blocks):
            if i % (num_blocks // 2) == 0:
                self.blocks.append(PoolingBlock(num_filters, cX, board_size))
            else:
                self.blocks.append(ResBlock(num_filters))
        self.blocks.append(OutBlock(board_size, num_filters))
        self.blocks = torch.nn.Sequential(*self.blocks)

    def forward(self, s):
        return self.blocks(s)


class AlphaLoss(torch.nn.Module):
    def __init__(self):
        super(AlphaLoss, self).__init__()
        self.mse_loss = nn.MSELoss(reduction='mean')
        self.cross_entropy_loss = nn.CrossEntropyLoss(reduction='mean')

    def forward(self, y_policy, policy, y_value, value):
        return self.mse_loss(y_value[:, 0], value) + self.cross_entropy_loss(y_policy, policy)
