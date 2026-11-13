// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_PSBT_H
#define BITCOIN_TEST_FUZZ_UTIL_PSBT_H

#include <cstddef>
#include <cstdint>

size_t PSBTCustomMutator(uint8_t* data, size_t size, size_t max_size, unsigned int seed);

#endif // BITCOIN_TEST_FUZZ_UTIL_PSBT_H
