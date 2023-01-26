// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/pkcs5_pbkdf2.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <valarray>
#include <vector>

#include <crypto/hmac_sha512.h>

// PBKDF2
//
// Options:    PRF          underlying pseudorandom function
//                          ==> HMAC-SHA512
//             hLen         length in octets of the pseudorandom function output
//                          ==> 64
//
// Input:      P            password, an octet string
//             S            salt, an octet string
//             c            iteration count, a positive integer
//             dkLen        intended length in octets of the derived key,
//                          a positive integer, at most (2^32-1) * hLen
//                          ==> 64
//
// Output:     DK           derived key, a dkLen-octet string
//

std::vector<uint8_t> pkcs5_pbkdf2_hmacsha512(const std::vector<uint8_t> P, const std::vector<uint8_t> S, const int c)
{
    constexpr int digest_size = 64;
    constexpr int block_index = 1;
    std::vector<uint8_t> salt (S);
    std::vector<uint8_t> digest1 (digest_size), digest2 (digest_size), buffer (digest_size);

    assert (c);

    // salt = S | INT (i)
    salt.push_back( (block_index >> 24) & 0xff );
    salt.push_back( (block_index >> 16) & 0xff );
    salt.push_back( (block_index >> 8) & 0xff );
    salt.push_back( (block_index >> 0) & 0xff );

    {
        CHMAC_SHA512 hmac (P.data(), P.size());
        hmac.Write(salt.data(), salt.size());
        hmac.Finalize(digest1.data());

        buffer.assign(digest1.begin(), digest1.end());
    }

    for (int iteration = 1; iteration < c; ++iteration)
    {
        CHMAC_SHA512 hmac (P.data(), P.size());   // key
        hmac.Write(digest1.data(), digest1.size());    // data
        hmac.Finalize(digest2.data());

        digest1.assign(digest2.begin(), digest2.end());

        std::transform(buffer.begin(), buffer.end(),
                       digest1.begin(),
                       buffer.begin(),
                       std::bit_xor<uint8_t>());
    }

    std::fill(digest1.begin(), digest1.end(), 0);
    std::fill(digest2.begin(), digest2.end(), 0);
    std::fill(salt.begin(), salt.end(), 0);

    assert (buffer.size() == 64);
    return buffer;
}
