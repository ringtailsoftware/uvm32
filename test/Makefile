TESTS = \
    basic_syscalls  \
    stackoverflow   \
    custom_syscall  \
    syscall_args \
    meter \
    extram \
    badcode \
    invalid_opcodes

RUNCMD = $(foreach TEST,${TESTS},make -C ${TEST} &&)
CLEANCMD = $(foreach TEST,${TESTS},make -C ${TEST} clean &&)

ifeq (,$(shell which gcovr))
	GCOVRCMD=echo Install gcovr for code coverage reports
else
    GCOVRCMD=gcovr -r ../ --filter ".*uvm32.c"
	PERC=$(shell gcovr -r ../ --filter ".*uvm32.c" | grep uvm | awk '{print $$4}')
endif

all:
	${RUNCMD} true
	@${GCOVRCMD}

ci:
	@${GCOVRCMD}
	mkdir -p badge
	curl "https://img.shields.io/badge/Code%20Coverage-${PERC}25-success?style=flat" > badge/badge.svg

clean:
	${CLEANCMD} true
	rm -f coverage.xml

