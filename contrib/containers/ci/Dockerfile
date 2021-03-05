FROM ubuntu:focal

# Needed to prevent tzdata hanging while expecting user input
ENV DEBIAN_FRONTEND="noninteractive" TZ="Europe/London"

# Build and base stuff
# (zlib1g-dev is needed for the Qt host binary builds, but should not be used by target binaries)
ENV APT_ARGS="-y --no-install-recommends --no-upgrade"


# Install packages for i386; disabled on aarch64 and arm64 hosts
RUN (dpkg --print-architecture | grep -Eq 'aarch64|arm64' || dpkg --add-architecture i386)
RUN (dpkg --print-architecture | grep -Eq 'aarch64|arm64' || (apt-get update && apt-get install $APT_ARGS \
    g++-9-multilib \
    wine32) && rm -rf /var/lib/apt/lists/*)

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
    g++ \
    gettext \
    git \
    libc++-dev \
    libc++abi-dev \
    libtool \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-shape0 \
    libxcb-sync1 \
    libxcb-xfixes0 \
    libxcb-xinerama0 \
    libxcb-xkb1 \
    libxkbcommon-x11-0 \
    wget \
    unzip \
    m4 \
    pkg-config \
    zlib1g-dev

# Python setup
# PYTHON_VERSION should match the value in .python-version
ARG PYTHON_VERSION=3.8.16
RUN apt-get update && apt-get install $APT_ARGS \
    ca-certificates \
    libbz2-dev \
    libffi-dev \
    liblzma-dev \
    libncurses5-dev \
    libncursesw5-dev \
    libreadline-dev \
    libsqlite3-dev \
    libssl-dev \
    llvm \
    make \
    tk-dev \
    xz-utils
ENV PYENV_ROOT "/usr/local/pyenv"
ENV PATH "${PYENV_ROOT}/shims:${PYENV_ROOT}/bin:${PATH}"
RUN curl https://pyenv.run | bash \
    && pyenv update \
    && pyenv install $PYTHON_VERSION \
    && pyenv global $PYTHON_VERSION \
    && pyenv rehash
RUN pip3 install \
    codespell==1.17.1 \
    flake8==3.8.3 \
    jinja2 \
    lief==0.12.1 \
    pyzmq \
    vulture==2.3 \
    mypy==0.781 \
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
    g++-arm-linux-gnueabihf \
    g++-mingw-w64-x86-64 \
    jq \
    libz-dev \
    libncurses5 \
    nsis \
    python3-zmq \
    parallel \
    valgrind \
    wine-stable \
    wine64 \
    xorriso

ARG CPPCHECK_VERSION=2.13.0
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

ARG LLVM_VERSION=16
# Setup Clang+LLVM support
RUN apt-get update && apt-get install $APT_ARGS \
    lsb-release \
    software-properties-common \
    gnupg \
    && rm -rf /var/lib/apt/lists/*

RUN cd /tmp && \
    wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    /tmp/llvm.sh ${LLVM_VERSION} && \
    rm -rf /tmp/llvm.sh

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
