#!/usr/bin/env python3
# Copyright (c) 2014 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
A script to check that the executables produced by gitian only contain
certain symbols and are only linked against allowed libraries.

Example usage:

    find ../gitian-builder/build -type f -executable | xargs python3 contrib/devtools/symbol-check.py
'''
import subprocess
import sys
import os
from typing import List, Optional

import lief
import pixie

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
'GLIBC': {
    pixie.EM_386:    (2,17),
    pixie.EM_X86_64: (2,17),
    pixie.EM_ARM:    (2,17),
    pixie.EM_AARCH64:(2,17),
    pixie.EM_PPC64:  (2,17),
    pixie.EM_RISCV:  (2,27),
},
'LIBATOMIC': (1,0),
'V':         (0,5,0),  # xkb (bitcoin-qt only)
}
# See here for a description of _IO_stdin_used:
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=634261#109

# Ignore symbols that are exported as part of every executable
IGNORE_EXPORTS = {
'_edata', '_end', '__end__', '_init', '__bss_start', '__bss_start__', '_bss_end__', '__bss_end__', '_fini', '_IO_stdin_used', 'stdin', 'stdout', 'stderr',
'environ', '_environ', '__environ',
}
CPPFILT_CMD = os.getenv('CPPFILT', '/usr/bin/c++filt')

# Allowed NEEDED libraries
ELF_ALLOWED_LIBRARIES = {
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
'ld64.so.1', # POWER64 ABIv1 dynamic linker
'ld64.so.2', # POWER64 ABIv2 dynamic linker
'ld-linux-riscv64-lp64d.so.1', # 64-bit RISC-V dynamic linker
# bitcoin-qt only
'libxcb.so.1', # part of X11
'libxkbcommon.so.0', # keyboard keymapping
'libxkbcommon-x11.so.0', # keyboard keymapping
'libfontconfig.so.1', # font support
'libfreetype.so.6', # font parsing
'libdl.so.2' # programming interface to dynamic linker
}

MACHO_ALLOWED_LIBRARIES = {
# bitcoind and bitcoin-qt
'libc++.1.dylib', # C++ Standard Library
'libSystem.B.dylib', # libc, libm, libpthread, libinfo
# bitcoin-qt only
'AppKit', # user interface
'ApplicationServices', # common application tasks.
'Carbon', # deprecated c back-compat API
'CoreFoundation', # low level func, data types
'CoreGraphics', # 2D rendering
'CoreServices', # operating system services
'CoreText', # interface for laying out text and handling fonts.
'CoreVideo', # video processing
'Foundation', # base layer functionality for apps/frameworks
'ImageIO', # read and write image file formats.
'IOKit', # user-space access to hardware devices and drivers.
'IOSurface', # cross process image/drawing buffers
'libobjc.A.dylib', # Objective-C runtime library
'Metal', # 3D graphics
'Security', # access control and authentication
'QuartzCore', # animation
}

PE_ALLOWED_LIBRARIES = {
'ADVAPI32.dll', # security & registry
'IPHLPAPI.DLL', # IP helper API
'KERNEL32.dll', # win32 base APIs
'msvcrt.dll', # C standard library for MSVC
'SHELL32.dll', # shell API
'USER32.dll', # user interface
'WS2_32.dll', # sockets
# bitcoin-qt only
'dwmapi.dll', # desktop window manager
'GDI32.dll', # graphics device interface
'IMM32.dll', # input method editor
'NETAPI32.dll',
'ole32.dll', # component object model
'OLEAUT32.dll', # OLE Automation API
'SHLWAPI.dll', # light weight shell API
'USERENV.dll',
'UxTheme.dll',
'VERSION.dll', # version checking
'WINMM.dll', # WinMM audio API
'WTSAPI32.dll',
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

def check_version(max_versions, version, arch) -> bool:
    if '_' in version:
        (lib, _, ver) = version.rpartition('_')
    else:
        lib = version
        ver = '0'
    ver = tuple([int(x) for x in ver.split('.')])
    if not lib in max_versions:
        return False
    if isinstance(max_versions[lib], tuple):
        return ver <= max_versions[lib]
    else:
        return ver <= max_versions[lib][arch]

def check_imported_symbols(filename) -> bool:
    elf = pixie.load(filename)
    cppfilt = CPPFilt()
    ok: bool = True

    for symbol in elf.dyn_symbols:
        if not symbol.is_import:
            continue
        sym = symbol.name.decode()
        version = symbol.version.decode() if symbol.version is not None else None
        if version and not check_version(MAX_VERSIONS, version, elf.hdr.e_machine):
            print('{}: symbol {} from unsupported version {}'.format(filename, cppfilt(sym), version))
            ok = False
    return ok

def check_exported_symbols(filename) -> bool:
    elf = pixie.load(filename)
    cppfilt = CPPFilt()
    ok: bool = True
    for symbol in elf.dyn_symbols:
        if not symbol.is_export:
            continue
        sym = symbol.name.decode()
        if elf.hdr.e_machine == pixie.EM_RISCV or sym in IGNORE_EXPORTS:
            continue
        print('{}: export of symbol {} not allowed'.format(filename, cppfilt(sym)))
        ok = False
    return ok

def check_ELF_libraries(filename) -> bool:
    ok: bool = True
    elf = pixie.load(filename)
    for library_name in elf.query_dyn_tags(pixie.DT_NEEDED):
        assert(isinstance(library_name, bytes))
        if library_name.decode() not in ELF_ALLOWED_LIBRARIES:
            print('{}: NEEDED library {} is not allowed'.format(filename, library_name.decode()))
            ok = False
    return ok

def check_MACHO_libraries(filename) -> bool:
    ok: bool = True
    binary = lief.parse(filename)
    for dylib in binary.libraries:
        split = dylib.name.split('/')
        if split[-1] not in MACHO_ALLOWED_LIBRARIES:
            print(f'{split[-1]} is not in ALLOWED_LIBRARIES!')
            ok = False
    return ok

def check_PE_libraries(filename) -> bool:
    ok: bool = True
    binary = lief.parse(filename)
    for dylib in binary.libraries:
        if dylib not in PE_ALLOWED_LIBRARIES:
            print(f'{dylib} is not in ALLOWED_LIBRARIES!')
            ok = False
    return ok

CHECKS = {
'ELF': [
    ('IMPORTED_SYMBOLS', check_imported_symbols),
    ('EXPORTED_SYMBOLS', check_exported_symbols),
    ('LIBRARY_DEPENDENCIES', check_ELF_libraries)
],
'MACHO': [
    ('DYNAMIC_LIBRARIES', check_MACHO_libraries)
],
'PE' : [
    ('DYNAMIC_LIBRARIES', check_PE_libraries)
]
}

def identify_executable(executable) -> Optional[str]:
    with open(filename, 'rb') as f:
        magic = f.read(4)
    if magic.startswith(b'MZ'):
        return 'PE'
    elif magic.startswith(b'\x7fELF'):
        return 'ELF'
    elif magic.startswith(b'\xcf\xfa'):
        return 'MACHO'
    return None

if __name__ == '__main__':
    retval: int = 0
    for filename in sys.argv[1:]:
        try:
            etype = identify_executable(filename)
            if etype is None:
                print(f'{filename}: unknown format')
                retval = 1
                continue

            failed: List[str] = []
            for (name, func) in CHECKS[etype]:
                if not func(filename):
                    failed.append(name)
            if failed:
                print(f'{filename}: failed {" ".join(failed)}')
                retval = 1
        except IOError:
            print(f'{filename}: cannot open')
            retval = 1
    sys.exit(retval)
