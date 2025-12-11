FROM ubuntu:25.04
ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8
RUN apt-get -y update

# tzdata
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC
RUN apt-get install -y tzdata

# for unity test
RUN apt-get install -y ruby xxd

# risc-v cross compiler
RUN apt-get install -y gcc-riscv64-unknown-elf build-essential

# zig
RUN apt-get install -y curl
RUN curl https://raw.githubusercontent.com/tristanisham/zvm/master/install.sh | bash
ENV ZVM_INSTALL /root/.zvm/self
ENV PATH "$PATH:/root/.zvm/bin:/root/.zvm/self"
ENV PATH "$PATH:/root/.zvm/bin"
RUN zvm i 0.15.2

# rust
RUN apt-get install -y rustup libclang1
RUN rustup default stable
RUN rustup target add riscv32im-unknown-none-elf
RUN apt-get install -y libclang1

# arduino
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
RUN arduino-cli core install arduino:avr
RUN apt-get install -y qemu-system-misc
