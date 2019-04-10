#!/bin/sh
export LC_ALL=C
set -e

if [ ! -z "${1}" ]; then
  echo "Usage: $0"
  echo
  echo "Downloads the latest stable rust toolchain and supported targets"
  exit 1
fi

export CARGO_HOME=`pwd`/rust-install/.cargo
export RUSTUP_HOME=`pwd`/rust-install/.rustup
mkdir -p ${CARGO_HOME} ${RUSTUP_HOME}

wget -P rust-install https://static.rust-lang.org/rustup/dist/x86_64-unknown-linux-gnu/rustup-init
chmod +x rust-install/rustup-init

./rust-install/rustup-init -y

RUST_HOST=`${CARGO_HOME}/bin/rustc --version -v | grep "host:" | cut -f2 -d' '`
RUST_RELEASE=`${CARGO_HOME}/bin/rustc --version -v | grep "release:" | cut -f2 -d' '`
RUST_COMMIT=`${CARGO_HOME}/bin/rustc --version -v | grep "commit-hash:" | cut -f2 -d' '`
OUT_FILE=rust-stable-${RUST_HOST}-${RUST_RELEASE}.tar.xz

${CARGO_HOME}/bin/rustup target install x86_64-apple-darwin i686-pc-windows-gnu x86_64-pc-windows-gnu aarch64-unknown-linux-gnu arm-unknown-linux-gnueabihf i686-unknown-linux-gnu x86_64-unknown-linux-gnu

echo Creating ${OUT_FILE}. This will take a while.
tar -C rust-install -cJf ${OUT_FILE} .cargo .rustup
rm -rf ./rust-install
