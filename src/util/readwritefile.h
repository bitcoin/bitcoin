// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_READWRITEFILE_H
#define BITCOIN_UTIL_READWRITEFILE_H

#include <fs.h>

#include <limits>
#include <string>
#include <utility>

/** Read full contents of a file and return them in a std::string.
 * Returns a pair <status, string>.
 * If an error occurred, status will be false, otherwise status will be true and the data will be returned in string.
 *
 * @param maxsize Puts a maximum size limit on the file that is read. If the file is larger than this, truncated data
 *         (with len > maxsize) will be returned.
 */
std::pair<bool,std::string> ReadBinaryFile(const fs::path &filename, size_t maxsize=std::numeric_limits<size_t>::max());

/** Write contents of std::string to a file.
 * @return true on success.
 */
bool WriteBinaryFile(const fs::path &filename, const std::string &data);

#endif /* BITCOIN_UTIL_READWRITEFILE_H */
