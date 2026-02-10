# Builder for cppcheck
FROM debian:bookworm-slim AS cppcheck-builder
ARG CPPCHECK_VERSION=2.17.1
RUN set -ex; \
    apt-get update && apt-get install -y --no-install-recommends \
        curl \
        ca-certificates \
        cmake \
        make \
        g++ \
    && rm -rf /var/lib/apt/lists/*; \
    echo "Downloading Cppcheck version: ${CPPCHECK_VERSION}"; \
    curl -fL "https://github.com/danmar/cppcheck/archive/${CPPCHECK_VERSION}.tar.gz" -o /tmp/cppcheck.tar.gz; \
    mkdir -p /src/cppcheck && tar -xzf /tmp/cppcheck.tar.gz -C /src/cppcheck --strip-components=1; \
    rm /tmp/cppcheck.tar.gz; \
    cd /src/cppcheck; \
    mkdir build && cd build && cmake .. && cmake --build . -j"$(nproc)"; \
    strip bin/cppcheck

# Main image
FROM ubuntu:noble
ARG TARGETARCH

# Include built assets
COPY --from=cppcheck-builder /src/cppcheck/build/bin/cppcheck /usr/local/bin/cppcheck
COPY --from=cppcheck-builder /src/cppcheck/cfg /usr/local/share/Cppcheck/cfg
ENV PATH="/usr/local/bin:${PATH}"

# Needed to prevent tzdata hanging while expecting user input
ENV DEBIAN_FRONTEND="noninteractive" TZ="Europe/London"

# Build and base stuff
ENV APT_ARGS="-y --no-install-recommends --no-upgrade"

# Packages needed for builds and tests
RUN set -ex; \
    apt-get update && apt-get install ${APT_ARGS} \
    bsdmainutils \
    build-essential \
    ca-certificates \
    curl \
    g++ \
    git \
    libbz2-dev \
    liblzma-dev \
    libreadline-dev \
    libsqlite3-dev \
    libssl-dev \
    make \
    parallel \
    xz-utils \
    zlib1g-dev \
    zstd \
    && rm -rf /var/lib/apt/lists/*

# Install uv by copying from the official Docker image
COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /bin/

# Install Python to a system-wide location and set it as default
# PYTHON_VERSION should match the value in .python-version
ARG PYTHON_VERSION=3.10.19
ENV UV_PYTHON_INSTALL_DIR=/usr/local/python
RUN uv python install ${PYTHON_VERSION}

# Create symlinks to make python available system-wide
RUN set -ex; \
    PYTHON_PATH=$(uv python find ${PYTHON_VERSION}); \
    PYTHON_DIR=$(dirname $PYTHON_PATH); \
    ln -sf $PYTHON_DIR/python3 /usr/local/bin/python3; \
    ln -sf $PYTHON_DIR/python3 /usr/local/bin/python; \
    ln -sf $PYTHON_DIR/pip3 /usr/local/bin/pip3 2>/dev/null || true; \
    ln -sf $PYTHON_DIR/pip /usr/local/bin/pip 2>/dev/null || true

# Use system Python for installations
ENV UV_SYSTEM_PYTHON=1

# Install Python packages
# NOTE: if versions are changed, update ci/lint/04_install.sh
RUN uv pip install --system --break-system-packages \
    codespell==2.2.1 \
    flake8==5.0.4 \
    jinja2 \
    lief==0.13.2 \
    multiprocess \
    mypy==0.981 \
    pyzmq==24.0.1 \
    vulture==2.6

# Install packages relied on by tests
ARG DASH_HASH_VERSION=1.4.0
RUN set -ex; \
    cd /tmp; \
    git clone --depth 1 --no-tags --branch=${DASH_HASH_VERSION} https://github.com/dashpay/dash_hash; \
    cd dash_hash && uv pip install --system --break-system-packages -r requirements.txt .; \
    cd .. && rm -rf dash_hash

# Symlink all Python package executables to /usr/local/bin
RUN set -ex; \
    PYTHON_PATH=$(uv python find ${PYTHON_VERSION}); \
    PYTHON_BIN=$(dirname $PYTHON_PATH); \
    for exe in $PYTHON_BIN/*; do \
        if [ -x "$exe" ] && [ -f "$exe" ]; then \
            basename_exe=$(basename $exe); \
            if [ "$basename_exe" != "python" ] && [ "$basename_exe" != "python3" ] && [ "$basename_exe" != "pip" ] && [ "$basename_exe" != "pip3" ]; then \
                ln -sf "$exe" "/usr/local/bin/$basename_exe"; \
            fi; \
        fi; \
    done

ARG SHELLCHECK_VERSION=v0.8.0
RUN set -ex; \
    ARCH_INFERRED="${TARGETARCH}"; \
    if [ -z "${ARCH_INFERRED}" ]; then \
        ARCH_INFERRED="$(dpkg --print-architecture || true)"; \
    fi; \
    case "${ARCH_INFERRED}" in \
        amd64|x86_64) SC_ARCH="x86_64" ;; \
        arm64|aarch64) SC_ARCH="aarch64" ;; \
        *) echo "Unsupported architecture for ShellCheck: ${ARCH_INFERRED}"; exit 1 ;; \
    esac; \
    curl -fL "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.${SC_ARCH}.tar.xz" -o /tmp/shellcheck.tar.xz; \
    mkdir -p /opt/shellcheck && tar -xf /tmp/shellcheck.tar.xz -C /opt/shellcheck --strip-components=1 && rm /tmp/shellcheck.tar.xz
ENV PATH="/opt/shellcheck:${PATH}"

# Packages needed to be able to run sanitizer builds
ARG LLVM_VERSION=19
RUN set -ex; \
    . /etc/os-release; \
    curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key > /etc/apt/trusted.gpg.d/apt.llvm.org.asc; \
    echo "deb [signed-by=/etc/apt/trusted.gpg.d/apt.llvm.org.asc] http://apt.llvm.org/${UBUNTU_CODENAME}/  llvm-toolchain-${UBUNTU_CODENAME}-${LLVM_VERSION} main" > /etc/apt/sources.list.d/llvm.list; \
    apt-get update && apt-get install ${APT_ARGS} \
    "llvm-${LLVM_VERSION}-dev"; \
    rm -rf /var/lib/apt/lists/*;

# Setup unprivileged user and configuration files
ARG USER_ID=1000 \
    GROUP_ID=1000
RUN set -ex; \
    (getent group ${GROUP_ID} && usermod -g ${GROUP_ID} ubuntu) || groupmod -g ${GROUP_ID} -n dash ubuntu; \
    usermod -u ${USER_ID} -md /home/dash -l dash ubuntu; \
    chown ${USER_ID}:${GROUP_ID} -R /home/dash; \
    mkdir -p /src/dash && \
    chown ${USER_ID}:${GROUP_ID} /src && \
    chown ${USER_ID}:${GROUP_ID} -R /src

WORKDIR /src/dash

USER dash
