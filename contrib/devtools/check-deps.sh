#!/usr/bin/env bash

export LC_ALL=C
set -Eeuo pipefail

# Declare paths to libraries
declare -A LIBS
LIBS[cli]="libbitcoin_cli.a"
LIBS[common]="libbitcoin_common.a"
LIBS[consensus]="libbitcoin_consensus.a"
LIBS[crypto]="libbitcoin_crypto.a"
LIBS[node]="libbitcoin_node.a"
LIBS[util]="libbitcoin_util.a"
LIBS[wallet]="libbitcoin_wallet.a"

# Declare allowed dependencies "X Y" where X is allowed to depend on Y. This
# list is taken from doc/design/libraries.md.
ALLOWED_DEPENDENCIES=(
    "cli common"
    "cli util"
    "common consensus"
    "common crypto"
    "common util"
    "consensus crypto"
    "node common"
    "node consensus"
    "node crypto"
    "node kernel"
    "node util"
    "util crypto"
    "wallet common"
    "wallet crypto"
    "wallet util"
)

# Add minor dependencies omitted from doc/design/libraries.md to keep the
# dependency diagram simple.
ALLOWED_DEPENDENCIES+=(
    "wallet consensus"
)

# Declare list of known errors that should be suppressed.
declare -A SUPPRESS
# init.cpp file currently calls Berkeley DB sanity check function on startup, so
# there is an undocumented dependency of the node library on the wallet library.
SUPPRESS["init.cpp.o bdb.cpp.o _ZN6wallet27BerkeleyDatabaseSanityCheckEv"]=1
# init/common.cpp file calls InitError and InitWarning from interface_ui which
# is currently part of the node library. interface_ui should just be part of the
# common library instead, and is moved in
# https://github.com/bitcoin/bitcoin/issues/10102
SUPPRESS["common.cpp.o interface_ui.cpp.o _Z11InitWarningRK13bilingual_str"]=1
SUPPRESS["common.cpp.o interface_ui.cpp.o _Z9InitErrorRK13bilingual_str"]=1

usage() {
   echo "Usage: $(basename "${BASH_SOURCE[0]}") [BUILD_DIR]"
}

# Output makefile targets, converting library .a paths to CMake targets
lib_targets() {
  for lib in "${!LIBS[@]}"; do
      for lib_path in ${LIBS[$lib]}; do
          local name="${lib_path##*/}"
          name="${name#lib}"
          name="${name%.a}"
          echo "$name"
      done
  done
}

# Extract symbol names and object names and write to text files
extract_symbols() {
    local temp_dir="$1"
    for lib in "${!LIBS[@]}"; do
        for lib_path in ${LIBS[$lib]}; do
            nm -o "$lib_path" | { grep ' T \| W ' || true; } | awk '{print $3, $1}' >> "${temp_dir}/${lib}_exports.txt"
            nm -o "$lib_path" | { grep ' U ' || true; } | awk '{print $3, $1}' >> "${temp_dir}/${lib}_imports.txt"
            awk '{print $1}' "${temp_dir}/${lib}_exports.txt" | sort -u > "${temp_dir}/${lib}_exported_symbols.txt"
            awk '{print $1}' "${temp_dir}/${lib}_imports.txt" | sort -u > "${temp_dir}/${lib}_imported_symbols.txt"
        done
    done
}

# Lookup object name(s) corresponding to symbol name in text file
obj_names() {
    local symbol="$1"
    local txt_file="$2"
    sed -n "s/^$symbol [^:]\\+:\\([^:]\\+\\):[^:]*\$/\\1/p" "$txt_file" | sort -u
}

# Iterate through libraries and find disallowed dependencies
check_libraries() {
    local temp_dir="$1"
    local result=0
    for src in "${!LIBS[@]}"; do
        for dst in "${!LIBS[@]}"; do
            if [ "$src" != "$dst" ] && ! is_allowed "$src" "$dst"; then
                if ! check_disallowed "$src" "$dst" "$temp_dir"; then
                    result=1
                fi
            fi
        done
    done
    check_not_suppressed
    return $result
}

# Return whether src library is allowed to depend on dst.
is_allowed() {
    local src="$1"
    local dst="$2"
    for allowed in "${ALLOWED_DEPENDENCIES[@]}"; do
        if [ "$src $dst" = "$allowed" ]; then
            return 0
        fi
    done
    return 1
}

# Return whether src library imports any symbols from dst, assuming src is not
# allowed to depend on dst.
check_disallowed() {
    local src="$1"
    local dst="$2"
    local temp_dir="$3"
    local result=0

    # Loop over symbol names exported by dst and imported by src
    while read symbol; do
        local dst_obj
        dst_obj=$(obj_names "$symbol" "${temp_dir}/${dst}_exports.txt")
        while read src_obj; do
            if ! check_suppress "$src_obj" "$dst_obj" "$symbol"; then
                echo "Error: $src_obj depends on $dst_obj symbol '$(c++filt "$symbol")', can suppress with:"
                echo "    SUPPRESS[\"$src_obj $dst_obj $symbol\"]=1"
                result=1
            fi
        done < <(obj_names "$symbol" "${temp_dir}/${src}_imports.txt")
    done < <(comm -12 "${temp_dir}/${dst}_exported_symbols.txt" "${temp_dir}/${src}_imported_symbols.txt")
    return $result
}

# Declare array to track errors which were suppressed.
declare -A SUPPRESSED

# Return whether error should be suppressed and record suppression in
# SUPPRESSED array.
check_suppress() {
    local src_obj="$1"
    local dst_obj="$2"
    local symbol="$3"
    for suppress in "${!SUPPRESS[@]}"; do
        read suppress_src suppress_dst suppress_pattern <<<"$suppress"
        if [[ "$src_obj" == "$suppress_src" && "$dst_obj" == "$suppress_dst" && "$symbol" =~ $suppress_pattern ]]; then
            SUPPRESSED["$suppress"]=1
            return 0
        fi
    done
    return 1
}

# Warn about error which were supposed to be suppressed, but were not encountered.
check_not_suppressed() {
    for suppress in "${!SUPPRESS[@]}"; do
        if [[ ! -v SUPPRESSED[$suppress] ]]; then
            echo >&2 "Warning: suppression '$suppress' was ignored, consider deleting."
        fi
    done
}

# Check arguments.
if [ "$#" = 0 ]; then
    BUILD_DIR="$(dirname "${BASH_SOURCE[0]}")/../../build"
elif [ "$#" = 1 ]; then
    BUILD_DIR="$1"
else
    echo >&2 "Error: wrong number of arguments."
    usage >&2
    exit 1
fi
if [ ! -f "$BUILD_DIR/Makefile" ]; then
    echo >&2 "Error: directory '$BUILD_DIR' does not contain a makefile, please specify path to build directory for library targets."
    usage >&2
    exit 1
fi

# Build libraries and run checks.
# shellcheck disable=SC2046
cmake --build "$BUILD_DIR" -j"$(nproc)" -t $(lib_targets)
TEMP_DIR="$(mktemp -d)"
cd "$BUILD_DIR/lib"
extract_symbols "$TEMP_DIR"
if check_libraries "$TEMP_DIR"; then
    echo "Success! No unexpected dependencies were detected."
    RET=0
else
    echo >&2 "Error: Unexpected dependencies were detected. Check previous output."
    RET=1
fi
rm -r "$TEMP_DIR"
exit $RET
