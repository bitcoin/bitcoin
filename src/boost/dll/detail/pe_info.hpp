// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright 2015-2021 Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DLL_DETAIL_WINDOWS_PE_INFO_HPP
#define BOOST_DLL_DETAIL_WINDOWS_PE_INFO_HPP

#include <boost/dll/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

#include <cstring>
#include <fstream>
#include <string> // for std::getline
#include <vector>

#include <boost/assert.hpp>
#include <boost/cstdint.hpp>

namespace boost { namespace dll { namespace detail {

// reference:
// http://www.joachim-bauch.de/tutorials/loading-a-dll-from-memory/
// http://msdn.microsoft.com/en-us/magazine/ms809762.aspx
// http://msdn.microsoft.com/en-us/magazine/cc301808.aspx
//

// Basic Windows typedefs. We can not use <boost/winapi/basic_types.hpp> header
// because that header must be included only on Windows platform
typedef unsigned char BYTE_;
typedef unsigned short WORD_;
typedef boost::uint32_t DWORD_;
typedef boost::int32_t LONG_;
typedef boost::uint32_t ULONG_;
typedef boost::int64_t LONGLONG_;
typedef boost::uint64_t ULONGLONG_;

struct IMAGE_DOS_HEADER_ { // 32/64 independent header
    boost::dll::detail::WORD_   e_magic;        // Magic number
    boost::dll::detail::WORD_   e_cblp;         // Bytes on last page of file
    boost::dll::detail::WORD_   e_cp;           // Pages in file
    boost::dll::detail::WORD_   e_crlc;         // Relocations
    boost::dll::detail::WORD_   e_cparhdr;      // Size of header in paragraphs
    boost::dll::detail::WORD_   e_minalloc;     // Minimum extra paragraphs needed
    boost::dll::detail::WORD_   e_maxalloc;     // Maximum extra paragraphs needed
    boost::dll::detail::WORD_   e_ss;           // Initial (relative) SS value
    boost::dll::detail::WORD_   e_sp;           // Initial SP value
    boost::dll::detail::WORD_   e_csum;         // Checksum
    boost::dll::detail::WORD_   e_ip;           // Initial IP value
    boost::dll::detail::WORD_   e_cs;           // Initial (relative) CS value
    boost::dll::detail::WORD_   e_lfarlc;       // File address of relocation table
    boost::dll::detail::WORD_   e_ovno;         // Overlay number
    boost::dll::detail::WORD_   e_res[4];       // Reserved words
    boost::dll::detail::WORD_   e_oemid;        // OEM identifier (for e_oeminfo)
    boost::dll::detail::WORD_   e_oeminfo;      // OEM information; e_oemid specific
    boost::dll::detail::WORD_   e_res2[10];     // Reserved words
    boost::dll::detail::LONG_   e_lfanew;       // File address of new exe header
};

struct IMAGE_FILE_HEADER_ { // 32/64 independent header
    boost::dll::detail::WORD_   Machine;
    boost::dll::detail::WORD_   NumberOfSections;
    boost::dll::detail::DWORD_  TimeDateStamp;
    boost::dll::detail::DWORD_  PointerToSymbolTable;
    boost::dll::detail::DWORD_  NumberOfSymbols;
    boost::dll::detail::WORD_   SizeOfOptionalHeader;
    boost::dll::detail::WORD_   Characteristics;
};

struct IMAGE_DATA_DIRECTORY_ { // 32/64 independent header
    boost::dll::detail::DWORD_  VirtualAddress;
    boost::dll::detail::DWORD_  Size;
};

struct IMAGE_EXPORT_DIRECTORY_ { // 32/64 independent header
    boost::dll::detail::DWORD_  Characteristics;
    boost::dll::detail::DWORD_  TimeDateStamp;
    boost::dll::detail::WORD_   MajorVersion;
    boost::dll::detail::WORD_   MinorVersion;
    boost::dll::detail::DWORD_  Name;
    boost::dll::detail::DWORD_  Base;
    boost::dll::detail::DWORD_  NumberOfFunctions;
    boost::dll::detail::DWORD_  NumberOfNames;
    boost::dll::detail::DWORD_  AddressOfFunctions;
    boost::dll::detail::DWORD_  AddressOfNames;
    boost::dll::detail::DWORD_  AddressOfNameOrdinals;
};

struct IMAGE_SECTION_HEADER_ { // 32/64 independent header
    static const std::size_t    IMAGE_SIZEOF_SHORT_NAME_ = 8;

