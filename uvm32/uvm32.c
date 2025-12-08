#define MINIRV32_IMPLEMENTATION
#include "uvm32.h"
#include <stdio.h>
#include <string.h>

#ifndef UVM32_MEMORY_SIZE
#error Define UVM32_MEMORY_SIZE
#endif

#include "mini-rv32ima.h"

#define X(name) #name,
static const char *errNames[] = {
    LIST_OF_UVM32_ERRS
};
#undef X

static void setup_err_evt(uvm32_state_t *vmst, uvm32_evt_t *evt) {
    evt->typ = UVM32_EVT_ERR;
    evt->data.err.errcode = vmst->err;
    evt->data.err.errstr = errNames[vmst->err];
}

static void setStatus(uvm32_state_t *vmst, uvm32_status_t newStatus) {
    if (vmst->status == UVM32_STATUS_ERROR) {
        // always stay in error state until a uvm32_init()
        return;
    } else {
        vmst->status = newStatus;
    }
}

static void setStatusErr(uvm32_state_t *vmst, uvm32_err_t err) {
    setStatus(vmst, UVM32_STATUS_ERROR);
    vmst->err = err;
}

void uvm32_init(uvm32_state_t *vmst, const uvm32_mapping_t *mappings, uint32_t numMappings) {
    vmst->status = UVM32_STATUS_PAUSED;

    memset(vmst->memory, 0x00, UVM32_MEMORY_SIZE);
    // The core lives at the end of RAM.
    vmst->core = (struct MiniRV32IMAState*)(vmst->memory + UVM32_MEMORY_SIZE - sizeof(struct MiniRV32IMAState));
    vmst->core->pc = MINIRV32_RAM_IMAGE_OFFSET;
    // https://projectf.io/posts/riscv-cheat-sheet/
    // setup stack pointer
    // la	sp, _sstack
    // addi	sp,sp,-16
    vmst->core->regs[2] = (MINIRV32_RAM_IMAGE_OFFSET + UVM32_MEMORY_SIZE - sizeof(struct MiniRV32IMAState)) - 16;
    vmst->core->regs[10] = 0x00;  //hart ID
    vmst->core->regs[11] = 0;
    vmst->core->extraflags |= 3;  // Machine-mode.

    vmst->mappings = mappings;
    vmst->numMappings = numMappings;
}

bool uvm32_load(uvm32_state_t *vmst, uint8_t *rom, int len) {
    // RAM needs at least image then MiniRV32IMAState (core)
    if (len > UVM32_MEMORY_SIZE - sizeof(struct MiniRV32IMAState)) {
        // too big
        return false;
    }

    memcpy(vmst->memory, rom, len);
    return true;
}

// Read C-string up to terminator and return len,ptr
static void get_safeptr_terminated(uvm32_state_t *vmst, uint32_t addr, uint8_t terminator, uvm32_evt_syscall_buf_t *buf) {
    uint32_t ptrstart = addr - MINIRV32_RAM_IMAGE_OFFSET;
    uint32_t p = ptrstart;
    if (p >= UVM32_MEMORY_SIZE) {
        setStatusErr(vmst, UVM32_ERR_MEM_RD);
        buf->ptr = NULL;
        buf->len = 0;
        return;
    }
    while(vmst->memory[p] != terminator) {
        p++;
        if (p >= UVM32_MEMORY_SIZE) {
            setStatusErr(vmst, UVM32_ERR_MEM_RD);
            buf->ptr = NULL;
            buf->len = 0;
            return;
        }
    }
    buf->ptr = &vmst->memory[ptrstart];
    buf->len = p - ptrstart;
}

#if 0
static void get_safeptr(uvm32_state_t *vmst, uint32_t addr, uint32_t len, uvm32_evt_syscall_buf_t *buf) {
    uint32_t ptrstart = addr - MINIRV32_RAM_IMAGE_OFFSET;
    if (ptrstart + len >= UVM32_MEMORY_SIZE) {
        setStatusErr(vmst, UVM32_ERR_MEM_RD);
    }
    buf->ptr = &vmst->memory[ptrstart];
    buf->len = len;
}
#endif


