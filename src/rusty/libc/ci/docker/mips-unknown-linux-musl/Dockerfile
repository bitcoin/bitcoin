FROM ubuntu:19.04

RUN apt-get update && apt-get install -y --no-install-recommends \
        gcc libc6-dev qemu-user ca-certificates qemu-system-mips curl \
        bzip2

RUN mkdir /toolchain

# Note that this originally came from:
# https://downloads.openwrt.org/snapshots/trunk/ar71xx/generic/OpenWrt-SDK-ar71xx-generic_gcc-5.3.0_musl-1.1.15.Linux-x86_64.tar.bz2
RUN curl --retry 5 -L https://rust-lang-ci-mirrors.s3-us-west-1.amazonaws.com/libc/OpenWrt-SDK-ar71xx-generic_gcc-5.3.0_musl-1.1.15.Linux-x86_64.tar.bz2 | \
    tar xjf - -C /toolchain --strip-components=1

ENV PATH=$PATH:/rust/bin:/toolchain/staging_dir/toolchain-mips_34kc_gcc-5.3.0_musl-1.1.15/bin \
    CC_mips_unknown_linux_musl=mips-openwrt-linux-gcc \
    CARGO_TARGET_MIPS_UNKNOWN_LINUX_MUSL_LINKER=mips-openwrt-linux-gcc \
    CARGO_TARGET_MIPS_UNKNOWN_LINUX_MUSL_RUNNER="qemu-mips -L /toolchain/staging_dir/toolchain-mips_34kc_gcc-5.3.0_musl-1.1.15"
