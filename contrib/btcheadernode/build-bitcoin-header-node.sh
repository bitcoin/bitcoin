#!/usr/bin/env bash
# Build pinned Bitcoin Core binaries for sentry BTC header-node checks.
set -euo pipefail

usage() {
    cat <<'EOF'
Usage:
  build-bitcoin-header-node.sh --src-root <path> --build-root <path> [--jobs <n>] [--force-clean]

Options:
  --src-root      Syscoin source root (contains contrib/btcheadernode/bitcoin.lock)
  --build-root    Syscoin build root (output goes to <build-root>/src/bin/btcheadernode)
  --jobs          Parallel make jobs (default: detected CPU count)
  --force-clean   Remove existing Bitcoin source/build workdirs before rebuilding
  -h, --help      Show this help

Environment:
  BTCHEADERNODE_SOURCE_ARCHIVE     Optional local Bitcoin source archive (tar/tar.gz/tar.xz)
                                   used instead of cloning from BITCOIN_REPO.
  BTCHEADERNODE_STATIC_LINK_FLAGS  Linker flags for bitcoind/bitcoin-cli
                                   (default: "-static-libstdc++").
  LDFLAGS                          Optional base linker flags inherited from outer build
                                   (e.g. Guix HOST_LDFLAGS); helper appends static flags.
EOF
}

hash_file() {
    local file_path="$1"
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$file_path" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$file_path" | awk '{print $1}'
    else
        echo "no-sha256-tool"
    fi
}

first_existing_file() {
    for candidate in "$@"; do
        if [[ -f "$candidate" ]]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done
    return 1
}

detect_jobs() {
    if command -v nproc >/dev/null 2>&1; then
        nproc
    elif command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.ncpu
    else
        echo 1
    fi
}

detect_compiler() {
    local env_value="$1"
    shift
    local env_bin="${env_value%% *}"
    if [[ -n "$env_bin" ]] && command -v "$env_bin" >/dev/null 2>&1; then
        printf '%s\n' "$env_bin"
        return 0
    fi
    local candidate
    for candidate in "$@"; do
        if command -v "$candidate" >/dev/null 2>&1; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done
    return 1
}

detect_boost_version() {
    local include_dir="$1"
    local version_header="$include_dir/boost/version.hpp"
    if [[ -f "$version_header" ]]; then
        local boost_version
        boost_version="$(awk '/^#define[[:space:]]+BOOST_VERSION[[:space:]]+[0-9]+/{print $3; exit}' "$version_header")"
        if [[ "$boost_version" =~ ^[0-9]+$ ]]; then
            local major minor patch
            major=$((boost_version / 100000))
            minor=$(((boost_version / 100) % 1000))
            patch=$((boost_version % 100))
            printf '%s\n' "${major}.${minor}.${patch}"
            return 0
        fi
    fi
    printf '%s\n' "1.74.0"
}

extract_include_dirs_from_flags() {
    local flags="$1"
    local token=""
    local expect_path=0
    for token in $flags; do
        if [[ "$expect_path" -eq 1 ]]; then
            printf '%s\n' "$token"
            expect_path=0
            continue
        fi
        case "$token" in
            -I|-isystem)
                expect_path=1
                ;;
            -I*)
                printf '%s\n' "${token#-I}"
                ;;
            -isystem*)
                printf '%s\n' "${token#-isystem}"
                ;;
        esac
    done
}

detect_boost_include_dir_from_flags() {
    local flags="$1"
    local include_dir=""
    while IFS= read -r include_dir; do
        [[ -n "$include_dir" ]] || continue
        include_dir="${include_dir%/}"
        if [[ -d "$include_dir/boost" ]]; then
            printf '%s\n' "$include_dir"
            return 0
        fi
    done < <(extract_include_dirs_from_flags "$flags")
    return 1
}

fix_cmake_libevent_link_order_for_static() {
    local cmake_lists="$1/src/CMakeLists.txt"
    [[ -f "$cmake_lists" ]] || return 0

    # With static libevent archives, link order matters: extra must precede core.
    local tmp_file
    tmp_file="$(mktemp "${cmake_lists}.tmp.XXXXXX")"
    sed '/libevent::core/{N;s/libevent::core\n\([[:space:]]*\)libevent::extra/libevent::extra\n\1libevent::core/;}' "$cmake_lists" > "$tmp_file"
    mv "$tmp_file" "$cmake_lists"
}

