// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_GETUNIQUEPATH_H
#define BITCOIN_UTIL_GETUNIQUEPATH_H

#include <fs.h>

/**
 * Helper function for getting a unique path
 *
 * @param[in] base  Base path
 * @returns base joined with a random 8-character long string.
 * @post Returned path is unique with high probability.
 */
fs::path GetUniquePath(const fs::path& base);

#endif // BITCOIN_UTIL_GETUNIQUEPATH_H