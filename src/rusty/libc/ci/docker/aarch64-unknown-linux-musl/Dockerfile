FROM ubuntu:19.04

RUN apt-get update && apt-get install -y --no-install-recommends \
  gcc make libc6-dev git curl ca-certificates \
  gcc-aarch64-linux-gnu qemu-user

COPY install-musl.sh /
RUN sh /install-musl.sh aarch64

# FIXME: shouldn't need the `-lgcc` here, shouldn't that be in libstd?
ENV PATH=$PATH:/musl-aarch64/bin:/rust/bin \
    CC_aarch64_unknown_linux_musl=musl-gcc \
    RUSTFLAGS='-Clink-args=-lgcc' \
    CARGO_TARGET_AARCH64_UNKNOWN_LINUX_MUSL_LINKER=musl-gcc \
    CARGO_TARGET_AARCH64_UNKNOWN_LINUX_MUSL_RUNNER="qemu-aarch64 -L /musl-aarch64"
