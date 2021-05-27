#!/usr/bin/env python3
# Copyright (c) 2020 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Compact, self-contained ELF implementation for bitcoin-core security checks.
'''
import struct
import types
from typing import Dict, List, Optional, Union, Tuple

# you can find all these values in elf.h
EI_NIDENT = 16

# Byte indices in e_ident
EI_CLASS = 4 # ELFCLASSxx
EI_DATA = 5  # ELFDATAxxxx

ELFCLASS32 = 1 # 32-bit
ELFCLASS64 = 2 # 64-bit

ELFDATA2LSB = 1 # little endian
ELFDATA2MSB = 2 # big endian

# relevant values for e_machine
EM_386 = 3
EM_PPC64 = 21
EM_ARM = 40
EM_AARCH64 = 183
EM_X86_64 = 62
EM_RISCV = 243

# relevant values for e_type
ET_DYN = 3

# relevant values for sh_type
SHT_PROGBITS = 1
SHT_STRTAB = 3
SHT_DYNAMIC = 6
SHT_DYNSYM = 11
SHT_GNU_verneed = 0x6ffffffe
SHT_GNU_versym = 0x6fffffff

# relevant values for p_type
PT_LOAD = 1
PT_GNU_STACK = 0x6474e551
PT_GNU_RELRO = 0x6474e552

# relevant values for p_flags
PF_X = (1 << 0)
PF_W = (1 << 1)
PF_R = (1 << 2)

# relevant values for d_tag
DT_NEEDED = 1
DT_FLAGS = 30

# relevant values of `d_un.d_val' in the DT_FLAGS entry
DF_BIND_NOW = 0x00000008

# relevant d_tags with string payload
STRING_TAGS = {DT_NEEDED}

# rrlevant values for ST_BIND subfield of st_info (symbol binding)
STB_LOCAL = 0

class ELFRecord(types.SimpleNamespace):
    '''Unified parsing for ELF records.'''
    def __init__(self, data: bytes, offset: int, eh: 'ELFHeader', total_size: Optional[int]) -> None:
        hdr_struct = self.STRUCT[eh.ei_class][0][eh.ei_data]
        if total_size is not None and hdr_struct.size > total_size:
            raise ValueError(f'{self.__class__.__name__} header size too small ({total_size} < {hdr_struct.size})')
        for field, value in zip(self.STRUCT[eh.ei_class][1], hdr_struct.unpack(data[offset:offset + hdr_struct.size])):
            setattr(self, field, value)

def BiStruct(chars: str) -> Dict[int, struct.Struct]:
    '''Compile a struct parser for both endians.'''
    return {
        ELFDATA2LSB: struct.Struct('<' + chars),
        ELFDATA2MSB: struct.Struct('>' + chars),
    }

class ELFHeader(ELFRecord):
    FIELDS = ['e_type', 'e_machine', 'e_version', 'e_entry', 'e_phoff', 'e_shoff', 'e_flags', 'e_ehsize', 'e_phentsize', 'e_phnum', 'e_shentsize', 'e_shnum', 'e_shstrndx']
    STRUCT = {
        ELFCLASS32: (BiStruct('HHIIIIIHHHHHH'), FIELDS),
        ELFCLASS64: (BiStruct('HHIQQQIHHHHHH'), FIELDS),
    }

    def __init__(self, data: bytes, offset: int) -> None:
        self.e_ident = data[offset:offset + EI_NIDENT]
        if self.e_ident[0:4] != b'\x7fELF':
            raise ValueError('invalid ELF magic')
        self.ei_class = self.e_ident[EI_CLASS]
        self.ei_data = self.e_ident[EI_DATA]

        super().__init__(data, offset + EI_NIDENT, self, None)

    def __repr__(self) -> str:
        return f'Header(e_ident={self.e_ident!r}, e_type={self.e_type}, e_machine={self.e_machine}, e_version={self.e_version}, e_entry={self.e_entry}, e_phoff={self.e_phoff}, e_shoff={self.e_shoff}, e_flags={self.e_flags}, e_ehsize={self.e_ehsize}, e_phentsize={self.e_phentsize}, e_phnum={self.e_phnum}, e_shentsize={self.e_shentsize}, e_shnum={self.e_shnum}, e_shstrndx={self.e_shstrndx})'

class Section(ELFRecord):
    name: Optional[bytes] = None
    FIELDS = ['sh_name', 'sh_type', 'sh_flags', 'sh_addr', 'sh_offset', 'sh_size', 'sh_link', 'sh_info', 'sh_addralign', 'sh_entsize']
    STRUCT = {
        ELFCLASS32: (BiStruct('IIIIIIIIII'), FIELDS),
        ELFCLASS64: (BiStruct('IIQQQQIIQQ'), FIELDS),
    }

    def __init__(self, data: bytes, offset: int, eh: ELFHeader) -> None:
        super().__init__(data, offset, eh, eh.e_shentsize)
        self._data = data

    def __repr__(self) -> str:
        return f'Section(sh_name={self.sh_name}({self.name!r}), sh_type=0x{self.sh_type:x}, sh_flags={self.sh_flags}, sh_addr=0x{self.sh_addr:x}, sh_offset=0x{self.sh_offset:x}, sh_size={self.sh_size}, sh_link={self.sh_link}, sh_info={self.sh_info}, sh_addralign={self.sh_addralign}, sh_entsize={self.sh_entsize})'

    def contents(self) -> bytes:
        '''Return section contents.'''
        return self._data[self.sh_offset:self.sh_offset + self.sh_size]

class ProgramHeader(ELFRecord):
    STRUCT = {
        # different ELF classes have the same fields, but in a different order to optimize space versus alignment
        ELFCLASS32: (BiStruct('IIIIIIII'), ['p_type', 'p_offset', 'p_vaddr', 'p_paddr', 'p_filesz', 'p_memsz', 'p_flags', 'p_align']),
        ELFCLASS64: (BiStruct('IIQQQQQQ'), ['p_type', 'p_flags', 'p_offset', 'p_vaddr', 'p_paddr', 'p_filesz', 'p_memsz', 'p_align']),
    }

    def __init__(self, data: bytes, offset: int, eh: ELFHeader) -> None:
        super().__init__(data, offset, eh, eh.e_phentsize)

    def __repr__(self) -> str:
        return f'ProgramHeader(p_type={self.p_type}, p_offset={self.p_offset}, p_vaddr={self.p_vaddr}, p_paddr={self.p_paddr}, p_filesz={self.p_filesz}, p_memsz={self.p_memsz}, p_flags={self.p_flags}, p_align={self.p_align})'

class Symbol(ELFRecord):
    STRUCT = {
        # different ELF classes have the same fields, but in a different order to optimize space versus alignment
        ELFCLASS32: (BiStruct('IIIBBH'), ['st_name', 'st_value', 'st_size', 'st_info', 'st_other', 'st_shndx']),
        ELFCLASS64: (BiStruct('IBBHQQ'), ['st_name', 'st_info', 'st_other', 'st_shndx', 'st_value', 'st_size']),
    }

    def __init__(self, data: bytes, offset: int, eh: ELFHeader, symtab: Section, strings: bytes, version: Optional[bytes]) -> None:
        super().__init__(data, offset, eh, symtab.sh_entsize)
        self.name = _lookup_string(strings, self.st_name)
        self.version = version

    def __repr__(self) -> str:
        return f'Symbol(st_name={self.st_name}({self.name!r}), st_value={self.st_value}, st_size={self.st_size}, st_info={self.st_info}, st_other={self.st_other}, st_shndx={self.st_shndx}, version={self.version!r})'

    @property
    def is_import(self) -> bool:
        '''Returns whether the symbol is an imported symbol.'''
        return self.st_bind != STB_LOCAL and self.st_shndx == 0

    @property
    def is_export(self) -> bool:
        '''Returns whether the symbol is an exported symbol.'''
        return self.st_bind != STB_LOCAL and self.st_shndx != 0

    @property
    def st_bind(self) -> int:
        '''Returns STB_*.'''
        return self.st_info >> 4

class Verneed(ELFRecord):
    DEF = (BiStruct('HHIII'), ['vn_version', 'vn_cnt', 'vn_file', 'vn_aux', 'vn_next'])
    STRUCT = { ELFCLASS32: DEF, ELFCLASS64: DEF }

    def __init__(self, data: bytes, offset: int, eh: ELFHeader) -> None:
        super().__init__(data, offset, eh, None)

    def __repr__(self) -> str:
        return f'Verneed(vn_version={self.vn_version}, vn_cnt={self.vn_cnt}, vn_file={self.vn_file}, vn_aux={self.vn_aux}, vn_next={self.vn_next})'

class Vernaux(ELFRecord):
    DEF = (BiStruct('IHHII'), ['vna_hash', 'vna_flags', 'vna_other', 'vna_name', 'vna_next'])
    STRUCT = { ELFCLASS32: DEF, ELFCLASS64: DEF }

    def __init__(self, data: bytes, offset: int, eh: ELFHeader, strings: bytes) -> None:
        super().__init__(data, offset, eh, None)
        self.name = _lookup_string(strings, self.vna_name)

    def __repr__(self) -> str:
        return f'Veraux(vna_hash={self.vna_hash}, vna_flags={self.vna_flags}, vna_other={self.vna_other}, vna_name={self.vna_name}({self.name!r}), vna_next={self.vna_next})'

class DynTag(ELFRecord):
    STRUCT = {
        ELFCLASS32: (BiStruct('II'), ['d_tag', 'd_val']),
        ELFCLASS64: (BiStruct('QQ'), ['d_tag', 'd_val']),
    }

    def __init__(self, data: bytes, offset: int, eh: ELFHeader, section: Section) -> None:
        super().__init__(data, offset, eh, section.sh_entsize)

    def __repr__(self) -> str:
        return f'DynTag(d_tag={self.d_tag}, d_val={self.d_val})'

def _lookup_string(data: bytes, index: int) -> bytes:
    '''Look up string by offset in ELF string table.'''
    endx = data.find(b'\x00', index)
    assert endx != -1
    return data[index:endx]

VERSYM_S = BiStruct('H') # .gnu_version section has a single 16-bit integer per symbol in the linked section
def _parse_symbol_table(section: Section, strings: bytes, eh: ELFHeader, versym: bytes, verneed: Dict[int, bytes]) -> List[Symbol]:
    '''Parse symbol table, return a list of symbols.'''
    data = section.contents()
    symbols = []
    versym_iter = (verneed.get(v[0]) for v in VERSYM_S[eh.ei_data].iter_unpack(versym))
    for ofs, version in zip(range(0, len(data), section.sh_entsize), versym_iter):
        symbols.append(Symbol(data, ofs, eh, section, strings, version))
    return symbols

def _parse_verneed(section: Section, strings: bytes, eh: ELFHeader) -> Dict[int, bytes]:
    '''Parse .gnu.version_r section, return a dictionary of {versym: 'GLIBC_...'}.'''
    data = section.contents()
    ofs = 0
    result = {}
    while True:
        verneed = Verneed(data, ofs, eh)
        aofs = verneed.vn_aux
        while True:
            vernaux = Vernaux(data, aofs, eh, strings)
            result[vernaux.vna_other] = vernaux.name
            if not vernaux.vna_next:
                break
            aofs += vernaux.vna_next

        if not verneed.vn_next:
            break
        ofs += verneed.vn_next

    return result

def _parse_dyn_tags(section: Section, strings: bytes, eh: ELFHeader) -> List[Tuple[int, Union[bytes, int]]]:
    '''Parse dynamic tags. Return array of tuples.'''
    data = section.contents()
    ofs = 0
    result = []
    for ofs in range(0, len(data), section.sh_entsize):
        tag = DynTag(data, ofs, eh, section)
        val = _lookup_string(strings, tag.d_val) if tag.d_tag in STRING_TAGS else tag.d_val
        result.append((tag.d_tag, val))

    return result

class ELFFile:
    sections: List[Section]
    program_headers: List[ProgramHeader]
    dyn_symbols: List[Symbol]
    dyn_tags: List[Tuple[int, Union[bytes, int]]]

    def __init__(self, data: bytes) -> None:
        self.data = data
        self.hdr = ELFHeader(self.data, 0)
        self._load_sections()
        self._load_program_headers()
        self._load_dyn_symbols()
        self._load_dyn_tags()
        self._section_to_segment_mapping()

    def _load_sections(self) -> None:
        self.sections = []
        for idx in range(self.hdr.e_shnum):
            offset = self.hdr.e_shoff + idx * self.hdr.e_shentsize
            self.sections.append(Section(self.data, offset, self.hdr))

        shstr = self.sections[self.hdr.e_shstrndx].contents()
        for section in self.sections:
            section.name = _lookup_string(shstr, section.sh_name)

    def _load_program_headers(self) -> None:
        self.program_headers = []
        for idx in range(self.hdr.e_phnum):
            offset = self.hdr.e_phoff + idx * self.hdr.e_phentsize
            self.program_headers.append(ProgramHeader(self.data, offset, self.hdr))

    def _load_dyn_symbols(self) -> None:
        # first, load 'verneed' section
        verneed = None
        for section in self.sections:
            if section.sh_type == SHT_GNU_verneed:
                strtab = self.sections[section.sh_link].contents() # associated string table
                assert verneed is None # only one section of this kind please
                verneed = _parse_verneed(section, strtab, self.hdr)
        assert verneed is not None

        # then, correlate GNU versym sections with dynamic symbol sections
        versym = {}
        for section in self.sections:
            if section.sh_type == SHT_GNU_versym:
                versym[section.sh_link] = section

        # finally, load dynsym sections
        self.dyn_symbols = []
        for idx, section in enumerate(self.sections):
            if section.sh_type == SHT_DYNSYM: # find dynamic symbol tables
                strtab_data = self.sections[section.sh_link].contents() # associated string table
                versym_data = versym[idx].contents() # associated symbol version table
                self.dyn_symbols += _parse_symbol_table(section, strtab_data, self.hdr, versym_data, verneed)

    def _load_dyn_tags(self) -> None:
        self.dyn_tags = []
        for idx, section in enumerate(self.sections):
            if section.sh_type == SHT_DYNAMIC: # find dynamic tag tables
                strtab = self.sections[section.sh_link].contents() # associated string table
                self.dyn_tags += _parse_dyn_tags(section, strtab, self.hdr)

    def _section_to_segment_mapping(self) -> None:
        for ph in self.program_headers:
            ph.sections = []
            for section in self.sections:
                if ph.p_vaddr <= section.sh_addr < (ph.p_vaddr + ph.p_memsz):
                    ph.sections.append(section)

    def query_dyn_tags(self, tag_in: int) -> List[Union[int, bytes]]:
        '''Return the values of all dyn tags with the specified tag.'''
        return [val for (tag, val) in self.dyn_tags if tag == tag_in]


def load(filename: str) -> ELFFile:
    with open(filename, 'rb') as f:
        data = f.read()
    return ELFFile(data)
