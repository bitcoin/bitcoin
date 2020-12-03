# ItCoin
#
# Dockerfile for building itcoin-core in a container
#
# Author: Antonio Muci <antonio.muci@bancaditalia.it>

FROM ubuntu:20.04 as build-stage

LABEL \
    maintainer.0="Antonio Muci <antonio.muci@bancaditalia.it>" \
    maintainer.1="Giuseppe Galano <giuseppe.galano2@bancaditalia.it>"

ENV TZ=Europe/Rome
RUN ln --symbolic --no-dereference --force /usr/share/zoneinfo/$TZ /etc/localtime && \
    echo $TZ > /etc/timezone

RUN apt update && \
    apt install --no-install-recommends -y \
        autoconf \
        automake \
        build-essential \
        libtool \
        g++-10 \
        pkg-config \
    && echo "Foregoing deletion of package cache: this is a throwaway building container"

RUN apt install -y --no-install-recommends \
    libboost1.71-dev \
    libboost-filesystem1.71-dev \
    libboost-thread1.71-dev \
    libdb5.3++-dev \
    libevent-dev \
    libsqlite3-dev \
    libzmqpp-dev

RUN mkdir /opt/itcoin-core-source

WORKDIR /opt/itcoin-core-source

COPY . .

RUN ./configure-itcoin-core.sh

# run the build on all the available cores (sparing one), with load limiting.
RUN make --jobs=$(nproc --ignore=1) --max-load=$(nproc --ignore=1)

# the build is put in /opt/itcoin-core-source/target
RUN make install

# strip the debug symbols from the installed target. This shrinks considerably
# the binaries size. For example, bitcoind goes from 210 MB to 7 MB.
RUN strip \
    target/bin/bitcoind \
    target/bin/bitcoin-tx \
    target/bin/bitcoin-wallet \
    target/lib/libbitcoinconsensus.a

# ==============================================================================
FROM ubuntu:20.04

LABEL \
    maintainer.0="Antonio Muci <antonio.muci@bancaditalia.it>" \
    maintainer.1="Giuseppe Galano <giuseppe.galano2@bancaditalia.it>"

RUN mkdir /opt/itcoin-core

WORKDIR /opt/itcoin-core

RUN apt update && \
    apt install --no-install-recommends -y \
        libboost-filesystem1.71.0 \
        libboost-thread1.71.0 \
        libdb5.3++ \
        libevent-2.1-7 \
        libevent-pthreads-2.1-7 \
        libsqlite3-0 \
        libzmqpp4 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build-stage \
    /opt/itcoin-core-source/target \
    /opt/itcoin-core/

# Put a symlink to the bitcoin* programs in /usr/local/bin, so that they can be
# easily executed from anywhere, including from outside the container.
RUN \
    ln -s /opt/itcoin-core/bin/bitcoin-cli    /usr/local/bin/ && \
    ln -s /opt/itcoin-core/bin/bitcoin-tx     /usr/local/bin/ && \
    ln -s /opt/itcoin-core/bin/bitcoin-wallet /usr/local/bin/ && \
    ln -s /opt/itcoin-core/bin/bitcoind       /usr/local/bin/ && \
    echo "The bitcoin* programs are in the PATH: they can be executed from anywhere, even outside the container"
