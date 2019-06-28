FROM ubuntu:xenial

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN apt-get update \
  && apt-get -y install --no-install-recommends \
    autoconf software-properties-common wget libzmq3-dev curl \
    unzip git python build-essential libtool autotools-dev automake \
    pkg-config libssl-dev libevent-dev bsdmainutils libdb-dev libdb++-dev \
    libboost-system-dev libboost-filesystem-dev libboost-chrono-dev \
    libboost-program-options-dev libboost-test-dev libboost-thread-dev \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

ARG VERSION=4.8.30

RUN add-apt-repository ppa:bitcoin/bitcoin \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
      libdb4.8-dev=${VERSION}-xenial4 \
      libdb4.8++-dev=${VERSION}-xenial4 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

COPY . /omnicore
WORKDIR /omnicore

RUN ./autogen.sh && \
  ./configure && \
  make -j2 && \
  make install

VOLUME /omni-data

# RPC P2P
EXPOSE 8332 8333

ENTRYPOINT ["omnicored", "-datadir=/omni-data", "-server=1", "-txindex=1"]
