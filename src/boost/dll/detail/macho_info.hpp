// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright 2015-2021 Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DLL_DETAIL_MACHO_INFO_HPP
#define BOOST_DLL_DETAIL_MACHO_INFO_HPP

#include <boost/dll/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

#include <algorithm>
#include <fstream>
#include <string> // for std::getline
#include <vector>

#include <boost/cstdint.hpp>

namespace boost { namespace dll { namespace detail {

typedef int integer_t;
typedef int vm_prot_t;
typedef integer_t cpu_type_t;
typedef integer_t cpu_subtype_t;

template <class AddressOffsetT>
struct mach_header_template {
    boost::uint32_t     magic;
    cpu_type_t          cputype;
    cpu_subtype_t       cpusubtype;
    boost::uint32_t     filetype;
    boost::uint32_t     ncmds;
    boost::uint32_t     sizeofcmds;
    boost::uint32_t     flags[sizeof(AddressOffsetT) / sizeof(uint32_t)]; // Flags and reserved
};

typedef mach_header_template<boost::uint32_t> mach_header_32_;
typedef mach_header_template<boost::uint64_t> mach_header_64_;

struct load_command_ {
    boost::uint32_t        cmd;        /* type of command */
    boost::uint32_t        cmdsize;
};

struct load_command_types {
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_SEGMENT_          = 0x1);   /* segment of this file to be mapped */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_SYMTAB_           = 0x2);   /* link-edit stab symbol table info */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_SYMSEG_           = 0x3);   /* link-edit gdb symbol table info (obsolete) */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_THREAD_           = 0x4);   /* thread */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_UNIXTHREAD_       = 0x5);   /* unix thread (includes a stack) */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_LOADFVMLIB_       = 0x6);   /* load a specified fixed VM shared library */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_IDFVMLIB_         = 0x7);   /* fixed VM shared library identification */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_IDENT_            = 0x8);   /* object identification info (obsolete) */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_FVMFILE_          = 0x9);   /* fixed VM file inclusion (internal use) */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_PREPAGE_          = 0xa);   /* prepage command (internal use) */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_DYSYMTAB_         = 0xb);   /* dynamic link-edit symbol table info */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_LOAD_DYLIB_       = 0xc);   /* load a dynamically linked shared library */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_ID_DYLIB_         = 0xd);   /* dynamically linked shared lib ident */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_LOAD_DYLINKER_    = 0xe);   /* load a dynamic linker */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_ID_DYLINKER_      = 0xf);   /* dynamic linker identification */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_PREBOUND_DYLIB_   = 0x10);  /* modules prebound for a dynamically linked shared library */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_ROUTINES_         = 0x11);  /* image routines */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_SUB_FRAMEWORK_    = 0x12);  /* sub framework */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_SUB_UMBRELLA_     = 0x13);  /* sub umbrella */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_SUB_CLIENT_       = 0x14);  /* sub client */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_SUB_LIBRARY_      = 0x15);  /* sub library */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_TWOLEVEL_HINTS_   = 0x16);  /* two-level namespace lookup hints */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_PREBIND_CKSUM_    = 0x17);  /* prebind checksum */
/*
 * After MacOS X 10.1 when a new load command is added that is required to be
 * understood by the dynamic linker for the image to execute properly the
 * LC_REQ_DYLD bit will be or'ed into the load command constant.  If the dynamic
 * linker sees such a load command it it does not understand will issue a
 * "unknown load command required for execution" error and refuse to use the
 * image.  Other load commands without this bit that are not understood will
 * simply be ignored.
 */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_REQ_DYLD_         = 0x80000000);

/*
 * load a dynamically linked shared library that is allowed to be missing
 * (all symbols are weak imported).
 */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_LOAD_WEAK_DYLIB_  = (0x18 | LC_REQ_DYLD_));

    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_SEGMENT_64_       = 0x19);                    /* 64-bit segment of this file to be mapped */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_ROUTINES_64_      = 0x1a);                    /* 64-bit image routines */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_UUID_             = 0x1b);                    /* the uuid */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_RPATH_            = (0x1c | LC_REQ_DYLD_));   /* runpath additions */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_CODE_SIGNATURE_   = 0x1d);                    /* local of code signature */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_SEGMENT_SPLIT_INFO_= 0x1e);                   /* local of info to split segments */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_REEXPORT_DYLIB_   = (0x1f | LC_REQ_DYLD_));   /* load and re-export dylib */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_LAZY_LOAD_DYLIB_  = 0x20);                    /* delay load of dylib until first use */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_ENCRYPTION_INFO_  = 0x21);                    /* encrypted segment information */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_DYLD_INFO_        = 0x22);                    /* compressed dyld information */
    BOOST_STATIC_CONSTANT(boost::uint32_t, LC_DYLD_INFO_ONLY_   = (0x22|LC_REQ_DYLD_));     /* compressed dyld information only */
};

