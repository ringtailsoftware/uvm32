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

void test_extram_byte_write(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(false, uvm32_extramDirty(&vmst));

    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_setval(&vmst, &evt, RET, TEST5);

    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(true, uvm32_extramDirty(&vmst));

    // check that byte 7 only of extram has changed
    uint8_t *p = (uint8_t *)extram;
    for (int i=0;i<sizeof(extram);i++) {
        if (i == 7) {
            TEST_ASSERT_EQUAL(0xAB, p[i]);
        } else {
            TEST_ASSERT_EQUAL(0x00, p[i]);
        }
    }
}

void test_extram_byte_read(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(false, uvm32_extramDirty(&vmst));

    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_setval(&vmst, &evt, RET, TEST6);

    // set only byte 7 in extram
    uint8_t *p = (uint8_t *)extram;
    p[7] = 0xCD;

    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(false, uvm32_extramDirty(&vmst));
    // check for printdec of val
    TEST_ASSERT_EQUAL(UVM32_EVT_SYSCALL, evt.typ);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, UVM32_SYSCALL_PRINTDEC);
    TEST_ASSERT_EQUAL(0xCD, uvm32_getval(&vmst, &evt, ARG0));
}

void test_extram_short_write(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(false, uvm32_extramDirty(&vmst));

    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_setval(&vmst, &evt, RET, TEST7);

    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(true, uvm32_extramDirty(&vmst));

    // check that short 7 only of extram has changed
    uint16_t *p = (uint16_t *)extram;
    for (int i=0;i<sizeof(extram)/2;i++) {
        if (i == 7) {
            TEST_ASSERT_EQUAL(0xABCD, p[i]);
        } else {
            TEST_ASSERT_EQUAL(0x00, p[i]);
        }
    }
}

void test_extram_short_read(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(false, uvm32_extramDirty(&vmst));

    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_setval(&vmst, &evt, RET, TEST8);

    // set only short 7 in extram
    uint16_t *p = (uint16_t *)extram;
    p[7] = 0xCDEF;

    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(false, uvm32_extramDirty(&vmst));
    // check for printdec of val
    TEST_ASSERT_EQUAL(UVM32_EVT_SYSCALL, evt.typ);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, UVM32_SYSCALL_PRINTDEC);
    TEST_ASSERT_EQUAL(0xCDEF, uvm32_getval(&vmst, &evt, ARG0));
}

void test_extram_buf_unterminated(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(false, uvm32_extramDirty(&vmst));

    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_setval(&vmst, &evt, RET, TEST9);

    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(true, uvm32_extramDirty(&vmst));
    // check for printbuf of val
    TEST_ASSERT_EQUAL(UVM32_EVT_SYSCALL, evt.typ);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, UVM32_SYSCALL_PRINTBUF);
    uvm32_evt_syscall_buf_t buf = uvm32_getbuf(&vmst, &evt, ARG0, ARG1);
    TEST_ASSERT_EQUAL(5, buf.len);
    TEST_ASSERT_EQUAL(0, memcmp(buf.ptr, "hello", 5));
}

void test_extram_buf_terminated(void) {
    // run the vm
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(false, uvm32_extramDirty(&vmst));

    // check for picktest syscall
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_SYSCALL);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, SYSCALL_PICKTEST);
    uvm32_setval(&vmst, &evt, RET, TEST9);

    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(true, uvm32_extramDirty(&vmst));
    // check for printbuf of val
    TEST_ASSERT_EQUAL(UVM32_EVT_SYSCALL, evt.typ);
    TEST_ASSERT_EQUAL(evt.data.syscall.code, UVM32_SYSCALL_PRINTBUF);
    const char *str = uvm32_getcstr(&vmst, &evt, ARG0);
    TEST_ASSERT_NOT_EQUAL(NULL, str);
    TEST_ASSERT_EQUAL(0, strcmp(str, "hello"));
}


