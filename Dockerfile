# A base Docker image running Python 3.6.2 and Debian 8 "Jessie"
# See: https://github.com/docker-library/python/blob/master/3.6/jessie/Dockerfile
FROM python:3.6.2-jessie

RUN apt-get update && \
    apt-get install -y \
    libboost-system-dev libboost-filesystem-dev libboost-chrono-dev \
    libboost-program-options-dev libboost-test-dev libboost-thread-dev \
    build-essential libtool autotools-dev automake pkg-config libssl-dev \
    libevent-dev bsdmainutils \
    libzmq3 miniupnpc \
    # Install clang for potential use over g++.
    clang

# The Bitcoin source code will be located at /code and mounted by 
# docker-compose.
ENV HOME /code
WORKDIR /code

# Install Berkeley DB 4.8 for use with wallet functionality.
ENV BDB_PREFIX /db4
ENV BDB_VERSION db-4.8.30.NC
RUN mkdir -p $BDB_PREFIX && \
    wget --no-check-certificate \
        "https://download.oracle.com/berkeley-db/${BDB_VERSION}.tar.gz" \
        -O ${BDB_VERSION}.tar.gz && \
    echo "12edc0df75bf9abd7f82f821795bcee50f42cb2e5f76a6a281b85732798364ef  ${BDB_VERSION}.tar.gz" \
        | sha256sum -c && \
    tar -xzvf ${BDB_VERSION}.tar.gz -C $BDB_PREFIX && \
    rm ${BDB_VERSION}.tar.gz && \
    ${BDB_PREFIX}/${BDB_VERSION}/dist/configure \
        --enable-cxx --disable-shared --with-pic --prefix=$BDB_PREFIX && \
    make install

ENTRYPOINT ["./bin/docker_entrypoint.sh"]
