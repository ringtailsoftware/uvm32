.PHONY: test

all:
	(cd test && make)
	(cd hosts && make)
	(cd apps && make)

clean:
	(cd test && make clean)
	(cd hosts && make clean)
	(cd apps && make clean)

ci:
	(cd test && make ci)

test:
	make -C test

distrib: all
	cp apps/*/*.bin precompiled/

dockerbuild:
	DOCKER_BUILDKIT=1 docker build -t uvm32 . --no-cache

dockerbuild_cached:
	DOCKER_BUILDKIT=1 docker build -t uvm32 .

dockershell:
	docker run -v `pwd`:/data -w /data --rm -ti uvm32 /bin/bash

docker:
	docker run -v `pwd`:/data -w /data --rm uvm32 make

dockerci:
	docker run -v `pwd`:/data -w /data --rm uvm32 make ci


