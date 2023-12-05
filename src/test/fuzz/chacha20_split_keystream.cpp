// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/crypto.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

FUZZ_TARGET(chacha20_split_keystream)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    ChaCha20SplitFuzz<false>(provider);
}
