ANDROID BUILD NOTES
======================

This guide describes how to build and package the `bitcoin-qt` GUI for Android on Linux and macOS.

## Preparation

You will need to get the Android NDK and build dependencies for Android as described in [depends/README.md](../depends/README.md).

## Building and packaging

After the depends are built configure with one of the resulting prefixes and run `make && make apk` in `src/qt`.