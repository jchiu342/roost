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

BLOCKS = 5
FILTERS = 64


# we require 5 channels: 2-history and 1 for whose move it is
class ConvBlock(nn.Module):
    def __init__(self):
        super(ConvBlock, self).__init__()
        self.conv1 = nn.Conv2d(5, FILTERS, 3, stride=1, padding=1)
        self.bn1 = nn.BatchNorm2d(FILTERS)

    def forward(self, s):
        s = s.view(-1, 5, BOARD_SIZE, BOARD_SIZE)  # batch_size x channels x board_x x board_y
        s = F.relu(self.bn1(self.conv1(s)))
        return s


class ResBlock(nn.Module):
    def __init__(self, inplanes=FILTERS, planes=FILTERS, stride=1, downsample=None):
        super().__init__()
        self.conv1 = nn.Conv2d(inplanes, planes, kernel_size=3, stride=stride,
                               padding=1, bias=False)
        self.bn1 = nn.BatchNorm2d(planes)
        self.conv2 = nn.Conv2d(planes, planes, kernel_size=3, stride=stride,
                               padding=1, bias=False)
        self.bn2 = nn.BatchNorm2d(planes)

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
    def __init__(self):
        super().__init__()
        self.conv = nn.Conv2d(FILTERS, 1, kernel_size=1)  # value head
        self.bn = nn.BatchNorm2d(1)
        self.fc1 = nn.Linear(BOARD_SIZE * BOARD_SIZE, FILTERS)
        self.fc2 = nn.Linear(FILTERS, 1)

        self.conv1 = nn.Conv2d(FILTERS, 2, kernel_size=1)  # policy head
        self.bn1 = nn.BatchNorm2d(2)
        self.logsoftmax = nn.LogSoftmax(dim=1)
        self.fc = nn.Linear(BOARD_SIZE * BOARD_SIZE * 2, BOARD_SIZE * BOARD_SIZE + 1)

    def forward(self, s):
        v = F.relu(self.bn(self.conv(s)))  # value head
        v = v.view(-1, BOARD_SIZE * BOARD_SIZE)  # batch_size X channel X height X width
        v = F.relu(self.fc1(v))
        v = torch.tanh(self.fc2(v))

        p = F.relu(self.bn1(self.conv1(s)))  # policy head
        p = p.view(-1, BOARD_SIZE * BOARD_SIZE * 2)
        p = self.fc(p)
        p = self.logsoftmax(p).exp()
        return p, v


class ConnectNet(nn.Module):
    def __init__(self):
        super(ConnectNet, self).__init__()
        self.blocks = [ConvBlock()]
        for i in range(BLOCKS):
            self.blocks.append(ResBlock())
        self.blocks.append(OutBlock())
        self.blocks = torch.nn.Sequential(*self.blocks)

    def forward(self, s):
        return self.blocks(s)
#         self.conv = ConvBlock()
#         for block in range(19):
#             setattr(self, "res_%i" % block, ResBlock())
#         self.outblock = OutBlock()
#
#     def forward(self, s):
#         s = self.conv(s)
#         for block in range(19):
#             s = getattr(self, "res_%i" % block)(s)
#         s = self.outblock(s)
#         return s


class AlphaLoss(torch.nn.Module):
    def __init__(self):
        super(AlphaLoss, self).__init__()
        self.mse_loss = nn.MSELoss(reduction='mean')
        self.cross_entropy_loss = nn.CrossEntropyLoss(reduction='mean')

    def forward(self, y_policy, policy, y_value, value):
        # value_error = (value - y_value) ** 2
        # policy_error = torch.sum((-policy *
        #                           (1e-8 + y_policy.float()).float().log()), 1)
        # total_error = (value_error.view(-1).float() + policy_error).mean()
        # print(self.mse_loss(y_value, value))
        # print(self.cross_entropy_loss(y_policy, policy))
        # print(value_error)
        # print(policy_error)
        return self.mse_loss(y_value[:, 0], value) + self.cross_entropy_loss(y_policy, policy)