    boost::dll::detail::BYTE_   Name[IMAGE_SIZEOF_SHORT_NAME_];
    union {
        boost::dll::detail::DWORD_   PhysicalAddress;
        boost::dll::detail::DWORD_   VirtualSize;
    } Misc;
    boost::dll::detail::DWORD_  VirtualAddress;
    boost::dll::detail::DWORD_  SizeOfRawData;
    boost::dll::detail::DWORD_  PointerToRawData;
    boost::dll::detail::DWORD_  PointerToRelocations;
    boost::dll::detail::DWORD_  PointerToLinenumbers;
    boost::dll::detail::WORD_   NumberOfRelocations;
    boost::dll::detail::WORD_   NumberOfLinenumbers;
    boost::dll::detail::DWORD_  Characteristics;
};


template <class AddressOffsetT>
struct IMAGE_OPTIONAL_HEADER_template {
    static const std::size_t IMAGE_NUMBEROF_DIRECTORY_ENTRIES_ = 16;

    boost::dll::detail::WORD_   Magic;
    boost::dll::detail::BYTE_   MajorLinkerVersion;
    boost::dll::detail::BYTE_   MinorLinkerVersion;
    boost::dll::detail::DWORD_  SizeOfCode;
    boost::dll::detail::DWORD_  SizeOfInitializedData;
    boost::dll::detail::DWORD_  SizeOfUninitializedData;
    boost::dll::detail::DWORD_  AddressOfEntryPoint;
    union {
        boost::dll::detail::DWORD_   BaseOfCode;
        unsigned char padding_[sizeof(AddressOffsetT) == 8 ? 4 : 8]; // in x64 version BaseOfData does not exist
    } BaseOfCode_and_BaseOfData;

    AddressOffsetT              ImageBase;
    boost::dll::detail::DWORD_  SectionAlignment;
    boost::dll::detail::DWORD_  FileAlignment;
    boost::dll::detail::WORD_   MajorOperatingSystemVersion;
    boost::dll::detail::WORD_   MinorOperatingSystemVersion;
    boost::dll::detail::WORD_   MajorImageVersion;
    boost::dll::detail::WORD_   MinorImageVersion;
    boost::dll::detail::WORD_   MajorSubsystemVersion;
    boost::dll::detail::WORD_   MinorSubsystemVersion;
    boost::dll::detail::DWORD_  Win32VersionValue;
    boost::dll::detail::DWORD_  SizeOfImage;
    boost::dll::detail::DWORD_  SizeOfHeaders;
    boost::dll::detail::DWORD_  CheckSum;
    boost::dll::detail::WORD_   Subsystem;
    boost::dll::detail::WORD_   DllCharacteristics;
    AddressOffsetT              SizeOfStackReserve;
    AddressOffsetT              SizeOfStackCommit;
    AddressOffsetT              SizeOfHeapReserve;
    AddressOffsetT              SizeOfHeapCommit;
    boost::dll::detail::DWORD_  LoaderFlags;
    boost::dll::detail::DWORD_  NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY_       DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES_];
};

typedef IMAGE_OPTIONAL_HEADER_template<boost::dll::detail::DWORD_>      IMAGE_OPTIONAL_HEADER32_;
typedef IMAGE_OPTIONAL_HEADER_template<boost::dll::detail::ULONGLONG_>  IMAGE_OPTIONAL_HEADER64_;

template <class AddressOffsetT>
struct IMAGE_NT_HEADERS_template {
    boost::dll::detail::DWORD_                      Signature;
    IMAGE_FILE_HEADER_                              FileHeader;
    IMAGE_OPTIONAL_HEADER_template<AddressOffsetT>  OptionalHeader;
};

typedef IMAGE_NT_HEADERS_template<boost::dll::detail::DWORD_>      IMAGE_NT_HEADERS32_;
typedef IMAGE_NT_HEADERS_template<boost::dll::detail::ULONGLONG_>  IMAGE_NT_HEADERS64_;


template <class AddressOffsetT>
class pe_info {
    typedef IMAGE_NT_HEADERS_template<AddressOffsetT>   header_t;
    typedef IMAGE_EXPORT_DIRECTORY_                     exports_t;
    typedef IMAGE_SECTION_HEADER_                       section_t;
    typedef IMAGE_DOS_HEADER_                           dos_t;

