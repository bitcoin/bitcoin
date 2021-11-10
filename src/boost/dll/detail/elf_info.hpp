// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright 2015-2021 Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DLL_DETAIL_POSIX_ELF_INFO_HPP
#define BOOST_DLL_DETAIL_POSIX_ELF_INFO_HPP

#include <boost/dll/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

#include <cstring>
#include <fstream>
#include <limits>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/throw_exception.hpp>

namespace boost { namespace dll { namespace detail {

template <class AddressOffsetT>
struct Elf_Ehdr_template {
  unsigned char     e_ident[16];    /* Magic number and other info */
  boost::uint16_t   e_type;         /* Object file type */
  boost::uint16_t   e_machine;      /* Architecture */
  boost::uint32_t   e_version;      /* Object file version */
  AddressOffsetT    e_entry;        /* Entry point virtual address */
  AddressOffsetT    e_phoff;        /* Program header table file offset */
  AddressOffsetT    e_shoff;        /* Section header table file offset */
  boost::uint32_t   e_flags;        /* Processor-specific flags */
  boost::uint16_t   e_ehsize;       /* ELF header size in bytes */
  boost::uint16_t   e_phentsize;    /* Program header table entry size */
  boost::uint16_t   e_phnum;        /* Program header table entry count */
  boost::uint16_t   e_shentsize;    /* Section header table entry size */
  boost::uint16_t   e_shnum;        /* Section header table entry count */
  boost::uint16_t   e_shstrndx;     /* Section header string table index */
};

typedef Elf_Ehdr_template<boost::uint32_t> Elf32_Ehdr_;
typedef Elf_Ehdr_template<boost::uint64_t> Elf64_Ehdr_;

template <class AddressOffsetT>
struct Elf_Shdr_template {
  boost::uint32_t   sh_name;        /* Section name (string tbl index) */
  boost::uint32_t   sh_type;        /* Section type */
  AddressOffsetT    sh_flags;       /* Section flags */
  AddressOffsetT    sh_addr;        /* Section virtual addr at execution */
  AddressOffsetT    sh_offset;      /* Section file offset */
  AddressOffsetT    sh_size;        /* Section size in bytes */
  boost::uint32_t   sh_link;        /* Link to another section */
  boost::uint32_t   sh_info;        /* Additional section information */
  AddressOffsetT    sh_addralign;   /* Section alignment */
  AddressOffsetT    sh_entsize;     /* Entry size if section holds table */
};

typedef Elf_Shdr_template<boost::uint32_t> Elf32_Shdr_;
typedef Elf_Shdr_template<boost::uint64_t> Elf64_Shdr_;

template <class AddressOffsetT>
struct Elf_Sym_template;

template <>
struct Elf_Sym_template<boost::uint32_t> {
  typedef boost::uint32_t AddressOffsetT;

  boost::uint32_t   st_name;    /* Symbol name (string tbl index) */
  AddressOffsetT    st_value;   /* Symbol value */
  AddressOffsetT    st_size;    /* Symbol size */
  unsigned char     st_info;    /* Symbol type and binding */
  unsigned char     st_other;   /* Symbol visibility */
  boost::uint16_t   st_shndx;   /* Section index */
};

template <>
struct Elf_Sym_template<boost::uint64_t> {
  typedef boost::uint64_t AddressOffsetT;

  boost::uint32_t   st_name;    /* Symbol name (string tbl index) */
  unsigned char     st_info;    /* Symbol type and binding */
  unsigned char     st_other;   /* Symbol visibility */
  boost::uint16_t   st_shndx;   /* Section index */
  AddressOffsetT    st_value;   /* Symbol value */
  AddressOffsetT    st_size;    /* Symbol size */
};


typedef Elf_Sym_template<boost::uint32_t> Elf32_Sym_;
typedef Elf_Sym_template<boost::uint64_t> Elf64_Sym_;

template <class AddressOffsetT>
class elf_info {
    typedef boost::dll::detail::Elf_Ehdr_template<AddressOffsetT>  header_t;
    typedef boost::dll::detail::Elf_Shdr_template<AddressOffsetT>  section_t;
    typedef boost::dll::detail::Elf_Sym_template<AddressOffsetT>   symbol_t;

    BOOST_STATIC_CONSTANT(boost::uint32_t, SHT_SYMTAB_ = 2);
    BOOST_STATIC_CONSTANT(boost::uint32_t, SHT_STRTAB_ = 3);
    BOOST_STATIC_CONSTANT(boost::uint32_t, SHT_DYNSYM_ = 11);

