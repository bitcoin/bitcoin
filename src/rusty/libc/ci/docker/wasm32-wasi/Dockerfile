# In the first container we want to assemble the `wasi-sysroot` by compiling it
# from source. This requires a clang 8.0+ compiler with enough wasm support and
# then we're just running a standard `make` inside of what we clone.
FROM ubuntu:18.04 as wasi-sysroot

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    clang \
    cmake \
    curl \
    g++ \
    git \
    libc6-dev \
    libclang-dev \
    make \
    ssh \
    xz-utils

# Fetch clang 8.0+ which is used to compile the wasi target and link our
# programs together.
RUN curl http://releases.llvm.org/8.0.0/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz | tar xJf -
RUN mv /clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-18.04 /wasmcc

# Note that we're using `git reset --hard` to pin to a specific commit for
# verification for now. The sysroot is currently in somewhat of a state of flux
# and is expected to have breaking changes, so this is an attempt to mitigate
# those breaking changes on `libc`'s own CI
RUN git clone https://github.com/CraneStation/wasi-sysroot && \
  cd wasi-sysroot && \
  git reset --hard eee6ee7566e26f2535eb6088c8494a112ff423b9
RUN make -C wasi-sysroot install -j $(nproc) WASM_CC=/wasmcc/bin/clang INSTALL_DIR=/wasi-sysroot

# This is a small wrapper script which executes the actual clang binary in
# `/wasmcc` and then is sure to pass the right `--sysroot` argument which we
# just built above.
COPY docker/wasm32-wasi/clang.sh /wasi-sysroot/bin/clang

# In the second container we're going to build the `wasmtime` binary which is
# used to execute wasi executables. This is a standard Rust project so we're
# just checking out a known revision (which pairs with the sysroot one we
# downlaoded above) and then we're building it with Cargo
FROM ubuntu:18.04 as wasmtime

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    clang \
    cmake \
    curl \
    g++ \
    git \
    libclang-dev \
    make \
    ssh

RUN curl -sSf https://sh.rustup.rs |  sh -s -- -y
ENV PATH=/root/.cargo/bin:$PATH

RUN apt-get install -y --no-install-recommends python
RUN git clone --recursive https://github.com/CraneStation/wasmtime wasmtime && \
  cd wasmtime && \
  git reset --hard a2647878977726935c3d04c05cabad9607ec7606
RUN cargo build --release --manifest-path wasmtime/Cargo.toml

# And finally in the last image we're going to assemble everything together.
# We'll install things needed at runtime for now and then copy over the
# sysroot/wasmtime artifacts into their final location.
FROM ubuntu:18.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    gcc \
    libc6-dev \
    libxml2 \
    ca-certificates

# Copy over clang we downloaded to link executables ...
COPY --from=wasi-sysroot /wasmcc /wasmcc/
# ... and the sysroot we built to link executables against ...
COPY --from=wasi-sysroot /wasi-sysroot/ /wasi-sysroot/
# ... and finally wasmtime to actually execute binaries
COPY --from=wasmtime /wasmtime/target/release/wasmtime /usr/bin/

# Of note here is our clang wrapper which just executes a normal clang
# executable with the right sysroot, and then we're sure to turn off the
# crt-static feature to ensure that the CRT that we're specifying with `clang`
# is used.
ENV CARGO_TARGET_WASM32_WASI_RUNNER=wasmtime \
  CARGO_TARGET_WASM32_WASI_LINKER=/wasi-sysroot/bin/clang \
  CC_wasm32_wasi=/wasi-sysroot/bin/clang \
  PATH=$PATH:/rust/bin \
  RUSTFLAGS=-Ctarget-feature=-crt-static
