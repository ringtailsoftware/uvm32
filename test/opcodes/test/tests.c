#include <string.h>
#include "unity.h"
#include "uvm32.h"
#include "../common/uvm32_common_custom.h"

#include "rom-header.h"
#include "../shared.h"

static uvm32_state_t vmst;
static uvm32_evt_t evt;

void setUp(void) {
    uvm32_init(&vmst);
    uvm32_load(&vmst, rom_bin, rom_bin_len);
}

void tearDown(void) {
}

//void test_invalid_opcode_rd_extram(void) {
//    // https://www.cs.sfu.ca/~ashriram/Courses/CS295/assets/notebooks/RISCV/RISCV_CARD.pdf
//    // lb,lbu etc only have funct3 values of 0,1,2,4,5
//    uint8_t bad_funct3_3[] = {
//        0xb7, 0x0f, 0x00, 0x10, // lui t6,0x10000
//        0x83, 0xB2, 0x0f, 0x00  // l? t0,0(t6)
//    };
//
//    uvm32_init(&vmst);
//    uvm32_load(&vmst, bad_funct3_3, 8);
//    uvm32_run(&vmst, &evt, 100);
//    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_ERR);
//    TEST_ASSERT_EQUAL(evt.data.err.errcode, UVM32_ERR_INTERNAL_CORE);
//
//    uint8_t bad_funct3_6[] = {
//        0xb7, 0x0f, 0x00, 0x10, // lui t6,0x10000
//        0x83, 0xE2, 0x0f, 0x00  // l? t0,0(t6)
//    };
//
//    uvm32_init(&vmst);
//    uvm32_load(&vmst, bad_funct3_6, 8);
//    uvm32_run(&vmst, &evt, 100);
//    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_ERR);
//    TEST_ASSERT_EQUAL(evt.data.err.errcode, UVM32_ERR_INTERNAL_CORE);
//}


void test_auipc(void) {
    uvm32_run(&vmst, &evt, 100);
    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_arg_setval(&vmst, &evt, RET, TEST1);

    // run vm to completion
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_END);

    // The two PC values stored by auipc should be adjacent
    TEST_ASSERT_EQUAL(vmst._core.regs[6], vmst._core.regs[5] + 4);
}

void test_blt(void) {
    uvm32_run(&vmst, &evt, 100);
    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_arg_setval(&vmst, &evt, RET, TEST2);

    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, UVM32_SYSCALL_PRINTLN);

    // run vm to completion
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_END);
}


