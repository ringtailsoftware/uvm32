PROJECT=rust-hello
TOPDIR=../../

all:
	cargo build -r --target riscv32im-unknown-none-elf && ${PREFIX}objcopy target/riscv32im-unknown-none-elf/release/${PROJECT} -O binary ${PROJECT}.bin

clean: clean_common
	rm -rf target

test: all
	${TOPDIR}/host/host ${PROJECT}.bin

include ${TOPDIR}/apps/makefile.common
