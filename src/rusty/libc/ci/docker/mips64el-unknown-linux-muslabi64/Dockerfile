FROM ubuntu:19.04

RUN apt-get update && apt-get install -y --no-install-recommends \
  gcc make libc6-dev git curl ca-certificates \
  gcc-mips64el-linux-gnuabi64 qemu-user

COPY install-musl.sh /
RUN sh /install-musl.sh mips64el

# FIXME: shouldn't need the `-lgcc` here, shouldn't that be in libstd?
ENV PATH=$PATH:/musl-mips64el/bin:/rust/bin \
    CC_mips64el_unknown_linux_muslabi64=musl-gcc \
    RUSTFLAGS='-Clink-args=-lgcc' \
    CARGO_TARGET_MIPS64EL_UNKNOWN_LINUX_MUSLABI64_LINKER=musl-gcc \
    CARGO_TARGET_MIPS64EL_UNKNOWN_LINUX_MUSLABI64_RUNNER="qemu-mips64el -L /musl-mips64el"
