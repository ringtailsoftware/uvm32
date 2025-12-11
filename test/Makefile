TESTS = \
    basic_syscalls  \
    stackoverflow   \
    custom_syscall  \
    syscall_args \
    meter \
    extram

RUNCMD = $(foreach TEST,${TESTS},make -C ${TEST} &&)
CLEANCMD = $(foreach TEST,${TESTS},make -C ${TEST} clean &&)

all:
	${RUNCMD} true

clean:
	${CLEANCMD} true

