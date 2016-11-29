#!/usr/bin/python2
# Copyright (c) 2014 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
A script to check that the (Linux) executables produced by gitian only contain
allowed gcc, glibc and libstdc++ version symbols.  This makes sure they are
still compatible with the minimum supported Linux distribution versions.

Example usage:

    find ../gitian-builder/build -type f -executable | xargs python contrib/devtools/symbol-check.py
'''
from __future__ import division, print_function, unicode_literals
import subprocess
import re
import sys
import os

# Debian 6.0.9 (Squeeze) has:
#
# - g++ version 4.4.5 (https://packages.debian.org/search?suite=default&section=all&arch=any&searchon=names&keywords=g%2B%2B)
# - libc version 2.11.3 (https://packages.debian.org/search?suite=default&section=all&arch=any&searchon=names&keywords=libc6)
# - libstdc++ version 4.4.5 (https://packages.debian.org/search?suite=default&section=all&arch=any&searchon=names&keywords=libstdc%2B%2B6)
#
# Ubuntu 10.04.4 (Lucid Lynx) has:
#
# - g++ version 4.4.3 (http://packages.ubuntu.com/search?keywords=g%2B%2B&searchon=names&suite=lucid&section=all)
# - libc version 2.11.1 (http://packages.ubuntu.com/search?keywords=libc6&searchon=names&suite=lucid&section=all)
# - libstdc++ version 4.4.3 (http://packages.ubuntu.com/search?suite=lucid&section=all&arch=any&keywords=libstdc%2B%2B&searchon=names)
#
# Taking the minimum of these as our target.
#
# According to GNU ABI document (http://gcc.gnu.org/onlinedocs/libstdc++/manual/abi.html) this corresponds to:
#   GCC 4.4.0: GCC_4.4.0
#   GCC 4.4.2: GLIBCXX_3.4.13, CXXABI_1.3.3
#   (glibc)    GLIBC_2_11
#
MAX_VERSIONS = {
'GCC':     (4,4,0),
'CXXABI':  (1,3,3),
'GLIBCXX': (3,4,13),
'GLIBC':   (2,11)
}
# See here for a description of _IO_stdin_used:
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=634261#109

# Ignore symbols that are exported as part of every executable
IGNORE_EXPORTS = {
b'_edata', b'_end', b'_init', b'__bss_start', b'_fini', b'_IO_stdin_used'
}
READELF_CMD = os.getenv('READELF', '/usr/bin/readelf')
CPPFILT_CMD = os.getenv('CPPFILT', '/usr/bin/c++filt')
# Allowed NEEDED libraries
ALLOWED_LIBRARIES = {
# bitcoind and bitcoin-qt
b'libgcc_s.so.1', # GCC base support
b'libc.so.6', # C library
b'libpthread.so.0', # threading
b'libanl.so.1', # DNS resolve
b'libm.so.6', # math library
b'librt.so.1', # real-time (clock)
b'ld-linux-x86-64.so.2', # 64-bit dynamic linker
b'ld-linux.so.2', # 32-bit dynamic linker
# bitcoin-qt only
b'libX11-xcb.so.1', # part of X11
b'libX11.so.6', # part of X11
b'libxcb.so.1', # part of X11
b'libfontconfig.so.1', # font support
b'libfreetype.so.6', # font parsing
b'libdl.so.2' # programming interface to dynamic linker
}

class CPPFilt(object):
    '''
    Demangle C++ symbol names.

    Use a pipe to the 'c++filt' command.
    '''
    def __init__(self):
        self.proc = subprocess.Popen(CPPFILT_CMD, stdin=subprocess.PIPE, stdout=subprocess.PIPE)

    def __call__(self, mangled):
        self.proc.stdin.write(mangled + b'\n')
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
    p = subprocess.Popen([READELF_CMD, '--dyn-syms', '-W', executable], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    if p.returncode:
        raise IOError('Could not read symbols for %s: %s' % (executable, stderr.strip()))
    syms = []
    for line in stdout.split(b'\n'):
        line = line.split()
        if len(line)>7 and re.match(b'[0-9]+:$', line[0]):
            (sym, _, version) = line[7].partition(b'@')
            is_import = line[6] == b'UND'
            if version.startswith(b'@'):
                version = version[1:]
            if is_import == imports:
                syms.append((sym, version))
    return syms

def check_version(max_versions, version):
    if b'_' in version:
        (lib, _, ver) = version.rpartition(b'_')
    else:
        lib = version
        ver = '0'
    ver = tuple([int(x) for x in ver.split(b'.')])
    if not lib in max_versions:
        return False
    return ver <= max_versions[lib]

def read_libraries(filename):
    p = subprocess.Popen([READELF_CMD, '-d', '-W', filename], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    if p.returncode:
        raise IOError('Error opening file')
    libraries = []
    for line in stdout.split(b'\n'):
        tokens = line.split()
        if len(tokens)>2 and tokens[1] == b'(NEEDED)':
            match = re.match(b'^Shared library: \[(.*)\]$', b' '.join(tokens[2:]))
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
        for sym,version in read_symbols(filename, True):
            if version and not check_version(MAX_VERSIONS, version):
                print('%s: symbol %s from unsupported version %s' % (filename, cppfilt(sym).decode('utf-8'), version.decode('utf-8')))
                retval = 1
        # Check exported symbols
        for sym,version in read_symbols(filename, False):
            if sym in IGNORE_EXPORTS:
                continue
            print('%s: export of symbol %s not allowed' % (filename, cppfilt(sym).decode('utf-8')))
            retval = 1
        # Check dependency libraries
        for library_name in read_libraries(filename):
            if library_name not in ALLOWED_LIBRARIES:
                print('%s: NEEDED library %s is not allowed' % (filename, library_name.decode('utf-8')))
                retval = 1

    exit(retval)