    BOOST_STATIC_CONSTANT(unsigned char, STB_LOCAL_ = 0);   /* Local symbol */
    BOOST_STATIC_CONSTANT(unsigned char, STB_GLOBAL_ = 1);  /* Global symbol */
    BOOST_STATIC_CONSTANT(unsigned char, STB_WEAK_ = 2);    /* Weak symbol */

    /* Symbol visibility specification encoded in the st_other field.  */
    BOOST_STATIC_CONSTANT(unsigned char, STV_DEFAULT_ = 0);      /* Default symbol visibility rules */
    BOOST_STATIC_CONSTANT(unsigned char, STV_INTERNAL_ = 1);     /* Processor specific hidden class */
    BOOST_STATIC_CONSTANT(unsigned char, STV_HIDDEN_ = 2);       /* Sym unavailable in other modules */
    BOOST_STATIC_CONSTANT(unsigned char, STV_PROTECTED_ = 3);    /* Not preemptible, not exported */

public:
    static bool parsing_supported(std::ifstream& fs) {
        const unsigned char magic_bytes[5] = { 
            0x7f, 'E', 'L', 'F', sizeof(boost::uint32_t) == sizeof(AddressOffsetT) ? 1 : 2
        };

        unsigned char ch;
        fs.seekg(0);
        for (std::size_t i = 0; i < sizeof(magic_bytes); ++i) {
            fs >> ch;
            if (ch != magic_bytes[i]) {
                return false;
            }
        }

        return true;
    }

    static std::vector<std::string> sections(std::ifstream& fs) {
        std::vector<std::string> ret;
        std::vector<char> names;
        sections_names_raw(fs, names);

        const char* name_begin = &names[0];
        const char* const name_end = name_begin + names.size();
        ret.reserve(header(fs).e_shnum);
        do {
            if (*name_begin) {
                ret.push_back(name_begin);
                name_begin += ret.back().size() + 1;
            } else {
                ++name_begin;
            }
        } while (name_begin != name_end);

        return ret;
    }

private:
    template <class Integer>
    static void checked_seekg(std::ifstream& fs, Integer pos) {
        /* TODO: use cmp_less, cmp_greater
        if ((std::numeric_limits<std::streamoff>::max)() < pos) {
            boost::throw_exception(std::runtime_error("Integral overflow while getting info from ELF file"));
        }
        if ((std::numeric_limits<std::streamoff>::min)() > pos){
            boost::throw_exception(std::runtime_error("Integral underflow while getting info from ELF file"));
        }
        */
        fs.seekg(static_cast<std::streamoff>(pos));
    }

    template <class T>
    static void read_raw(std::ifstream& fs, T& value, std::size_t size = sizeof(T)) {
        fs.read(reinterpret_cast<char*>(&value), size);
    }

    static header_t header(std::ifstream& fs) {
        header_t elf;

        fs.seekg(0);
        read_raw(fs, elf);

        return elf;
    }

    static void sections_names_raw(std::ifstream& fs, std::vector<char>& sections) {
        const header_t elf = header(fs);

        section_t section_names_section;
        checked_seekg(fs, elf.e_shoff + elf.e_shstrndx * sizeof(section_t));
        read_raw(fs, section_names_section);

        sections.resize(static_cast<std::size_t>(section_names_section.sh_size) + 1, '\0');
        checked_seekg(fs, section_names_section.sh_offset);
        read_raw(fs, sections[0], static_cast<std::size_t>(section_names_section.sh_size));
    }

    static void symbols_text(std::ifstream& fs, std::vector<symbol_t>& symbols, std::vector<char>& text) {
        std::vector<char> names;
        sections_names_raw(fs, names);
        symbols_text(fs, symbols, text, names);
    }

