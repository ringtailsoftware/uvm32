#ifndef UVM32_H
#define UVM32_H 1

#include <stdint.h>
#include <stdbool.h>
#include "common/uvm32_sys.h"

#define LIST_OF_UVM32_ERRS \
    X(UVM32_ERR_NONE) \
    X(UVM32_ERR_NOTREADY) \
    X(UVM32_ERR_MEM_RD) \
    X(UVM32_ERR_MEM_WR) \
    X(UVM32_ERR_BAD_SYSCALL) \
    X(UVM32_ERR_HUNG) \
    X(UVM32_ERR_INTERNAL_CORE) \
    X(UVM32_ERR_INTERNAL_STATE) \
    X(UVM32_ERR_ARGS) \

#define X(name) name,
typedef enum {
  LIST_OF_UVM32_ERRS
} uvm32_err_t;
#undef X

typedef enum {
    UVM32_EVT_ERR,
    UVM32_EVT_SYSCALL,
    UVM32_EVT_YIELD,
    UVM32_EVT_END,
} uvm32_evt_typ_t;

typedef struct {
    uvm32_err_t errcode;
    const char *errstr;
} uvm32_evt_err_t;

typedef struct {
    uint32_t code;  // syscall number
    uint32_t *ret;
    uint32_t *params[2];
} uvm32_evt_syscall_t;

typedef struct {
    uvm32_evt_typ_t typ;
    union {
        uvm32_evt_syscall_t syscall;
        uvm32_evt_err_t err;
    } data;
} uvm32_evt_t;

#define MINIRV32_DECORATE static
#define MINI_RV32_RAM_SIZE UVM32_MEMORY_SIZE
#define MINIRV32_POSTEXEC(pc, ir, retval) {if (retval > 0) return 3;}
#ifndef MINIRV32_IMPLEMENTATION
#define MINIRV32_STEPPROTO
#endif
#include "mini-rv32ima.h"

typedef enum {
    UVM32_STATUS_PAUSED,
    UVM32_STATUS_RUNNING,
    UVM32_STATUS_ERROR,
    UVM32_STATUS_ENDED,
} uvm32_status_t;

typedef struct {
    uvm32_status_t status;
    uvm32_err_t err;
    struct MiniRV32IMAState core;
    uint8_t memory[UVM32_MEMORY_SIZE];
    uvm32_evt_t ioevt; // for building up in callbacks
} uvm32_state_t;

void uvm32_init(uvm32_state_t *vmst);
bool uvm32_load(uvm32_state_t *vmst, const uint8_t *rom, int len);
bool uvm32_hasEnded(const uvm32_state_t *vmst);
uint32_t uvm32_run(uvm32_state_t *vmst, uvm32_evt_t *evt, uint32_t instr_meter);

// convenience for getptr
typedef struct {
    uint8_t *ptr;
    uint32_t len;
} uvm32_evt_syscall_buf_t;

typedef enum {
    ARG0,
    ARG1,
    RET
} uvm32_arg_t;

uint32_t uvm32_getval(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t);
const char *uvm32_getcstr(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t);
void uvm32_setval(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t, uint32_t val);
uvm32_evt_syscall_buf_t uvm32_getbuf(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t argPtr, uvm32_arg_t argLen);
void uvm32_clearError(uvm32_state_t *vmst);


#endif
