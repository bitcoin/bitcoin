// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_FS_H
#define BITCOIN_FS_H

#include <stdio.h>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>

#ifdef WIN32
#include <ext/stdio_filebuf.h>
#endif

/** Filesystem operations and types */
namespace fs = boost::filesystem;

/** Bridge operations to C stdio */
namespace fsbridge {
    FILE *fopen(const fs::path& p, const char *mode);
    FILE *freopen(const fs::path& p, const char *mode, FILE *stream);
#ifdef WIN32
    class ifstream : public std::istream
    {
    public:
        ifstream(const fs::path& p, std::ios_base::openmode mode = std::ios_base::in);
        ~ifstream();
        void open(const fs::path& p, std::ios_base::openmode mode = std::ios_base::in);
        void close();
        bool is_open();

    private:
        __gnu_cxx::stdio_filebuf<char> filebuf;
        FILE* file = nullptr;
    };
    class ofstream : public std::ostream
    {
    public:
        ofstream(const fs::path& p, std::ios_base::openmode mode = std::ios_base::out);
        ~ofstream();
        void open(const fs::path& p, std::ios_base::openmode mode = std::ios_base::out);
        void close();
        bool is_open();

    private:
        __gnu_cxx::stdio_filebuf<char> filebuf;
        FILE* file = nullptr;
    };
#else
    typedef fs::ifstream ifstream;
    typedef fs::ofstream ofstream;
#endif
};

#endif // BITCOIN_FS_H
