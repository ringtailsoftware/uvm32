PROJECT:=c3-hello

DOCKER_IMAGE=riscv-dev
DOCKER_CMD:=docker run --rm -v ${PWD}../../../:/data -w /data/apps/${PROJECT} ${DOCKER_IMAGE}
PREFIX:=${DOCKER_CMD} riscv64-unknown-elf-

CFLAGS+=-I../../common
CFLAGS+=-fno-stack-protector
CFLAGS+=-static-libgcc -fdata-sections -ffunction-sections
CFLAGS+=-g -Os -march=rv32ima_zicsr -mabi=ilp32 -static
LDFLAGS:= -T ../linker.ld -nostdlib -Wl,--gc-sections

SRCS_C3 := $(wildcard *.c3)

default: ${PROJECT}.bin

${PROJECT}.a: $(SRCS_C3)
	c3c --use-stdlib=no --no-entry --target elf-riscv32 static-lib $(SRCS_C3)

crt0.o: ../crt0.s
	${PREFIX}gcc ${CFLAGS} -c -o crt0.o ../crt0.s

#${PROJECT}.elf: ${PROJECT}.a crt0.o ../linker.ld
#	${PREFIX}ld ${LDFLAGS} -o ${PROJECT}.elf crt0.o hello.a

${PROJECT}.elf: ${PROJECT}.a ../linker.ld
	${PREFIX}gcc -o ${PROJECT}.elf ${CFLAGS} ${LDFLAGS} ../crt0.s hello.a

${PROJECT}.bin: ${PROJECT}.elf
	$(PREFIX)objcopy ${PROJECT}.elf -O binary ${PROJECT}.bin


clean:
	rm -f *.o *.a ${PROJECT}.elf ${PROJECT}.bin
