#include <string.h>
#include "unity.h"
#include "uvm32.h"
#include "../common/uvm32_common_custom.h"

#include "rom-header.h"
#include "../shared.h"

static uvm32_state_t vmst;
static uvm32_evt_t evt;

void setUp(void) {
    // runs before each test
    uvm32_init(&vmst);
    uvm32_load(&vmst, rom_bin, rom_bin_len);
}

void tearDown(void) {
}

void test_syscall_args_two_strings(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 1000);
    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_arg_setval(&vmst, &evt, RET, SYSCALL_A);

    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_A);
    TEST_ASSERT_EQUAL(0, strcmp(uvm32_arg_getcstr(&vmst, &evt, ARG0), SYSCALL_A_DATA0));
    TEST_ASSERT_EQUAL(0, strcmp(uvm32_arg_getcstr(&vmst, &evt, ARG1), SYSCALL_A_DATA1));
    // run vm to completion
    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_END);
}

void test_syscall_args_bad_string(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 1000);
    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_arg_setval(&vmst, &evt, RET, SYSCALL_B);

    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_B);

    // try to read C string from the first null ptr
    const char *s0 = uvm32_arg_getcstr(&vmst, &evt, ARG0);
    TEST_ASSERT_EQUAL(0, strlen(s0)); // tried to read a null pointer, get back a zero length string (not a NULL we might read)
    // attempt to run vm, should be stuck in err
    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_ERR);

    // try to read C string from the second null ptr
    const char *s1 = uvm32_arg_getcstr(&vmst, &evt, ARG1);
    TEST_ASSERT_EQUAL(0, strlen(s1)); // tried to read a null pointer, get back a zero length string (not a NULL we might read)
    // attempt to run vm, should be stuck in err
    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_ERR);
}

void test_syscall_args_bufrd(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 1000);
    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_arg_setval(&vmst, &evt, RET, SYSCALL_C);

    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_C);

    // get buffer described by ptr=ARG0,len=ARG1
    uvm32_slice_t buf = uvm32_arg_getbuf(&vmst, &evt, ARG0, ARG1);
    uint8_t expected[32];
    for (int i=0;i<32;i++) {
        expected[i] = i;
    }
    TEST_ASSERT_EQUAL(32, buf.len);
    TEST_ASSERT_EQUAL(0, memcmp(buf.ptr, expected, 32));
}

void test_syscall_args_bufrd_bad_addr(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 1000);
    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_arg_setval(&vmst, &evt, RET, SYSCALL_D);

    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_D);

    // get buffer described by ptr=ARG0,len=ARG1
    uvm32_slice_t buf = uvm32_arg_getbuf(&vmst, &evt, ARG0, ARG1);
    TEST_ASSERT_EQUAL(0, buf.len);  // it was invalid, should get a safe zero length buffer
    // attempt to run vm, should be stuck in err
    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_ERR);
}

void test_syscall_args_bufrd_ptrs(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 1000);
    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_arg_setval(&vmst, &evt, RET, SYSCALL_E);

    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_E);

    // get uint32_t buffer described by ptr=ARG0
    uvm32_slice_t buf0 = uvm32_arg_getbuf_fixed(&vmst, &evt, ARG0, 4);
    TEST_ASSERT_EQUAL(4, buf0.len);  // check read ok
    uint32_t rspa = 0xABCD1234;
    memcpy(buf0.ptr, &rspa, 4);  // send it back

    // get uint32_t buffer described by ptr=ARG1
    uvm32_slice_t buf1 = uvm32_arg_getbuf_fixed(&vmst, &evt, ARG1, 4);
    TEST_ASSERT_EQUAL(4, buf1.len);  // check read ok
    uint32_t rspb = 0x9876DECF;
    memcpy(buf1.ptr, &rspb, 4);  // send it back

    // check the same values are sent back
    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_F);
    TEST_ASSERT_EQUAL(rspa, uvm32_arg_getval(&vmst, &evt, ARG0));
    TEST_ASSERT_EQUAL(rspb, uvm32_arg_getval(&vmst, &evt, ARG1));
}

void test_syscall_args_buf_pass(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 1000);
    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_arg_setval(&vmst, &evt, RET, SYSCALL_G);

    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_G);

    // get buffer described by ptr=ARG0,len=ARG1
    uvm32_slice_t buf0 = uvm32_arg_getbuf(&vmst, &evt, ARG0, ARG1);
    TEST_ASSERT_EQUAL(32, buf0.len);
    // fill with a pattern
    for (int i=0;i<32;i++) {
        buf0.ptr[i] = i;
    }
    // expect every element to be doubled then sent back
    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_H);

    // get buffer described by ptr=ARG0,len=ARG1
    uvm32_slice_t buf1 = uvm32_arg_getbuf(&vmst, &evt, ARG0, ARG1);
    TEST_ASSERT_EQUAL(32, buf1.len);
    for (int i=0;i<32;i++) {
        TEST_ASSERT_EQUAL(i*2, buf1.ptr[i]);
    }
}

void test_syscall_args_string_never_terminates(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 1000);
    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_arg_setval(&vmst, &evt, RET, SYSCALL_I);

    uvm32_run(&vmst, &evt, 1000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_I);
    // To ensure c string read goes off end looking for termination...
    memset(vmst._memory, 0xFF, UVM32_MEMORY_SIZE);
    TEST_ASSERT_EQUAL(0, strlen(uvm32_arg_getcstr(&vmst, &evt, ARG0)));

    // check for error state
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_ERR);
    TEST_ASSERT_EQUAL(evt.data.err.errcode, UVM32_ERR_MEM_RD);
}

