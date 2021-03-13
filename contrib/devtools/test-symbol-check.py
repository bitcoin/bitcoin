#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Test script for symbol-check.py
'''
import os
import subprocess
import unittest

def call_symbol_check(cc, source, executable, options):
    subprocess.run([cc,source,'-o',executable] + options, check=True)
    p = subprocess.run(['./contrib/devtools/symbol-check.py',executable], stdout=subprocess.PIPE, universal_newlines=True)
    os.remove(source)
    os.remove(executable)
    return (p.returncode, p.stdout.rstrip())

class TestSymbolChecks(unittest.TestCase):
    def test_ELF(self):
        source = 'test1.c'
        executable = 'test1'
        cc = 'gcc'

        # renameat2 was introduced in GLIBC 2.28, so is newer than the upper limit
        # of glibc for all platforms
        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #define _GNU_SOURCE
                #include <stdio.h>
                #include <linux/fs.h>

                int renameat2(int olddirfd, const char *oldpath,
                    int newdirfd, const char *newpath, unsigned int flags);

                int main()
                {
                    renameat2(0, "test", 0, "test_", RENAME_EXCHANGE);
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cc, source, executable, []),
                (1, executable + ': symbol renameat2 from unsupported version GLIBC_2.28\n' +
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

        # finally, check a conforming file that simply uses a math function
        source = 'test3.c'
        executable = 'test3'
        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #include <math.h>

                int main()
                {
                    return (int)pow(2.0, 4.0);
                }
        ''')

        self.assertEqual(call_symbol_check(cc, source, executable, ['-lm']),
                (0, ''))

    def test_MACHO(self):
        source = 'test1.c'
        executable = 'test1'
        cc = 'clang'

        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #include <expat.h>

                int main()
                {
                    XML_ExpatVersion();
                    return 0;
                }

        ''')

        self.assertEqual(call_symbol_check(cc, source, executable, ['-lexpat']),
            (1, 'libexpat.1.dylib is not in ALLOWED_LIBRARIES!\n' +
                executable + ': failed DYNAMIC_LIBRARIES'))

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

        self.assertEqual(call_symbol_check(cc, source, executable, ['-framework', 'CoreGraphics']),
                (0, ''))

    def test_PE(self):
        source = 'test1.c'
        executable = 'test1.exe'
        cc = 'x86_64-w64-mingw32-gcc'

        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #include <pdh.h>

                int main()
                {
                    PdhConnectMachineA(NULL);
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cc, source, executable, ['-lpdh']),
            (1, 'pdh.dll is not in ALLOWED_LIBRARIES!\n' +
                 executable + ': failed DYNAMIC_LIBRARIES'))

        source = 'test2.c'
        executable = 'test2.exe'
        with open(source, 'w', encoding="utf8") as f:
            f.write('''
                #include <windows.h>

                int main()
                {
                    CoFreeUnusedLibrariesEx(0,0);
                    return 0;
                }
        ''')

        self.assertEqual(call_symbol_check(cc, source, executable, ['-lole32']),
                (0, ''))


if __name__ == '__main__':
    unittest.main()

