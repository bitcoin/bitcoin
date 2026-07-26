#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8
set -o errexit -o xtrace -o pipefail

GCC=/opt/riscv-ilp32/bin/riscv32-unknown-elf-gcc
GXX=/opt/riscv-ilp32/bin/riscv32-unknown-elf-g++
CRTBEGIN=$("${GCC}" -print-file-name=crtbegin.o)
LIBGCC=$("${GCC}" -print-libgcc-file-name)
LIBSTDCXX=$("${GXX}" -print-file-name=libstdc++.a)
LIBC=$("${GCC}" -print-file-name=libc.a)
LIBM=$("${GCC}" -print-file-name=libm.a)

echo -e "#include <script/script_error.h>\n int main() { return ScriptErrorString(ScriptError_t::SCRIPT_ERR_UNKNOWN_ERROR).size() > 0; }" > test.cpp

"${GXX}" -I "${BASE_ROOT_DIR}"/src -g -std=c++20 -c test.cpp -o test.o

# Make the binary executable on linux for testing purposes
echo -e ".section .text
      .global _start
      .type _start, @function

      _start:
          .option push
          .option norelax
          la gp, __global_pointer$
          .option pop

          call main

          # Put Exit2 system call number into the a7 register
          li a7, 93
          ecall" > start.s

"${GCC}" -c start.s -o start.o

echo -e "#include <sys/stat.h>
      void _exit(int code) { while(1); }
      int _sbrk(int incr) { return 0; }
      int _write(int file, char *ptr, int len) { return 0; }
      int _close(int file) { return -1; }
      int _fstat(int file, struct stat *st) { st->st_mode = S_IFCHR; return 0; }
      int _isatty(int file) { return 1; }
      int _lseek(int file, int ptr, int dir) { return 0; }
      int _read(int file, char *ptr, int len) { return 0; }
      int _kill(int pid, int sig) { return -1; }
      int _getpid(void) { return -1; }" > syscalls.c

"${GCC}" -g -c syscalls.c -o syscalls.o

"${GXX}" -g -std=c++20 \
    -nostdlib \
    "${CRTBEGIN}" \
    test.o \
    start.o \
    syscalls.o \
    -Wl,--whole-archive \
    "${BASE_BUILD_DIR}"/lib/libbitcoin_consensus.a \
    "${BASE_BUILD_DIR}"/lib/libbitcoin_crypto.a \
    "${BASE_BUILD_DIR}"/src/secp256k1/lib/libsecp256k1.a \
    -Wl,--no-whole-archive \
    "${LIBSTDCXX}" \
    "${LIBC}" \
    "${LIBM}" \
    "${LIBGCC}" \
    -o test.elf

file test.elf

