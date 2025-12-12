#define MINIRV32_IMPLEMENTATION
#include "uvm32.h"

#ifndef UVM32_MEMORY_SIZE
#error Define UVM32_MEMORY_SIZE
#endif

#ifndef CUSTOM_STDLIB_H
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#else
#include CUSTOM_STDLIB_H
#endif

// On an invalid operation, an error is set in uvm32_state_t, but a valid pointer still needs to be temporarily used
static uint32_t garbage;

// magic value for stack canary
#define STACK_CANARY_VALUE 0x42

#ifndef UVM32_MEMCPY
#define UVM32_MEMCPY uvm32_memcpy
void uvm32_memcpy(void *dst, const void *src, int len) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while(len--) {
        *(d++) = *(s++);
    }
}
#endif

#ifndef UVM32_MEMSET
#define UVM32_MEMSET uvm32_memset
void uvm32_memset(void *buf, int c, int len) {
    uint8_t *b = (uint8_t *)buf;
    while(len--) {
        *(b++) = c;
    }
}
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
    // if already in error state, stay in the first error state
    if (vmst->status != UVM32_STATUS_ERROR) {
        setStatus(vmst, UVM32_STATUS_ERROR);
        vmst->err = err;
    }
}

void uvm32_init(uvm32_state_t *vmst) {
    UVM32_MEMSET(vmst, 0x00, sizeof(uvm32_state_t));
    vmst->status = UVM32_STATUS_PAUSED;

    vmst->extramLen = 0;
    vmst->extram = (uint32_t *)NULL;
    vmst->extramDirty = false;

    vmst->core.pc = MINIRV32_RAM_IMAGE_OFFSET;
    // https://projectf.io/posts/riscv-cheat-sheet/
    // setup stack pointer
    // la	sp, _sstack
    // addi	sp,sp,-16
    vmst->core.regs[2] = ((MINIRV32_RAM_IMAGE_OFFSET + UVM32_MEMORY_SIZE) & ~0xF) - 16; // 16 byte align stack
    vmst->core.regs[10] = 0x00;  //hart ID
    vmst->core.regs[11] = 0;
    vmst->core.extraflags |= 3;  // Machine-mode.
}

bool uvm32_load(uvm32_state_t *vmst, const uint8_t *rom, int len) {
    if (len > UVM32_MEMORY_SIZE) {
        // too big
        return false;
    }

    UVM32_MEMCPY(vmst->memory, rom, len);
    vmst->stack_canary = (uint8_t *)NULL;
    
    return true;
}

// Read C-string up to terminator and return len,ptr
bool get_safeptr_null_terminated(uvm32_state_t *vmst, uint32_t addr, uvm32_evt_syscall_buf_t *buf) {
    if (MINIRV32_MMIO_RANGE(addr)) {
        if (vmst->extram == NULL) {
            return false;
        } else {
            
            uint32_t ptrstart = addr - UVM32_EXTRAM_BASE;;
            uint32_t p = ptrstart;
            if (p >= vmst->extramLen) {
                setStatusErr(vmst, UVM32_ERR_MEM_RD);
                buf->ptr = (uint8_t *)NULL;
                buf->len = 0;
                return false;
            }
            while(vmst->memory[p] != '\0') {
                p++;
                if (p >= vmst->extramLen) {
                    setStatusErr(vmst, UVM32_ERR_MEM_RD);
                    buf->ptr = (uint8_t *)NULL;
                    buf->len = 0;
                    return false;
                }
            }
            buf->ptr = (uint8_t *)&vmst->extram[ptrstart/4];    // extram is uint32_t*
            buf->len = p - ptrstart;
            return true;
        }
    } else {
        uint32_t ptrstart = addr - MINIRV32_RAM_IMAGE_OFFSET;
        uint32_t p = ptrstart;
        if (p >= UVM32_MEMORY_SIZE) {
            setStatusErr(vmst, UVM32_ERR_MEM_RD);
            buf->ptr = (uint8_t *)NULL;
            buf->len = 0;
            return false;
        }
        while(vmst->memory[p] != '\0') {
            p++;
            if (p >= UVM32_MEMORY_SIZE) {
                setStatusErr(vmst, UVM32_ERR_MEM_RD);
                buf->ptr = (uint8_t *)NULL;
                buf->len = 0;
                return false;
            }
        }
        buf->ptr = &vmst->memory[ptrstart];
        buf->len = p - ptrstart;
        return true;
    }
}

bool get_safeptr(uvm32_state_t *vmst, uint32_t addr, uint32_t len, uvm32_evt_syscall_buf_t *buf) {
    if (MINIRV32_MMIO_RANGE(addr)) {
        if (vmst->extram == NULL) {
            return false;
        } else {
            uint32_t ptrstart = addr - UVM32_EXTRAM_BASE;
            if ((ptrstart > vmst->extramLen) || (ptrstart + len >= vmst->extramLen)) {
                setStatusErr(vmst, UVM32_ERR_MEM_RD);
                buf->ptr = (uint8_t *)NULL;
                buf->len = 0;
                return false;
            }
            buf->ptr = (uint8_t *)&vmst->extram[ptrstart/4];    // extram is uint32_t*
            buf->len = len;
            return true;
        }
    } else {
        uint32_t ptrstart = addr - MINIRV32_RAM_IMAGE_OFFSET;
        if ((ptrstart > UVM32_MEMORY_SIZE) || (ptrstart + len >= UVM32_MEMORY_SIZE)) {
            setStatusErr(vmst, UVM32_ERR_MEM_RD);
            buf->ptr = (uint8_t *)NULL;
            buf->len = 0;
            return false;
        }
        buf->ptr = &vmst->memory[ptrstart];
        buf->len = len;
        return true;
    }
}

