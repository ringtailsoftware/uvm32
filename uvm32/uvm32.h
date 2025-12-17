/*!
https://github.com/ringtailsoftware/uvm32

MIT License

Copyright (c) 2025 Toby Jaffey <toby@ringtailsoftware.co.uk>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef UVM32_H
#define UVM32_H 1

/*! To use uvm32 in an environment without standard library headers, define CUSTOM_STDLIB_H to the name of header providing the equivalent types to stdint.h and stdbool.h */
#ifndef CUSTOM_STDLIB_H
#include <stdint.h>
#include <stdbool.h>
#else
#include CUSTOM_STDLIB_H
#endif

// Include definitions for required syscalls
#include "uvm32_sys.h"

// Setup and hooks for mini-rv32ima emulator core
#define MINIRV32_DECORATE static
#define MINIRV32_RETURN_TRAP
#define MINIRV32_NO_TIMERS_NO_CYCLES
#define MINIRV32_NO_ZICSR
#define MINIRV32_NO_ATOMICS
#define MINIRV32_NO_BREAKPOINT_NO_INTERRUPTS
#define MINI_RV32_RAM_SIZE UVM32_MEMORY_SIZE
#define MINIRV32_POSTEXEC(pc, ir, retval) {if (retval > 0) return retval;}
#define MINIRV32_HANDLE_MEM_LOAD_CONTROL( addy, rval ) rval = _uvm32_extramLoad(userdata, addy, ( ir >> 12 ) & 0x7);
#define MINIRV32_HANDLE_MEM_STORE_CONTROL( addy, val ) if( _uvm32_extramStore(userdata, addy, val, ( ir >> 12 ) & 0x7) ) return val;
#define MINIRV32_CUSTOM_MEMORY_BUS
#define MINIRV32_STORE4( ofs, val ) _uvm32_store4(image, ofs, val)
#define MINIRV32_STORE2( ofs, val ) _uvm32_store2(image, ofs, val)
#define MINIRV32_STORE1( ofs, val ) _uvm32_store1(image, ofs, val)
#define MINIRV32_LOAD4( ofs ) _uvm32_load4(image, ofs)
#define MINIRV32_LOAD2( ofs ) _uvm32_load2(image, ofs)
#define MINIRV32_LOAD1( ofs ) _uvm32_load1(image, ofs)
#define MINIRV32_LOAD2_SIGNED( ofs ) _uvm32_load2s(image, ofs)
#define MINIRV32_LOAD1_SIGNED( ofs ) _uvm32_load1s(image, ofs)
#ifndef MINIRV32_IMPLEMENTATION
#define MINIRV32_STEPPROTO
#else
static uint32_t _uvm32_extramLoad(void *userdata, uint32_t addr, uint32_t accessTyp);
static uint32_t _uvm32_extramStore(void *userdata, uint32_t addr, uint32_t val, uint32_t accessTyp);
static void _uvm32_store4(void *p, uint32_t off, uint32_t val);
static void _uvm32_store2(void *p, uint32_t off, uint16_t val);
static void _uvm32_store1(void *p, uint32_t off, uint8_t val);
static uint32_t _uvm32_load4(void *p, uint32_t off);
static uint16_t _uvm32_load2(void *p, uint32_t off);
static uint8_t _uvm32_load1(void *p, uint32_t off);
static int16_t _uvm32_load2s(void *p, uint32_t off);
static int8_t _uvm32_load1s(void *p, uint32_t off);
#endif

#define MINIRV32_COMPRESSED_INSTRUCTIONS
#ifdef MINIRV32_COMPRESSED_INSTRUCTIONS
#include <stdio.h>  // FIXME
#define MINIRV32_ALIGNMENT 1
#include "rv32c.h"
#else
#define MINIRV32_ALIGNMENT 3
#endif


#include "mini-rv32ima.h"

// Define all errors returned in a uvm32_evt_err_t
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

/*! Event types, giving the reason that the VM has stopped running */
typedef enum {
    UVM32_EVT_ERR,      /*! Error has occurred, details in uvm32_evt_t data.err field */
    UVM32_EVT_SYSCALL,  /*! A syscall has been requested, details in uvm32__evt_t data.syscall field */
    UVM32_EVT_END,      /*! The program has ended by making a UVM32_SYSCALL_HALT */
} uvm32_evt_typ_t;

/*! Details for an error event */
typedef struct {
    uvm32_err_t errcode; /*! Error code */
#ifdef UVM32_ERROR_STRINGS
    const char *errstr;  /*! Error as a string */
#endif
} uvm32_evt_err_t;

