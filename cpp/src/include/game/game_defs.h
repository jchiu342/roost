//
// Created by Jeremy on 11/23/2021.
//

#ifndef ROOST_GAME_DEFS_H
#define ROOST_GAME_DEFS_H

#define BOARD_SIZE 9
#define GAME_HISTORY_LEN 8
#define MAX_GAME_LENGTH (2 * BOARD_SIZE * BOARD_SIZE + 1)

namespace game {

// this allows us to quickly set empty boards via memset()
constexpr int neighbors[4][2] = {{-1, 0}, {1, 0}, {0, 1}, {0, -1}};
enum Color {EMPTY = 0, BLACK = 1, WHITE = -1};

};

#endif //ROOST_GAME_DEFS_H
