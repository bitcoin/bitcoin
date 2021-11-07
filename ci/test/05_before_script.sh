#!/usr/bin/env bash
#
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

# Make sure default datadir does not exist and is never read by creating a dummy file
if [ "$CI_OS_NAME" == "macos" ]; then
  echo > "${HOME}/Library/Application Support/Bitcoin"
else
  DOCKER_EXEC echo \> \$HOME/.bitcoin
fi

DOCKER_EXEC mkdir -p "${DEPENDS_DIR}/SDKs" "${DEPENDS_DIR}/sdk-sources"

OSX_SDK_BASENAME="Xcode-${XCODE_VERSION}-${XCODE_BUILD_ID}-extracted-SDK-with-libcxx-headers.tar.gz"
OSX_SDK_PATH="${DEPENDS_DIR}/sdk-sources/${OSX_SDK_BASENAME}"

if [ -n "$XCODE_VERSION" ] && [ ! -f "$OSX_SDK_PATH" ]; then
  DOCKER_EXEC curl --location --fail "${SDK_URL}/${OSX_SDK_BASENAME}" -o "$OSX_SDK_PATH"
fi

if [ -n "$ANDROID_TOOLS_URL" ]; then
  ANDROID_TOOLS_PATH=$DEPENDS_DIR/sdk-sources/android-tools.zip

  DOCKER_EXEC curl --location --fail "${ANDROID_TOOLS_URL}" -o "$ANDROID_TOOLS_PATH"
  DOCKER_EXEC mkdir -p "${ANDROID_HOME}/cmdline-tools"
  DOCKER_EXEC unzip -o "$ANDROID_TOOLS_PATH" -d "${ANDROID_HOME}/cmdline-tools"
  DOCKER_EXEC "yes | ${ANDROID_HOME}/cmdline-tools/tools/bin/sdkmanager --install \"build-tools;${ANDROID_BUILD_TOOLS_VERSION}\" \"platform-tools\" \"platforms;android-${ANDROID_API_LEVEL}\" \"ndk;${ANDROID_NDK_VERSION}\""
fi

if [[ ${USE_MEMORY_SANITIZER} == "true" ]]; then
  # Use BDB compiled using install_db4.sh script to work around linking issue when using BDB
  # from depends. See https://github.com/bitcoin/bitcoin/pull/18288#discussion_r433189350 for
  # details.
  DOCKER_EXEC "contrib/install_db4.sh \$(pwd) --enable-umrw CC=clang CXX=clang++ CFLAGS='${MSAN_FLAGS}' CXXFLAGS='${MSAN_AND_LIBCXX_FLAGS}'"
fi

if [ -n "$XCODE_VERSION" ] && [ -f "$OSX_SDK_PATH" ]; then
  DOCKER_EXEC tar -C "${DEPENDS_DIR}/SDKs" -xf "$OSX_SDK_PATH"
fi
if [[ $HOST = *-mingw32 ]]; then
  DOCKER_EXEC update-alternatives --set "${HOST}-g++" \$\(which "${HOST}-g++-posix"\)
fi
if [ -z "$NO_DEPENDS" ]; then
  if [[ $DOCKER_NAME_TAG == centos* ]]; then
    # CentOS has problems building the depends if the config shell is not explicitly set
    # (i.e. for libevent a Makefile with an empty SHELL variable is generated, leading to
    #  an error as the first command is executed)
    SHELL_OPTS="LC_ALL=en_US.UTF-8 CONFIG_SHELL=/bin/bash"
  else
    SHELL_OPTS="CONFIG_SHELL="
  fi
  DOCKER_EXEC "$SHELL_OPTS" make "$MAKEJOBS" -C depends HOST="$HOST" "$DEP_OPTS"
fi
if [ -n "$PREVIOUS_RELEASES_TO_DOWNLOAD" ]; then
  DOCKER_EXEC test/get_previous_releases.py -b -t "$PREVIOUS_RELEASES_DIR" "${PREVIOUS_RELEASES_TO_DOWNLOAD}"
fi
