// Copyright (c) 2015-2020 The Bitcoin Core developers
// Copyright (c) 2017 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>

#include <limits>
#include <stdio.h>
#include <string>
#include <utility>

std::pair<bool,std::string> ReadBinaryFile(const fs::path &filename, size_t maxsize=std::numeric_limits<size_t>::max())
{
    FILE *f = fsbridge::fopen(filename, "rb");
    if (f == nullptr)
        return std::make_pair(false,"");
    std::string retval;
    char buffer[128];
    do {
        const size_t n = fread(buffer, 1, sizeof(buffer), f);
        // Check for reading errors so we don't return any data if we couldn't
        // read the entire file (or up to maxsize)
        if (ferror(f)) {
            fclose(f);
            return std::make_pair(false,"");
        }
        retval.append(buffer, buffer+n);
    } while (!feof(f) && retval.size() <= maxsize);
    fclose(f);
    return std::make_pair(true,retval);
}

bool WriteBinaryFile(const fs::path &filename, const std::string &data)
{
    FILE *f = fsbridge::fopen(filename, "wb");
    if (f == nullptr)
        return false;
    if (fwrite(data.data(), 1, data.size(), f) != data.size()) {
        fclose(f);
        return false;
    }
    if (fclose(f) != 0) {
        return false;
    }
    return true;
}
