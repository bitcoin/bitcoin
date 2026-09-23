// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_HEADERSSYNC_H
#define BITCOIN_TEST_FUZZ_UTIL_HEADERSSYNC_H

#include <cstddef>
#include <utility>

/** Fixed headers-sync parameters for fuzz targets that construct a PeerManager per iteration,
 *  avoiding both recomputing them every time and any dependence on the (mocked) clock at
 *  construction time, as {commitment period, redownload buffer size}. The values are the regtest
 *  constants that were committed in chainparams before the parameters became runtime-computed,
 *  but any sane values would do. */
inline constexpr std::pair<size_t, size_t> FUZZ_HEADERS_SYNC_PARAMS{275, 7017};

#endif // BITCOIN_TEST_FUZZ_UTIL_HEADERSSYNC_H
