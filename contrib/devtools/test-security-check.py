#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Test script for security-check.py
'''
import lief #type:ignore
import os
import subprocess
from typing import List
import unittest

from utils import determine_wellknown_cmd

def write_testcode(filename):
    with open(filename, 'w', encoding="utf8") as f:
        f.write('''
    #include <stdio.h>
    int main()
    {
        printf("the quick brown fox jumps over the lazy god\\n");
        return 0;
    }
    ''')

def clean_files(source, executable):
    os.remove(source)
    os.remove(executable)

def call_security_check(cc, source, executable, options):
    # This should behave the same as AC_TRY_LINK, so arrange well-known flags
    # in the same order as autoconf would.
    #
    # See the definitions for ac_link in autoconf's lib/autoconf/c.m4 file for
    # reference.
    env_flags: List[str] = []
    for var in ['CFLAGS', 'CPPFLAGS', 'LDFLAGS']:
        env_flags += filter(None, os.environ.get(var, '').split(' '))

    subprocess.run([*cc,source,'-o',executable] + env_flags + options, check=True)
    p = subprocess.run([os.path.join(os.path.dirname(__file__), 'security-check.py'), executable], stdout=subprocess.PIPE, text=True)
    return (p.returncode, p.stdout.rstrip())

def get_arch(cc, source, executable):
    subprocess.run([*cc, source, '-o', executable], check=True)
    binary = lief.parse(executable)
    arch = binary.abstract.header.architecture
    os.remove(executable)
    return arch

class TestSecurityChecks(unittest.TestCase):
    def test_ELF(self):
        source = 'test1.c'
        executable = 'test1'
        cc = determine_wellknown_cmd('CC', 'gcc')
        write_testcode(source)
        arch = get_arch(cc, source, executable)

        if arch == lief.ARCHITECTURES.X86:
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-zexecstack','-fno-stack-protector','-Wl,-znorelro','-no-pie','-fno-PIE', '-Wl,-z,separate-code']),
                    (1, executable+': failed PIE NX RELRO Canary CONTROL_FLOW'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-znoexecstack','-fno-stack-protector','-Wl,-znorelro','-no-pie','-fno-PIE', '-Wl,-z,separate-code']),
                    (1, executable+': failed PIE RELRO Canary CONTROL_FLOW'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-znoexecstack','-fstack-protector-all','-Wl,-znorelro','-no-pie','-fno-PIE', '-Wl,-z,separate-code']),
                    (1, executable+': failed PIE RELRO CONTROL_FLOW'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-znoexecstack','-fstack-protector-all','-Wl,-znorelro','-pie','-fPIE', '-Wl,-z,separate-code']),
                    (1, executable+': failed RELRO CONTROL_FLOW'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-znoexecstack','-fstack-protector-all','-Wl,-zrelro','-Wl,-z,now','-pie','-fPIE', '-Wl,-z,noseparate-code']),
                    (1, executable+': failed separate_code CONTROL_FLOW'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-znoexecstack','-fstack-protector-all','-Wl,-zrelro','-Wl,-z,now','-pie','-fPIE', '-Wl,-z,separate-code']),
                    (1, executable+': failed CONTROL_FLOW'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-znoexecstack','-fstack-protector-all','-Wl,-zrelro','-Wl,-z,now','-pie','-fPIE', '-Wl,-z,separate-code', '-fcf-protection=full']),
                    (0, ''))
        else:
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-zexecstack','-fno-stack-protector','-Wl,-znorelro','-no-pie','-fno-PIE', '-Wl,-z,separate-code']),
                    (1, executable+': failed PIE NX RELRO Canary'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-znoexecstack','-fno-stack-protector','-Wl,-znorelro','-no-pie','-fno-PIE', '-Wl,-z,separate-code']),
                    (1, executable+': failed PIE RELRO Canary'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-znoexecstack','-fstack-protector-all','-Wl,-znorelro','-no-pie','-fno-PIE', '-Wl,-z,separate-code']),
                    (1, executable+': failed PIE RELRO'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-znoexecstack','-fstack-protector-all','-Wl,-znorelro','-pie','-fPIE', '-Wl,-z,separate-code']),
                    (1, executable+': failed RELRO'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-znoexecstack','-fstack-protector-all','-Wl,-zrelro','-Wl,-z,now','-pie','-fPIE', '-Wl,-z,noseparate-code']),
                    (1, executable+': failed separate_code'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-znoexecstack','-fstack-protector-all','-Wl,-zrelro','-Wl,-z,now','-pie','-fPIE', '-Wl,-z,separate-code']),
                    (0, ''))

        clean_files(source, executable)

    def test_PE(self):
        source = 'test1.c'
        executable = 'test1.exe'
        cc = determine_wellknown_cmd('CC', 'x86_64-w64-mingw32-gcc')
        write_testcode(source)

        self.assertEqual(call_security_check(cc, source, executable, ['-Wl,--disable-nxcompat','-Wl,--disable-reloc-section','-Wl,--disable-dynamicbase','-Wl,--disable-high-entropy-va','-no-pie','-fno-PIE','-fno-stack-protector']),
            (1, executable+': failed PIE DYNAMIC_BASE HIGH_ENTROPY_VA NX RELOC_SECTION CONTROL_FLOW Canary'))
        self.assertEqual(call_security_check(cc, source, executable, ['-Wl,--nxcompat','-Wl,--disable-reloc-section','-Wl,--disable-dynamicbase','-Wl,--disable-high-entropy-va','-no-pie','-fno-PIE','-fstack-protector-all', '-lssp']),
            (1, executable+': failed PIE DYNAMIC_BASE HIGH_ENTROPY_VA RELOC_SECTION CONTROL_FLOW'))
        self.assertEqual(call_security_check(cc, source, executable, ['-Wl,--nxcompat','-Wl,--enable-reloc-section','-Wl,--disable-dynamicbase','-Wl,--disable-high-entropy-va','-no-pie','-fno-PIE','-fstack-protector-all', '-lssp']),
            (1, executable+': failed PIE DYNAMIC_BASE HIGH_ENTROPY_VA CONTROL_FLOW'))
        self.assertEqual(call_security_check(cc, source, executable, ['-Wl,--nxcompat','-Wl,--enable-reloc-section','-Wl,--disable-dynamicbase','-Wl,--disable-high-entropy-va','-pie','-fPIE','-fstack-protector-all', '-lssp']),
            (1, executable+': failed PIE DYNAMIC_BASE HIGH_ENTROPY_VA CONTROL_FLOW'))  # -pie -fPIE does nothing unless --dynamicbase is also supplied
        self.assertEqual(call_security_check(cc, source, executable, ['-Wl,--nxcompat','-Wl,--enable-reloc-section','-Wl,--dynamicbase','-Wl,--disable-high-entropy-va','-pie','-fPIE','-fstack-protector-all', '-lssp']),
            (1, executable+': failed HIGH_ENTROPY_VA CONTROL_FLOW'))
        self.assertEqual(call_security_check(cc, source, executable, ['-Wl,--nxcompat','-Wl,--enable-reloc-section','-Wl,--dynamicbase','-Wl,--high-entropy-va','-pie','-fPIE','-fstack-protector-all', '-lssp']),
            (1, executable+': failed CONTROL_FLOW'))
        self.assertEqual(call_security_check(cc, source, executable, ['-Wl,--nxcompat','-Wl,--enable-reloc-section','-Wl,--dynamicbase','-Wl,--high-entropy-va','-pie','-fPIE', '-fcf-protection=full','-fstack-protector-all', '-lssp']),
            (0, ''))

        clean_files(source, executable)

    def test_MACHO(self):
        source = 'test1.c'
        executable = 'test1'
        cc = determine_wellknown_cmd('CC', 'clang')
        write_testcode(source)
        arch = get_arch(cc, source, executable)

        if arch == lief.ARCHITECTURES.X86:
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-no_pie','-Wl,-flat_namespace','-Wl,-allow_stack_execute','-fno-stack-protector']),
                (1, executable+': failed NOUNDEFS LAZY_BINDINGS Canary PIE NX CONTROL_FLOW'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-no_pie','-Wl,-flat_namespace','-Wl,-allow_stack_execute','-fstack-protector-all']),
                (1, executable+': failed NOUNDEFS LAZY_BINDINGS PIE NX CONTROL_FLOW'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-no_pie','-Wl,-flat_namespace','-fstack-protector-all']),
                (1, executable+': failed NOUNDEFS LAZY_BINDINGS PIE CONTROL_FLOW'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-no_pie','-fstack-protector-all']),
                (1, executable+': failed LAZY_BINDINGS PIE CONTROL_FLOW'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-no_pie','-Wl,-bind_at_load','-fstack-protector-all']),
                (1, executable+': failed PIE CONTROL_FLOW'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-no_pie','-Wl,-bind_at_load','-fstack-protector-all', '-fcf-protection=full']),
                (1, executable+': failed PIE'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-pie','-Wl,-bind_at_load','-fstack-protector-all', '-fcf-protection=full']),
                (0, ''))
        else:
            # arm64 darwin doesn't support non-PIE binaries, control flow or executable stacks
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-flat_namespace','-fno-stack-protector']),
                (1, executable+': failed NOUNDEFS LAZY_BINDINGS Canary'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-flat_namespace','-fstack-protector-all']),
                (1, executable+': failed NOUNDEFS LAZY_BINDINGS'))
            self.assertEqual(call_security_check(cc, source, executable, ['-fstack-protector-all']),
                (1, executable+': failed LAZY_BINDINGS'))
            self.assertEqual(call_security_check(cc, source, executable, ['-Wl,-bind_at_load','-fstack-protector-all']),
                (0, ''))


        clean_files(source, executable)

if __name__ == '__main__':
    unittest.main()