    template <class T>
    static void read_raw(std::ifstream& fs, T& value, std::size_t size = sizeof(T)) {
        fs.read(reinterpret_cast<char*>(&value), size);
    }

public:
    static bool parsing_supported(std::ifstream& fs) {
        dos_t dos;
        fs.seekg(0);
        fs.read(reinterpret_cast<char*>(&dos), sizeof(dos));

        // 'MZ' and 'ZM' according to Wikipedia
        if (dos.e_magic != 0x4D5A && dos.e_magic != 0x5A4D) {
            return false;
        }

        header_t h;
        fs.seekg(dos.e_lfanew);
        fs.read(reinterpret_cast<char*>(&h), sizeof(h));

        return h.Signature == 0x00004550 // 'PE00'
                && h.OptionalHeader.Magic == (sizeof(boost::uint32_t) == sizeof(AddressOffsetT) ? 0x10B : 0x20B);
    }

private:
    static header_t header(std::ifstream& fs) {
        header_t h;

        dos_t dos;
        fs.seekg(0);
        read_raw(fs, dos);

        fs.seekg(dos.e_lfanew);
        read_raw(fs, h);

        return h;
    }

    static exports_t exports(std::ifstream& fs, const header_t& h) {
        static const unsigned int IMAGE_DIRECTORY_ENTRY_EXPORT_ = 0;
        const std::size_t exp_virtual_address = h.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT_].VirtualAddress;
        exports_t exports;

        if (exp_virtual_address == 0) {
            // The virtual address can be 0 in case there are no exported symbols
            std::memset(&exports, 0, sizeof(exports));
            return exports;
        }

        const std::size_t real_offset = get_file_offset(fs, exp_virtual_address, h);
        BOOST_ASSERT(real_offset);

        fs.seekg(real_offset);
        read_raw(fs, exports);

        return exports;
    }

    static std::size_t get_file_offset(std::ifstream& fs, std::size_t virtual_address, const header_t& h) {
        BOOST_ASSERT(virtual_address);

        section_t image_section_header;
        
        {   // fs.seekg to the beginning on section headers
            dos_t dos;
            fs.seekg(0);
            read_raw(fs, dos);
            fs.seekg(dos.e_lfanew + sizeof(header_t));
        }

        for (std::size_t i = 0;i < h.FileHeader.NumberOfSections;++i) {
            read_raw(fs, image_section_header);
            if (virtual_address >= image_section_header.VirtualAddress 
                && virtual_address < image_section_header.VirtualAddress + image_section_header.SizeOfRawData) 
            {
                return image_section_header.PointerToRawData + virtual_address - image_section_header.VirtualAddress;
            }
        }

        return 0;
    }

public:
    static std::vector<std::string> sections(std::ifstream& fs) {
        std::vector<std::string> ret;

        const header_t h = header(fs);
        ret.reserve(h.FileHeader.NumberOfSections);

        // get names, e.g: .text .rdata .data .rsrc .reloc
        section_t image_section_header;
        char name_helper[section_t::IMAGE_SIZEOF_SHORT_NAME_ + 1];
        std::memset(name_helper, 0, sizeof(name_helper));
        for (std::size_t i = 0;i < h.FileHeader.NumberOfSections;++i) {
            // There is no terminating null character if the string is exactly eight characters long
            read_raw(fs, image_section_header);
            std::memcpy(name_helper, image_section_header.Name, section_t::IMAGE_SIZEOF_SHORT_NAME_);
            
            if (name_helper[0] != '/') {
                ret.push_back(name_helper);
            } else {
                // For longer names, image_section_header.Name contains a slash (/) followed by ASCII representation of a decimal number.
                // this number is an offset into the string table.
                // TODO: fixme
                ret.push_back(name_helper);
            }
        }

        return ret;
    }

    static std::vector<std::string> symbols(std::ifstream& fs) {
        std::vector<std::string> ret;

        const header_t h = header(fs);
        const exports_t exprt = exports(fs, h);
        const std::size_t exported_symbols = exprt.NumberOfNames;

        if (exported_symbols == 0) {
            return ret;
        }

        const std::size_t fixed_names_addr = get_file_offset(fs, exprt.AddressOfNames, h);

        ret.reserve(exported_symbols);
        boost::dll::detail::DWORD_ name_offset;
        std::string symbol_name;
        for (std::size_t i = 0;i < exported_symbols;++i) {
            fs.seekg(fixed_names_addr + i * sizeof(name_offset));
            read_raw(fs, name_offset);
            fs.seekg(get_file_offset(fs, name_offset, h));
            std::getline(fs, symbol_name, '\0');
            ret.push_back(symbol_name);
        }

        return ret;
    }

