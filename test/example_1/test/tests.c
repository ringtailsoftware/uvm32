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

void test_helloworld1(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 10000);
    // check for println syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, UVM32_SYSCALL_PRINTLN);
    const char *str = uvm32_getcstr(&vmst, &evt, ARG0);
    TEST_ASSERT_EQUAL(0, strcmp(str, "Hello world"));
    // run vm to completion
    uvm32_run(&vmst, &evt, 10000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_END);
}


