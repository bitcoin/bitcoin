// Copyright (c) 2015-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_READWRITEFILE_H
#define BITCOIN_UTIL_READWRITEFILE_H

#include <util/fs.h>

#include <limits>
#include <optional>
#include <string>
#include <utility>

/**
 * Read full contents of a file and return one of the following formats:
 * 1. std::vector<unsigned char>
 * 2. std::string
 *
 * @param[in] filename Filename. Returns false it doesn't exist.
 * @param[in] maxsize  Puts a maximum size limit on the file that is read. If the file
 *                 is larger than this, truncated data (with len > maxsize) will be returned.
 * @return result if successful, std::nullopt otherwise
 */
template<class T>
std::optional<T> ReadBinaryFile(const fs::path& filename, size_t maxsize=std::numeric_limits<size_t>::max());

/** Write contents of std::string or std::vector<unsigned char> to a file.
 * @return true on success.
 */
template <class T>
bool WriteBinaryFile(const fs::path& filename, const T& data);

#endif // BITCOIN_UTIL_READWRITEFILE_H
