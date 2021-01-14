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

RUN mkdir --parents /opt/itcoin-core-source/infra

WORKDIR /opt/itcoin-core-source

COPY /build-aux build-aux/
COPY /contrib contrib/
COPY /doc doc/
COPY /share share/
COPY /src src/
COPY /test test/

COPY \
    /autogen.sh \
    /configure.ac \
    /libbitcoinconsensus.pc.in \
    /Makefile.am \
    ./

COPY \
    /infra/configure-itcoin-core.sh \
    /infra/Makefile \
    /opt/itcoin-core-source/infra/

WORKDIR /opt/itcoin-core-source/infra

RUN ./configure-itcoin-core.sh

# run the build on all the available cores (sparing one), with load limiting.
RUN make --jobs=$(nproc --ignore=1) --max-load=$(nproc --ignore=1)

# the build is stripped and put in /opt/itcoin-core-source/target
# Stripping shrinks considerably the binaries size. For example, bitcoind goes
# from 210 MB to 7 MB.
RUN make install-strip

# ==============================================================================
FROM ubuntu:20.04

LABEL \
    maintainer.0="Antonio Muci <antonio.muci@bancaditalia.it>" \
    maintainer.1="Giuseppe Galano <giuseppe.galano2@bancaditalia.it>"

RUN mkdir /opt/itcoin-core

WORKDIR /opt/itcoin-core

RUN apt update && \
    apt install --no-install-recommends -y \
        gettext-base \
        libboost-filesystem1.71.0 \
        libboost-thread1.71.0 \
        libdb5.3++ \
        libevent-2.1-7 \
        libevent-pthreads-2.1-7 \
        libpython3-stdlib \
        libsqlite3-0 \
        libzmqpp4 \
        jq \
        python3-minimal \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build-stage \
    /opt/itcoin-core-source/target \
    /opt/itcoin-core/

# precompile test_framework module, so that the container can be safely run
# with --read-only, if desired.
RUN python3 -m compileall /opt/itcoin-core/bin/test_framework

COPY \
    /infra/bitcoin.conf.tmpl \
    /opt/itcoin-core

COPY \
    /infra/create-keypair.sh \
    /infra/entrypoint.sh \
    /infra/render-template.sh \
    /opt/itcoin-core/bin/

# Put a symlink to the bitcoin* programs in /usr/local/bin, so that they can be
# easily executed from anywhere, including from outside the container.
RUN \
    ln -s /opt/itcoin-core/bin/bitcoin-cli    /usr/local/bin/ && \
    ln -s /opt/itcoin-core/bin/bitcoin-tx     /usr/local/bin/ && \
    ln -s /opt/itcoin-core/bin/bitcoin-util   /usr/local/bin/ && \
    ln -s /opt/itcoin-core/bin/bitcoin-wallet /usr/local/bin/ && \
    ln -s /opt/itcoin-core/bin/bitcoind       /usr/local/bin/ && \
    ln -s /opt/itcoin-core/bin/miner          /usr/local/bin/ && \
    ln -s /opt/itcoin-core/bin/render-template.sh /usr/local/bin/ && \
    ln -s /opt/itcoin-core/bin/create-keypair.sh  /usr/local/bin/ && \
    ln -s /opt/itcoin-core/bin/entrypoint.sh  /usr/local/bin/ && \
    echo "The bitcoin* programs are in the PATH: they can be executed from anywhere, even outside the container"

ENTRYPOINT [ "/opt/itcoin-core/bin/entrypoint.sh" ]

CMD [ "--help" ]
