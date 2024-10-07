#!/usr/bin/env python3
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Test script for symbol-check.py
'''
import os
import subprocess
import unittest

from utils import determine_wellknown_cmd

def call_symbol_check(cxx: list[str], source, executable, options):
    # This should behave the same as AC_TRY_LINK, so arrange well-known flags
    # in the same order as autoconf would.
    #
    # See the definitions for ac_link in autoconf's lib/autoconf/c.m4 file for
    # reference.
    env_flags: list[str] = []
    for var in ['CXXFLAGS', 'CPPFLAGS', 'LDFLAGS']:
        env_flags += filter(None, os.environ.get(var, '').split(' '))

    subprocess.run([*cxx,source,'-o',executable] + env_flags + options, check=True)
    p = subprocess.run([os.path.join(os.path.dirname(__file__), 'symbol-check.py'), executable], stdout=subprocess.PIPE, text=True)
    os.remove(source)
    os.remove(executable)
    return (p.returncode, p.stdout.rstrip())

class TestSymbolChecks(unittest.TestCase):
    def test_ELF(self):
        source = 'test1.cpp'
        executable = 'test1'
        cxx = determine_wellknown_cmd('CXX', 'g++')

        # -lutil is part of the libc6 package so a safe bet that it's installed
        # it's also out of context enough that it's unlikely to ever become a real dependency
        source = 'test2.cpp'
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

        self.assertEqual(call_symbol_check(cxx, source, executable, ['-lutil']),
                (1, executable + ': libutil.so.1 is not in ALLOWED_LIBRARIES!\n' +
                    executable + ': failed LIBRARY_DEPENDENCIES'))

        # finally, check a simple conforming binary
        source = 'test3.cpp'
        executable = 'test3'
        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #include <cstdio>

                int main()
                {
                    std::printf("42");
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cxx, source, executable, []),
                (0, ''))

    def test_MACHO(self):
        source = 'test1.cpp'
        executable = 'test1'
        cxx = determine_wellknown_cmd('CXX', 'clang++')

        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #include <expat.h>

                int main()
                {
                    XML_ExpatVersion();
                    return 0;
                }

        ''')

        self.assertEqual(call_symbol_check(cxx, source, executable, ['-lexpat', '-Wl,-platform_version','-Wl,macos', '-Wl,11.4', '-Wl,11.4']),
            (1, 'libexpat.1.dylib is not in ALLOWED_LIBRARIES!\n' +
                f'{executable}: failed DYNAMIC_LIBRARIES MIN_OS SDK'))

        source = 'test2.cpp'
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

        self.assertEqual(call_symbol_check(cxx, source, executable, ['-framework', 'CoreGraphics', '-Wl,-platform_version','-Wl,macos', '-Wl,11.4', '-Wl,11.4']),
                (1, f'{executable}: failed MIN_OS SDK'))

        source = 'test3.cpp'
        executable = 'test3'
        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                int main()
                {
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cxx, source, executable, ['-Wl,-platform_version','-Wl,macos', '-Wl,13.0', '-Wl,11.4']),
                (1, f'{executable}: failed SDK'))

    def test_PE(self):
        source = 'test1.cpp'
        executable = 'test1.exe'
        cxx = determine_wellknown_cmd('CXX', 'x86_64-w64-mingw32-g++')

        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #include <pdh.h>

                int main()
                {
                    PdhConnectMachineA(NULL);
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cxx, source, executable, ['-lpdh', '-Wl,--major-subsystem-version', '-Wl,6', '-Wl,--minor-subsystem-version', '-Wl,1']),
            (1, 'pdh.dll is not in ALLOWED_LIBRARIES!\n' +
                 executable + ': failed DYNAMIC_LIBRARIES'))

        source = 'test2.cpp'
        executable = 'test2.exe'

        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                int main()
                {
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cxx, source, executable, ['-Wl,--major-subsystem-version', '-Wl,9', '-Wl,--minor-subsystem-version', '-Wl,9']),
            (1, executable + ': failed SUBSYSTEM_VERSION'))

        source = 'test3.cpp'
        executable = 'test3.exe'
        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #include <combaseapi.h>

                int main()
                {
                    CoFreeUnusedLibrariesEx(0,0);
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cxx, source, executable, ['-lole32', '-Wl,--major-subsystem-version', '-Wl,6', '-Wl,--minor-subsystem-version', '-Wl,1']),
                (0, ''))


if __name__ == '__main__':
    unittest.main()