write_boost_config_compat() {
    local config_dir="$1"
    local include_dir="$2"
    local boost_version="$3"

    mkdir -p "$config_dir"

    cat > "$config_dir/BoostConfig.cmake" <<EOF
set(Boost_FOUND TRUE)
set(Boost_INCLUDE_DIR "${include_dir}")
set(Boost_VERSION_STRING "${boost_version}")
set(boost_headers_FOUND TRUE)
set(boost_headers_DIR "\${CMAKE_CURRENT_LIST_DIR}")
if(NOT TARGET Boost::headers)
  add_library(Boost::headers INTERFACE IMPORTED)
  set_target_properties(Boost::headers PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${include_dir}"
  )
endif()
if(NOT TARGET Boost::boost)
  add_library(Boost::boost INTERFACE IMPORTED)
  set_target_properties(Boost::boost PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${include_dir}"
  )
endif()
EOF

    cat > "$config_dir/BoostConfigVersion.cmake" <<EOF
set(PACKAGE_VERSION "${boost_version}")
if(PACKAGE_FIND_VERSION VERSION_GREATER PACKAGE_VERSION)
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
endif()
if(PACKAGE_FIND_VERSION VERSION_EQUAL PACKAGE_VERSION)
  set(PACKAGE_VERSION_EXACT TRUE)
endif()
EOF
}

