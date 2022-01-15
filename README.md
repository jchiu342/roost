# Roost

A bot that plays Tetris Go, a variant of the popular board game Go that doesn't allow chains of 4 stones. Using machine learning with no human knowledge, Roost is based on [Deepmind's AlphaGo Zero](https://deepmind.com/blog/article/alphago-zero-starting-scratch) framework and later improvements from projects such as [KataGo](https://github.com/lightvector/KataGo).

All components of the AlphaGo Zero framework are fully implemented in Python, using [PyTorch](https://pytorch.org/) to train the neural networks. Self-play and evaluation are also implemented in C++ with an emphasis on achieving high performance while maintaining readability. 

Roost is still early in progress -- check later for training run results and a more detailed guide.