template <class AddressOffsetT>
struct segment_command_template {
    boost::uint32_t     cmd;            /* LC_SEGMENT_ */
    boost::uint32_t     cmdsize;        /* includes sizeof section structs */
    char                segname[16];    /* segment name */
    AddressOffsetT      vmaddr;         /* memory address of this segment */
    AddressOffsetT      vmsize;         /* memory size of this segment */
    AddressOffsetT      fileoff;        /* file offset of this segment */
    AddressOffsetT      filesize;       /* amount to map from the file */
    vm_prot_t           maxprot;        /* maximum VM protection */
    vm_prot_t           initprot;       /* initial VM protection */
    boost::uint32_t     nsects;         /* number of sections in segment */
    boost::uint32_t     flags;          /* flags */
};

typedef segment_command_template<boost::uint32_t> segment_command_32_;
typedef segment_command_template<boost::uint64_t> segment_command_64_;

template <class AddressOffsetT>
struct section_template {
    char                sectname[16];   /* name of this section */
    char                segname[16];    /* segment this section goes in */
    AddressOffsetT      addr;           /* memory address of this section */
    AddressOffsetT      size;           /* size in bytes of this section */
    boost::uint32_t     offset;         /* file offset of this section */
    boost::uint32_t     align;          /* section alignment (power of 2) */
    boost::uint32_t     reloff;         /* file offset of relocation entries */
    boost::uint32_t     nreloc;         /* number of relocation entries */
    boost::uint32_t     flags;          /* flags (section type and attributes)*/
    boost::uint32_t     reserved[1 + sizeof(AddressOffsetT) / sizeof(uint32_t)];
};

typedef section_template<boost::uint32_t> section_32_;
typedef section_template<boost::uint64_t> section_64_;

struct symtab_command_ {
    boost::uint32_t    cmd;        /* LC_SYMTAB_ */
    boost::uint32_t    cmdsize;    /* sizeof(struct symtab_command) */
    boost::uint32_t    symoff;     /* symbol table offset */
    boost::uint32_t    nsyms;      /* number of symbol table entries */
    boost::uint32_t    stroff;     /* string table offset */
    boost::uint32_t    strsize;    /* string table size in bytes */
};

template <class AddressOffsetT>
struct nlist_template {
    boost::uint32_t     n_strx;
    boost::uint8_t      n_type;
    boost::uint8_t      n_sect;
    boost::uint16_t     n_desc;
    AddressOffsetT      n_value;
};

typedef nlist_template<boost::uint32_t> nlist_32_;
typedef nlist_template<boost::uint64_t> nlist_64_;

template <class AddressOffsetT>
class macho_info {
    typedef boost::dll::detail::mach_header_template<AddressOffsetT>        header_t;
    typedef boost::dll::detail::load_command_                               load_command_t;
    typedef boost::dll::detail::segment_command_template<AddressOffsetT>    segment_t;
    typedef boost::dll::detail::section_template<AddressOffsetT>            section_t;
    typedef boost::dll::detail::symtab_command_                             symbol_header_t;
    typedef boost::dll::detail::nlist_template<AddressOffsetT>              nlist_t;

    BOOST_STATIC_CONSTANT(boost::uint32_t, SEGMENT_CMD_NUMBER = (sizeof(AddressOffsetT) > 4 ? load_command_types::LC_SEGMENT_64_ : load_command_types::LC_SEGMENT_));

public:
    static bool parsing_supported(std::ifstream& fs) {
        static const uint32_t magic_bytes = (sizeof(AddressOffsetT) <= sizeof(uint32_t) ? 0xfeedface : 0xfeedfacf);

        uint32_t magic;
        fs.seekg(0);
        fs.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        return (magic_bytes == magic);
    }

private:
    template <class T>
    static void read_raw(std::ifstream& fs, T& value, std::size_t size = sizeof(T)) {
        fs.read(reinterpret_cast<char*>(&value), size);
    }

