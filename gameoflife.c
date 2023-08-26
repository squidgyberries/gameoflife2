#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

size_t width, height, size, length;
uint8_t *activeboard;
uint8_t *otherboard;

bool board_read(uint8_t *board, size_t index) {
  size_t i = index / 8;
  uint8_t mask = 1 << (index - i * 8);
  return board[i] & mask;
}

void board_write(uint8_t *board, size_t index, bool val) {
  size_t i = index / 8;
  size_t rem = index - i * 8;
  uint8_t mask = 1 << rem;
  board[i] += (val - ((board[i] & mask) > 0)) * mask;
}

static inline bool board_get(uint8_t *board, size_t x, size_t y) {
  return board_read(board, y * width + x);
}

static inline void board_set(uint8_t *board, size_t x, size_t y, bool val) {
  board_write(board, y * width + x, val);
}

// todo: write directly with uint8_t
void board_load(uint8_t *board, FILE *fp) {
  char c;
  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      c = fgetc(fp);
      if (c == EOF) {
        return;
      } else if (c == '\n') {
        break;
      } else if (c == '1') {
        board_set(board, x, y, true);
      }
    }
    while (c != '\n') {
      c = fgetc(fp);
      if (c == EOF)
        return;
    }
  }
}

void board_print(uint8_t *board) {
  fputs("\033[2J", stdout);
  fputs("\033[H", stdout);
  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      if (board_get(board, x, y)) {
        fputs("\033[47m", stdout);
        for (; board_get(board, x, y) && x < width; ++x)
          fputs("  ", stdout);
        fputs("\033[0m", stdout);
      }
      fputs("  ", stdout);
    }
    fputc('\n', stdout);
  }
}

void board_update() {
  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      int neighbours =
        ((y > 0)                       && board_get(activeboard, x, y-1)) +
        ((x < width-1 && y > 0)        && board_get(activeboard, x+1, y-1)) +
        ((x < width-1)                 && board_get(activeboard, x+1, y)) +
        ((x < width-1 && y < height-1) && board_get(activeboard, x+1, y+1)) + 
        ((y < height-1)                && board_get(activeboard, x, y+1)) +
        ((x > 0 && y < height-1)       && board_get(activeboard, x-1, y+1)) +
        ((x > 0)                       && board_get(activeboard, x-1, y)) +
        ((x > 0 && y > 0)              && board_get(activeboard, x-1, y-1));
      if (board_get(activeboard, x, y)) {
        board_set(otherboard, x, y, neighbours == 2 || neighbours == 3);
      } else {
        board_set(otherboard, x, y, neighbours == 3);
      }
    }
  }
  uint8_t *tempboard = otherboard;
  otherboard = activeboard;
  activeboard = tempboard;
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    fprintf(stderr, "usage: %s <width> <height> <filename>\n", argv[0]);
    return 1;
  }
  width = strtol(argv[1], NULL, 10);
  height = strtol(argv[2], NULL, 10);
  size = width * height;
  length = size / 8 + (size % 8 > 0);
  uint8_t *board1 = malloc(sizeof(uint8_t) * length);
  uint8_t *board2 = malloc(sizeof(uint8_t) * length);
  activeboard = board1;
  otherboard = board2;

  FILE *fp = fopen(argv[3], "r");
  if (!fp) {
    fprintf(stderr, "failed to open file %s\n", argv[3]);
    return 1;
  }
  board_load(activeboard, fp);
  fclose(fp);
  uint64_t tick = 0;
  while (true) {
    board_print(activeboard);
    printf("tick: %lu", tick++);
    fflush(stdout);
    for (char c; (c = fgetc(stdin)) != '\n';)
      if (c == 'q')
        return 0;
    board_update();
  }
  return 0;
}
