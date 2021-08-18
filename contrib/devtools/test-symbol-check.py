#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Test script for symbol-check.py
'''
import os
import subprocess
from typing import List
import unittest

from utils import determine_wellknown_cmd

def call_symbol_check(cc: List[str], source, executable, options):
    subprocess.run([*cc,source,'-o',executable] + options, check=True)
    p = subprocess.run(['./contrib/devtools/symbol-check.py',executable], stdout=subprocess.PIPE, universal_newlines=True)
    os.remove(source)
    os.remove(executable)
    return (p.returncode, p.stdout.rstrip())

def get_machine(cc: List[str]):
    p = subprocess.run([*cc,'-dumpmachine'], stdout=subprocess.PIPE, universal_newlines=True)
    return p.stdout.rstrip()

class TestSymbolChecks(unittest.TestCase):
    def test_ELF(self):
        source = 'test1.c'
        executable = 'test1'
        cc = determine_wellknown_cmd('CC', 'gcc')

        # there's no way to do this test for RISC-V at the moment; we build for
        # RISC-V in a glibc 2.27 envinonment and we allow all symbols from 2.27.
        if 'riscv' in get_machine(cc):
            self.skipTest("test not available for RISC-V")

        # nextup was introduced in GLIBC 2.24, so is newer than our supported
        # glibc (2.17), and available in our release build environment (2.24).
        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #define _GNU_SOURCE
                #include <math.h>

                double nextup(double x);

                int main()
                {
                    nextup(3.14);
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cc, source, executable, ['-lm']),
                (1, executable + ': symbol nextup from unsupported version GLIBC_2.24\n' +
                    executable + ': failed IMPORTED_SYMBOLS'))

        # -lutil is part of the libc6 package so a safe bet that it's installed
        # it's also out of context enough that it's unlikely to ever become a real dependency
        source = 'test2.c'
        executable = 'test2'
        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #include <utmp.h>

                int main()
                {
                    login(0);
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cc, source, executable, ['-lutil']),
                (1, executable + ': NEEDED library libutil.so.1 is not allowed\n' +
                    executable + ': failed LIBRARY_DEPENDENCIES'))

        # finally, check a simple conforming binary
        source = 'test3.c'
        executable = 'test3'
        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #include <stdio.h>

                int main()
                {
                    printf("42");
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cc, source, executable, []),
                (0, ''))

    def test_MACHO(self):
        source = 'test1.c'
        executable = 'test1'
        cc = determine_wellknown_cmd('CC', 'clang')

        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #include <expat.h>

                int main()
                {
                    XML_ExpatVersion();
                    return 0;
                }

        ''')

        self.assertEqual(call_symbol_check(cc, source, executable, ['-lexpat', '-Wl,-platform_version','-Wl,macos', '-Wl,11.4', '-Wl,11.4']),
            (1, 'libexpat.1.dylib is not in ALLOWED_LIBRARIES!\n' +
                f'{executable}: failed DYNAMIC_LIBRARIES MIN_OS SDK'))

        source = 'test2.c'
        executable = 'test2'
        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #include <CoreGraphics/CoreGraphics.h>

                int main()
                {
                    CGMainDisplayID();
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cc, source, executable, ['-framework', 'CoreGraphics', '-Wl,-platform_version','-Wl,macos', '-Wl,11.4', '-Wl,11.4']),
                (1, f'{executable}: failed MIN_OS SDK'))

        source = 'test3.c'
        executable = 'test3'
        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                int main()
                {
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cc, source, executable, ['-Wl,-platform_version','-Wl,macos', '-Wl,10.14', '-Wl,11.4']),
                (1, f'{executable}: failed SDK'))

    def test_PE(self):
        source = 'test1.c'
        executable = 'test1.exe'
        cc = determine_wellknown_cmd('CC', 'x86_64-w64-mingw32-gcc')

        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #include <pdh.h>

                int main()
                {
                    PdhConnectMachineA(NULL);
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cc, source, executable, ['-lpdh', '-Wl,--major-subsystem-version', '-Wl,6', '-Wl,--minor-subsystem-version', '-Wl,1']),
            (1, 'pdh.dll is not in ALLOWED_LIBRARIES!\n' +
                 executable + ': failed DYNAMIC_LIBRARIES'))

        source = 'test2.c'
        executable = 'test2.exe'

        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                int main()
                {
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cc, source, executable, ['-Wl,--major-subsystem-version', '-Wl,9', '-Wl,--minor-subsystem-version', '-Wl,9']),
            (1, executable + ': failed SUBSYSTEM_VERSION'))

        source = 'test3.c'
        executable = 'test3.exe'
        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #include <windows.h>

                int main()
                {
                    CoFreeUnusedLibrariesEx(0,0);
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cc, source, executable, ['-lole32', '-Wl,--major-subsystem-version', '-Wl,6', '-Wl,--minor-subsystem-version', '-Wl,1']),
                (0, ''))


if __name__ == '__main__':
    unittest.main()
