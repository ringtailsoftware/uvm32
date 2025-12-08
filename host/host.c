#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <termios.h>
#include "uvm32.h"

#include "../common/uvm32_common_custom.h"

// stash terminal settings on startup
static struct termios orig_termios;

// syscalls exposed to vm environement
typedef enum {
    F_PRINT,
    F_PRINTDEC,
    F_PRINTHEX,
    F_PUTC,
    F_PRINTLN,
    F_MILLIS,
    F_GETC,
} f_code_t;

// Map exposed syscalls to syscalls
const uvm32_mapping_t env[] = {
    { .syscall = UVM32_SYSCALL_PRINTLN, .typ = UVM32_SYSCALL_TYP_BUF_TERMINATED_WR, .code = F_PRINTLN },
    { .syscall = UVM32_SYSCALL_PRINT, .typ = UVM32_SYSCALL_TYP_BUF_TERMINATED_WR, .code = F_PRINT },
    { .syscall = UVM32_SYSCALL_PRINTDEC, .typ = UVM32_SYSCALL_TYP_U32_WR, .code = F_PRINTDEC },
    { .syscall = UVM32_SYSCALL_PRINTHEX, .typ = UVM32_SYSCALL_TYP_U32_WR, .code = F_PRINTHEX },
    { .syscall = UVM32_SYSCALL_PUTC, .typ = UVM32_SYSCALL_TYP_U32_WR, .code = F_PUTC },
    { .syscall = UVM32_SYSCALL_MILLIS, .typ = UVM32_SYSCALL_TYP_U32_RD, .code = F_MILLIS },
    { .syscall = UVM32_SYSCALL_GETC, .typ = UVM32_SYSCALL_TYP_U32_RD, .code = F_GETC },
};

void disableRawMode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_cflag |= (CS8);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
}

static uint8_t *read_file(const char* filename, int *len) {
    FILE* f = fopen(filename, "rb");
    uint8_t *buf = NULL;

    if (f == NULL) {
        fprintf(stderr, "error: can't open file '%s'.\n", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    rewind(f);

    if (NULL == (buf = malloc(file_size))) {
        fclose(f);
        return NULL;
    }

    size_t result = fread(buf, sizeof(uint8_t), file_size, f);
    if (result != file_size) {
        fprintf(stderr, "error: while reading file '%s'\n", filename);
        free(buf);
        fclose(f);
        return NULL;
    }

    *len = file_size;
    return buf;
}

// Poll for input, inelegant, but works and doesn't confuse the main
// flow of the program with select/poll
bool poll_getch(uint8_t* c) {
    struct timeval tv;
    fd_set readfds;
    int ret;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

    if (ret == -1) {
        printf("console read failed\r\n");
        exit(1);
    } else if (!ret) {
        // timeout
        return false;
    }

    if (FD_ISSET(STDIN_FILENO, &readfds)) {
        int len;

        len = read(STDIN_FILENO, c, 1);
        if (len == -1) {
            printf("console read failed\r\n");
            exit(1);
        }

        if (len) {
            if (*c == 0x03 || *c == 0x04) {   // ctrl-c ctrl-d
                exit(0);
            }
            if (*c == 0x0d) {
                *c = '\n';
            }
            return true;
        }
    }
    return false;
}

void hexdump(const uint8_t *p, int len) {
    while(len--) {
        printf("%02x", *p++);
    }
}

int main(int argc, char *argv[]) {
    uvm32_state_t vmst;
    clock_t start_time = clock() / (CLOCKS_PER_SEC / 1000);

    argc--;
    argv++;

    if (argc < 1) {
        printf("<romfile>\n");
        return 1;
    }

    int romlen = 0;
    uint8_t *rom = read_file(argv[0], &romlen);
    if (NULL == rom) {
        printf("file read failed!\n");
        return 1;
    }

    uvm32_init(&vmst, env, sizeof(env) / sizeof(env[0]));
    if (!uvm32_load(&vmst, rom, romlen)) {
        printf("load failed!\n");
        return 1;
    }

    uvm32_evt_t evt;
    bool isrunning = true;
    uint32_t total_instrs = 0;
    uint32_t num_syscalls = 0;

    // setup terminal for getch()
    enableRawMode();

    while(isrunning) {
        total_instrs += uvm32_run(&vmst, &evt, 1000000);   // num instructions before vm considered hung
        num_syscalls++;

        switch(evt.typ) {
            case UVM32_EVT_END:
                printf("UVM32_EVT_END\n");
                isrunning = false;
            break;
            case UVM32_EVT_YIELD:
                //printf("UVM32_EVT_YIELD\n");
                // program has paused, but no syscall
            break;
            case UVM32_EVT_ERR:
                printf("UVM32_EVT_ERR '%s' (%d)\n", evt.data.err.errstr, (int)evt.data.err.errcode);
                isrunning = false;
            break;
            case UVM32_EVT_SYSCALL:
                switch((f_code_t)evt.data.syscall.code) {
                    case F_PRINT:
                        printf("%.*s", evt.data.syscall.val.buf.len, evt.data.syscall.val.buf.ptr);
                    break;
                    case F_PRINTLN:
                        printf("%.*s\n", evt.data.syscall.val.buf.len, evt.data.syscall.val.buf.ptr);
                    break;
                    case F_PRINTDEC:
                        printf("%d\n", evt.data.syscall.val.u32);
                    break;
                    case F_PUTC:
                        printf("%c", evt.data.syscall.val.u32);
                    break;
                    case F_PRINTHEX:
                        printf("%08x", evt.data.syscall.val.u32);
                    break;
                    case F_GETC: {
                        uint8_t ch;
                        if (poll_getch(&ch)) {
                            *evt.data.syscall.val.u32p = ch;
                        } else {
                            *evt.data.syscall.val.u32p = 0xFFFFFFFF;  // nothing
                        }
                    } break;
                    case F_MILLIS: {
                        clock_t now = clock() / (CLOCKS_PER_SEC / 1000);
                        *evt.data.syscall.val.u32p = now - start_time;
                    } break;
                    default:    // catch any others
                        switch(evt.data.syscall.typ) {
                            case UVM32_SYSCALL_TYP_BUF_TERMINATED_WR:
                                printf("UVM32_SYSCALL_TYP_BUF_TERMINATED_WR code=%d val=", evt.data.syscall.code);
                                hexdump(evt.data.syscall.val.buf.ptr, evt.data.syscall.val.buf.len);
                                printf("\n");
                            break;
                            case UVM32_SYSCALL_TYP_VOID:
                                printf("UVM32_SYSCALL_TYP_VOID code=%d\n", evt.data.syscall.code);
                            break;
                            case UVM32_SYSCALL_TYP_U32_WR:
                                printf("UVM32_SYSCALL_TYP_U32_WR code=%d val=%d (0x%08x)\n", evt.data.syscall.code, evt.data.syscall.val.u32, evt.data.syscall.val.u32);
                            break;
                            case UVM32_SYSCALL_TYP_U32_RD:
                                printf("UVM32_SYSCALL_TYP_U32_RD code=%d\n", evt.data.syscall.code);
                                *evt.data.syscall.val.u32p = 123456;
                            break;
                        }
                    break;
                }
            break;
            default:
                printf("Bad evt %d\n", evt.typ);
                return 1;
            break;
        }
        fflush(stdout);
    }

    printf("Executed total of %d instructions and %d syscalls\n", (int)total_instrs, (int)num_syscalls);

    free(rom);

    // put terminal back to how it was
    disableRawMode();
    return 0;
}
