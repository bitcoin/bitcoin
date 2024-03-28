// Copyright (c) 2015-2022 The Bitcoin Core developers
// Copyright (c) 2017 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/readwritefile.h>

#include <streams.h>
#include <util/fs.h>

#include <algorithm>
#include <cstdio>
#include <limits>
#include <string>
#include <utility>
#include <vector>

template <typename T>
std::optional<T> ReadBinaryFile(const fs::path& filename, size_t maxsize)
{
    std::FILE *f = fsbridge::fopen(filename, "rb");
    if (f == nullptr) return {};
    T output{};
    size_t file_size = fs::file_size(filename);
    output.resize(std::min(file_size, maxsize));
    try {
        AutoFile{f} >> Span{output};
    } catch (const std::ios_base::failure&) {
        return {};
    }
    return output;
}

template std::optional<std::string> ReadBinaryFile(const fs::path &filename, size_t maxsize);
template std::optional<std::vector<unsigned char>> ReadBinaryFile(const fs::path &filename, size_t maxsize);

template <typename T>
bool WriteBinaryFile(const fs::path& filename, const T& data)
{
    try {
        AutoFile{fsbridge::fopen(filename, "wb")} << Span{data};
    } catch (const std::ios_base::failure&) {
        return false;
    }
    return true;
}

template bool WriteBinaryFile(const fs::path& filename, const std::string& data);
template bool WriteBinaryFile(const fs::path& filename, const std::vector<unsigned char>& data);
