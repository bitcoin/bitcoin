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
    && wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    add-apt-repository -y "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-9 main" && \
    apt-add-repository -y ppa:bitcoin/bitcoin && \
    add-apt-repository -y ppa:ubuntu-toolchain-r/test

# install dependencies
RUN apt-get update && apt-get upgrade -y && \
    apt-get install --no-install-recommends -y \
        libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools \
        libqrencode-dev \
        libprotobuf-dev protobuf-compiler \
        libzmq3-dev \
        libdb4.8-dev \
        libdb4.8++-dev \
        libzmq3-dev \
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
        libb2-dev

# install tools
RUN apt-get install --no-install-recommends -y \
        libz-dev \
        xz-utils \
        gcc-9 \
        g++-9 \
        llvm-9 \
        clang-9 \
        clang-tidy-9 \
        clang-format-9 \
        git \
        ccache \
        lcov \
        vim \
        unzip \
        cmake \
    && rm -rf /var/lib/apt/lists/*

# set default compilers and tools
RUN update-alternatives --install /usr/bin/gcov         gcov         /usr/bin/gcov-9               90 && \
    update-alternatives --install /usr/bin/gcc          gcc          /usr/bin/gcc-9                90 && \
    update-alternatives --install /usr/bin/g++          g++          /usr/bin/g++-9                90 && \
    update-alternatives --install /usr/bin/clang        clang        /usr/lib/llvm-9/bin/clang-9   90 && \
    update-alternatives --install /usr/bin/clang++      clang++      /usr/bin/clang++-9            90 && \
    update-alternatives --install /usr/bin/clang-tidy   clang-tidy   /usr/bin/clang-tidy-9         90 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-9       90 && \
    update-alternatives --install /usr/bin/python       python       /usr/bin/python3              90

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
RUN git clone --progress -b v1.25.0 http://github.com/grpc/grpc/ && \
    ( \
      cd grpc; \
      git submodule update --init third_party/udpa; \
      git apply /tmp/grpc-patch.diff; \
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

ENV SONAR_CLI_VERSION=4.2.0.1873
RUN set -e; \
    mkdir -p /opt/sonar; \
    curl -L -o /tmp/sonar.zip https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-${SONAR_CLI_VERSION}-linux.zip; \
    unzip -o -d /tmp/sonar-scanner /tmp/sonar.zip; \
    mv /tmp/sonar-scanner/sonar-scanner-${SONAR_CLI_VERSION}-linux /opt/sonar/scanner; \
    ln -s -f /opt/sonar/scanner/bin/sonar-scanner /usr/local/bin/sonar-scanner; \
    rm -rf /tmp/sonar*

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8
RUN pip3 install --upgrade setuptools wheel bashlex compiledb gcovr

WORKDIR /