FROM ubuntu:18.04

RUN apt-get update && \
    apt-get install --no-install-recommends -y \
        gpg \
        gpg-agent \
        wget \
        software-properties-common \
        curl \
        python3 \
        python3-pip \
        python3-setuptools \
    && \
    apt-add-repository -y ppa:bitcoin/bitcoin

RUN apt-get update && apt-get install --no-install-recommends -y \
    libzmq3-dev \
    libdb4.8-dev \
    libdb4.8++-dev \
    libminiupnpc-dev \
    build-essential \
    libtool \
    autotools-dev \
    automake \
    pkg-config \
    libssl-dev \
    libevent-dev \
    bsdmainutils \
    libboost-system-dev \
    libboost-filesystem-dev \
    libboost-chrono-dev \
    libboost-test-dev \
    libboost-thread-dev \
    libb2-dev \
    libz-dev \
    xz-utils \
    git \
    ccache \
    lcov \
    vim \
    unzip \
    cmake \
    golang

WORKDIR /tmp

# install vcpkg
RUN git clone --progress -b master https://github.com/microsoft/vcpkg /opt/vcpkg && \
    ( \
      cd /opt/vcpkg; \
      ./bootstrap-vcpkg.sh -disableMetrics; \
    )
ENV VCPKG_PATH=/opt/vcpkg

RUN ldconfig

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8
