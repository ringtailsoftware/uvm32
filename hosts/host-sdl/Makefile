TOPDIR=../..

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
LIBS = ${SDL} -lm -lSDL3
else
LIBS = `pkg-config sdl3 --libs --static`
CFLAGS += `pkg-config sdl3 --cflags`
endif

CFLAGS += -Wall -Werror -Wno-gnu-binary-literal
CFLAGS += -O0 -g
CFLAGS += -DUVM32_ERROR_STRINGS -DUVM32_MEMORY_SIZE=$(shell echo "1024 * 1024 * 8" | bc)

all:
	gcc ${CFLAGS} -I${TOPDIR}/uvm32 -I${TOPDIR}/common -o host-sdl ${TOPDIR}/uvm32/uvm32.c host-sdl.c ${LIBS}

clean:
	rm -f host-sdl
