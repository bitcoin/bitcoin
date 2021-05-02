#!/usr/bin/env python3
# Copyright (c) 2015-2020 The XBit Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Perform basic security checks on a series of executables.
Exit status will be 0 if successful, and the program will be silent.
Otherwise the exit status will be 1 and it will log which executables failed which checks.
Needs `readelf` (for ELF), `objdump` (for PE) and `otool` (for MACHO).
'''
import subprocess
import sys
import os

from typing import List, Optional

READELF_CMD = os.getenv('READELF', '/usr/bin/readelf')
OBJDUMP_CMD = os.getenv('OBJDUMP', '/usr/bin/objdump')
OTOOL_CMD = os.getenv('OTOOL', '/usr/bin/otool')

def run_command(command) -> str:
    p = subprocess.run(command, stdout=subprocess.PIPE, check=True, universal_newlines=True)
    return p.stdout

def check_ELF_PIE(executable) -> bool:
    '''
    Check for position independent executable (PIE), allowing for address space randomization.
    '''
    stdout = run_command([READELF_CMD, '-h', '-W', executable])

    ok = False
    for line in stdout.splitlines():
        tokens = line.split()
        if len(line)>=2 and tokens[0] == 'Type:' and tokens[1] == 'DYN':
            ok = True
    return ok

def get_ELF_program_headers(executable):
    '''Return type and flags for ELF program headers'''
    stdout = run_command([READELF_CMD, '-l', '-W', executable])

    in_headers = False
    headers = []
    for line in stdout.splitlines():
        if line.startswith('Program Headers:'):
            in_headers = True
            count = 0
        if line == '':
            in_headers = False
        if in_headers:
            if count == 1: # header line
                header = [x.strip() for x in line.split()]
                ofs_typ = header.index('Type')
                ofs_flags = header.index('Flg')
                # assert readelf output is what we expect
                if ofs_typ == -1 or ofs_flags == -1:
                    raise ValueError('Cannot parse elfread -lW output')
            elif count > 1:
                splitline = [x.strip() for x in line.split()]
                typ = splitline[ofs_typ]
                if not typ.startswith('[R'): # skip [Requesting ...]
                    splitline = [x.strip() for x in line.split()]
                    flags = splitline[ofs_flags]
                    # check for 'R', ' E'
                    if splitline[ofs_flags + 1] == 'E':
                        flags += ' E'
                    headers.append((typ, flags, []))
            count += 1

        if line.startswith(' Section to Segment mapping:'):
            in_mapping = True
            count = 0
        if line == '':
            in_mapping = False
        if in_mapping:
            if count == 1: # header line
                ofs_segment = line.find('Segment')
                ofs_sections = line.find('Sections...')
                if ofs_segment == -1 or ofs_sections == -1:
                    raise ValueError('Cannot parse elfread -lW output')
            elif count > 1:
                segment = int(line[ofs_segment:ofs_sections].strip())
                sections = line[ofs_sections:].strip().split()
                headers[segment][2].extend(sections)
            count += 1
    return headers

def check_ELF_NX(executable) -> bool:
    '''
    Check that no sections are writable and executable (including the stack)
    '''
    have_wx = False
    have_gnu_stack = False
    for (typ, flags, _) in get_ELF_program_headers(executable):
        if typ == 'GNU_STACK':
            have_gnu_stack = True
        if 'W' in flags and 'E' in flags: # section is both writable and executable
            have_wx = True
    return have_gnu_stack and not have_wx

def check_ELF_RELRO(executable) -> bool:
    '''
    Check for read-only relocations.
    GNU_RELRO program header must exist
    Dynamic section must have BIND_NOW flag
    '''
    have_gnu_relro = False
    for (typ, flags, _) in get_ELF_program_headers(executable):
        # Note: not checking flags == 'R': here as linkers set the permission differently
        # This does not affect security: the permission flags of the GNU_RELRO program
        # header are ignored, the PT_LOAD header determines the effective permissions.
        # However, the dynamic linker need to write to this area so these are RW.
        # Glibc itself takes care of mprotecting this area R after relocations are finished.
        # See also https://marc.info/?l=binutils&m=1498883354122353
        if typ == 'GNU_RELRO':
            have_gnu_relro = True

    have_bindnow = False
    stdout = run_command([READELF_CMD, '-d', '-W', executable])

    for line in stdout.splitlines():
        tokens = line.split()
        if len(tokens)>1 and tokens[1] == '(BIND_NOW)' or (len(tokens)>2 and tokens[1] == '(FLAGS)' and 'BIND_NOW' in tokens[2:]):
            have_bindnow = True
    return have_gnu_relro and have_bindnow

def check_ELF_Canary(executable) -> bool:
    '''
    Check for use of stack canary
    '''
    stdout = run_command([READELF_CMD, '--dyn-syms', '-W', executable])

    ok = False
    for line in stdout.splitlines():
        if '__stack_chk_fail' in line:
            ok = True
    return ok

def check_ELF_separate_code(executable):
    '''
    Check that sections are appropriately separated in virtual memory,
    based on their permissions. This checks for missing -Wl,-z,separate-code
    and potentially other problems.
    '''
    EXPECTED_FLAGS = {
        # Read + execute
        '.init': 'R E',
        '.plt': 'R E',
        '.plt.got': 'R E',
        '.plt.sec': 'R E',
        '.text': 'R E',
        '.fini': 'R E',
        # Read-only data
        '.interp': 'R',
        '.note.gnu.property': 'R',
        '.note.gnu.build-id': 'R',
        '.note.ABI-tag': 'R',
        '.gnu.hash': 'R',
        '.dynsym': 'R',
        '.dynstr': 'R',
        '.gnu.version': 'R',
        '.gnu.version_r': 'R',
        '.rela.dyn': 'R',
        '.rela.plt': 'R',
        '.rodata': 'R',
        '.eh_frame_hdr': 'R',
        '.eh_frame': 'R',
        '.qtmetadata': 'R',
        '.gcc_except_table': 'R',
        '.stapsdt.base': 'R',
        # Writable data
        '.init_array': 'RW',
        '.fini_array': 'RW',
        '.dynamic': 'RW',
        '.got': 'RW',
        '.data': 'RW',
        '.bss': 'RW',
    }
    # For all LOAD program headers get mapping to the list of sections,
    # and for each section, remember the flags of the associated program header.
    flags_per_section = {}
    for (typ, flags, sections) in get_ELF_program_headers(executable):
        if typ == 'LOAD':
            for section in sections:
                assert(section not in flags_per_section)
                flags_per_section[section] = flags
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

def get_MACHO_executable_flags(executable) -> List[str]:
    stdout = run_command([OTOOL_CMD, '-vh', executable])

    flags = []
    for line in stdout.splitlines():
        tokens = line.split()
        # filter first two header lines
        if 'magic' in tokens or 'Mach' in tokens:
            continue
        # filter ncmds and sizeofcmds values
        flags += [t for t in tokens if not t.isdigit()]
    return flags

def check_MACHO_PIE(executable) -> bool:
    '''
    Check for position independent executable (PIE), allowing for address space randomization.
    '''
    flags = get_MACHO_executable_flags(executable)
    if 'PIE' in flags:
        return True
    return False

def check_MACHO_NOUNDEFS(executable) -> bool:
    '''
    Check for no undefined references.
    '''
    flags = get_MACHO_executable_flags(executable)
    if 'NOUNDEFS' in flags:
        return True
    return False

def check_MACHO_NX(executable) -> bool:
    '''
    Check for no stack execution
    '''
    flags = get_MACHO_executable_flags(executable)
    if 'ALLOW_STACK_EXECUTION' in flags:
        return False
    return True

def check_MACHO_LAZY_BINDINGS(executable) -> bool:
    '''
    Check for no lazy bindings.
    We don't use or check for MH_BINDATLOAD. See #18295.
    '''
    stdout = run_command([OTOOL_CMD, '-l', executable])

    for line in stdout.splitlines():
        tokens = line.split()
        if 'lazy_bind_off' in tokens or 'lazy_bind_size' in tokens:
            if tokens[1] != '0':
                return False
    return True

def check_MACHO_Canary(executable) -> bool:
    '''
    Check for use of stack canary
    '''
    stdout = run_command([OTOOL_CMD, '-Iv', executable])

    ok = False
    for line in stdout.splitlines():
        if '___stack_chk_fail' in line:
            ok = True
    return ok

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
    retval = 0
    for filename in sys.argv[1:]:
        try:
            etype = identify_executable(filename)
            if etype is None:
                print('%s: unknown format' % filename)
                retval = 1
                continue

            failed = []
            for (name, func) in CHECKS[etype]:
                if not func(filename):
                    failed.append(name)
            if failed:
                print('%s: failed %s' % (filename, ' '.join(failed)))
                retval = 1
        except IOError:
            print('%s: cannot open' % filename)
            retval = 1
    sys.exit(retval)

