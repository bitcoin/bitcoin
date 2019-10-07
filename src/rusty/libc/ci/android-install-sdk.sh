#!/usr/bin/env sh
# Copyright 2016 The Rust Project Developers. See the COPYRIGHT
# file at the top-level directory of this distribution and at
# http://rust-lang.org/COPYRIGHT.
#
# Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
# http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
# <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
# option. This file may not be copied, modified, or distributed
# except according to those terms.

set -ex

# Prep the SDK and emulator
#
# Note that the update process requires that we accept a bunch of licenses, and
# we can't just pipe `yes` into it for some reason, so we take the same strategy
# located in https://github.com/appunite/docker by just wrapping it in a script
# which apparently magically accepts the licenses.

SDK=4333796
mkdir sdk
curl --retry 20 https://dl.google.com/android/repository/sdk-tools-linux-${SDK}.zip -O
unzip -q -d sdk sdk-tools-linux-${SDK}.zip

case "$1" in
  arm | armv7)
    api=24
    image="system-images;android-${api};google_apis;armeabi-v7a"
    ;;
  aarch64)
    api=24
    image="system-images;android-${api};google_apis;arm64-v8a"
    ;;
  i686)
    api=28
    image="system-images;android-${api};default;x86"
    ;;
  x86_64)
    api=28
    image="system-images;android-${api};default;x86_64"
    ;;
  *)
    echo "invalid arch: $1"
    exit 1
    ;;
esac;

# Try to fix warning about missing file.
# See https://askubuntu.com/a/1078784
mkdir -p /root/.android/
echo '### User Sources for Android SDK Manager' >> /root/.android/repositories.cfg
echo '#Fri Nov 03 10:11:27 CET 2017 count=0' >> /root/.android/repositories.cfg

# Print all available packages
# yes | ./sdk/tools/bin/sdkmanager --list --verbose

# --no_https avoids
# javax.net.ssl.SSLHandshakeException: sun.security.validator.ValidatorException: No trusted certificate found
#
# | grep -v = || true    removes the progress bar output from the sdkmanager
# which produces an insane amount of output.
yes | ./sdk/tools/bin/sdkmanager --licenses --no_https | grep -v = || true
yes | ./sdk/tools/bin/sdkmanager --no_https \
        "emulator" \
        "platform-tools" \
        "platforms;android-${api}" \
        "${image}" | grep -v = || true

echo "no" |
    ./sdk/tools/bin/avdmanager create avd \
        --name "${1}" \
        --package "${image}" | grep -v = || true
