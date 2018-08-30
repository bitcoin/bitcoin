#!/usr/bin/env python3
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
import argparse
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
    '__bss_start__',
    '_bss_end__', '__bss_end__', '__end__',
'_edata', '_end', '_init', '__bss_start', '_fini', '_IO_stdin_used', 'stdin', 'stdout', 'stderr'
}
READELF_CMD = os.getenv('READELF', '/usr/bin/readelf')
CPPFILT_CMD = os.getenv('CPPFILT', '/usr/bin/c++filt')
# Allowed NEEDED libraries
ALLOWED_LIBRARIES = {
# bitcoind and bitcoin-qt
'libgcc_s.so.1', # GCC base support
'libc.so.6', # C library
'libpthread.so.0', # threading
'libanl.so.1', # DNS resolve
'libm.so.6', # math library
'librt.so.1', # real-time (clock)
'ld-linux-x86-64.so.2', # 64-bit dynamic linker
'ld-linux.so.2', # 32-bit dynamic linker
# bitcoin-qt only
'libX11-xcb.so.1', # part of X11
'libX11.so.6', # part of X11
'libxcb.so.1', # part of X11
'libfontconfig.so.1', # font support
'libfreetype.so.6', # font parsing
'libdl.so.2' # programming interface to dynamic linker
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
    p = subprocess.Popen([READELF_CMD, '--dyn-syms', '-W', executable], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE, universal_newlines=True)
    (stdout, stderr) = p.communicate()
    if p.returncode:
        raise IOError('Could not read symbols for %s: %s' % (executable, stderr.strip()))
    syms = []
    for line in stdout.splitlines():
        m = re.match(r'^\s*\d+:\s*[\da-f]+\s+\d+\s(?:(?:\S+\s+){3})(?:\[.*\]\s+)?(\S+)\s+(\S+).*$', line)
        if m:
            (sym, _, version) = m.group(2).partition('@')
            is_import = (m.group(1) == 'UND')
            if version.startswith('@'):
                version = version[1:]
            if is_import == imports:
                syms.append((sym, version))
    return syms

def check_version(max_versions, version):
    if '_' in version:
        (lib, _, ver) = version.rpartition('_')
    else:
        lib = version
        ver = '0'
    ver = tuple([int(x) for x in ver.split('.')])
    if not lib in max_versions:
        return False
    return ver <= max_versions[lib]

def read_libraries(filename):
    p = subprocess.Popen([READELF_CMD, '-d', '-W', filename], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE, universal_newlines=True)
    (stdout, stderr) = p.communicate()
    if p.returncode:
        raise IOError('Error opening file')
    libraries = []
    for line in stdout.splitlines():
        tokens = line.split()
        if len(tokens)>2 and tokens[1] == '(NEEDED)':
            match = re.match('^Shared library: \[(.*)\]$', ' '.join(tokens[2:]))
            if match:
                libraries.append(match.group(1))
            else:
                raise ValueError('Unparseable (NEEDED) specification')
    return libraries

if __name__ == '__main__':
    ap = argparse.ArgumentParser()

    ap.set_defaults(
        max_versions=dict(MAX_VERSIONS),
        allow_exports=set(IGNORE_EXPORTS),
        allow_libraries=set(ALLOWED_LIBRARIES),
    )

    class SetVersionAction(argparse.Action):
        def __call__(self, parser, namespace, value, option_string=None):
            value = value.partition('_')
            try:
                parsed_version = tuple(map(int, value[2].split('.')))
            except:
                value = (None, None)  # raise format exception
            if not value[1]:
                raise argparse.ArgumentError(self, 'must be in the format %s' % (self.metavar,))
            getattr(namespace, self.dest)[value[0]] = parsed_version
    ap.add_argument('--max-version', dest='max_versions', metavar='LIBNAME_VERSION', action=SetVersionAction, help='limit symbol version')

    class AddAction(argparse.Action):
        def __call__(self, parser, namespace, value, option_string=None):
            getattr(namespace, self.dest).add(value)
    class RemoveAction(argparse.Action):
        def __call__(self, parser, namespace, value, option_string=None):
            dest_set = getattr(namespace, self.dest)
            if value in dest_set:
                dest_set.remove(value)

    ap.add_argument('--allow-export', dest='allow_exports', metavar='SYMBOL', action=AddAction, help='allow SYMBOL to be exported')
    ap.add_argument('--no-allow-export', dest='allow_exports', metavar='SYMBOL', action=RemoveAction, help='disallow SYMBOL to be exported')
    ap.add_argument('--allow-all-exports', dest='allow_all_exports', action='store_true', default=False, help='allow anything to be exported')
    ap.add_argument('--allow-library', dest='allow_libraries', metavar='FILENAME', action=AddAction, help='allow library to be linked')
    ap.add_argument('--no-allow-library', dest='allow_libraries', metavar='FILENAME', action=RemoveAction, help='disallow library to be linked')

    ap.add_argument('binaries', nargs='+', help='binaries to check')
    opts = ap.parse_args()

    cppfilt = CPPFilt()
    retval = 0
    for filename in opts.binaries:
        # Check imported symbols
        for sym,version in read_symbols(filename, True):
            if version and not check_version(opts.max_versions, version):
                print('%s: symbol %s from unsupported version %s' % (filename, cppfilt(sym), version))
                retval = 1
        if not opts.allow_all_exports:
            # Check exported symbols
            for sym,version in read_symbols(filename, False):
                if sym in opts.allow_exports:
                    continue
                print('%s: export of symbol %s not allowed' % (filename, cppfilt(sym)))
                retval = 1
        # Check dependency libraries
        for library_name in read_libraries(filename):
            if library_name not in opts.allow_libraries:
                print('%s: NEEDED library %s is not allowed' % (filename, library_name))
                retval = 1

    sys.exit(retval)


