C_COMPILER=gcc

UNITY_ROOT=../unity

CFLAGS=-std=c99
CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -DUVM32_MEMORY_SIZE=16384

SUITE_NAME=tests

TARGET_BASE1=test1
TARGET1 = $(TARGET_BASE1)
SRC_FILES1=$(UNITY_ROOT)/src/unity.c test/${SUITE_NAME}.c  test/test_runners/${SUITE_NAME}_Runner.c ../../uvm32/uvm32.c
INC_DIRS=-I$(UNITY_ROOT)/src -I../../uvm32/ -I../../common -Irom

.PHONY: rom

default: $(SRC_FILES1) rom
	@$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $(SRC_FILES1) rom/rom-header.c -o $(TARGET1)
	@ ./$(TARGET1)

rom:
	@(cd rom && make)

test/test_runners/${SUITE_NAME}_Runner.c: test/${SUITE_NAME}.c
	@mkdir -p test/test_runners
	@ruby $(UNITY_ROOT)/auto/generate_test_runner.rb test/${SUITE_NAME}.c  test/test_runners/${SUITE_NAME}_Runner.c

clean:
	rm -rf $(TARGET1) test/test_runners
	(cd rom && make clean)

