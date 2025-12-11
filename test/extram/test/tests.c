#include <string.h>
#include "unity.h"
#include "uvm32.h"
#include "../common/uvm32_common_custom.h"

#include "rom-header.h"
#include "../shared.h"

static uvm32_state_t vmst;
static uvm32_evt_t evt;

uint32_t extram[32];

void setUp(void) {
    // runs before each test
    uvm32_init(&vmst);
    uvm32_load(&vmst, rom_bin, rom_bin_len);
    memset(extram, 0x00, sizeof(extram));
    uvm32_extram(&vmst, extram, sizeof(extram));
}

void tearDown(void) {
}

void test_extram_access(void) {
    extram[0] = 1234;

    // run the vm
    uvm32_run(&vmst, &evt, 100);

    TEST_ASSERT_EQUAL(false, uvm32_extramDirty(&vmst));

    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_setval(&vmst, &evt, RET, TEST1);

    uvm32_run(&vmst, &evt, 100);
    // check for printdec of val
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, UVM32_SYSCALL_PRINTDEC);
    TEST_ASSERT_EQUAL(1234, uvm32_getval(&vmst, &evt, ARG0));

    // run vm to completion
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_END);

    TEST_ASSERT_EQUAL(true, uvm32_extramDirty(&vmst));
    TEST_ASSERT_EQUAL(1234*2, extram[0]);
}

void test_extram_out_of_bounds_rd(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(false, uvm32_extramDirty(&vmst));

    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_setval(&vmst, &evt, RET, TEST2);

    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_ERR);
    TEST_ASSERT_EQUAL(evt.data.err.errcode, UVM32_ERR_MEM_RD);
    TEST_ASSERT_EQUAL(false, uvm32_extramDirty(&vmst));
}

void test_extram_out_of_bounds_wr(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(false, uvm32_extramDirty(&vmst));

    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_setval(&vmst, &evt, RET, TEST3);

    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_ERR);
    TEST_ASSERT_EQUAL(evt.data.err.errcode, UVM32_ERR_MEM_WR);
    TEST_ASSERT_EQUAL(false, uvm32_extramDirty(&vmst));
}

void test_extram_out_of_bounds_dirty_flag(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(false, uvm32_extramDirty(&vmst));

    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_setval(&vmst, &evt, RET, TEST4);

    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(true, uvm32_extramDirty(&vmst));
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(false, uvm32_extramDirty(&vmst));
}

