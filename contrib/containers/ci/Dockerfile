FROM ubuntu:focal

# Needed to prevent tzdata hanging while expecting user input
ENV DEBIAN_FRONTEND="noninteractive" TZ="Europe/London"

# Build and base stuff
# (zlib1g-dev is needed for the Qt host binary builds, but should not be used by target binaries)
ENV APT_ARGS="-y --no-install-recommends --no-upgrade"
RUN dpkg --add-architecture i386
RUN apt-get update && apt-get install $APT_ARGS \
    autotools-dev \
    automake \
    autoconf \
    bison \
    build-essential \
    bsdmainutils \
    curl \
    ccache \
    clang \
    cmake \
    git \
    g++ \
    gettext \
    wget \
    unzip \
    libtool \
    m4 \
    pkg-config \
    python3 \
    python3-dev \
    python3-pip \
    python3-setuptools \
    zlib1g-dev

# Python stuff
RUN pip3 install \
    codespell==1.17.1 \
    flake8==3.8.3 \
    jinja2 \
    pyzmq \
    vulture==2.3 \
    yq \
    multiprocess

# dash_hash
ARG DASH_HASH_VERSION=1.4.0
RUN git clone --depth 1 --no-tags --branch=${DASH_HASH_VERSION} https://github.com/dashpay/dash_hash
RUN cd dash_hash && pip3 install -r requirements.txt .

ARG USER_ID=1000
ARG GROUP_ID=1000

# add user with specified (or default) user/group ids
ENV USER_ID ${USER_ID}
ENV GROUP_ID ${GROUP_ID}
RUN groupadd -g ${GROUP_ID} dash
RUN useradd -u ${USER_ID} -g dash -s /bin/bash -m -d /home/dash dash

# Packages needed for all target builds
RUN apt-get update && apt-get install $APT_ARGS \
    bc \
    gawk \
    g++-9-multilib \
    g++-arm-linux-gnueabihf \
    g++-mingw-w64-x86-64 \
    imagemagick \
    jq \
    libcap-dev \
    librsvg2-bin \
    libz-dev \
    libbz2-dev \
    libtiff-tools \
    libncurses5 \
    nsis \
    python3-zmq \
    parallel \
    wine-stable \
    wine32 \
    wine64 \
    xorriso \
    && rm -rf /var/lib/apt/lists/*

ARG CPPCHECK_VERSION=2.10
RUN curl -sL "https://github.com/danmar/cppcheck/archive/${CPPCHECK_VERSION}.tar.gz" | tar -xvzf - --directory /tmp/
RUN cd /tmp/cppcheck-${CPPCHECK_VERSION} && mkdir build && cd build && cmake .. && cmake --build . -j 8
ENV PATH "/tmp/cppcheck-${CPPCHECK_VERSION}/build/bin:${PATH}"
RUN mkdir /usr/local/share/Cppcheck && ln -s /tmp/cppcheck-${CPPCHECK_VERSION}/cfg/ /usr/local/share/Cppcheck/cfg

ARG SHELLCHECK_VERSION=v0.7.1
RUN curl -sL "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.x86_64.tar.xz" | tar --xz -xf - --directory /tmp/
ENV PATH "/tmp/shellcheck-${SHELLCHECK_VERSION}:${PATH}"

# This is a hack. It is needed because gcc-multilib and g++-multilib are conflicting with g++-arm-linux-gnueabihf. This is
# due to gcc-multilib installing the following symbolic link, which is needed for -m32 support. However, this causes
# arm builds to also have the asm folder implicitly in the include search path. This is kind of ok, because the asm folder
# for arm has precedence.
RUN ln -s x86_64-linux-gnu/asm /usr/include/asm

# Make sure std::thread and friends is available
RUN \
  update-alternatives --set x86_64-w64-mingw32-gcc  /usr/bin/x86_64-w64-mingw32-gcc-posix; \
  update-alternatives --set x86_64-w64-mingw32-g++  /usr/bin/x86_64-w64-mingw32-g++-posix; \
  exit 0

RUN \
  mkdir -p /src/dash && \
  mkdir -p /cache/ccache && \
  mkdir /cache/depends && \
  mkdir /cache/sdk-sources && \
  chown ${USER_ID}:${GROUP_ID} /src && \
  chown ${USER_ID}:${GROUP_ID} -R /src && \
  chown ${USER_ID}:${GROUP_ID} /cache && \
  chown ${USER_ID}:${GROUP_ID} -R /cache

WORKDIR /src/dash

USER dash
