#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Test script for security-check.py
'''
import lief
import os
import subprocess
import unittest

from utils import determine_wellknown_cmd

def write_testcode(filename):
    with open(filename, 'w', encoding="utf8") as f:
        f.write('''
    #include <cstdio>
    int main()
    {
        std::printf("the quick brown fox jumps over the lazy god\\n");
        return 0;
    }
    ''')

def clean_files(source, executable):
    os.remove(source)
    os.remove(executable)

def env_flags() -> list[str]:
    # This should behave the same as AC_TRY_LINK, so arrange well-known flags
    # in the same order as autoconf would.
    #
    # See the definitions for ac_link in autoconf's lib/autoconf/c.m4 file for
    # reference.
    flags: list[str] = []
    for var in ['CXXFLAGS', 'CPPFLAGS', 'LDFLAGS']:
        flags += filter(None, os.environ.get(var, '').split(' '))
    return flags

def call_security_check(cxx: str, source: str, executable: str, options) -> tuple:
    subprocess.run([*cxx,source,'-o',executable] + env_flags() + options, check=True)
    p = subprocess.run([os.path.join(os.path.dirname(__file__), 'security-check.py'), executable], stdout=subprocess.PIPE, text=True)
    return (p.returncode, p.stdout.rstrip())

def get_arch(cxx, source, executable):
    subprocess.run([*cxx, source, '-o', executable] + env_flags(), check=True)
    binary = lief.parse(executable)
    arch = binary.abstract.header.architecture
    os.remove(executable)
    return arch

class TestSecurityChecks(unittest.TestCase):
    def test_ELF(self):
        source = 'test1.cpp'
        executable = 'test1'
        cxx = determine_wellknown_cmd('CXX', 'g++')
        write_testcode(source)
        arch = get_arch(cxx, source, executable)

        if arch == lief.ARCHITECTURES.X86:
            pass_flags = ['-Wl,-znoexecstack', '-Wl,-zrelro', '-Wl,-z,now', '-pie', '-fPIE', '-Wl,-z,separate-code', '-fcf-protection=full']
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-Wl,-zexecstack']), (1, executable + ': failed NX'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-no-pie','-fno-PIE']), (1, executable + ': failed PIE'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-Wl,-znorelro']), (1, executable + ': failed RELRO'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-Wl,-z,noseparate-code']), (1, executable + ': failed SEPARATE_CODE'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-fcf-protection=none']), (1, executable + ': failed CONTROL_FLOW'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags), (0, ''))
        else:
            pass_flags = ['-Wl,-znoexecstack', '-Wl,-zrelro', '-Wl,-z,now', '-pie', '-fPIE', '-Wl,-z,separate-code']
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-Wl,-zexecstack']), (1, executable + ': failed NX'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-no-pie','-fno-PIE']), (1, executable + ': failed PIE'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-Wl,-znorelro']), (1, executable + ': failed RELRO'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-Wl,-z,noseparate-code']), (1, executable + ': failed SEPARATE_CODE'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags), (0, ''))

        clean_files(source, executable)

    def test_PE(self):
        source = 'test1.cpp'
        executable = 'test1.exe'
        cxx = determine_wellknown_cmd('CXX', 'x86_64-w64-mingw32-g++')
        write_testcode(source)

        pass_flags = ['-Wl,--nxcompat', '-Wl,--enable-reloc-section', '-Wl,--dynamicbase', '-Wl,--high-entropy-va', '-pie', '-fPIE', '-fcf-protection=full', '-fstack-protector-all', '-lssp']

        self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-fno-stack-protector']), (1, executable + ': failed CANARY'))
        # https://github.com/lief-project/LIEF/issues/1076 - in future, we could test this individually.
        # self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-Wl,--disable-reloc-section']), (1, executable + ': failed RELOC_SECTION'))
        self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-Wl,--disable-nxcompat']), (1, executable + ': failed NX'))
        self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-Wl,--disable-dynamicbase']), (1, executable + ': failed PIE DYNAMIC_BASE HIGH_ENTROPY_VA')) # -pie -fPIE does nothing without --dynamicbase
        self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-Wl,--disable-high-entropy-va']), (1, executable + ': failed HIGH_ENTROPY_VA'))
        self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-fcf-protection=none']), (1, executable + ': failed CONTROL_FLOW'))
        self.assertEqual(call_security_check(cxx, source, executable, pass_flags), (0, ''))

        clean_files(source, executable)

    def test_MACHO(self):
        source = 'test1.cpp'
        executable = 'test1'
        cxx = determine_wellknown_cmd('CXX', 'clang++')
        write_testcode(source)
        arch = get_arch(cxx, source, executable)

        if arch == lief.ARCHITECTURES.X86:
            pass_flags = ['-Wl,-pie', '-fstack-protector-all', '-fcf-protection=full', '-Wl,-fixup_chains']
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-Wl,-no_pie', '-Wl,-no_fixup_chains']), (1, executable+': failed FIXUP_CHAINS PIE')) # -fixup_chains is incompatible with -no_pie
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-Wl,-no_fixup_chains']), (1, executable + ': failed FIXUP_CHAINS'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-fno-stack-protector']), (1, executable + ': failed CANARY'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-Wl,-flat_namespace']), (1, executable + ': failed NOUNDEFS'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-fcf-protection=none']), (1, executable + ': failed CONTROL_FLOW'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags), (0, ''))
        else:
            # arm64 darwin doesn't support non-PIE binaries or executable stacks
            pass_flags = ['-fstack-protector-all', '-Wl,-fixup_chains', '-mbranch-protection=bti']
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-mbranch-protection=none']), (1, executable + ': failed BRANCH_PROTECTION'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-Wl,-no_fixup_chains']), (1, executable + ': failed FIXUP_CHAINS'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-fno-stack-protector']), (1, executable + ': failed CANARY'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags + ['-Wl,-flat_namespace']), (1, executable + ': failed NOUNDEFS'))
            self.assertEqual(call_security_check(cxx, source, executable, pass_flags), (0, ''))

        clean_files(source, executable)

if __name__ == '__main__':
    unittest.main()
