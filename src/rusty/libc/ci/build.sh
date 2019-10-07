#!/usr/bin/env sh

# Checks that libc builds properly for all supported targets on a particular
# Rust version:
# The FILTER environment variable can be used to select which target(s) to build.
# For example: set FILTER to vxworks to select the targets that has vxworks in name

set -ex

: "${TOOLCHAIN?The TOOLCHAIN environment variable must be set.}"
: "${OS?The OS environment variable must be set.}"

RUST=${TOOLCHAIN}

echo "Testing Rust ${RUST} on ${OS}"

if [ "${TOOLCHAIN}" = "nightly" ] ; then
    cargo +nightly install cargo-xbuild -Z install-upgrade
    rustup component add rust-src
fi

test_target() {
    BUILD_CMD="${1}"
    TARGET="${2}"
    NO_STD="${3}"

    opt=
    if [ "${TARGET}" = "x86_64-unknown-linux-gnux32" ]; then
        # FIXME: x86_64-unknown-linux-gnux32 fail to compile without
        # --release
        #
        # See https://github.com/rust-lang/rust/issues/45417
        opt="--release"
    fi
    # FIXME: https://github.com/rust-lang/rust/issues/61174
    if [ "${TARGET}" = "sparcv9-sun-solaris" ] ||
       [ "${TARGET}" = "x86_64-sun-solaris" ]; then
        return 0
    fi

    # If there is a std component, fetch it:
    if [ "${NO_STD}" != "1" ]; then
        # FIXME: rustup often fails to download some artifacts due to network
        # issues, so we retry this N times.
        N=5
        n=0
        until [ $n -ge $N ]
        do
            if rustup target add "${TARGET}" --toolchain "${RUST}" ; then
                break
            fi
            n=$((n+1))
            sleep 1
        done
    fi

    # Test that libc builds without any default features (no libstd)
    cargo "+${RUST}" "${BUILD_CMD}" -vv $opt --no-default-features --target "${TARGET}"

    # Test that libc builds with default features (e.g. libstd)
    # if the target supports libstd
    if [ "$NO_STD" != "1" ]; then
        cargo "+${RUST}" "${BUILD_CMD}" -vv $opt --target "${TARGET}"
    fi

    # Test that libc builds with the `extra_traits` feature
    cargo "+${RUST}" "${BUILD_CMD}" -vv $opt --no-default-features --target "${TARGET}" \
          --features extra_traits

    # Also test that it builds with `extra_traits` and default features:
    if [ "$NO_STD" != "1" ]; then
        cargo "+${RUST}" "${BUILD_CMD}" -vv $opt --target "${TARGET}" \
              --features extra_traits
    fi
}

RUST_LINUX_TARGETS="\
aarch64-linux-android \
aarch64-unknown-linux-gnu \
arm-linux-androideabi \
arm-unknown-linux-gnueabi \
arm-unknown-linux-gnueabihf \
armv7-linux-androideabi \
armv7-unknown-linux-gnueabihf \
i586-unknown-linux-gnu \
i686-linux-android \
i686-unknown-freebsd \
i686-unknown-linux-gnu \
i686-unknown-linux-musl \
mips-unknown-linux-gnu \
mips-unknown-linux-musl \
mips64-unknown-linux-gnuabi64 \
mips64el-unknown-linux-gnuabi64 \
mipsel-unknown-linux-gnu \
mipsel-unknown-linux-gnu \
mipsel-unknown-linux-musl \
powerpc-unknown-linux-gnu \
powerpc64-unknown-linux-gnu \
powerpc64le-unknown-linux-gnu \
s390x-unknown-linux-gnu \
x86_64-unknown-freebsd \
x86_64-unknown-linux-gnu \
x86_64-unknown-linux-musl \
x86_64-unknown-netbsd \
"

RUST_GT_1_13_LINUX_TARGETS="\
arm-unknown-linux-musleabi \
arm-unknown-linux-musleabihf \
armv7-unknown-linux-musleabihf \
sparc64-unknown-linux-gnu \
wasm32-unknown-emscripten \
x86_64-linux-android \
x86_64-rumprun-netbsd \
"
RUST_GT_1_19_LINUX_TARGETS="\
aarch64-unknown-linux-musl \
sparcv9-sun-solaris \
wasm32-unknown-unknown \
x86_64-sun-solaris \
"
RUST_GT_1_24_LINUX_TARGETS="\
i586-unknown-linux-musl \
x86_64-unknown-cloudabi \
"

# FIXME: temporarirly disable the redox target
# https://github.com/rust-lang/libc/issues/1457
# x86_64-unknown-redox
RUST_NIGHTLY_LINUX_TARGETS="\
aarch64-fuchsia \
armv5te-unknown-linux-gnueabi \
armv5te-unknown-linux-musleabi \
i686-pc-windows-gnu \
wasm32-wasi \
x86_64-fortanix-unknown-sgx \
x86_64-fuchsia \
x86_64-pc-windows-gnu \
x86_64-unknown-linux-gnux32 \
"

