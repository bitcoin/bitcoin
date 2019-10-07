FROM ubuntu:19.04

RUN apt-get update && apt-get install -y --no-install-recommends \
        gcc libc6-dev qemu-user ca-certificates qemu-system-mips curl \
        bzip2

RUN mkdir /toolchain

# Note that this originally came from:
# https://downloads.openwrt.org/snapshots/trunk/malta/generic/OpenWrt-Toolchain-malta-le_gcc-5.3.0_musl-1.1.15.Linux-x86_64.tar.bz2
RUN curl --retry 5 -L https://rust-lang-ci-mirrors.s3-us-west-1.amazonaws.com/libc/OpenWrt-Toolchain-malta-le_gcc-5.3.0_musl-1.1.15.Linux-x86_64.tar.bz2 | \
      tar xjf - -C /toolchain --strip-components=2

ENV PATH=$PATH:/rust/bin:/toolchain/bin \
    CC_mipsel_unknown_linux_musl=mipsel-openwrt-linux-gcc \
    CARGO_TARGET_MIPSEL_UNKNOWN_LINUX_MUSL_LINKER=mipsel-openwrt-linux-gcc \
    CARGO_TARGET_MIPSEL_UNKNOWN_LINUX_MUSL_RUNNER="qemu-mipsel -L /toolchain"
