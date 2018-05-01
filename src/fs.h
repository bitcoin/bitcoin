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

#include <utilstrencodings.h>

/** Filesystem operations and types */
namespace fs = boost::filesystem;

/** Bridge operations to C stdio */
namespace fsbridge {
    FILE *fopen(const fs::path& p, const char *mode);
    FILE *freopen(const fs::path& p, const char *mode, FILE *stream);

    class ifstream : public std::ifstream
    {
    public:
        ifstream() {}

        virtual ~ifstream() {}

        explicit ifstream(const fs::path& p) : std::ifstream(Utf8ToLocal(p.string())) {}

        ifstream(const fs::path& p, std::ios::openmode mode) : std::ifstream(Utf8ToLocal(p.string()), mode) {}

        void open(const fs::path& p)
        {
            std::ifstream::open(Utf8ToLocal(p.string()));
        }

        void open(const fs::path& p, std::ios::openmode mode)
        {
            std::ifstream::open(Utf8ToLocal(p.string()), mode);
        }

    private:
        ifstream(const ifstream&) = delete;
        const ifstream& operator=(const ifstream&) = delete;
    };

    class ofstream : public std::ofstream
    {
    public:
        ofstream() {}

        virtual ~ofstream() {}

        explicit ofstream(const fs::path& p) : std::ofstream(Utf8ToLocal(p.string())) {}

        ofstream(const fs::path& p, std::ios::openmode mode) : std::ofstream(Utf8ToLocal(p.string()), mode) {}

        void open(const fs::path& p)
        {
            std::ofstream::open(Utf8ToLocal(p.string()));
        }

        void open(const fs::path& p, std::ios::openmode mode)
        {
            std::ofstream::open(Utf8ToLocal(p.string()), mode);
        }

    private:
        ofstream(const ofstream&) = delete;
        const ofstream& operator=(const ofstream&) = delete;
    };
};

#endif // BITCOIN_FS_H