RUST_OSX_TARGETS="\
aarch64-apple-ios \
armv7-apple-ios \
armv7s-apple-ios \
i386-apple-ios \
i686-apple-darwin \
x86_64-apple-darwin \
x86_64-apple-ios \
"

# The targets are listed here alphabetically
TARGETS=""
case "${OS}" in
    linux*)
        TARGETS="${RUST_LINUX_TARGETS}"

        if [ "${RUST}" != "1.13.0" ]; then
            TARGETS="${TARGETS} ${RUST_GT_1_13_LINUX_TARGETS}"
            if [ "${RUST}" != "1.19.0" ]; then
                TARGETS="${TARGETS} ${RUST_GT_1_19_LINUX_TARGETS}"
                if [ "${RUST}" != "1.24.0" ]; then
                    TARGETS="${TARGETS} ${RUST_GT_1_24_LINUX_TARGETS}"
                fi
            fi
        fi

        if [ "${RUST}" = "nightly" ]; then
            TARGETS="${TARGETS} ${RUST_NIGHTLY_LINUX_TARGETS}"
        fi

        ;;
    osx*)
        TARGETS="${RUST_OSX_TARGETS}"
        ;;
    *)
        ;;
esac

for TARGET in $TARGETS; do
    if echo "$TARGET"|grep -q "$FILTER";then
        test_target build "$TARGET"
    fi
done

# FIXME: https://github.com/rust-lang/rust/issues/58564
# sparc-unknown-linux-gnu
# FIXME: https://github.com/rust-lang/rust/issues/62932
# thumbv6m-none-eabi
RUST_LINUX_NO_CORE_TARGETS="\
aarch64-pc-windows-msvc \
aarch64-unknown-cloudabi \
aarch64-unknown-freebsd \
aarch64-unknown-hermit \
aarch64-unknown-netbsd \
aarch64-unknown-openbsd \
armebv7r-none-eabi \
armebv7r-none-eabihf \
armv7-unknown-cloudabi-eabihf \
armv7r-none-eabi \
armv7r-none-eabihf \
hexagon-unknown-linux-musl \
i586-pc-windows-msvc \
i686-pc-windows-msvc \
i686-unknown-cloudabi \
i686-unknown-haiku \
i686-unknown-netbsd \
i686-unknown-openbsd \
mips-unknown-linux-uclibc \
mipsel-unknown-linux-uclibc \
mips64-unknown-linux-muslabi64 \
mips64el-unknown-linux-muslabi64 \
nvptx64-nvidia-cuda \
powerpc-unknown-linux-gnuspe \
powerpc-unknown-netbsd \
powerpc64-unknown-freebsd \
riscv32imac-unknown-none-elf \
riscv32imc-unknown-none-elf \
sparc64-unknown-netbsd \

thumbv7em-none-eabi \
thumbv7em-none-eabihf \
thumbv7m-none-eabi \
thumbv7neon-linux-androideabi \
thumbv7neon-unknown-linux-gnueabihf \
thumbv8m.main-none-eabi \
x86_64-pc-windows-msvc
x86_64-unknown-dragonfly \
x86_64-unknown-haiku \
x86_64-unknown-hermit \
x86_64-unknown-l4re-uclibc \
x86_64-unknown-openbsd \
armv7-wrs-vxworks-eabihf \
aarch64-wrs-vxworks \
i686-wrs-vxworks \
x86_64-wrs-vxworks \
powerpc-wrs-vxworks \
powerpc-wrs-vxworks-spe \
powerpc64-wrs-vxworks \
"

if [ "${RUST}" = "nightly" ] && [ "${OS}" = "linux" ]; then
    for TARGET in $RUST_LINUX_NO_CORE_TARGETS; do
        if echo "$TARGET"|grep -q "$FILTER";then
            test_target xbuild "$TARGET" 1
        fi
    done

    # Nintendo switch
    cargo clean
    mkdir -p target
    (
        cd target
        wget https://github.com/devkitPro/pacman/releases/download/devkitpro-pacman-1.0.1/devkitpro-pacman.deb
        sudo dpkg -i devkitpro-pacman.deb
        sudo dkp-pacman -Sy
        sudo dkp-pacman -Syu
        sudo dkp-pacman -S -v --noconfirm switch-dev devkitA64
    )
    cp ci/switch.json switch.json
    PATH="$PATH:/opt/devkitpro/devkitA64/bin"
    PATH="$PATH:/opt/devkitpro/tools/bin"
    cargo xbuild --target switch.json
fi

