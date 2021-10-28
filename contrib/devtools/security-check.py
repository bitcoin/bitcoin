#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Perform basic security checks on a series of executables.
Exit status will be 0 if successful, and the program will be silent.
Otherwise the exit status will be 1 and it will log which executables failed which checks.
'''
import sys
from typing import List

import lief #type:ignore

def check_ELF_RELRO(binary) -> bool:
    '''
    Check for read-only relocations.
    GNU_RELRO program header must exist
    Dynamic section must have BIND_NOW flag
    '''
    have_gnu_relro = False
    for segment in binary.segments:
        # Note: not checking p_flags == PF_R: here as linkers set the permission differently
        # This does not affect security: the permission flags of the GNU_RELRO program
        # header are ignored, the PT_LOAD header determines the effective permissions.
        # However, the dynamic linker need to write to this area so these are RW.
        # Glibc itself takes care of mprotecting this area R after relocations are finished.
        # See also https://marc.info/?l=binutils&m=1498883354122353
        if segment.type == lief.ELF.SEGMENT_TYPES.GNU_RELRO:
            have_gnu_relro = True

    have_bindnow = False
    try:
        flags = binary.get(lief.ELF.DYNAMIC_TAGS.FLAGS)
        if flags.value & lief.ELF.DYNAMIC_FLAGS.BIND_NOW:
            have_bindnow = True
    except:
        have_bindnow = False

    return have_gnu_relro and have_bindnow

def check_ELF_Canary(binary) -> bool:
    '''
    Check for use of stack canary
    '''
    return binary.has_symbol('__stack_chk_fail')

def check_ELF_separate_code(binary):
    '''
    Check that sections are appropriately separated in virtual memory,
    based on their permissions. This checks for missing -Wl,-z,separate-code
    and potentially other problems.
    '''
    R = lief.ELF.SEGMENT_FLAGS.R
    W = lief.ELF.SEGMENT_FLAGS.W
    E = lief.ELF.SEGMENT_FLAGS.X
    EXPECTED_FLAGS = {
        # Read + execute
        '.init': R | E,
        '.plt': R | E,
        '.plt.got': R | E,
        '.plt.sec': R | E,
        '.text': R | E,
        '.fini': R | E,
        # Read-only data
        '.interp': R,
        '.note.gnu.property': R,
        '.note.gnu.build-id': R,
        '.note.ABI-tag': R,
        '.gnu.hash': R,
        '.dynsym': R,
        '.dynstr': R,
        '.gnu.version': R,
        '.gnu.version_r': R,
        '.rela.dyn': R,
        '.rela.plt': R,
        '.rodata': R,
        '.eh_frame_hdr': R,
        '.eh_frame': R,
        '.qtmetadata': R,
        '.gcc_except_table': R,
        '.stapsdt.base': R,
        # Writable data
        '.init_array': R | W,
        '.fini_array': R | W,
        '.dynamic': R | W,
        '.got': R | W,
        '.data': R | W,
        '.bss': R | W,
    }
    if binary.header.machine_type == lief.ELF.ARCH.PPC64:
        # .plt is RW on ppc64 even with separate-code
        EXPECTED_FLAGS['.plt'] = R | W
    # For all LOAD program headers get mapping to the list of sections,
    # and for each section, remember the flags of the associated program header.
    flags_per_section = {}
    for segment in binary.segments:
        if segment.type ==  lief.ELF.SEGMENT_TYPES.LOAD:
            for section in segment.sections:
                assert(section.name not in flags_per_section)
                flags_per_section[section.name] = segment.flags
    # Spot-check ELF LOAD program header flags per section
    # If these sections exist, check them against the expected R/W/E flags
    for (section, flags) in flags_per_section.items():
        if section in EXPECTED_FLAGS:
            if int(EXPECTED_FLAGS[section]) != int(flags):
                return False
    return True

def check_PE_DYNAMIC_BASE(binary) -> bool:
    '''PIE: DllCharacteristics bit 0x40 signifies dynamicbase (ASLR)'''
    return lief.PE.DLL_CHARACTERISTICS.DYNAMIC_BASE in binary.optional_header.dll_characteristics_lists

# Must support high-entropy 64-bit address space layout randomization
# in addition to DYNAMIC_BASE to have secure ASLR.
def check_PE_HIGH_ENTROPY_VA(binary) -> bool:
    '''PIE: DllCharacteristics bit 0x20 signifies high-entropy ASLR'''
    return lief.PE.DLL_CHARACTERISTICS.HIGH_ENTROPY_VA in binary.optional_header.dll_characteristics_lists

def check_PE_RELOC_SECTION(binary) -> bool:
    '''Check for a reloc section. This is required for functional ASLR.'''
    return binary.has_relocations

def check_MACHO_NOUNDEFS(binary) -> bool:
    '''
    Check for no undefined references.
    '''
    return binary.header.has(lief.MachO.HEADER_FLAGS.NOUNDEFS)

def check_MACHO_LAZY_BINDINGS(binary) -> bool:
    '''
    Check for no lazy bindings.
    We don't use or check for MH_BINDATLOAD. See #18295.
    '''
    return binary.dyld_info.lazy_bind == (0,0)

def check_MACHO_Canary(binary) -> bool:
    '''
    Check for use of stack canary
    '''
    return binary.has_symbol('___stack_chk_fail')

def check_PIE(binary) -> bool:
    '''
    Check for position independent executable (PIE),
    allowing for address space randomization.
    '''
    return binary.is_pie

def check_NX(binary) -> bool:
    '''
    Check for no stack execution
    '''
    return binary.has_nx

def check_control_flow(binary) -> bool:
    '''
    Check for control flow instrumentation
    '''
    content = binary.get_content_from_virtual_address(binary.entrypoint, 4, lief.Binary.VA_TYPES.AUTO)

    if content == [243, 15, 30, 250]: # endbr64
        return True
    return False


CHECKS = {
'ELF': [
    ('PIE', check_PIE),
    ('NX', check_NX),
    ('RELRO', check_ELF_RELRO),
    ('Canary', check_ELF_Canary),
    ('separate_code', check_ELF_separate_code),
],
'PE': [
    ('PIE', check_PIE),
    ('DYNAMIC_BASE', check_PE_DYNAMIC_BASE),
    ('HIGH_ENTROPY_VA', check_PE_HIGH_ENTROPY_VA),
    ('NX', check_NX),
    ('RELOC_SECTION', check_PE_RELOC_SECTION)
],
'MACHO': [
    ('PIE', check_PIE),
    ('NOUNDEFS', check_MACHO_NOUNDEFS),
    ('NX', check_NX),
    ('LAZY_BINDINGS', check_MACHO_LAZY_BINDINGS),
    ('Canary', check_MACHO_Canary),
    ('CONTROL_FLOW', check_control_flow),
]
}

if __name__ == '__main__':
    retval: int = 0
    for filename in sys.argv[1:]:
        try:
            binary = lief.parse(filename)
            etype = binary.format.name
            if etype == lief.EXE_FORMATS.UNKNOWN:
                print(f'{filename}: unknown executable format')
                retval = 1
                continue

            failed: List[str] = []
            for (name, func) in CHECKS[etype]:
                if not func(binary):
                    failed.append(name)
            if failed:
                print(f'{filename}: failed {" ".join(failed)}')
                retval = 1
        except IOError:
            print(f'{filename}: cannot open')
            retval = 1
    sys.exit(retval)

