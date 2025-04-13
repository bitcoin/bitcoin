# Android Build Guide

This guide describes how to build and package the `bitcoin-qt` GUI for Android on Linux and macOS.


## Dependencies

Before proceeding with an Android build one needs to get the [Android SDK](https://developer.android.com/studio) and use the "SDK Manager" tool to download the NDK and one or more "Platform packages" (these are Android versions and have a corresponding API level).

Qt 6.7.3 used in depends supports Android NDK version is [r26b (26.1.10909125)](https://github.com/android/ndk/wiki/Changelog-r26).

In order to build `ANDROID_API_LEVEL` (API level corresponding to the Android version targeted, e.g. Android 9.0 Pie is 28 and its "Platform package" needs to be available) needs to be set.

Qt 6.7.3 used in depends supports API levels from 26 to 34.

When building [depends](../depends/README.md), additional variables `ANDROID_SDK` and `ANDROID_NDK` need to be set.

This is an example command for a default build with no disabled dependencies:

    gmake HOST=aarch64-linux-android ANDROID_SDK=/home/user/Android/Sdk ANDROID_NDK=/home/user/Android/Sdk/ndk/26.1.10909125 ANDROID_API_LEVEL=34


## Building and packaging

After the depends are built configure, build and create an Android Application Package (APK) as follows (based on the example above):

```bash
cmake -B build --toolchain depends/aarch64-linux-android/toolchain.cmake
cmake --build build --target apk_package  # Use "-j N" for N parallel jobs.
```

The APKs will be available in the following directory:
```bash
$ tree build/src/qt/android/build/outputs/apk
build/src/qt/android/build/outputs/apk
├── debug
│   ├── android-debug.apk
│   └── output-metadata.json
└── release
    ├── android-release-unsigned.apk
    └── output-metadata.json

3 directories, 4 files
```