void uvm32_clearError(uvm32_state_t *vmst) {
    if (vmst->status == UVM32_STATUS_ERROR) {
        // vm is in an error state, but user wants to continue
        // most likely after UVM32_ERR_HUNG
        vmst->status = UVM32_STATUS_PAUSED;
    }
}

uint32_t uvm32_run(uvm32_state_t *vmst, uvm32_evt_t *evt, uint32_t instr_meter) {
    const uint32_t min_instrs = 1;
    uint32_t orig_instr_meter = instr_meter;

    vmst->extramDirty = false;

    if (instr_meter < min_instrs) {
        instr_meter = min_instrs;
        orig_instr_meter = min_instrs;
    }

    if (vmst->stack_canary != NULL && *vmst->stack_canary != STACK_CANARY_VALUE) {
        setStatusErr(vmst, UVM32_ERR_INTERNAL_CORE);
        setup_err_evt(vmst, evt);
        return orig_instr_meter - instr_meter;
    }

    if (vmst->status != UVM32_STATUS_PAUSED) {
        setStatusErr(vmst, UVM32_ERR_NOTREADY);
        setup_err_evt(vmst, evt);
        return orig_instr_meter - instr_meter;
    }

    setStatus(vmst, UVM32_STATUS_RUNNING);

    // run CPU until no longer in running state
    while(vmst->status == UVM32_STATUS_RUNNING && instr_meter > 0) {
        uint64_t elapsedUs = 1;
        uint32_t ret;
        ret = MiniRV32IMAStep(vmst, &vmst->core, vmst->memory, 0, elapsedUs, 1);
        instr_meter--;
    
        switch(ret) {
            case 0:  // ok
            break;
            case 12: { // ecall
                // Fetch registers used by syscall
                const uint32_t syscall = vmst->core.regs[17];  // a7
                // on exception we should jump to mtvec, but we handle directly
                // and skip over the ecall instruction
                vmst->core.pc += 4;
                switch(syscall) {
                    // inbuilt syscalls
                     case UVM32_SYSCALL_HALT:
                        setStatus(vmst, UVM32_STATUS_ENDED);
                    break;
                    case UVM32_SYSCALL_STACKPROTECT: {
                        // don't allow errant code to change it once set
                        if (vmst->stack_canary == (uint8_t *)NULL) {
                            uint32_t param0 = vmst->core.regs[10]; // a0
                            uint32_t mem_offset = param0 - MINIRV32_RAM_IMAGE_OFFSET;
                            mem_offset &= ~0xF; // round up by 16 bytes
                            mem_offset += 16*4;

                            // check data fits in ram
                            if (mem_offset > UVM32_MEMORY_SIZE) {

                                setStatusErr(vmst, UVM32_ERR_INTERNAL_CORE);
                                setup_err_evt(vmst, evt);
                            }
                            // check canary is inside valid memory
                            if (mem_offset < UVM32_MEMORY_SIZE) {
                                // set canary
                                vmst->stack_canary = &vmst->memory[mem_offset];
                                *vmst->stack_canary = STACK_CANARY_VALUE;
                            }
                        }
                    } break;
                    default:
                        // user defined syscalls
                        vmst->ioevt.typ = UVM32_EVT_SYSCALL;
                        vmst->ioevt.data.syscall.code = syscall;
                        vmst->ioevt.data.syscall.ret = &vmst->core.regs[12];        // a2
                        vmst->ioevt.data.syscall.params[0] = &vmst->core.regs[10];  // a0
                        vmst->ioevt.data.syscall.params[1] = &vmst->core.regs[11];  // a1
                        setStatus(vmst, UVM32_STATUS_PAUSED);
                    break;
                }   // end switch(syscall)
            } break; // end ecall
            case 6:
                setStatusErr(vmst, UVM32_ERR_MEM_RD);
                setup_err_evt(vmst, evt);
            break;
            default:
                // unhandled exception
                setStatusErr(vmst, UVM32_ERR_INTERNAL_CORE);
                setup_err_evt(vmst, evt);
            break;
        }

        if (vmst->status == UVM32_STATUS_RUNNING && instr_meter == 0) {
            // no syscall occurred, so we've hung
            setStatusErr(vmst, UVM32_ERR_HUNG);
            setup_err_evt(vmst, evt);
            return orig_instr_meter - instr_meter;
        }
    }


    if (vmst->status == UVM32_STATUS_ENDED) {
        evt->typ = UVM32_EVT_END;
        return orig_instr_meter - instr_meter;
    }

    // an event is ready
    if (vmst->status == UVM32_STATUS_PAUSED) {
        // send back the built up event
        UVM32_MEMCPY(evt, &vmst->ioevt, sizeof(uvm32_evt_t));
        return orig_instr_meter - instr_meter;
    } else {
        if (vmst->status == UVM32_STATUS_ERROR) {
            setup_err_evt(vmst, evt);
        } else {
            setStatusErr(vmst, UVM32_ERR_INTERNAL_STATE);
            setup_err_evt(vmst, evt);
        }
        return orig_instr_meter - instr_meter;
    }
}