    template <class F>
    static void command_finder(std::ifstream& fs, uint32_t cmd_num, F callback_f) {
        const header_t h = header(fs);
        load_command_t command;
        fs.seekg(sizeof(header_t));
        for (std::size_t i = 0; i < h.ncmds; ++i) {
            const std::ifstream::pos_type pos = fs.tellg();
            read_raw(fs, command);
            if (command.cmd != cmd_num) {
                fs.seekg(pos + static_cast<std::ifstream::pos_type>(command.cmdsize));
                continue;
            }

            fs.seekg(pos);
            callback_f(fs);
            fs.seekg(pos + static_cast<std::ifstream::pos_type>(command.cmdsize));
        }
    }

    struct section_names_gather {
        std::vector<std::string>&       ret;

        void operator()(std::ifstream& fs) const {
            segment_t segment;
            read_raw(fs, segment);

            section_t section;
            ret.reserve(ret.size() + segment.nsects);
            for (std::size_t j = 0; j < segment.nsects; ++j) {
                read_raw(fs, section);
                // `segname` goes right after the `sectname`.
                // Forcing `sectname` to end on '\0'
                section.segname[0] = '\0';
                ret.push_back(section.sectname);
                if (ret.back().empty()) {
                    ret.pop_back(); // Do not show empty names
                }
            }
        }
    };

    struct symbol_names_gather {
        std::vector<std::string>&       ret;
        std::size_t                     section_index;

        void operator()(std::ifstream& fs) const {
            symbol_header_t symbh;
            read_raw(fs, symbh);
            ret.reserve(ret.size() + symbh.nsyms);

            nlist_t symbol;
            std::string symbol_name;
            for (std::size_t j = 0; j < symbh.nsyms; ++j) {
                fs.seekg(symbh.symoff + j * sizeof(nlist_t));
                read_raw(fs, symbol);
                if (!symbol.n_strx) {
                    continue; // Symbol has no name
                }

                if ((symbol.n_type & 0x0e) != 0xe || !symbol.n_sect) {
                    continue; // Symbol has no section
                }

                if (section_index && section_index != symbol.n_sect) {
                    continue; // Not in the required section
                }

                fs.seekg(symbh.stroff + symbol.n_strx);
                std::getline(fs, symbol_name, '\0');
                if (symbol_name.empty()) {
                    continue;
                }

                if (symbol_name[0] == '_') {
                    // Linker adds additional '_' symbol. Could not find official docs for that case.
                    ret.push_back(symbol_name.c_str() + 1);
                } else {
                    ret.push_back(symbol_name);
                }
            }
        }
    };

public:
    static std::vector<std::string> sections(std::ifstream& fs) {
        std::vector<std::string> ret;
        section_names_gather f = { ret };
        command_finder(fs, SEGMENT_CMD_NUMBER, f);
        return ret;
    }

private:
    static header_t header(std::ifstream& fs) {
        header_t h;

        fs.seekg(0);
        read_raw(fs, h);

        return h;
    }

public:
    static std::vector<std::string> symbols(std::ifstream& fs) {
        std::vector<std::string> ret;
        symbol_names_gather f = { ret, 0 };
        command_finder(fs, load_command_types::LC_SYMTAB_, f);
        return ret;
    }

    static std::vector<std::string> symbols(std::ifstream& fs, const char* section_name) {
        // Not very optimal solution
        std::vector<std::string> ret = sections(fs);
        std::vector<std::string>::iterator it = std::find(ret.begin(), ret.end(), section_name);
        if (it == ret.end()) {
            // No section with such name
            ret.clear();
            return ret;
        }

        // section indexes start from 1
        symbol_names_gather f = { ret, static_cast<std::size_t>(1 + (it - ret.begin())) };
        ret.clear();
        command_finder(fs, load_command_types::LC_SYMTAB_, f);
        return ret;
    }
};

typedef macho_info<boost::uint32_t> macho_info32;
typedef macho_info<boost::uint64_t> macho_info64;

}}} // namespace boost::dll::detail

#endif // BOOST_DLL_DETAIL_MACHO_INFO_HPP
