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

/** Filesystem operations and types */
namespace fs = boost::filesystem;

/** Bridge operations to C stdio */
namespace fsbridge {
    FILE *fopen(const fs::path& p, const char *mode);
    FILE *freopen(const fs::path& p, const char *mode, FILE *stream);

    class Path : public fs::path
    {
    public:
        Path();
        template<typename Source>
        Path(const Source & source) : fs::path(source){}
        template<typename Source>
        Path(const Source & source, const codecvt_type& cvt) : fs::path(source, cvt){}
        Path(const fs::path& p);
        std::string u8string() const;
    };

    Path U8Path(const std::string& source);
};

#endif // BITCOIN_FS_H
