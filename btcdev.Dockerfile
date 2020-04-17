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
    apt-get install -y \
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
        libb2-dev \
        libz-dev \
        xz-utils \
        llvm-9 \
        clang-9 \
        clang-tidy-9 \
        clang-format-9 \
        git \
        ccache \
        lcov \
        vim \
        unzip \
        cmake

# set default compilers and tools
RUN update-alternatives --install /usr/bin/clang        clang        /usr/lib/llvm-9/bin/clang-9   90 && \
    update-alternatives --install /usr/bin/clang++      clang++      /usr/bin/clang++-9            90 && \
    update-alternatives --install /usr/bin/clang-tidy   clang-tidy   /usr/bin/clang-tidy-9         90 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-9       90 && \
    update-alternatives --install /usr/bin/python       python       /usr/bin/python3              90

WORKDIR /tmp

RUN git clone --progress -b release-1.10.0 https://github.com/google/googletest.git && \
    ( \
      cd googletest; \
      mkdir build; \
      cd build; \
      cmake .. -DCMAKE_BUILD_TYPE=Release; \
      make -j4 install; \
    ) && \
    rm -rf googletest

ENV SONAR_CLI_VERSION=4.2.0.1873
RUN set -e; \
    mkdir -p /opt/sonar; \
    curl -L -o /tmp/sonar.zip https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-${SONAR_CLI_VERSION}-linux.zip; \
    unzip -o -d /tmp/sonar-scanner /tmp/sonar.zip; \
    mv /tmp/sonar-scanner/sonar-scanner-${SONAR_CLI_VERSION}-linux /opt/sonar/scanner; \
    ln -s -f /opt/sonar/scanner/bin/sonar-scanner /usr/local/bin/sonar-scanner; \
    rm -rf /tmp/sonar*


# install vcpkg
RUN git clone --progress -b master https://github.com/microsoft/vcpkg /opt/vcpkg && \
    ( \
      cd /opt/vcpkg; \
      ./bootstrap-vcpkg.sh -disableMetrics; \
    )
ENV VCPKG_PATH=/opt/vcpkg
ENV PATH="${PATH}:${VCPKG_PATH}"
ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8
RUN pip3 install --upgrade setuptools wheel bashlex compiledb gcovr

RUN apt-get install -y g++-mingw-w64-x86-64 mingw-w64-x86-64-dev mingw-w64

WORKDIR /