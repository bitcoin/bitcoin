FROM debian:stable

RUN dpkg --add-architecture i386
RUN dpkg --add-architecture s390x
RUN apt-get update

# dkpg-dev: to make pkg-config work in cross-builds
RUN apt-get install --no-install-recommends --no-upgrade -y \
        git ca-certificates \
        make automake libtool pkg-config dpkg-dev valgrind qemu-user \
        gcc clang libc6-dbg \
        gcc-i686-linux-gnu libc6-dev-i386-cross libc6-dbg:i386 \
        gcc-s390x-linux-gnu libc6-dev-s390x-cross libc6-dbg:s390x
