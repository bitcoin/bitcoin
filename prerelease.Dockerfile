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

# install gflags
RUN git clone --progress -b v2.2.2 https://github.com/gflags/gflags && \
   ( \
     cd gflags; \
     mkdir build; \
     cd build; \
     cmake .. -DCMAKE_BUILD_TYPE=Release; \
     make -j4 install; \
   ) && \
   rm -rf gflags

# install c-ares
RUN git clone --progress -b cares-1_15_0 https://github.com/c-ares/c-ares && \
   ( \
     cd c-ares; \
     mkdir build; \
     cd build; \
     cmake .. -DCMAKE_BUILD_TYPE=Release; \
     make -j4 install; \
   ) && \
   rm -rf c-ares

# install protobuf
RUN git clone --progress -b v3.10.0 https://github.com/protocolbuffers/protobuf && \
   ( \
     cd protobuf; \
     mkdir build; \
     cd build; \
     cmake ../cmake \
       -DCMAKE_BUILD_TYPE=Release \
       -Dprotobuf_BUILD_SHARED_LIBS=ON \
       -Dprotobuf_BUILD_TESTS=OFF; \
     make -j4 install; \
   ) && \
   rm -rf protobuf

# install grpc; depends on protobuf, c-ares, gflags
RUN git clone --progress -b v1.26.x http://github.com/grpc/grpc/
RUN ( \
      cd grpc; \
      git submodule update --init third_party/udpa; \
      mkdir build; \
      cd build; \
      cmake .. \
        -DCMAKE_C_FLAGS="-Wno-error" \
        -DCMAKE_CXX_FLAGS="-Wno-error" \
        -DCMAKE_BUILD_TYPE=Release \
        -DgRPC_BENCHMARK_PROVIDER="" \
        -DgRPC_ZLIB_PROVIDER=package \
        -DgRPC_CARES_PROVIDER=package \
        -DgRPC_SSL_PROVIDER=package \
        -DgRPC_PROTOBUF_PROVIDER=package \
        -DgRPC_GFLAGS_PROVIDER=package \
        -DgRPC_BUILD_CSHARP_EXT=OFF \
        -DgRPC_BUILD_TESTS=OFF \
        -DBUILD_SHARED_LIBS=ON \
        -DgRPC_BUILD_GRPC_CSHARP_PLUGIN=OFF \
        -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF \
        -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF \
        -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \
        -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF \
        -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF \
        -DgRPC_BUILD_GRPC_CPP_PLUGIN=ON \
        ; \
      make -j8 install; \
    ) && \
    rm -rf grpc

RUN ldconfig

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8
