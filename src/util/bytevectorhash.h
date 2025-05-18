// Copyright (c) 2018-2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_UTIL_BYTEVECTORHASH_H
#define TORTOISECOIN_UTIL_BYTEVECTORHASH_H

#include <cstdint>
#include <cstddef>
#include <vector>

/**
 * Implementation of Hash named requirement for types that internally store a byte array. This may
 * be used as the hash function in std::unordered_set or std::unordered_map over such types.
 * Internally, this uses a random instance of SipHash-2-4.
 */
class ByteVectorHash final
{
private:
    uint64_t m_k0, m_k1;

public:
    ByteVectorHash();
    size_t operator()(const std::vector<unsigned char>& input) const;
};

#endif // TORTOISECOIN_UTIL_BYTEVECTORHASH_H
