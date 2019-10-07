FROM ubuntu:19.04

RUN apt-get update && apt-get install -y --no-install-recommends \
  gcc make libc6-dev git curl ca-certificates \
  gcc-arm-linux-gnueabihf qemu-user

COPY install-musl.sh /
RUN sh /install-musl.sh arm

ENV PATH=$PATH:/musl-arm/bin:/rust/bin \
    CC_arm_unknown_linux_musleabihf=musl-gcc \
    CARGO_TARGET_ARM_UNKNOWN_LINUX_MUSLEABIHF_LINKER=musl-gcc \
    CARGO_TARGET_ARM_UNKNOWN_LINUX_MUSLEABIHF_RUNNER="qemu-arm -L /musl-arm"
