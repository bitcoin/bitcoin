FROM ubuntu:jammy

# Needed to prevent tzdata hanging while expecting user input
ENV DEBIAN_FRONTEND="noninteractive" TZ="Europe/London"

# Build and base stuff
# (zlib1g-dev is needed for the Qt host binary builds, but should not be used by target binaries)
ENV APT_ARGS="-y --no-install-recommends --no-upgrade"

# Install packages for i386 on amd64 hosts, then install common packages
RUN set -ex; \
    apt-get update && \
    if [ "$(dpkg --print-architecture)" = "amd64" ]; then \
        dpkg --add-architecture i386 && \
        apt-get update && \
        apt-get install $APT_ARGS \
        g++-multilib \
        wine32; \
    fi; \
    apt-get install $APT_ARGS \
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
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Clang+LLVM and set it as default
# We don't need all packages but the default set doesn't include some
# packages we want so we will need to install some of them manually.
ARG LLVM_VERSION=18
RUN set -ex; \
    echo "Installing LLVM and Clang ${LLVM_VERSION}..."; \
    curl -sL https://apt.llvm.org/llvm.sh | bash -s -- "${LLVM_VERSION}"; \
    echo "Installing additional packages..."; \
    apt-get update && apt-get install $APT_ARGS \
    "clang-format-${LLVM_VERSION}" \
    "clang-tidy-${LLVM_VERSION}" \
    "libc++-${LLVM_VERSION}-dev" \
    "libc++abi-${LLVM_VERSION}-dev" \
    "libclang-rt-${LLVM_VERSION}-dev" \
    "lld-${LLVM_VERSION}"; \
    rm -rf /var/lib/apt/lists/*; \
    echo "Setting defaults..."; \
    lldbUpdAltArgs="update-alternatives --install /usr/bin/llvm-config llvm-config /usr/bin/llvm-config-${LLVM_VERSION} 100"; \
    for binName in clang clang++ clang-format clang-tidy clangd dsymutil lld lldb lldb-server llvm-ar llvm-cov llvm-nm llvm-objdump llvm-ranlib llvm-strip; do \
        lldbUpdAltArgs="${lldbUpdAltArgs} --slave /usr/bin/${binName} ${binName} /usr/bin/${binName}-${LLVM_VERSION}"; \
    done; \
    for binName in ld64.lld ld.lld lld-link wasm-ld; do \
        lldbUpdAltArgs="${lldbUpdAltArgs} --slave /usr/bin/${binName} ${binName} /usr/bin/lld-${LLVM_VERSION}"; \
    done; \
    sh -c "${lldbUpdAltArgs}";
# LD_LIBRARY_PATH is empty by default, this is the first entry
ENV LD_LIBRARY_PATH="/usr/lib/llvm-${LLVM_VERSION}/lib"

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
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

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

ARG DASH_HASH_VERSION=1.4.0
RUN set -ex; \
    cd /tmp; \
    git clone --depth 1 --no-tags --branch=${DASH_HASH_VERSION} https://github.com/dashpay/dash_hash; \
    cd dash_hash && pip3 install -r requirements.txt .; \
    cd .. && rm -rf dash_hash

ARG CPPCHECK_VERSION=2.13.0
RUN set -ex; \
    curl -fL "https://github.com/danmar/cppcheck/archive/${CPPCHECK_VERSION}.tar.gz" -o /tmp/cppcheck.tar.gz; \
    mkdir -p /opt/cppcheck && tar -xzf /tmp/cppcheck.tar.gz -C /opt/cppcheck --strip-components=1 && rm /tmp/cppcheck.tar.gz; \
    cd /opt/cppcheck; \
    mkdir build && cd build && cmake .. && cmake --build . -j "$(( $(nproc) - 1 ))"; \
    mkdir /usr/local/share/Cppcheck && ln -s /opt/cppcheck/cfg/ /usr/local/share/Cppcheck/cfg; \
    rm -rf /tmp/cppcheck.tar.gz
ENV PATH="/opt/cppcheck/build/bin:${PATH}"

ARG SHELLCHECK_VERSION=v0.7.1
RUN set -ex; \
    curl -fL "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.x86_64.tar.xz" -o /tmp/shellcheck.tar.xz; \
    mkdir -p /opt/shellcheck && tar -xf /tmp/shellcheck.tar.xz -C /opt/shellcheck --strip-components=1 && rm /tmp/shellcheck.tar.xz
ENV PATH="/opt/shellcheck:${PATH}"

# Add user with specified (or default) user/group ids and setup configuration files
ARG USER_ID=1000
ARG GROUP_ID=1000
RUN set -ex; \
    groupadd -g ${GROUP_ID} dash; \
    useradd -u ${USER_ID} -g dash -s /bin/bash -m -d /home/dash dash; \
    mkdir -p /home/dash/.config/gdb; \
    echo "add-auto-load-safe-path /usr/lib/llvm-${LLVM_VERSION}/lib" | tee /home/dash/.config/gdb/gdbinit; \
    chown ${USER_ID}:${GROUP_ID} -R /home/dash

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
    zip \
    && rm -rf /var/lib/apt/lists/*

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
