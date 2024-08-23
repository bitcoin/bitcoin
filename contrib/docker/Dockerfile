FROM debian:bullseye-slim as builder
LABEL maintainer="ETHAN LIU <liuyihen@gmail.com>"
LABEL description="Build Bitcoin Core Docker Image From Source Code"

ENV BUILD_JETS=2
ENV SRC_DIR=/usr/local/src/bitcoin
ENV VERSION="26.0"
ENV GITHUB_SOURCE_URL="https://github.com/bitcoin/bitcoin.git"

RUN apt-get update -y \
    && apt-get install -y \
        automake \
        autotools-dev \
        bsdmainutils \
        build-essential \
        git \
        libboost-dev \
        libevent-dev \
        libminiupnpc-dev \
        libssl-dev \
        libtool \
        libzmq3-dev \
        pkg-config \
        curl \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

WORKDIR $SRC_DIR

RUN git clone -n $GITHUB_SOURCE_URL $SRC_DIR \
    && cd $SRC_DIR \
    && git checkout ${VERSION:+tags/v}${VERSION:-master}

RUN ./autogen.sh

RUN make -C depends NO_BOOST=1 NO_LIBEVENT=1 NO_QT=1 NO_SQLITE=1 NO_NATPMP=1 NO_UPNP=1 NO_ZMQ=1 NO_USDT=1
ENV BDB_PREFIX="${SRC_DIR}/depends/x86_64-pc-linux-gnu"

RUN  ./configure \
    BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" \
    BDB_CFLAGS="-I${BDB_PREFIX}/include" \
    --without-gui \
    --disable-tests \
    --disable-bench

RUN V=1 make -j${BUILD_JETS}


FROM debian:bullseye-slim

ENV COIN_ROOT_DIR="/data"
ENV COIN_CONF_FILE="${COIN_ROOT_DIR}/bitcoin.conf"

RUN apt-get update -y \
    && apt-get install -y \
        libboost-dev \
        libevent-dev \
        libminiupnpc-dev \
        libssl-dev \
        libzmq3-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

COPY --from=builder /usr/local/src/bitcoin/src/bitcoind /usr/bin/
COPY --from=builder /usr/local/src/bitcoin/src/bitcoin-cli /usr/bin/
COPY --from=builder /usr/local/src/bitcoin/src/bitcoin-tx /usr/bin/
COPY --from=builder /usr/local/src/bitcoin/src/bitcoin-util /usr/bin/
COPY "docker-entrypoint.sh" /entrypoint.sh
COPY "bitcoin.conf" "${COIN_CONF_FILE}"
COPY "scripts/" "${COIN_SCRIPTS}/"

RUN groupadd -g 1000 coingroup \
    && useradd -u 1000 -g coingroup -m coinuser \
    && chown -R coinuser:coingroup "${COIN_ROOT_DIR}/"

EXPOSE 8332 18332 18443 8333 18333 18444

VOLUME ["${COIN_ROOT_DIR}"]

ENTRYPOINT ["/entrypoint.sh"]

CMD ["bitcoind"]
