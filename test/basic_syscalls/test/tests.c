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

void test_syscalls(void) {
    // check for println syscall
    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, UVM32_SYSCALL_PRINTLN);
    TEST_ASSERT_EQUAL(0, strcmp(uvm32_arg_getcstr(&vmst, &evt, ARG0), "Hello world"));

    // check for print syscall
    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, UVM32_SYSCALL_PRINT);
    TEST_ASSERT_EQUAL(0, strcmp(uvm32_arg_getcstr(&vmst, &evt, ARG0), "Hello world"));

    // check for printdec syscall
    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, UVM32_SYSCALL_PRINTDEC);
    TEST_ASSERT_EQUAL(42, uvm32_arg_getval(&vmst, &evt, ARG0));

    // check for printhex syscall
    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, UVM32_SYSCALL_PRINTHEX);
    TEST_ASSERT_EQUAL(0xDEADBEEF, uvm32_arg_getval(&vmst, &evt, ARG0));

    // check for putc syscall
    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, UVM32_SYSCALL_PUTC);
    TEST_ASSERT_EQUAL('G', uvm32_arg_getval(&vmst, &evt, ARG0));

    // run vm to completion
    TEST_ASSERT_EQUAL(false, uvm32_hasEnded(&vmst));
    uvm32_run(&vmst, &evt, 10000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_END);
    TEST_ASSERT_EQUAL(true, uvm32_hasEnded(&vmst));

    TEST_ASSERT_EQUAL(vmst._memory, uvm32_getMemory(&vmst));
}


