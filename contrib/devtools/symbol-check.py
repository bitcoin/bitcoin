#!/usr/bin/env python3
# Copyright (c) 2014 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
A script to check that the (Linux) executables produced by gitian only contain
allowed gcc and glibc version symbols. This makes sure they are still compatible
with the minimum supported Linux distribution versions.

Example usage:

    find ../gitian-builder/build -type f -executable | xargs python3 contrib/devtools/symbol-check.py
'''
import subprocess
import re
import sys
import os

# Debian 8 (Jessie) EOL: 2020. https://wiki.debian.org/DebianReleases#Production_Releases
#
# - g++ version 4.9.2 (https://packages.debian.org/search?suite=jessie&arch=any&searchon=names&keywords=g%2B%2B)
# - libc version 2.19 (https://packages.debian.org/search?suite=jessie&arch=any&searchon=names&keywords=libc6)
#
# Ubuntu 16.04 (Xenial) EOL: 2024. https://wiki.ubuntu.com/Releases
#
# - g++ version 5.3.1 (https://packages.ubuntu.com/search?keywords=g%2B%2B&searchon=names&suite=xenial&section=all)
# - libc version 2.23.0 (https://packages.ubuntu.com/search?keywords=libc6&searchon=names&suite=xenial&section=all)
#
# CentOS 7 EOL: 2024. https://wiki.centos.org/FAQ/General
#
# - g++ version 4.8.5 (http://mirror.centos.org/centos/7/os/x86_64/Packages/)
# - libc version 2.17 (http://mirror.centos.org/centos/7/os/x86_64/Packages/)
#
# Taking the minimum of these as our target.
#
# According to GNU ABI document (https://gcc.gnu.org/onlinedocs/libstdc++/manual/abi.html) this corresponds to:
#   GCC 4.8.5: GCC_4.8.0
#   (glibc)    GLIBC_2_17
#
MAX_VERSIONS = {
'GCC':       (4,8,0),
'GLIBC':     (2,17),
'LIBATOMIC': (1,0)
}
# See here for a description of _IO_stdin_used:
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=634261#109

# Ignore symbols that are exported as part of every executable
IGNORE_EXPORTS = {
'_edata', '_end', '__end__', '_init', '__bss_start', '__bss_start__', '_bss_end__', '__bss_end__', '_fini', '_IO_stdin_used', 'stdin', 'stdout', 'stderr'
}
READELF_CMD = os.getenv('READELF', '/usr/bin/readelf')
CPPFILT_CMD = os.getenv('CPPFILT', '/usr/bin/c++filt')
# Allowed NEEDED libraries
ALLOWED_LIBRARIES = {
# bitcoind and bitcoin-qt
'libgcc_s.so.1', # GCC base support
'libc.so.6', # C library
'libpthread.so.0', # threading
'libm.so.6', # math library
'librt.so.1', # real-time (clock)
'libatomic.so.1',
'ld-linux-x86-64.so.2', # 64-bit dynamic linker
'ld-linux.so.2', # 32-bit dynamic linker
'ld-linux-aarch64.so.1', # 64-bit ARM dynamic linker
'ld-linux-armhf.so.3', # 32-bit ARM dynamic linker
'ld-linux-riscv64-lp64d.so.1', # 64-bit RISC-V dynamic linker
# bitcoin-qt only
'libxcb.so.1', # part of X11
'libfontconfig.so.1', # font support
'libfreetype.so.6', # font parsing
'libdl.so.2' # programming interface to dynamic linker
}
ARCH_MIN_GLIBC_VER = {
'80386':  (2,1),
'X86-64': (2,2,5),
'ARM':    (2,4),
'AArch64':(2,17),
'RISC-V': (2,27)
}
class CPPFilt(object):
    '''
    Demangle C++ symbol names.

    Use a pipe to the 'c++filt' command.
    '''
    def __init__(self):
        self.proc = subprocess.Popen(CPPFILT_CMD, stdin=subprocess.PIPE, stdout=subprocess.PIPE, universal_newlines=True)

    def __call__(self, mangled):
        self.proc.stdin.write(mangled + '\n')
        self.proc.stdin.flush()
        return self.proc.stdout.readline().rstrip()

    def close(self):
        self.proc.stdin.close()
        self.proc.stdout.close()
        self.proc.wait()

def read_symbols(executable, imports=True):
    '''
    Parse an ELF executable and return a list of (symbol,version) tuples
    for dynamic, imported symbols.
    '''
    p = subprocess.Popen([READELF_CMD, '--dyn-syms', '-W', '-h', executable], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE, universal_newlines=True)
    (stdout, stderr) = p.communicate()
    if p.returncode:
        raise IOError('Could not read symbols for %s: %s' % (executable, stderr.strip()))
    syms = []
    for line in stdout.splitlines():
        line = line.split()
        if 'Machine:' in line:
            arch = line[-1]
        if len(line)>7 and re.match('[0-9]+:$', line[0]):
            (sym, _, version) = line[7].partition('@')
            is_import = line[6] == 'UND'
            if version.startswith('@'):
                version = version[1:]
            if is_import == imports:
                syms.append((sym, version, arch))
    return syms

def check_version(max_versions, version, arch):
    if '_' in version:
        (lib, _, ver) = version.rpartition('_')
    else:
        lib = version
        ver = '0'
    ver = tuple([int(x) for x in ver.split('.')])
    if not lib in max_versions:
        return False
    return ver <= max_versions[lib] or lib == 'GLIBC' and ver <= ARCH_MIN_GLIBC_VER[arch]

def read_libraries(filename):
    p = subprocess.Popen([READELF_CMD, '-d', '-W', filename], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE, universal_newlines=True)
    (stdout, stderr) = p.communicate()
    if p.returncode:
        raise IOError('Error opening file')
    libraries = []
    for line in stdout.splitlines():
        tokens = line.split()
        if len(tokens)>2 and tokens[1] == '(NEEDED)':
            match = re.match(r'^Shared library: \[(.*)\]$', ' '.join(tokens[2:]))
            if match:
                libraries.append(match.group(1))
            else:
                raise ValueError('Unparseable (NEEDED) specification')
    return libraries

if __name__ == '__main__':
    cppfilt = CPPFilt()
    retval = 0
    for filename in sys.argv[1:]:
        # Check imported symbols
        for sym,version,arch in read_symbols(filename, True):
            if version and not check_version(MAX_VERSIONS, version, arch):
                print('%s: symbol %s from unsupported version %s' % (filename, cppfilt(sym), version))
                retval = 1
        # Check exported symbols
        if arch != 'RISC-V':
            for sym,version,arch in read_symbols(filename, False):
                if sym in IGNORE_EXPORTS:
                    continue
                print('%s: export of symbol %s not allowed' % (filename, cppfilt(sym)))
                retval = 1
        # Check dependency libraries
        for library_name in read_libraries(filename):
            if library_name not in ALLOWED_LIBRARIES:
                print('%s: NEEDED library %s is not allowed' % (filename, library_name))
                retval = 1

    sys.exit(retval)
