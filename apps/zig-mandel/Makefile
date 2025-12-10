PROJECT=mandel
TOPDIR=../../

all:
	@# zig's objcopy is broken, so use external tool
	@# https://ziggit.dev/t/addobjcopy-producing-zero-padding-at-start-of-binary/13384
	zig build && ${PREFIX}objcopy zig-out/bin/${PROJECT} -O binary ${PROJECT}.bin

clean: clean_common
	rm -rf zig-out .zig-cache

test: all
	${TOPDIR}/host/host ${PROJECT}.bin

include ${TOPDIR}/apps/makefile.common