bool uvm32_hasEnded(const uvm32_state_t *vmst) {
    return vmst->status == UVM32_STATUS_ENDED;
}

static uint32_t *arg_to_ptr(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t arg) {
    switch(arg) {
        case ARG0:
            return evt->data.syscall.params[0];
        break;
        case ARG1:
            return evt->data.syscall.params[1];
        break;
        case RET:
            return evt->data.syscall.ret;
        break;
        default:
            // if something invalid is passed to arg, we should never crash
            // return a pointer to something readable
            setStatusErr(vmst, UVM32_ERR_ARGS);
            garbage = 0;
            return &garbage;
        break;
    }
}

void uvm32_setval(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t arg, uint32_t val) {
    *arg_to_ptr(vmst, evt, arg) = val;
}

uint32_t uvm32_getval(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t arg) {
    return *arg_to_ptr(vmst, evt, arg);
}

const char *uvm32_getcstr(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t arg) {
    uvm32_evt_syscall_buf_t scb;
    if (get_safeptr_null_terminated(vmst, uvm32_getval(vmst, evt, arg), &scb)) {
        return (const char *)scb.ptr; // we know the buffer in cpu memory is null terminated, so safe to pass back
    } else {
        setStatusErr(vmst, UVM32_ERR_MEM_RD);
        garbage = 0;
        return (const char *)&garbage;
    }
}

uvm32_evt_syscall_buf_t uvm32_getbuf(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t argPtr, uvm32_arg_t argLen) {
    uvm32_evt_syscall_buf_t scb;
    if (!get_safeptr(vmst, uvm32_getval(vmst, evt, argPtr), uvm32_getval(vmst, evt, argLen), &scb)) {
        setStatusErr(vmst, UVM32_ERR_MEM_RD);
        garbage = 0;
        scb.ptr = (uint8_t *)&garbage;
        scb.len = 0;
    }
    return scb;
}

uvm32_evt_syscall_buf_t uvm32_getbuf_fixed(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t argPtr, uint32_t len) {
    uvm32_evt_syscall_buf_t scb;
    if (!get_safeptr(vmst, uvm32_getval(vmst, evt, argPtr), len, &scb)) {
        setStatusErr(vmst, UVM32_ERR_MEM_RD);
        garbage = 0;
        scb.ptr = (uint8_t *)&garbage;
        scb.len = 0;
    }
    return scb;
}

uint32_t uvm32_extramLoad(void *userdata, uint32_t addr, uint32_t accessTyp) {

    uvm32_state_t *vmst = (uvm32_state_t *)userdata;
    addr -= UVM32_EXTRAM_BASE;

    if (vmst->extram != NULL) {
        if (addr < vmst->extramLen) {
            switch(accessTyp) {
                case 0:
                    return ((int8_t *)vmst->extram)[addr];
                break;
                case 1:
                    return ((int16_t *)vmst->extram)[addr/2];
                break;
                case 2:
                    return ((uint32_t *)vmst->extram)[addr / 4];
                break;
                case 4:
                    return ((uint8_t *)vmst->extram)[addr];
                break;
                case 5:
                    return ((uint16_t *)vmst->extram)[addr/2];
                break;
                default:
                    setStatusErr(vmst, UVM32_ERR_MEM_RD);
                    return 0;
                break;
            }
        } else {
            setStatusErr(vmst, UVM32_ERR_MEM_RD);
        }
    }
    return 0;
}

uint32_t uvm32_extramStore(void *userdata, uint32_t addr, uint32_t val, uint32_t accessTyp) {
    uvm32_state_t *vmst = (uvm32_state_t *)userdata;
    addr -= UVM32_EXTRAM_BASE;
    if (vmst->extram != NULL) {
        if (addr < vmst->extramLen) {
            switch(accessTyp) {
                case 0:
                    ((uint8_t *)vmst->extram)[addr] = val;
                break;
                case 1:
                    ((uint16_t *)vmst->extram)[addr/2] = val;
                break;
                case 2:
                    ((uint32_t *)vmst->extram)[addr/4] = val;
                break;
                default:
                    setStatusErr(vmst, UVM32_ERR_MEM_WR);
                break;
            }
            vmst->extramDirty = true;
        } else {
            setStatusErr(vmst, UVM32_ERR_MEM_WR);
        }
    }
	return 0;
}

void uvm32_extram(uvm32_state_t *vmst, uint32_t *ram, uint32_t len) {
    vmst->extram = ram;
    vmst->extramLen = len;
}

bool uvm32_extramDirty(uvm32_state_t *vmst) {
    return vmst->extramDirty;
}