    static void symbols_text(std::ifstream& fs, std::vector<symbol_t>& symbols, std::vector<char>& text, const std::vector<char>& names) {
        const header_t elf = header(fs);
        checked_seekg(fs, elf.e_shoff);

        // ".dynsym" section may not have info on symbols that could be used while self loading an executable,
        // so we prefer ".symtab" section.
        AddressOffsetT symtab_size = 0;
        AddressOffsetT symtab_offset = 0;
        AddressOffsetT strtab_size = 0;
        AddressOffsetT strtab_offset = 0;

        AddressOffsetT dynsym_size = 0;
        AddressOffsetT dynsym_offset = 0;
        AddressOffsetT dynstr_size = 0;
        AddressOffsetT dynstr_offset = 0;

        for (std::size_t i = 0; i < elf.e_shnum; ++i) {
            section_t section;
            read_raw(fs, section);
            if (section.sh_name >= names.size()) {
                continue;
            }
            const char* name = &names[section.sh_name];

            if (section.sh_type == SHT_SYMTAB_ && !std::strcmp(name, ".symtab")) {
                symtab_size = section.sh_size;
                symtab_offset = section.sh_offset;
            } else if (section.sh_type == SHT_STRTAB_) {
                if (!std::strcmp(name, ".dynstr")) {
                    dynstr_size = section.sh_size;
                    dynstr_offset = section.sh_offset;
                } else if (!std::strcmp(name, ".strtab")) {
                    strtab_size = section.sh_size;
                    strtab_offset = section.sh_offset;
                }
            } else if (section.sh_type == SHT_DYNSYM_ && !std::strcmp(name, ".dynsym")) {
                dynsym_size = section.sh_size;
                dynsym_offset = section.sh_offset;
            }
        }

        if (!symtab_size || !strtab_size) {
            // ".symtab" stripped from the binary and we have to fallback to ".dynsym"
            symtab_size = dynsym_size;
            symtab_offset = dynsym_offset;
            strtab_size = dynstr_size;
            strtab_offset = dynstr_offset;
        }

        if (!symtab_size || !strtab_size) {
            return;
        }

        text.resize(static_cast<std::size_t>(strtab_size) + 1, '\0');
        checked_seekg(fs, strtab_offset);
        read_raw(fs, text[0], static_cast<std::size_t>(strtab_size));

        symbols.resize(static_cast<std::size_t>(symtab_size / sizeof(symbol_t)));
        checked_seekg(fs, symtab_offset);
        read_raw(fs, symbols[0], static_cast<std::size_t>(symtab_size - (symtab_size % sizeof(symbol_t))) );
    }

    static bool is_visible(const symbol_t& sym) BOOST_NOEXCEPT {
        const unsigned char visibility = (sym.st_other & 0x03);
        // `(sym.st_info >> 4) != STB_LOCAL_ && !!sym.st_size` check also workarounds the
        // GCC's issue https://sourceware.org/bugzilla/show_bug.cgi?id=13621
        return (visibility == STV_DEFAULT_ || visibility == STV_PROTECTED_)
                && (sym.st_info >> 4) != STB_LOCAL_ && !!sym.st_size;
    }

public:
    static std::vector<std::string> symbols(std::ifstream& fs) {
        std::vector<std::string> ret;

        std::vector<symbol_t> symbols;
        std::vector<char>   text;
        symbols_text(fs, symbols, text);

        ret.reserve(symbols.size());
        for (std::size_t i = 0; i < symbols.size(); ++i) {
            if (is_visible(symbols[i]) && symbols[i].st_name < text.size()) {
                ret.push_back(&text[symbols[i].st_name]);
                if (ret.back().empty()) {
                    ret.pop_back(); // Do not show empty names
                }
            }
        }

        return ret;
    }

    static std::vector<std::string> symbols(std::ifstream& fs, const char* section_name) {
        std::vector<std::string> ret;
        
        std::size_t index = 0;
        std::size_t ptrs_in_section_count = 0;

        std::vector<char> names;
        sections_names_raw(fs, names);

        const header_t elf = header(fs);

        for (; index < elf.e_shnum; ++index) {
            section_t section;
            checked_seekg(fs, elf.e_shoff + index * sizeof(section_t));
            read_raw(fs, section);

            if (!std::strcmp(&names.at(section.sh_name), section_name)) {
                if (!section.sh_entsize) {
                    section.sh_entsize = 1;
                }
                ptrs_in_section_count = static_cast<std::size_t>(section.sh_size / section.sh_entsize);
                break;
            }
        }

        std::vector<symbol_t> symbols;
        std::vector<char> text;
        symbols_text(fs, symbols, text, names);
    
        if (ptrs_in_section_count < symbols.size()) {
            ret.reserve(ptrs_in_section_count);
        } else {
            ret.reserve(symbols.size());
        }

        for (std::size_t i = 0; i < symbols.size(); ++i) {
            if (symbols[i].st_shndx == index && is_visible(symbols[i]) && symbols[i].st_name < text.size()) {
                ret.push_back(&text[symbols[i].st_name]);
                if (ret.back().empty()) {
                    ret.pop_back(); // Do not show empty names
                }
            }
        }

        return ret;
    }
};

typedef elf_info<boost::uint32_t> elf_info32;
typedef elf_info<boost::uint64_t> elf_info64;

}}} // namespace boost::dll::detail

#endif // BOOST_DLL_DETAIL_POSIX_ELF_INFO_HPP
