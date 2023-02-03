#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Perform basic security checks on a series of executables.
Exit status will be 0 if successful, and the program will be silent.
Otherwise the exit status will be 1 and it will log which executables failed which checks.
'''
import re
import sys

import lief

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
    except Exception:
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
                flags_per_section[section.name] = segment.flags
    # Spot-check ELF LOAD program header flags per section
    # If these sections exist, check them against the expected R/W/E flags
    for (section, flags) in flags_per_section.items():
        if section in EXPECTED_FLAGS:
            if int(EXPECTED_FLAGS[section]) != int(flags):
                return False
    return True

def check_ELF_control_flow(binary) -> bool:
    '''
    Check for control flow instrumentation
    '''
    main = binary.get_function_address('main')
    content = binary.get_content_from_virtual_address(main, 4, lief.Binary.VA_TYPES.AUTO)

    if content.tolist() == [243, 15, 30, 250]: # endbr64
        return True
    return False

def check_ELF_FORTIFY(binary) -> bool:

    chk_funcs = set()

    for sym in binary.symbols:
        match = re.search(r'__[a-z]*_chk', sym.name)
        if match:
            chk_funcs.add(match.group(0))

    # ignore stack-protector and bdb
    chk_funcs.discard('__stack_chk')
    chk_funcs.discard('__db_chk')

    return len(chk_funcs) >= 1

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

def check_PE_control_flow(binary) -> bool:
    '''
    Check for control flow instrumentation
    '''
    main = binary.get_symbol('main').value

    section_addr = binary.section_from_rva(main).virtual_address
    virtual_address = binary.optional_header.imagebase + section_addr + main

    content = binary.get_content_from_virtual_address(virtual_address, 4, lief.Binary.VA_TYPES.VA)

    if content.tolist() == [243, 15, 30, 250]: # endbr64
        return True
    return False

def check_PE_Canary(binary) -> bool:
    '''
    Check for use of stack canary
    '''
    return binary.has_symbol('__stack_chk_fail')

def check_MACHO_NOUNDEFS(binary) -> bool:
    '''
    Check for no undefined references.
    '''
    return binary.header.has(lief.MachO.HEADER_FLAGS.NOUNDEFS)

def check_MACHO_FIXUP_CHAINS(binary) -> bool:
    '''
    Check for use of chained fixups.
    '''
    return binary.has_dyld_chained_fixups

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

def check_MACHO_control_flow(binary) -> bool:
    '''
    Check for control flow instrumentation
    '''
    content = binary.get_content_from_virtual_address(binary.entrypoint, 4, lief.Binary.VA_TYPES.AUTO)

    if content.tolist() == [243, 15, 30, 250]: # endbr64
        return True
    return False

def check_MACHO_branch_protection(binary) -> bool:
    '''
    Check for branch protection instrumentation
    '''
    content = binary.get_content_from_virtual_address(binary.entrypoint, 4, lief.Binary.VA_TYPES.AUTO)

    if content.tolist() == [95, 36, 3, 213]: # bti
        return True
    return False

BASE_ELF = [
    ('PIE', check_PIE),
    ('NX', check_NX),
    ('RELRO', check_ELF_RELRO),
    ('Canary', check_ELF_Canary),
    ('separate_code', check_ELF_separate_code),
    ('FORTIFY', check_ELF_FORTIFY),
]

BASE_PE = [
    ('PIE', check_PIE),
    ('DYNAMIC_BASE', check_PE_DYNAMIC_BASE),
    ('HIGH_ENTROPY_VA', check_PE_HIGH_ENTROPY_VA),
    ('NX', check_NX),
    ('RELOC_SECTION', check_PE_RELOC_SECTION),
    ('CONTROL_FLOW', check_PE_control_flow),
    ('Canary', check_PE_Canary),
]

BASE_MACHO = [
    ('NOUNDEFS', check_MACHO_NOUNDEFS),
    ('Canary', check_MACHO_Canary),
    ('FIXUP_CHAINS', check_MACHO_FIXUP_CHAINS),
]

CHECKS = {
    lief.EXE_FORMATS.ELF: {
        lief.ARCHITECTURES.X86: BASE_ELF + [('CONTROL_FLOW', check_ELF_control_flow)],
        lief.ARCHITECTURES.ARM: BASE_ELF,
        lief.ARCHITECTURES.ARM64: BASE_ELF,
        lief.ARCHITECTURES.PPC: BASE_ELF,
        lief.ARCHITECTURES.RISCV: BASE_ELF,
    },
    lief.EXE_FORMATS.PE: {
        lief.ARCHITECTURES.X86: BASE_PE,
    },
    lief.EXE_FORMATS.MACHO: {
        lief.ARCHITECTURES.X86: BASE_MACHO + [('PIE', check_PIE),
                                              ('NX', check_NX),
                                              ('CONTROL_FLOW', check_MACHO_control_flow)],
        lief.ARCHITECTURES.ARM64: BASE_MACHO + [('BRANCH_PROTECTION', check_MACHO_branch_protection)],
    }
}

if __name__ == '__main__':
    retval: int = 0
    for filename in sys.argv[1:]:
        try:
            binary = lief.parse(filename)
            etype = binary.format
            arch = binary.abstract.header.architecture
            binary.concrete

            if etype == lief.EXE_FORMATS.UNKNOWN:
                print(f'{filename}: unknown executable format')
                retval = 1
                continue

            if arch == lief.ARCHITECTURES.NONE:
                print(f'{filename}: unknown architecture')
                retval = 1
                continue

            failed: list[str] = []
            for (name, func) in CHECKS[etype][arch]:
                if not func(binary):
                    failed.append(name)
            if failed:
                print(f'{filename}: failed {" ".join(failed)}')
                retval = 1
        except IOError:
            print(f'{filename}: cannot open')
            retval = 1
    sys.exit(retval)