SRC_ROOT=""
BUILD_ROOT=""
JOBS=""
FORCE_CLEAN=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --src-root)
            [[ $# -ge 2 ]] || { echo "Missing value for --src-root" >&2; exit 1; }
            SRC_ROOT="$2"
            shift 2
            ;;
        --build-root)
            [[ $# -ge 2 ]] || { echo "Missing value for --build-root" >&2; exit 1; }
            BUILD_ROOT="$2"
            shift 2
            ;;
        --jobs)
            [[ $# -ge 2 ]] || { echo "Missing value for --jobs" >&2; exit 1; }
            JOBS="$2"
            shift 2
            ;;
        --force-clean)
            FORCE_CLEAN=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

[[ -n "$SRC_ROOT" && -n "$BUILD_ROOT" ]] || {
    echo "Both --src-root and --build-root are required." >&2
    usage >&2
    exit 1
}

LOCK_FILE="$SRC_ROOT/contrib/btcheadernode/bitcoin.lock"
if [[ ! -f "$LOCK_FILE" ]]; then
    echo "Missing lock file: $LOCK_FILE" >&2
    exit 1
fi

# shellcheck source=/dev/null
source "$LOCK_FILE"

: "${BITCOIN_REPO:?BITCOIN_REPO must be set in bitcoin.lock}"
: "${BITCOIN_COMMIT:?BITCOIN_COMMIT must be set in bitcoin.lock}"

if [[ -z "$JOBS" ]]; then
    JOBS="$(detect_jobs)"
fi

STATIC_LINK_FLAGS="${BTCHEADERNODE_STATIC_LINK_FLAGS:--static-libstdc++}"
SOURCE_ARCHIVE="${BTCHEADERNODE_SOURCE_ARCHIVE:-}"
BASE_LDFLAGS="${LDFLAGS:-}"
BASE_CPPFLAGS="${CPPFLAGS:-}"
BASE_CFLAGS="${CFLAGS:-}"
BASE_CXXFLAGS="${CXXFLAGS:-}"

OUT_ROOT="$BUILD_ROOT/src/bin/btcheadernode"
WORK_ROOT="$OUT_ROOT/work"
BITCOIN_SRC="$WORK_ROOT/bitcoin-src"
BITCOIN_BUILD="$WORK_ROOT/bitcoin-build"
OUTPUT_BIN_DIR="$OUT_ROOT/bin"
STAMP_FILE="$OUT_ROOT/.build-stamp"

PATCH_ABS=""
if [[ -n "${BITCOIN_PATCH:-}" ]]; then
    PATCH_ABS="$SRC_ROOT/contrib/btcheadernode/${BITCOIN_PATCH}"
    if [[ ! -f "$PATCH_ABS" ]]; then
        echo "Configured patch file not found: $PATCH_ABS" >&2
        exit 1
    fi
fi

mkdir -p "$OUT_ROOT"
mkdir -p "$WORK_ROOT"

PATCH_DIGEST="none"
if [[ -n "$PATCH_ABS" ]]; then
    PATCH_DIGEST="$(hash_file "$PATCH_ABS")"
fi
SOURCE_ARCHIVE_DIGEST="none"
if [[ -n "$SOURCE_ARCHIVE" ]]; then
    if [[ ! -f "$SOURCE_ARCHIVE" ]]; then
        echo "Configured BTCHEADERNODE_SOURCE_ARCHIVE not found: $SOURCE_ARCHIVE" >&2
        exit 1
    fi
    SOURCE_ARCHIVE_DIGEST="$(hash_file "$SOURCE_ARCHIVE")"
fi
LOCK_DIGEST="$(hash_file "$LOCK_FILE")"
BUILD_FINGERPRINT="${BITCOIN_COMMIT}:${LOCK_DIGEST}:${PATCH_DIGEST}:${SOURCE_ARCHIVE_DIGEST}:${BASE_LDFLAGS}:${STATIC_LINK_FLAGS}"

if [[ -f "$STAMP_FILE" ]] && [[ -x "$OUTPUT_BIN_DIR/bitcoind" ]] && [[ -x "$OUTPUT_BIN_DIR/bitcoin-cli" ]]; then
    if [[ "$(cat "$STAMP_FILE")" == "$BUILD_FINGERPRINT" ]]; then
        echo "btcheadernode: build is up-to-date ($BUILD_FINGERPRINT)"
        exit 0
    fi
fi

EFFECTIVE_LINKER_FLAGS="$BASE_LDFLAGS"
if [[ -n "$STATIC_LINK_FLAGS" ]]; then
    EFFECTIVE_LINKER_FLAGS="${EFFECTIVE_LINKER_FLAGS:+$EFFECTIVE_LINKER_FLAGS }$STATIC_LINK_FLAGS"
fi

if [[ "$FORCE_CLEAN" -eq 1 ]]; then
    rm -rf "$BITCOIN_SRC" "$BITCOIN_BUILD"
fi

if [[ -n "$SOURCE_ARCHIVE" ]]; then
    rm -rf "$BITCOIN_SRC"
    mkdir -p "$BITCOIN_SRC"
    tar -xf "$SOURCE_ARCHIVE" -C "$BITCOIN_SRC" --strip-components=1
    git -C "$BITCOIN_SRC" init -q
else
    if [[ ! -d "$BITCOIN_SRC/.git" ]]; then
        git clone "$BITCOIN_REPO" "$BITCOIN_SRC"
    elif ! git -C "$BITCOIN_SRC" remote get-url origin >/dev/null 2>&1; then
        # Recreate the clone if previous archive-based extraction left a git repo
        # without an origin remote.
        rm -rf "$BITCOIN_SRC"
        git clone "$BITCOIN_REPO" "$BITCOIN_SRC"
    fi

    git -C "$BITCOIN_SRC" fetch --tags --prune origin

    if [[ -n "${BITCOIN_REF:-}" ]]; then
        RESOLVED_REF="$(git -C "$BITCOIN_SRC" rev-parse --verify "${BITCOIN_REF}^{commit}")"
        if [[ "$RESOLVED_REF" != "$BITCOIN_COMMIT" ]]; then
            echo "bitcoin.lock mismatch: BITCOIN_REF ($BITCOIN_REF -> $RESOLVED_REF) != BITCOIN_COMMIT ($BITCOIN_COMMIT)" >&2
            exit 1
        fi
    fi

    git -C "$BITCOIN_SRC" checkout --detach "$BITCOIN_COMMIT"
    git -C "$BITCOIN_SRC" clean -fdx
    git -C "$BITCOIN_SRC" reset --hard "$BITCOIN_COMMIT"
fi

if [[ -n "$PATCH_ABS" ]]; then
    git -C "$BITCOIN_SRC" apply --check "$PATCH_ABS"
    git -C "$BITCOIN_SRC" apply "$PATCH_ABS"
fi

fix_cmake_libevent_link_order_for_static "$BITCOIN_SRC"

mkdir -p "$BITCOIN_BUILD"
mkdir -p "$OUTPUT_BIN_DIR"

if [[ -x "$BITCOIN_SRC/autogen.sh" ]]; then
    "$BITCOIN_SRC/autogen.sh"
    if [[ ! -x "$BITCOIN_SRC/configure" ]]; then
        echo "Autotools build selected but configure script was not generated." >&2
        exit 1
    fi

    (
        cd "$BITCOIN_BUILD"
        LDFLAGS="$EFFECTIVE_LINKER_FLAGS" "$BITCOIN_SRC/configure" \
            --disable-bench \
            --disable-fuzz-binary \
            --disable-man \
            --disable-shared \
            --disable-tests \
            --disable-wallet \
            --enable-static \
            --without-bdb \
            --without-gui \
            --without-miniupnpc \
            --without-natpmp \
            --without-zmq
        make -j"$JOBS" src/bitcoind src/bitcoin-cli
    )
else
    if ! command -v cmake >/dev/null 2>&1; then
        echo "cmake is required to build this pinned Bitcoin version." >&2
        exit 1
    fi

    C_COMPILER="$(detect_compiler "${CC:-}" x86_64-linux-gnu-gcc gcc cc)" || {
        echo "Could not locate a C compiler for Bitcoin CMake build." >&2
        exit 1
    }
    CXX_COMPILER="$(detect_compiler "${CXX:-}" x86_64-linux-gnu-g++ g++ c++)" || {
        echo "Could not locate a C++ compiler for Bitcoin CMake build." >&2
        exit 1
    }

    BOOST_INCLUDE_DIR="$(detect_boost_include_dir_from_flags "$BASE_CPPFLAGS" || true)"
    if [[ -z "$BOOST_INCLUDE_DIR" ]]; then
        BOOST_INCLUDE_DIR="$(detect_boost_include_dir_from_flags "$BASE_CXXFLAGS" || true)"
    fi
    if [[ -z "$BOOST_INCLUDE_DIR" && -n "${HOST:-}" && -d "$SRC_ROOT/depends/${HOST}/include/boost" ]]; then
        BOOST_INCLUDE_DIR="$SRC_ROOT/depends/${HOST}/include"
    elif [[ -z "$BOOST_INCLUDE_DIR" && -d "$SRC_ROOT/depends/x86_64-linux-gnu/include/boost" ]]; then
        BOOST_INCLUDE_DIR="$SRC_ROOT/depends/x86_64-linux-gnu/include"
    elif [[ -z "$BOOST_INCLUDE_DIR" && -d "/usr/include/boost" ]]; then
        BOOST_INCLUDE_DIR="/usr/include"
    elif [[ -z "$BOOST_INCLUDE_DIR" && -d "/usr/local/include/boost" ]]; then
        BOOST_INCLUDE_DIR="/usr/local/include"
    elif [[ -z "$BOOST_INCLUDE_DIR" && -d "/opt/homebrew/include/boost" ]]; then
        BOOST_INCLUDE_DIR="/opt/homebrew/include"
    fi

    if [[ -z "$BOOST_INCLUDE_DIR" ]]; then
        echo "Could not locate Boost headers for Bitcoin CMake build." >&2
        exit 1
    fi

    BOOST_VERSION="$(detect_boost_version "$BOOST_INCLUDE_DIR")"
    BOOST_CONFIG_DIR="$BITCOIN_BUILD/cmake/boost-config"
    write_boost_config_compat "$BOOST_CONFIG_DIR" "$BOOST_INCLUDE_DIR" "$BOOST_VERSION"

    CMAKE_C_FLAGS="$BASE_CFLAGS"
    if [[ -n "$BASE_CPPFLAGS" ]]; then
        CMAKE_C_FLAGS="${CMAKE_C_FLAGS:+$CMAKE_C_FLAGS }$BASE_CPPFLAGS"
    fi
    CMAKE_CXX_FLAGS="$BASE_CXXFLAGS"
    if [[ -n "$BASE_CPPFLAGS" ]]; then
        CMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS:+$CMAKE_CXX_FLAGS }$BASE_CPPFLAGS"
    fi

    cmake -S "$BITCOIN_SRC" -B "$BITCOIN_BUILD" \
        -DCMAKE_BUILD_TYPE=Release \
        "-DCMAKE_C_COMPILER=${C_COMPILER}" \
        "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}" \
        "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}" \
        "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}" \
        "-DBoost_DIR=${BOOST_CONFIG_DIR}" \
        -DBUILD_GUI=OFF \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_TESTS=OFF \
        -DBUILD_BENCH=OFF \
        -DBUILD_FUZZ_BINARY=OFF \
        -DBUILD_TX=OFF \
        -DBUILD_UTIL=OFF \
        -DBUILD_UTIL_CHAINSTATE=OFF \
        -DBUILD_KERNEL_LIB=OFF \
        -DENABLE_WALLET=OFF \
        -DBUILD_WALLET_TOOL=OFF \
        -DENABLE_EXTERNAL_SIGNER=OFF \
        -DENABLE_IPC=OFF \
        "-DCMAKE_EXE_LINKER_FLAGS=${EFFECTIVE_LINKER_FLAGS}" \
        -DWITH_ZMQ=OFF \
        -DINSTALL_MAN=OFF
    cmake --build "$BITCOIN_BUILD" --parallel "$JOBS" --target bitcoind bitcoin-cli
fi

BITCOIND_BUILT_PATH="$(first_existing_file "$BITCOIN_BUILD/src/bitcoind" "$BITCOIN_BUILD/bin/bitcoind")" || {
    echo "Could not locate built bitcoind binary." >&2
    exit 1
}
BITCOINCLI_BUILT_PATH="$(first_existing_file "$BITCOIN_BUILD/src/bitcoin-cli" "$BITCOIN_BUILD/bin/bitcoin-cli")" || {
    echo "Could not locate built bitcoin-cli binary." >&2
    exit 1
}

cp "$BITCOIND_BUILT_PATH" "$OUTPUT_BIN_DIR/bitcoind"
cp "$BITCOINCLI_BUILT_PATH" "$OUTPUT_BIN_DIR/bitcoin-cli"
chmod 755 "$OUTPUT_BIN_DIR/bitcoind" "$OUTPUT_BIN_DIR/bitcoin-cli"

printf '%s\n' "$BUILD_FINGERPRINT" > "$STAMP_FILE"
echo "btcheadernode: built binaries in $OUTPUT_BIN_DIR"
