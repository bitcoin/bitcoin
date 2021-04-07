#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Perform basic security checks on a series of executables.
Exit status will be 0 if successful, and the program will be silent.
Otherwise the exit status will be 1 and it will log which executables failed which checks.
Needs `objdump` (for PE).
'''
import subprocess
import sys
import os
from typing import List, Optional

import lief
import pixie

OBJDUMP_CMD = os.getenv('OBJDUMP', '/usr/bin/objdump')

def run_command(command) -> str:
    p = subprocess.run(command, stdout=subprocess.PIPE, check=True, universal_newlines=True)
    return p.stdout

def check_ELF_PIE(executable) -> bool:
    '''
    Check for position independent executable (PIE), allowing for address space randomization.
    '''
    elf = pixie.load(executable)
    return elf.hdr.e_type == pixie.ET_DYN

def check_ELF_NX(executable) -> bool:
    '''
    Check that no sections are writable and executable (including the stack)
    '''
    elf = pixie.load(executable)
    have_wx = False
    have_gnu_stack = False
    for ph in elf.program_headers:
        if ph.p_type == pixie.PT_GNU_STACK:
            have_gnu_stack = True
        if (ph.p_flags & pixie.PF_W) != 0 and (ph.p_flags & pixie.PF_X) != 0: # section is both writable and executable
            have_wx = True
    return have_gnu_stack and not have_wx

def check_ELF_RELRO(executable) -> bool:
    '''
    Check for read-only relocations.
    GNU_RELRO program header must exist
    Dynamic section must have BIND_NOW flag
    '''
    elf = pixie.load(executable)
    have_gnu_relro = False
    for ph in elf.program_headers:
        # Note: not checking p_flags == PF_R: here as linkers set the permission differently
        # This does not affect security: the permission flags of the GNU_RELRO program
        # header are ignored, the PT_LOAD header determines the effective permissions.
        # However, the dynamic linker need to write to this area so these are RW.
        # Glibc itself takes care of mprotecting this area R after relocations are finished.
        # See also https://marc.info/?l=binutils&m=1498883354122353
        if ph.p_type == pixie.PT_GNU_RELRO:
            have_gnu_relro = True

    have_bindnow = False
    for flags in elf.query_dyn_tags(pixie.DT_FLAGS):
        assert isinstance(flags, int)
        if flags & pixie.DF_BIND_NOW:
            have_bindnow = True

    return have_gnu_relro and have_bindnow

def check_ELF_Canary(executable) -> bool:
    '''
    Check for use of stack canary
    '''
    elf = pixie.load(executable)
    ok = False
    for symbol in elf.dyn_symbols:
        if symbol.name == b'__stack_chk_fail':
            ok = True
    return ok

def check_ELF_separate_code(executable):
    '''
    Check that sections are appropriately separated in virtual memory,
    based on their permissions. This checks for missing -Wl,-z,separate-code
    and potentially other problems.
    '''
    elf = pixie.load(executable)
    R = pixie.PF_R
    W = pixie.PF_W
    E = pixie.PF_X
    EXPECTED_FLAGS = {
        # Read + execute
        b'.init': R | E,
        b'.plt': R | E,
        b'.plt.got': R | E,
        b'.plt.sec': R | E,
        b'.text': R | E,
        b'.fini': R | E,
        # Read-only data
        b'.interp': R,
        b'.note.gnu.property': R,
        b'.note.gnu.build-id': R,
        b'.note.ABI-tag': R,
        b'.gnu.hash': R,
        b'.dynsym': R,
        b'.dynstr': R,
        b'.gnu.version': R,
        b'.gnu.version_r': R,
        b'.rela.dyn': R,
        b'.rela.plt': R,
        b'.rodata': R,
        b'.eh_frame_hdr': R,
        b'.eh_frame': R,
        b'.qtmetadata': R,
        b'.gcc_except_table': R,
        b'.stapsdt.base': R,
        # Writable data
        b'.init_array': R | W,
        b'.fini_array': R | W,
        b'.dynamic': R | W,
        b'.got': R | W,
        b'.data': R | W,
        b'.bss': R | W,
    }
    if elf.hdr.e_machine == pixie.EM_PPC64:
        # .plt is RW on ppc64 even with separate-code
        EXPECTED_FLAGS[b'.plt'] = R | W
    # For all LOAD program headers get mapping to the list of sections,
    # and for each section, remember the flags of the associated program header.
    flags_per_section = {}
    for ph in elf.program_headers:
        if ph.p_type == pixie.PT_LOAD:
            for section in ph.sections:
                assert(section.name not in flags_per_section)
                flags_per_section[section.name] = ph.p_flags
    # Spot-check ELF LOAD program header flags per section
    # If these sections exist, check them against the expected R/W/E flags
    for (section, flags) in flags_per_section.items():
        if section in EXPECTED_FLAGS:
            if EXPECTED_FLAGS[section] != flags:
                return False
    return True

def get_PE_dll_characteristics(executable) -> int:
    '''Get PE DllCharacteristics bits'''
    stdout = run_command([OBJDUMP_CMD, '-x',  executable])

    bits = 0
    for line in stdout.splitlines():
        tokens = line.split()
        if len(tokens)>=2 and tokens[0] == 'DllCharacteristics':
            bits = int(tokens[1],16)
    return bits

IMAGE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA = 0x0020
IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE    = 0x0040
IMAGE_DLL_CHARACTERISTICS_NX_COMPAT       = 0x0100

def check_PE_DYNAMIC_BASE(executable) -> bool:
    '''PIE: DllCharacteristics bit 0x40 signifies dynamicbase (ASLR)'''
    bits = get_PE_dll_characteristics(executable)
    return (bits & IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE) == IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE

# Must support high-entropy 64-bit address space layout randomization
# in addition to DYNAMIC_BASE to have secure ASLR.
def check_PE_HIGH_ENTROPY_VA(executable) -> bool:
    '''PIE: DllCharacteristics bit 0x20 signifies high-entropy ASLR'''
    bits = get_PE_dll_characteristics(executable)
    return (bits & IMAGE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA) == IMAGE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA

def check_PE_RELOC_SECTION(executable) -> bool:
    '''Check for a reloc section. This is required for functional ASLR.'''
    stdout = run_command([OBJDUMP_CMD, '-h',  executable])

    for line in stdout.splitlines():
        if '.reloc' in line:
            return True
    return False

def check_PE_NX(executable) -> bool:
    '''NX: DllCharacteristics bit 0x100 signifies nxcompat (DEP)'''
    bits = get_PE_dll_characteristics(executable)
    return (bits & IMAGE_DLL_CHARACTERISTICS_NX_COMPAT) == IMAGE_DLL_CHARACTERISTICS_NX_COMPAT

def check_MACHO_PIE(executable) -> bool:
    '''
    Check for position independent executable (PIE), allowing for address space randomization.
    '''
    binary = lief.parse(executable)
    return binary.is_pie

def check_MACHO_NOUNDEFS(executable) -> bool:
    '''
    Check for no undefined references.
    '''
    binary = lief.parse(executable)
    return binary.header.has(lief.MachO.HEADER_FLAGS.NOUNDEFS)

def check_MACHO_NX(executable) -> bool:
    '''
    Check for no stack execution
    '''
    binary = lief.parse(executable)
    return binary.has_nx

def check_MACHO_LAZY_BINDINGS(executable) -> bool:
    '''
    Check for no lazy bindings.
    We don't use or check for MH_BINDATLOAD. See #18295.
    '''
    binary = lief.parse(executable)
    return binary.dyld_info.lazy_bind == (0,0)

def check_MACHO_Canary(executable) -> bool:
    '''
    Check for use of stack canary
    '''
    binary = lief.parse(executable)
    return binary.has_symbol('___stack_chk_fail')

CHECKS = {
'ELF': [
    ('PIE', check_ELF_PIE),
    ('NX', check_ELF_NX),
    ('RELRO', check_ELF_RELRO),
    ('Canary', check_ELF_Canary),
    ('separate_code', check_ELF_separate_code),
],
'PE': [
    ('DYNAMIC_BASE', check_PE_DYNAMIC_BASE),
    ('HIGH_ENTROPY_VA', check_PE_HIGH_ENTROPY_VA),
    ('NX', check_PE_NX),
    ('RELOC_SECTION', check_PE_RELOC_SECTION)
],
'MACHO': [
    ('PIE', check_MACHO_PIE),
    ('NOUNDEFS', check_MACHO_NOUNDEFS),
    ('NX', check_MACHO_NX),
    ('LAZY_BINDINGS', check_MACHO_LAZY_BINDINGS),
    ('Canary', check_MACHO_Canary)
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

