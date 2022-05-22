FROM debian:11.3

ADD . /bitcoin
WORKDIR /bitcoin

# install required build dependencies
RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        build-essential \
        libtool \
        autotools-dev \
        automake \
        pkg-config \
        bsdmainutils \
        python3 \
    ;

# install other dependencies
RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        libevent-dev \
        libboost-dev \
        libsqlite3-dev \
        libzmq3-dev \
        systemtap-sdt-dev \
    ;

# build and install bitcoind and bitcoin-cli
RUN set -eux; \
    ./autogen.sh;
RUN set -eux; \
    ./configure;
RUN set -eux; \
    make;
RUN set -eux; \
    make install;

ENTRYPOINT ["/bin/bash"]