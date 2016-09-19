#!/usr/bin/env python
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Perform basic ELF security checks on a series of executables.
Exit status will be 0 if successful, and the program will be silent.
Otherwise the exit status will be 1 and it will log which executables failed which checks.
Needs `readelf` (for ELF) and `objdump` (for PE).
'''
from __future__ import division,print_function,unicode_literals
import subprocess
import sys
import os

READELF_CMD = os.getenv('READELF', '/usr/bin/readelf')
OBJDUMP_CMD = os.getenv('OBJDUMP', '/usr/bin/objdump')

def check_ELF_PIE(executable):
    '''
    Check for position independent executable (PIE), allowing for address space randomization.
    '''
    p = subprocess.Popen([READELF_CMD, '-h', '-W', executable], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    if p.returncode:
        raise IOError('Error opening file')

    ok = False
    for line in stdout.split(b'\n'):
        line = line.split()
        if len(line)>=2 and line[0] == b'Type:' and line[1] == b'DYN':
            ok = True
    return ok

def get_ELF_program_headers(executable):
    '''Return type and flags for ELF program headers'''
    p = subprocess.Popen([READELF_CMD, '-l', '-W', executable], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    if p.returncode:
        raise IOError('Error opening file')
    in_headers = False
    count = 0
    headers = []
    for line in stdout.split(b'\n'):
        if line.startswith(b'Program Headers:'):
            in_headers = True
        if line == b'':
            in_headers = False
        if in_headers:
            if count == 1: # header line
                ofs_typ = line.find(b'Type')
                ofs_offset = line.find(b'Offset')
                ofs_flags = line.find(b'Flg')
                ofs_align = line.find(b'Align')
                if ofs_typ == -1 or ofs_offset == -1 or ofs_flags == -1 or ofs_align  == -1:
                    raise ValueError('Cannot parse elfread -lW output')
            elif count > 1:
                typ = line[ofs_typ:ofs_offset].rstrip()
                flags = line[ofs_flags:ofs_align].rstrip()
                headers.append((typ, flags))
            count += 1
    return headers

def check_ELF_NX(executable):
    '''
    Check that no sections are writable and executable (including the stack)
    '''
    have_wx = False
    have_gnu_stack = False
    for (typ, flags) in get_ELF_program_headers(executable):
        if typ == b'GNU_STACK':
            have_gnu_stack = True
        if b'W' in flags and b'E' in flags: # section is both writable and executable
            have_wx = True
    return have_gnu_stack and not have_wx

def check_ELF_RELRO(executable):
    '''
    Check for read-only relocations.
    GNU_RELRO program header must exist
    Dynamic section must have BIND_NOW flag
    '''
    have_gnu_relro = False
    for (typ, flags) in get_ELF_program_headers(executable):
        # Note: not checking flags == 'R': here as linkers set the permission differently
        # This does not affect security: the permission flags of the GNU_RELRO program header are ignored, the PT_LOAD header determines the effective permissions.
        # However, the dynamic linker need to write to this area so these are RW.
        # Glibc itself takes care of mprotecting this area R after relocations are finished.
        # See also http://permalink.gmane.org/gmane.comp.gnu.binutils/71347
        if typ == b'GNU_RELRO':
            have_gnu_relro = True

    have_bindnow = False
    p = subprocess.Popen([READELF_CMD, '-d', '-W', executable], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    if p.returncode:
        raise IOError('Error opening file')
    for line in stdout.split(b'\n'):
        tokens = line.split()
        if len(tokens)>1 and tokens[1] == b'(BIND_NOW)' or (len(tokens)>2 and tokens[1] == b'(FLAGS)' and b'BIND_NOW' in tokens[2]):
            have_bindnow = True
    return have_gnu_relro and have_bindnow

def check_ELF_Canary(executable):
    '''
    Check for use of stack canary
    '''
    p = subprocess.Popen([READELF_CMD, '--dyn-syms', '-W', executable], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    if p.returncode:
        raise IOError('Error opening file')
    ok = False
    for line in stdout.split(b'\n'):
        if b'__stack_chk_fail' in line:
            ok = True
    return ok

def get_PE_dll_characteristics(executable):
    '''
    Get PE DllCharacteristics bits
    '''
    p = subprocess.Popen([OBJDUMP_CMD, '-x',  executable], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    if p.returncode:
        raise IOError('Error opening file')
    for line in stdout.split('\n'):
        tokens = line.split()
        if len(tokens)>=2 and tokens[0] == 'DllCharacteristics':
            return int(tokens[1],16)
    return 0


def check_PE_PIE(executable):
    '''PIE: DllCharacteristics bit 0x40 signifies dynamicbase (ASLR)'''
    return bool(get_PE_dll_characteristics(executable) & 0x40)

def check_PE_NX(executable):
    '''NX: DllCharacteristics bit 0x100 signifies nxcompat (DEP)'''
    return bool(get_PE_dll_characteristics(executable) & 0x100)

CHECKS = {
'ELF': [
    ('PIE', check_ELF_PIE),
    ('NX', check_ELF_NX),
    ('RELRO', check_ELF_RELRO),
    ('Canary', check_ELF_Canary)
],
'PE': [
    ('PIE', check_PE_PIE),
    ('NX', check_PE_NX)
]
}

def identify_executable(executable):
    with open(filename, 'rb') as f:
        magic = f.read(4)
    if magic.startswith(b'MZ'):
        return 'PE'
    elif magic.startswith(b'\x7fELF'):
        return 'ELF'
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
    exit(retval)