    static std::vector<std::string> symbols(std::ifstream& fs, const char* section_name) {
        std::vector<std::string> ret;

        const header_t h = header(fs);
        
        std::size_t section_begin_addr = 0;
        std::size_t section_end_addr = 0;
        
        {   // getting address range for the section
            section_t image_section_header;
            char name_helper[section_t::IMAGE_SIZEOF_SHORT_NAME_ + 1];
            std::memset(name_helper, 0, sizeof(name_helper));
            for (std::size_t i = 0;i < h.FileHeader.NumberOfSections;++i) {
                // There is no terminating null character if the string is exactly eight characters long
                read_raw(fs, image_section_header);
                std::memcpy(name_helper, image_section_header.Name, section_t::IMAGE_SIZEOF_SHORT_NAME_);
                if (!std::strcmp(section_name, name_helper)) {
                    section_begin_addr = image_section_header.PointerToRawData;
                    section_end_addr = section_begin_addr + image_section_header.SizeOfRawData;
                }
            }
            
            // returning empty result if section was not found
            if(section_begin_addr == 0 || section_end_addr == 0)
                return ret;
        }

        const exports_t exprt = exports(fs, h);
        const std::size_t exported_symbols = exprt.NumberOfFunctions;
        const std::size_t fixed_names_addr = get_file_offset(fs, exprt.AddressOfNames, h);
        const std::size_t fixed_ordinals_addr = get_file_offset(fs, exprt.AddressOfNameOrdinals, h);
        const std::size_t fixed_functions_addr = get_file_offset(fs, exprt.AddressOfFunctions, h);

        ret.reserve(exported_symbols);
        boost::dll::detail::DWORD_ ptr;
        boost::dll::detail::WORD_ ordinal;
        std::string symbol_name;
        for (std::size_t i = 0;i < exported_symbols;++i) {
            // getting ordinal
            fs.seekg(fixed_ordinals_addr + i * sizeof(ordinal));
            read_raw(fs, ordinal);

            // getting function addr
            fs.seekg(fixed_functions_addr + ordinal * sizeof(ptr));
            read_raw(fs, ptr);
            ptr = static_cast<boost::dll::detail::DWORD_>( get_file_offset(fs, ptr, h) );

            if (ptr >= section_end_addr || ptr < section_begin_addr) {
                continue;
            }

            fs.seekg(fixed_names_addr + i * sizeof(ptr));
            read_raw(fs, ptr);
            fs.seekg(get_file_offset(fs, ptr, h));
            std::getline(fs, symbol_name, '\0');
            ret.push_back(symbol_name);
        }

        return ret;
    }
    
    // a test method to get dependents modules,
    // who my plugin imports (1st level only)
    /*
    e.g. for myself I get:
      KERNEL32.dll
      MSVCP110D.dll
      boost_system-vc-mt-gd-1_56.dll
      MSVCR110D.dll
    */
    /*
    static std::vector<std::string> depend_of(boost::dll::fs::error_code &ec) BOOST_NOEXCEPT {
        std::vector<std::string> ret;

        IMAGE_DOS_HEADER* image_dos_header = (IMAGE_DOS_HEADER*)native();
        if(!image_dos_header) {
            // ERROR_BAD_EXE_FORMAT 
            ec = boost::dll::fs::make_error_code(
                 boost::dll::fs::errc::executable_format_error
                 );

            return ret;
        }

        IMAGE_OPTIONAL_HEADER* image_optional_header = (IMAGE_OPTIONAL_HEADER*)((boost::dll::detail::BYTE_*)native() + image_dos_header->e_lfanew + 24);
        if(!image_optional_header) {
            // ERROR_BAD_EXE_FORMAT 
            ec = boost::dll::fs::make_error_code(
                 boost::dll::fs::errc::executable_format_error
                 );

            return ret;
        }

        IMAGE_IMPORT_DESCRIPTOR* image_import_descriptor =  (IMAGE_IMPORT_DESCRIPTOR*)((boost::dll::detail::BYTE_*)native() + image_optional_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        if(!image_import_descriptor) {
            // ERROR_BAD_EXE_FORMAT 
            ec = boost::dll::fs::make_error_code(
                 boost::dll::fs::errc::executable_format_error
                 );

            return ret;
        }

        while(image_import_descriptor->FirstThunk) {
           std::string module_name = reinterpret_cast<char*>((boost::dll::detail::BYTE_*)native() + image_import_descriptor->Name);

           if(module_name.size()) {
              ret.push_back(module_name);
           }
                
           image_import_descriptor++;
        }

        return ret;
    }
*/
};

typedef pe_info<boost::dll::detail::DWORD_>      pe_info32;
typedef pe_info<boost::dll::detail::ULONGLONG_>  pe_info64;

}}} // namespace boost::dll::detail

#endif // BOOST_DLL_DETAIL_WINDOWS_PE_INFO_HPP
