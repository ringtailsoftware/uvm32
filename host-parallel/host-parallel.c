#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uvm32.h"
#include "../common/uvm32_common_custom.h"

// Run multiple scheduled VMs in parallel until all have ended
#define NUM_VM 4   // number of vms
#define SCHEDULE SCHEDULE_RANDOM    // scheduling algorithm

// scheduling algorithms
#define SCHEDULE_ROUNDROBIN()   scheduler_index = (scheduler_index + 1) % NUM_VM
#define SCHEDULE_RANDOM()       scheduler_index = rand()%NUM_VM

// Precompiled binary program to print integers
// This code expects to print via syscall 0x13C (UVM32_SYSCALL_PRINTD in common/uvm32_common_custom.h)
uint8_t rom[] = {
    0x23, 0x26, 0x11, 0x00, 0xef, 0x00, 0x00, 0x01, 0x93, 0x08, 0x80, 0x13,
    0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x05, 0x00, 0x00,
    0x93, 0x07, 0xa0, 0x00, 0x93, 0x08, 0xc0, 0x13, 0x73, 0x00, 0x00, 0x00,
    0x13, 0x05, 0x15, 0x00, 0xe3, 0x1a, 0xf5, 0xfe, 0x67, 0x80, 0x00, 0x00
};

// Create an identifier for our host handler
typedef enum {
    F_PRINTDEC,
} f_code_t;

// Map VM syscall UVM32_SYSCALL_PRINTDEC to F_PRINTDEC, tell VM to expect write of a U32
const uvm32_mapping_t env[] = {
    { .syscall = UVM32_SYSCALL_PRINTDEC, .typ = UVM32_SYSCALL_TYP_U32_WR, .code = F_PRINTDEC },
};

int main(int argc, char *argv[]) {
    uvm32_state_t vmst[NUM_VM];
    uvm32_evt_t evt;
    int numVmRunning = NUM_VM;
    int scheduler_index = 0;

    for (int i=0;i<NUM_VM;i++) {
        uvm32_init(&vmst[i], env, sizeof(env) / sizeof(env[0]));
        uvm32_load(&vmst[i], rom, sizeof(rom));
    }

    while(numVmRunning > 0) {
        if (uvm32_hasEnded(&vmst[scheduler_index])) {
            // this vm has already completed, pick another
            SCHEDULE();
            continue;
        }
        uvm32_run(&vmst[scheduler_index], &evt, 100);   // num instructions before vm considered hung

        switch(evt.typ) {
            case UVM32_EVT_END:
                printf("[VM %d ended]\n", scheduler_index);
                numVmRunning--;
            break;
            case UVM32_EVT_SYSCALL:    // vm has paused to handle UVM32_SYSCALL
                switch((f_code_t)evt.data.syscall.code) {
                    case F_PRINTDEC:
                        // Type of F_PRINTDEC is UVM32_SYSCALL_TYP_U32_WR, so expect value in evt.data.syscall.val.u32
                        printf("[VM %d]: %d\n", scheduler_index, evt.data.syscall.val.u32);
                    break;
                }
            break;
            default:
            break;
        }
        
        SCHEDULE();
    }

    return 0;
}
