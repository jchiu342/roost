import pygame
import time
from macros import *

halfBoxSize = 20
boxSize = 2 * halfBoxSize
black = (0, 0, 0)
white = (255, 255, 255)
red = (255, 0, 0)
background = (244, 148, 25)


class GameViewer:
    def __init__(self, board_size=15):
        self.board_size = board_size
        self.screen = None

    def _init_render(self):
        self.screen.fill(background)
        start = halfBoxSize
        end = (2 * self.board_size - 1) * halfBoxSize
        for a in range(self.board_size):
            coord = (2 * a + 1) * halfBoxSize
            pygame.draw.line(self.screen, black, (coord, start), (coord, end))
            pygame.draw.line(self.screen, black, (start, coord), (end, coord))
        pygame.display.flip()

    def _put_stone(self, color, x, y):
        coord = ((2 * x + 1) * halfBoxSize, (2 * y + 1) * halfBoxSize)
        if color == WHITE:
            pygame.draw.circle(self.screen, white, coord, halfBoxSize - 1)
        elif color == BLACK:
            pygame.draw.circle(self.screen, black, coord, halfBoxSize - 1)

    def _mark_square(self, x, y):
        coord = ((2 * x + 1) * halfBoxSize, (2 * y + 1) * halfBoxSize)
        pygame.draw.circle(self.screen, red, coord, halfBoxSize / 2)

    def start(self):
        pygame.init()
        self.screen = pygame.display.set_mode((self.board_size * boxSize,
                                               self.board_size * boxSize))
        self._init_render()

    def update_board(self, board, flip=True):
        pygame.event.pump()
        self._init_render()
        for x in range(self.board_size):
            for y in range(self.board_size):
                self._put_stone(board[x][y], x, y)
        if flip:
            pygame.display.flip()

    def update_from_state(self, state):
        self.update_board(state.board, flip=False)
        if state.most_recent_action is not None:
            color, type, x, y = state.most_recent_action.get_info()
            if type == PLAY:
                self._mark_square(x, y)
        pygame.display.flip()


if __name__ == "__main__":
    board = GameViewer()
    board.start()
    board.update_render(BLACK, 3, 4)
    board.update_render(WHITE, 3, 3)
    time.sleep(2)
