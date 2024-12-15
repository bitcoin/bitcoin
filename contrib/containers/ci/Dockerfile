FROM ubuntu:jammy

# Needed to prevent tzdata hanging while expecting user input
ENV DEBIAN_FRONTEND="noninteractive" TZ="Europe/London"

# Build and base stuff
# (zlib1g-dev is needed for the Qt host binary builds, but should not be used by target binaries)
ENV APT_ARGS="-y --no-install-recommends --no-upgrade"

# Install packages for i386; disabled on aarch64 and arm64 hosts
RUN (dpkg --print-architecture | grep -Eq 'aarch64|arm64' || dpkg --add-architecture i386)
RUN (dpkg --print-architecture | grep -Eq 'aarch64|arm64' || (apt-get update && apt-get install $APT_ARGS \
    g++-multilib \
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
    cmake \
    g++ \
    gettext \
    git \
    gnupg \
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
    lsb-release \
    software-properties-common \
    unzip \
    wget \
    m4 \
    pkg-config \
    zlib1g-dev

# Install Clang+LLVM and set it as default
# We don't need all packages but the default set doesn't include some
# packages we want so we will need to install some of them manually.
ARG LLVM_VERSION=16
RUN set -ex; \
    echo "Installing LLVM and Clang ${LLVM_VERSION}..."; \
    curl -sL https://apt.llvm.org/llvm.sh | bash -s "${LLVM_VERSION}"; \
    echo "Installing additional packages..."; \
    apt-get update && apt-get install $APT_ARGS \
    "clang-format-${LLVM_VERSION}" \
    "clang-tidy-${LLVM_VERSION}" \
    "libc++-${LLVM_VERSION}-dev" \
    "libc++abi-${LLVM_VERSION}-dev" \
    "libclang-rt-${LLVM_VERSION}-dev"; \
    rm -rf /var/lib/apt/lists/*; \
    echo "Setting defaults..."; \
    lldbUpdAltArgs="update-alternatives --install /usr/bin/llvm-config llvm-config /usr/bin/llvm-config-${LLVM_VERSION} 100"; \
    for binName in clang clang++ clang-format clang-tidy clangd ld.lld lldb lldb-server; do \
        lldbUpdAltArgs="${lldbUpdAltArgs} --slave /usr/bin/${binName} ${binName} /usr/bin/${binName}-${LLVM_VERSION}"; \
    done; \
    sh -c "${lldbUpdAltArgs}";

# Python setup
# PYTHON_VERSION should match the value in .python-version
ARG PYTHON_VERSION=3.9.18
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
    make \
    tk-dev \
    xz-utils
ENV PYENV_ROOT="/usr/local/pyenv"
ENV PATH="${PYENV_ROOT}/shims:${PYENV_ROOT}/bin:${PATH}"
RUN curl https://pyenv.run | bash \
    && pyenv update \
    && pyenv install $PYTHON_VERSION \
    && pyenv global $PYTHON_VERSION \
    && pyenv rehash
RUN pip3 install \
    codespell==1.17.1 \
    flake8==3.8.3 \
    jinja2 \
    lief==0.13.2 \
    multiprocess \
    mypy==0.910 \
    pyzmq==22.3.0 \
    vulture==2.3

# dash_hash
ARG DASH_HASH_VERSION=1.4.0
RUN git clone --depth 1 --no-tags --branch=${DASH_HASH_VERSION} https://github.com/dashpay/dash_hash
RUN cd dash_hash && pip3 install -r requirements.txt .

ARG USER_ID=1000
ARG GROUP_ID=1000

# add user with specified (or default) user/group ids
ENV USER_ID="${USER_ID}"
ENV GROUP_ID="${GROUP_ID}"
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
ENV PATH="/tmp/cppcheck-${CPPCHECK_VERSION}/build/bin:${PATH}"
RUN mkdir /usr/local/share/Cppcheck && ln -s /tmp/cppcheck-${CPPCHECK_VERSION}/cfg/ /usr/local/share/Cppcheck/cfg

ARG SHELLCHECK_VERSION=v0.7.1
RUN curl -sL "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.x86_64.tar.xz" | tar --xz -xf - --directory /tmp/
ENV PATH="/tmp/shellcheck-${SHELLCHECK_VERSION}:${PATH}"

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
