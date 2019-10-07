FROM ubuntu:19.04

RUN apt-get update && apt-get install -y --no-install-recommends \
        gcc libc6-dev qemu-user ca-certificates \
        gcc-powerpc-linux-gnu libc6-dev-powerpc-cross \
        qemu-system-ppc

ENV CARGO_TARGET_POWERPC_UNKNOWN_LINUX_GNU_LINKER=powerpc-linux-gnu-gcc \
    CARGO_TARGET_POWERPC_UNKNOWN_LINUX_GNU_RUNNER="qemu-ppc -L /usr/powerpc-linux-gnu" \
    CC=powerpc-linux-gnu-gcc \
    PATH=$PATH:/rust/bin
