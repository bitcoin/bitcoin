FROM ubuntu:19.04

RUN apt-get update && apt-get install -y --no-install-recommends \
        gcc libc6-dev qemu-user ca-certificates \
        gcc-mips64-linux-gnuabi64 libc6-dev-mips64-cross \
        qemu-system-mips64 linux-headers-generic

ENV CARGO_TARGET_MIPS64_UNKNOWN_LINUX_GNUABI64_LINKER=mips64-linux-gnuabi64-gcc \
    CARGO_TARGET_MIPS64_UNKNOWN_LINUX_GNUABI64_RUNNER="qemu-mips64 -L /usr/mips64-linux-gnuabi64" \
    CC_mips64_unknown_linux_gnuabi64=mips64-linux-gnuabi64-gcc \
    PATH=$PATH:/rust/bin