uint32_t uvm32_run(uvm32_state_t *vmst, uvm32_evt_t *evt, uint32_t instr_meter) {
    uint32_t num_instr = 0;
//    uvm32_evt_syscall_buf_t b;

    if (vmst->status != UVM32_STATUS_PAUSED) {
        setStatusErr(vmst, UVM32_ERR_NOTREADY);
        setup_err_evt(vmst, evt);
        return num_instr;
    }

    setStatus(vmst, UVM32_STATUS_RUNNING);

    // run CPU until no longer in running state
    while(vmst->status == UVM32_STATUS_RUNNING) {
        uint64_t elapsedUs = 1;
        uint32_t ret;
        ret = MiniRV32IMAStep(vmst, vmst->core, vmst->memory, 0, elapsedUs, 1);
        if (3 == ret) {
            const uint32_t syscall = vmst->core->regs[17];  // a7
            uint32_t value = vmst->core->regs[10]; // a0
            bool syscall_valid = false;
            // on exception we should jump to mtvec, but we handle directly
            // and skip over the ecall instruction
            vmst->core->pc += 4;
            switch(syscall) {
                // inbuilt syscalls
                 case UVM32_SYSCALL_HALT:
                    setStatus(vmst, UVM32_STATUS_ENDED);
                    syscall_valid = true;
                break;
                case UVM32_SYSCALL_YIELD:
                    vmst->ioevt.typ = UVM32_EVT_YIELD;
                    setStatus(vmst, UVM32_STATUS_PAUSED);
                    syscall_valid = true;
                break;

                // user defined syscalls
                default:
                    // search in mappings
                    for (int i=0;i<vmst->numMappings;i++) {
                        if (syscall == vmst->mappings[i].syscall) {
                            // setup ioevt.data according to mapping typ
                            switch(vmst->mappings[i].typ) {
                                case UVM32_SYSCALL_TYP_VOID:
                                break;
                                case UVM32_SYSCALL_TYP_U32_WR:
                                    vmst->ioevt.data.syscall.val.u32 = value;
                                break;
                                case UVM32_SYSCALL_TYP_BUF_TERMINATED_WR:
                                    get_safeptr_terminated(vmst, value, 0x00, &vmst->ioevt.data.syscall.val.buf);
                                break;
                                case UVM32_SYSCALL_TYP_U32_RD:
                                    // pass link to r1 for user function to update
                                    vmst->ioevt.data.syscall.val.u32p = &vmst->core->regs[11];
                                break;
                            }
                            vmst->ioevt.typ = UVM32_EVT_UVM32_SYSCALL;
                            vmst->ioevt.data.syscall.code = vmst->mappings[i].code;
                            vmst->ioevt.data.syscall.typ = vmst->mappings[i].typ;
                            setStatus(vmst, UVM32_STATUS_PAUSED);
                            syscall_valid = true;
                            break;  // stop searching
                        }
                    }
                    // no mapping found
                    if (!syscall_valid) {
                        setStatusErr(vmst, UVM32_ERR_BAD_SYSCALL);
                    }
                break;
            }
        } else if (ret != 0) {
            // unhandled exception
            setStatusErr(vmst, UVM32_ERR_INTERNAL_CORE);
            setup_err_evt(vmst, evt);
        }

        num_instr++;

        // check instruction meter, in case of hang/infinite loop
        if (instr_meter-- == 0) {
            setStatusErr(vmst, UVM32_ERR_HUNG);
            setup_err_evt(vmst, evt);
            return num_instr;
        }
    }

    if (vmst->status == UVM32_STATUS_ENDED) {
        evt->typ = UVM32_EVT_END;
        return num_instr;
    }

    // an event is ready
    if (vmst->status == UVM32_STATUS_PAUSED) {
        // send back the built up event
        memcpy(evt, &vmst->ioevt, sizeof(uvm32_evt_t));
        return num_instr;
    } else {
        if (vmst->status == UVM32_STATUS_ERROR) {
            setup_err_evt(vmst, evt);
        } else {
            setStatusErr(vmst, UVM32_ERR_INTERNAL_STATE);
            setup_err_evt(vmst, evt);
        }
        return num_instr;
    }
}

bool uvm32_hasEnded(const uvm32_state_t *vmst) {
    return vmst->status == UVM32_STATUS_ENDED;
}


