#include "uvm32_target.h"

// This example generates a random maze using the recursive backtracking algorithm.
// https://github.com/ccattuto/riscv-python/blob/main/tests/test_newlib_maze.c

#define WIDTH 79   // must be odd
#define HEIGHT 31  // must be odd

char maze[HEIGHT * WIDTH];

void* memcpy(void* dst, const void* src, int len) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    while (len--) {
        *(d++) = *(s++);
    }
    return dst;
}

void* memset(void* buf, int c, int len) {
    uint8_t* b = (uint8_t*)buf;
    while (len--) {
        *(b++) = c;
    }
    return buf;
}

int dx[] = {0, 1, 0, -1};
int dy[] = {-1, 0, 1, 0};

int seed = 123456789;

int rand() {
    uint32_t a = 1103515245;
    uint32_t c = 12345;
    uint32_t m = 0xFFFFFFFF;
    seed = (a * seed + c) % m;
    return seed;
}

void init_maze() {
    memset(maze, '#', sizeof(maze));
}

int in_bounds(int x, int y) {
    return x > 0 && y > 0 && x < WIDTH - 1 && y < HEIGHT - 1;
}

void carve(int x, int y) {
    maze[y * WIDTH + x] = ' ';

    int dirs[] = {0, 1, 2, 3};
    // Fisher-Yates shuffle
    for (int i = 3; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = dirs[i];
        dirs[i] = dirs[j];
        dirs[j] = tmp;
    }

    for (int i = 0; i < 4; i++) {
        int nx = x + dx[dirs[i]] * 2;
        int ny = y + dy[dirs[i]] * 2;

        if (in_bounds(nx, ny) && maze[ny * WIDTH + nx] == '#') {
            maze[(y + dy[dirs[i]]) * WIDTH + (x + dx[dirs[i]])] = ' ';
            carve(nx, ny);
        }
    }
}

void print_maze() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            putc(maze[y * WIDTH + x]);
        }
        putc('\n');
    }
}


void main(void) {
    init_maze();
    carve(1, 1);
    print_maze();
}
