#!/usr/bin/env python3
# Copyright (c) 2014 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
A script to check that release executables only contain certain symbols
and are only linked against allowed libraries.

Example usage:

    find ../path/to/guix/binaries -type f -executable | xargs python3 contrib/guix/symbol-check.py
'''
import sys

import lief

# Debian 11 (Bullseye) EOL: 2026. https://wiki.debian.org/LTS
#
# - libgcc version 10.2.1 (https://packages.debian.org/bullseye/libgcc-s1)
# - libc version 2.31 (https://packages.debian.org/source/bullseye/glibc)
#
# Ubuntu 20.04 (Focal) EOL: 2030. https://wiki.ubuntu.com/ReleaseTeam
#
# - libgcc version 10.5.0 (https://packages.ubuntu.com/focal/libgcc1)
# - libc version 2.31 (https://packages.ubuntu.com/focal/libc6)
#
# CentOS Stream 9 EOL: 2027. https://www.centos.org/cl-vs-cs/#end-of-life
#
# - libgcc version 12.2.1 (https://mirror.stream.centos.org/9-stream/AppStream/x86_64/os/Packages/)
# - libc version 2.34 (https://mirror.stream.centos.org/9-stream/AppStream/x86_64/os/Packages/)
#
# See https://gcc.gnu.org/onlinedocs/libstdc++/manual/abi.html for more info.

MAX_VERSIONS = {
'GCC':       (4,3,0),
'GLIBC': {
    lief.ELF.ARCH.X86_64: (2,31),
    lief.ELF.ARCH.ARM:    (2,31),
    lief.ELF.ARCH.AARCH64:(2,31),
    lief.ELF.ARCH.PPC64:  (2,31),
    lief.ELF.ARCH.RISCV:  (2,31),
},
'LIBATOMIC': (1,0),
'V':         (0,5,0),  # xkb (bitcoin-qt only)
}

# Ignore symbols that are exported as part of every executable
IGNORE_EXPORTS = {
'environ', '_environ', '__environ', '_fini', '_init', 'stdin',
'stdout', 'stderr',
}

# Expected linker-loader names can be found here:
# https://sourceware.org/glibc/wiki/ABIList?action=recall&rev=16
ELF_INTERPRETER_NAMES: dict[lief.ELF.ARCH, dict[lief.Header.ENDIANNESS, str]] = {
    lief.ELF.ARCH.X86_64:  {
        lief.Header.ENDIANNESS.LITTLE: "/lib64/ld-linux-x86-64.so.2",
    },
    lief.ELF.ARCH.ARM:     {
        lief.Header.ENDIANNESS.LITTLE: "/lib/ld-linux-armhf.so.3",
    },
    lief.ELF.ARCH.AARCH64: {
        lief.Header.ENDIANNESS.LITTLE: "/lib/ld-linux-aarch64.so.1",
    },
    lief.ELF.ARCH.PPC64:   {
        lief.Header.ENDIANNESS.BIG: "/lib64/ld64.so.1",
        lief.Header.ENDIANNESS.LITTLE: "/lib64/ld64.so.2",
    },
    lief.ELF.ARCH.RISCV:    {
        lief.Header.ENDIANNESS.LITTLE: "/lib/ld-linux-riscv64-lp64d.so.1",
    },
}

ELF_ABIS: dict[lief.ELF.ARCH, dict[lief.Header.ENDIANNESS, list[int]]] = {
    lief.ELF.ARCH.X86_64: {
        lief.Header.ENDIANNESS.LITTLE: [3,2,0],
    },
    lief.ELF.ARCH.ARM: {
        lief.Header.ENDIANNESS.LITTLE: [3,2,0],
    },
    lief.ELF.ARCH.AARCH64: {
        lief.Header.ENDIANNESS.LITTLE: [3,7,0],
    },
    lief.ELF.ARCH.PPC64: {
        lief.Header.ENDIANNESS.LITTLE: [3,10,0],
        lief.Header.ENDIANNESS.BIG: [3,2,0],
    },
    lief.ELF.ARCH.RISCV: {
        lief.Header.ENDIANNESS.LITTLE: [4,15,0],
    },
}

# Allowed NEEDED libraries
ELF_ALLOWED_LIBRARIES = {
# bitcoind and bitcoin-qt
'libgcc_s.so.1', # GCC base support
'libc.so.6', # C library
'libpthread.so.0', # threading
'libm.so.6', # math library
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
'libdl.so.2', # programming interface to dynamic linker
'libxcb-cursor.so.0',
'libxcb-icccm.so.4',
'libxcb-image.so.0',
'libxcb-shm.so.0',
'libxcb-keysyms.so.1',
'libxcb-randr.so.0',
'libxcb-render-util.so.0',
'libxcb-render.so.0',
'libxcb-shape.so.0',
'libxcb-sync.so.1',
'libxcb-xfixes.so.0',
'libxcb-xinerama.so.0',
'libxcb-xkb.so.1',
}

MACHO_ALLOWED_LIBRARIES = {
# bitcoind and bitcoin-qt
'libc++.1.dylib', # C++ Standard Library
'libSystem.B.dylib', # libc, libm, libpthread, libinfo
# bitcoin-qt only
'AppKit', # user interface
'ApplicationServices', # common application tasks.
'Carbon', # deprecated c back-compat API
'ColorSync',
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
'QuartzCore', # animation
'Security', # access control and authentication
'UniformTypeIdentifiers', # collection of types that map to MIME and file types
}

PE_ALLOWED_LIBRARIES = {
'ADVAPI32.dll', # legacy security & registry
'bcrypt.dll', # newer security and identity API
'IPHLPAPI.DLL', # IP helper API
'KERNEL32.dll', # win32 base APIs
'msvcrt.dll', # C standard library for MSVC
'SHELL32.dll', # shell API
'WS2_32.dll', # sockets
# bitcoin-qt only
'api-ms-win-core-synch-l1-2-0.dll', # Synchronization Primitives API
'api-ms-win-core-winrt-l1-1-0.dll', # Windows Runtime API
'api-ms-win-core-winrt-string-l1-1-0.dll', # WinRT String API
'AUTHZ.dll', # Windows Authorization Framework
'comdlg32.dll', # Common Dialog Box Library
'd3d11.dll', # Direct3D 11 API
'd3d12.dll', # Direct3D 12 API
'd3d9.dll', # Direct3D 9 API
'dwmapi.dll', # desktop window manager
'DWrite.dll', # DirectX Typography Services
'dxgi.dll', # DirectX Graphics Infrastructure
'GDI32.dll', # graphics device interface
'IMM32.dll', # input method editor
'NETAPI32.dll', # network management
'ole32.dll', # component object model
'OLEAUT32.dll', # OLE Automation API
'SHLWAPI.dll', # light weight shell API
'USER32.dll', # user interface
'USERENV.dll', # user management
'UxTheme.dll', # visual style
'VERSION.dll', # version checking
'WINMM.dll', # WinMM audio API
'WTSAPI32.dll', # Remote Desktop
'SETUPAPI.dll', # Windows Setup API
'SHCORE.dll', # Stream Handler Core
}

def check_version(max_versions, version, arch) -> bool:
    (lib, _, ver) = version.rpartition('_')
    ver = tuple([int(x) for x in ver.split('.')])
    if not lib in max_versions:
        return False
    if isinstance(max_versions[lib], tuple):
        return ver <= max_versions[lib]
    else:
        return ver <= max_versions[lib][arch]

def check_imported_symbols(binary) -> bool:
    ok: bool = True

    for symbol in binary.imported_symbols:
        if not symbol.imported:
            continue

        version = symbol.symbol_version if symbol.has_version else None

        if version:
            aux_version = version.symbol_version_auxiliary.name if version.has_auxiliary_version else None
            if aux_version and not check_version(MAX_VERSIONS, aux_version, binary.header.machine_type):
                print(f'{filename}: symbol {symbol.name} from unsupported version {version}')
                ok = False
    return ok

def check_exported_symbols(binary) -> bool:
    ok: bool = True

    for symbol in binary.dynamic_symbols:
        if not symbol.exported:
            continue
        name = symbol.name
        if binary.header.machine_type == lief.ELF.ARCH.RISCV or name in IGNORE_EXPORTS:
            continue
        print(f'{filename}: export of symbol {name} not allowed!')
        ok = False
    return ok

def check_RUNPATH(binary) -> bool:
    assert binary.get(lief.ELF.DynamicEntry.TAG.RUNPATH) is None
    assert binary.get(lief.ELF.DynamicEntry.TAG.RPATH) is None
    return True

def check_ELF_libraries(binary) -> bool:
    ok: bool = True
    for library in binary.libraries:
        if library not in ELF_ALLOWED_LIBRARIES:
            print(f'{filename}: {library} is not in ALLOWED_LIBRARIES!')
            ok = False
    return ok

def check_MACHO_libraries(binary) -> bool:
    ok: bool = True
    for dylib in binary.libraries:
        split = dylib.name.split('/')
        if split[-1] not in MACHO_ALLOWED_LIBRARIES:
            print(f'{split[-1]} is not in ALLOWED_LIBRARIES!')
            ok = False
    return ok

def check_MACHO_min_os(binary) -> bool:
    if binary.build_version.minos == [13,0,0]:
        return True
    return False

def check_MACHO_sdk(binary) -> bool:
    if binary.build_version.sdk == [14, 0, 0]:
        return True
    return False

def check_MACHO_lld(binary) -> bool:
    if binary.build_version.tools[0].version == [18, 1, 8]:
        return True
    return False

def check_PE_libraries(binary) -> bool:
    ok: bool = True
    for dylib in binary.libraries:
        if dylib not in PE_ALLOWED_LIBRARIES:
            print(f'{dylib} is not in ALLOWED_LIBRARIES!')
            ok = False
    return ok

def check_PE_subsystem_version(binary) -> bool:
    major: int = binary.optional_header.major_subsystem_version
    minor: int = binary.optional_header.minor_subsystem_version
    if major == 6 and minor == 2:
        return True
    return False

def check_PE_application_manifest(binary) -> bool:
    if not binary.has_resources:
        # No resources at all.
        return False

    rm = binary.resources_manager
    return rm.has_manifest

def check_ELF_interpreter(binary) -> bool:
    expected_interpreter = ELF_INTERPRETER_NAMES[binary.header.machine_type][binary.abstract.header.endianness]

    return binary.concrete.interpreter == expected_interpreter

def check_ELF_ABI(binary) -> bool:
    expected_abi = ELF_ABIS[binary.header.machine_type][binary.abstract.header.endianness]
    note = binary.concrete.get(lief.ELF.Note.TYPE.GNU_ABI_TAG)
    assert note.abi == lief.ELF.NoteAbi.ABI.LINUX
    return note.version == expected_abi

CHECKS = {
lief.Binary.FORMATS.ELF: [
    ('IMPORTED_SYMBOLS', check_imported_symbols),
    ('EXPORTED_SYMBOLS', check_exported_symbols),
    ('LIBRARY_DEPENDENCIES', check_ELF_libraries),
    ('INTERPRETER_NAME', check_ELF_interpreter),
    ('ABI', check_ELF_ABI),
    ('RUNPATH', check_RUNPATH),
],
lief.Binary.FORMATS.MACHO: [
    ('DYNAMIC_LIBRARIES', check_MACHO_libraries),
    ('MIN_OS', check_MACHO_min_os),
    ('SDK', check_MACHO_sdk),
    ('LLD', check_MACHO_lld),
],
lief.Binary.FORMATS.PE: [
    ('DYNAMIC_LIBRARIES', check_PE_libraries),
    ('SUBSYSTEM_VERSION', check_PE_subsystem_version),
    ('APPLICATION_MANIFEST', check_PE_application_manifest),
]
}

if __name__ == '__main__':
    retval: int = 0
    for filename in sys.argv[1:]:
        binary = lief.parse(filename)

        etype = binary.format

        failed: list[str] = []
        for (name, func) in CHECKS[etype]:
            if not func(binary):
                failed.append(name)
        if failed:
            print(f'{filename}: failed {" ".join(failed)}')
            retval = 1
    sys.exit(retval)
