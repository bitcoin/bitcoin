FROM ubuntu:19.04
RUN apt-get update && apt-get install -y --no-install-recommends \
  gcc libc6-dev ca-certificates \
  gcc-arm-linux-gnueabihf libc6-dev-armhf-cross qemu-user
ENV CARGO_TARGET_ARM_UNKNOWN_LINUX_GNUEABIHF_LINKER=arm-linux-gnueabihf-gcc \
    CARGO_TARGET_ARM_UNKNOWN_LINUX_GNUEABIHF_RUNNER="qemu-arm -L /usr/arm-linux-gnueabihf" \
    PATH=$PATH:/rust/bin
