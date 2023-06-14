#!/bin/sh
# Copyright (c) 2013-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Set error handling
set -e

# If verbose is non-zero, print messages
verbose=1

# Sets environment variables and navigates to the source directory
set_environment_variables() {
  export LC_ALL=C
  srcdir="$(dirname "$0")"
  cd "$srcdir"
}

# Check for libtoolize and set LIBTOOLIZE variable
check_libtoolize() {
  if [ -z "${LIBTOOLIZE}" ] && LIBTOOLIZE="$(command -v libtoolize)"; then
    LIBTOOLIZE="${LIBTOOLIZE}"
    export LIBTOOLIZE
  fi
}

# Run autoreconf
run_autoreconf() {
  command -V autoreconf >/dev/null || { echo "Error: autoreconf not found."; exit 1; }
  autoreconf --install --force --warnings=all || { echo "Error: autoreconf failed"; exit 1; }
}

# Copy file if newer
copy_if_newer() {
  local src=$1 dst=$2

  if [ ! -f "$src" ]; then
    echo "Source file $src does not exist. Skipping copy operation."
    return
  fi

  if [ "$verbose" -ne 0 ]; then
    echo "Checking if $src is newer than $dst..."
  fi
  
  if [ "$src" -nt "$dst" ]; then
    cp "$src" "$dst" || { echo "Error: Failed to copy $src to $dst."; exit 1; }
    if [ "$verbose" -ne 0 ]; then
      echo "Copied $src to $dst"
    fi
  else
    if [ "$verbose" -ne 0 ]; then
      echo "$src is not newer than $dst. Skipping copy operation."
    fi
  fi
}

# Call functions
set_environment_variables
check_libtoolize
run_autoreconf

copy_if_newer 'depends/config.guess' 'build-aux/config.guess'
copy_if_newer 'depends/config.guess' 'src/secp256k1/build-aux/config.guess'
copy_if_newer 'depends/config.sub' 'build-aux/config.sub'
copy_if_newer 'depends/config.sub' 'src/secp256k1/build-aux/config.sub'