#!/usr/bin/env bash
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

# Make sure default datadir does not exist and is never read by creating a dummy file
if [ "$CI_OS_NAME" == "macos" ]; then
  echo > "${HOME}/Library/Application Support/Syscoin"
else
  CI_EXEC echo \> \$HOME/.syscoin
  CI_EXEC_ROOT echo \> \$HOME/.syscoin
fi

CI_EXEC mkdir -p "${DEPENDS_DIR}/SDKs" "${DEPENDS_DIR}/sdk-sources"

OSX_SDK_BASENAME="Xcode-${XCODE_VERSION}-${XCODE_BUILD_ID}-extracted-SDK-with-libcxx-headers"

if [ -n "$XCODE_VERSION" ] && [ ! -d "${DEPENDS_DIR}/SDKs/${OSX_SDK_BASENAME}" ]; then
  OSX_SDK_FILENAME="${OSX_SDK_BASENAME}.tar.gz"
  OSX_SDK_PATH="${DEPENDS_DIR}/sdk-sources/${OSX_SDK_FILENAME}"
  if [ ! -f "$OSX_SDK_PATH" ]; then
    CI_EXEC curl --location --fail "${SDK_URL}/${OSX_SDK_FILENAME}" -o "$OSX_SDK_PATH"
  fi
  CI_EXEC tar -C "${DEPENDS_DIR}/SDKs" -xf "$OSX_SDK_PATH"
fi

if [ -n "$ANDROID_HOME" ] && [ ! -d "$ANDROID_HOME" ]; then
  ANDROID_TOOLS_PATH=${DEPENDS_DIR}/sdk-sources/android-tools.zip
  if [ ! -f "$ANDROID_TOOLS_PATH" ]; then
    CI_EXEC curl --location --fail "${ANDROID_TOOLS_URL}" -o "$ANDROID_TOOLS_PATH"
  fi
  CI_EXEC mkdir -p "$ANDROID_HOME"
  CI_EXEC unzip -o "$ANDROID_TOOLS_PATH" -d "$ANDROID_HOME"
  CI_EXEC "yes | ${ANDROID_HOME}/cmdline-tools/bin/sdkmanager --sdk_root=\"${ANDROID_HOME}\" --install \"build-tools;${ANDROID_BUILD_TOOLS_VERSION}\" \"platform-tools\" \"platforms;android-${ANDROID_API_LEVEL}\" \"ndk;${ANDROID_NDK_VERSION}\""
fi

if [ -z "$NO_DEPENDS" ]; then
  if [[ $CI_IMAGE_NAME_TAG == *centos* ]]; then
    # CentOS has problems building the depends if the config shell is not explicitly set
    # (i.e. for libevent a Makefile with an empty SHELL variable is generated, leading to
    #  an error as the first command is executed)
    SHELL_OPTS="LC_ALL=en_US.UTF-8 CONFIG_SHELL=/bin/dash"
  else
    SHELL_OPTS="CONFIG_SHELL="
  fi
  CI_EXEC "$SHELL_OPTS" make "$MAKEJOBS" -C depends HOST="$HOST" "$DEP_OPTS" LOG=1
fi
if [ "$DOWNLOAD_PREVIOUS_RELEASES" = "true" ]; then
  CI_EXEC test/get_previous_releases.py -b -t "$PREVIOUS_RELEASES_DIR"
fi
