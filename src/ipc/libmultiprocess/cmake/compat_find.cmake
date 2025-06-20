# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# compat_find.cmake -- compatibility workarounds meant to be included before
# cmake find_package() calls are made

# Set FOUND_LIBATOMIC to work around bug in debian capnproto package that is
# debian-specific and does not happpen upstream. Debian includes a patch
# https://sources.debian.org/patches/capnproto/1.0.1-4/07_libatomic.patch/ which
# uses check_library_exists(atomic __atomic_load_8 ...) and it fails because the
# symbol name conflicts with a compiler instrinsic as described
# https://github.com/bitcoin-core/libmultiprocess/issues/68#issuecomment-1135150171.
# This could be fixed by improving the check_library_exists function as
# described in the github comment, or by changing the debian patch to check for
# the symbol a different way, but simplest thing to do is work around the
# problem by setting FOUND_LIBATOMIC. This problem has probably not
# been noticed upstream because it only affects CMake packages depending on
# capnproto, not autoconf packages.
set(FOUND_LIBATOMIC TRUE)
