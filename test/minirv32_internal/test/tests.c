#include <string.h>
#include "unity.h"
#include "uvm32.h"
#include "../common/uvm32_common_custom.h"

#include "rom-header.h"

static uvm32_state_t vmst;
static uvm32_evt_t evt;

void setUp(void) {
    // runs before each test
    uvm32_init(&vmst);
    uvm32_load(&vmst, rom_bin, rom_bin_len);
}

void tearDown(void) {
}

void test_pc_too_big(void) {
    // No code should be doing this, but...
    TEST_ASSERT_EQUAL(0x80000000, uvm32_getProgramCounter(&vmst));
    vmst._core.pc = 0x80000000 + 1024 * 16 * 4; // off end
    uvm32_run(&vmst, &evt, 1);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_ERR);
    TEST_ASSERT_EQUAL(evt.data.err.errcode, UVM32_ERR_INTERNAL_CORE);
}

//void test_pc_unaligned_1(void) {
//    // No code should be doing this, but...
//    TEST_ASSERT_EQUAL(0x80000000, uvm32_getProgramCounter(&vmst));
//    vmst._core.pc = 0x80000000 + 1;
//    uvm32_run(&vmst, &evt, 1);
//    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_ERR);
//    TEST_ASSERT_EQUAL(evt.data.err.errcode, UVM32_ERR_INTERNAL_CORE);
//}
//
//void test_pc_unaligned_2(void) {
//    // No code should be doing this, but...
//    TEST_ASSERT_EQUAL(0x80000000, uvm32_getProgramCounter(&vmst));
//    vmst._core.pc = 0x80000000 + 2;
//    uvm32_run(&vmst, &evt, 1);
//    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_ERR);
//    TEST_ASSERT_EQUAL(evt.data.err.errcode, UVM32_ERR_INTERNAL_CORE);
//}
//
//void test_pc_unaligned_3(void) {
//    // No code should be doing this, but...
//    TEST_ASSERT_EQUAL(0x80000000, uvm32_getProgramCounter(&vmst));
//    vmst._core.pc = 0x80000000 + 3;
//    uvm32_run(&vmst, &evt, 1);
//    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_ERR);
//    TEST_ASSERT_EQUAL(evt.data.err.errcode, UVM32_ERR_INTERNAL_CORE);
//}
//