/*! Describes a syscall made from running code to the VM host
Typically, a host will switch on `code` to decide how to parse the syscall. Then must use the functions `uvm32_arg_getval()` `uvm32_arg_getcstr()` and so on to safely access the data
*/
typedef struct {
    uint32_t code;           /*! Syscall number, eg. UVM32_SYSCALL_YIELD */
    uint32_t *_ret;          /*! Value to be returned to caller, private, do not use directly */
    uint32_t *_params[2];    /*! The syscall's two parameters, private, do not use directly */
} uvm32_evt_syscall_t;

/*! An event passed from uvm32 to host when code must be paused */
typedef struct {
    uvm32_evt_typ_t typ;                /*! The type of this event */
    union {
        uvm32_evt_syscall_t syscall;    /*! Only valid when typ == UVM32_EVT_SYSCALL */
        uvm32_evt_err_t err;            /*! Only valid when typ == UVM32_EVT_ERR */
    } data;
} uvm32_evt_t;

/*! Internal state of uvm32 */
typedef enum {
    UVM32_STATUS_PAUSED,
    UVM32_STATUS_RUNNING,
    UVM32_STATUS_ERROR,
    UVM32_STATUS_ENDED,
} uvm32_status_t;


/*! State of uvm32. Each VM requires an instance of uvm32_state_t. All members of the struct are private and should only be accessed through provided functions */
typedef struct {
    uvm32_status_t _status;                 /*! Current VM running state */
    uvm32_err_t _err;                       /*! Current error code */
    struct MiniRV32IMAState _core;          /*! CPU registers */
    uint8_t _memory[UVM32_MEMORY_SIZE];     /*! Memory */
    uvm32_evt_t _ioevt;                     /*! Event to be returned on next pause */
#ifdef UVM32_STACK_PROTECTION
    uint8_t *_stack_canary;                 /*! Location of stack canary */
#endif
    uint8_t *_extram;                       /*! External RAM pointer, or NULL */
    uint32_t _extramLen;                    /*! Length of external RAM */
    bool _extramDirty;                      /*! Flag to indicate VM code has modified extram since last run */
    uint32_t garbage;                       /*! Used for returning valid pointer when operations fail */
} uvm32_state_t;

/*! Initialise a VM instance */
void uvm32_init(uvm32_state_t *vmst);

/*! Load compiled code into the instance for execution. `rom` is copied into the memory space of the VM so the caller may safely free it after calling */
bool uvm32_load(uvm32_state_t *vmst, const uint8_t *rom, int len);

/*! Run the VM for a maximum on `instr_meter` instructions. The VM will pause and return to the caller after executing `instr_meter` or less instructions and give the reason for stopping in `evt`. The number of instructions executed is returned. */
uint32_t uvm32_run(uvm32_state_t *vmst, uvm32_evt_t *evt, uint32_t instr_meter);

/*! Check if the VM has ended execution. On ending, the VM will send `UVM32_EVT_END`, but this function may be used to repeatedly check */
bool uvm32_hasEnded(const uvm32_state_t *vmst);

/* After VM has paused due to UVM32_STATUS_ERROR, calling `uvm32_clearError()` will allow it to continue normally */
void uvm32_clearError(uvm32_state_t *vmst);

/*! Describes a slice of memory, with a pointer and known length */
typedef struct {
    uint8_t *ptr;
    uint32_t len;
} uvm32_slice_t;

/*! Used for accessing the parts of a syscall */
typedef enum {
    ARG0,           /*! The first argument of a syscall */
    ARG1,           /*! The second argument of a syscall */
    RET,            /*! The return value of a syscall */
} uvm32_arg_t;

/*! Read a syscall argument as a uint32_t */
uint32_t uvm32_arg_getval(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t arg);

/*! Read a syscall argument as a C string */
const char *uvm32_arg_getcstr(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t arg);

/*! Write a syscall argument as a uint32_t value */
void uvm32_arg_setval(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t, uint32_t val);

/*! Read a syscall argument pair (ptr, length) as a slice */
uvm32_slice_t uvm32_arg_getslice(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t argPtr, uvm32_arg_t argLen);

/*! Read a syscall argument pointer as a slice of known length */
uvm32_slice_t uvm32_arg_getslice_fixed(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t arg, uint32_t len);


/*! Setup a block of memory to act as external RAM, it will be available on in VM code at address `UVM32_EXTRAM_BASE`. The memory is not copied, so the caller must ensure it remains available until `uvm32_extram()` is called to setup a different region or the VM is ended. */
void uvm32_extram(uvm32_state_t *vmst, uint8_t *extram, uint32_t len);

/*! Check to see if the external RAM is marked as dirty. If the VM code writes to the external RAM, this flag is set. The flag is automatically cleared next time uvm32_run() is called */
bool uvm32_extramDirty(uvm32_state_t *vmst);

/*! Get const pointer to raw memory, for debugging */
const uint8_t *uvm32_getMemory(const uvm32_state_t *vmst);
/*! Get program counter for, for debugging */
uint32_t uvm32_getProgramCounter(const uvm32_state_t *vmst);

#endif
