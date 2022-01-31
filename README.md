# Roost

A bot that plays Tetris Go, a variant of the popular board game Go that doesn't allow chains of 4 stones. Using machine learning with no human knowledge, Roost is based on [Deepmind's AlphaGo Zero](https://deepmind.com/blog/article/alphago-zero-starting-scratch) framework and later improvements from projects such as [KataGo](https://github.com/lightvector/KataGo).

All components of the AlphaGo Zero framework are fully implemented. Self-play and evaluation are implemented in C++ with an emphasis on achieving high performance while maintaining readability; training is implemented in Python using [PyTorch](https://pytorch.org/). A simple implementation of self-play and evaluation in Python is also included, but it isn't regularly maintained because of Python speed issues.
# Build

Roost is written in C++20. It has been tested on the following configuration:

* Ubuntu 20.04
* CMake version 3.16.3
* g++ 9.3.0
* LibTorch 1.10.2
* CUDA 11.4
* cuDNN v8.2.4

Plenty of other setups should work as well, but I don't have the time required to officially support them. You will definitely need a C++20 compiler and LibTorch, as well as CUDA and cuDNN if you plan to use a GPU.
## Example of Compiling

    # Clone github repo
    git clone https://github.com/jchiu342/roost.git

    # Compile with CMake
    cd roost/cpp
    mkdir build
    cd build
    cmake .. -DCMAKE_PREFIX_PATH=/path/to/libtorch
    make roost

This should create a `roost` executable in the current directory. 

# Play

You will need a model file (see section below). Roost implements a simple [GTP](https://www.lysator.liu.se/~gunnar/gtp/) protocol for connecting to GUIs or online servers. To run in GTP mode, use the following command: `./roost gtp <model_file> <playouts>`.

# Models
The first training run is on a 9x9 board size. The weights, which are stored in TorchScript models, can be found here: https://drive.google.com/drive/folders/1ASQZIIlLD2SozgpYMCoUW3VEVxzfWYRp?usp=sharing

At the time of the latest update to this document (01/30/22), the strongest net plays at a strong human level, perhaps close to 5-6d. It is expected to surpass human level in a couple of weeks. 
