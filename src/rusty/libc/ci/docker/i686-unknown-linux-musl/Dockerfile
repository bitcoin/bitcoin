FROM ubuntu:19.04

RUN dpkg --add-architecture i386
RUN apt-get update
RUN apt-get install -y --no-install-recommends \
  gcc-multilib make libc6-dev git curl ca-certificates libc6:i386

COPY install-musl.sh /
RUN sh /install-musl.sh i686

ENV PATH=$PATH:/musl-i686/bin:/rust/bin \
    CC_i686_unknown_linux_musl=musl-gcc
