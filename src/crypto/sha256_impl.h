// Copyright (c) 2014-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_SHA256_IMPL_H
#define BITCOIN_CRYPTO_SHA256_IMPL_H

#include <cstddef>
#include <cstdint>

// Hardware-specific SHA-256 implementations, selected at runtime by sha256.cpp.

namespace sha256_sse4
{
void Transform(uint32_t* s, const unsigned char* chunk, size_t blocks);
}

namespace sha256d64_sse41
{
void Transform_4way(unsigned char* out, const unsigned char* in);
}

namespace sha256d64_avx2
{
void Transform_8way(unsigned char* out, const unsigned char* in);
}

namespace sha256d64_x86_shani
{
void Transform_2way(unsigned char* out, const unsigned char* in);
}

namespace sha256_x86_shani
{
void Transform(uint32_t* s, const unsigned char* chunk, size_t blocks);
}

namespace sha256_arm_shani
{
void Transform(uint32_t* s, const unsigned char* chunk, size_t blocks);
}

namespace sha256d64_arm_shani
{
void Transform_2way(unsigned char* out, const unsigned char* in);
}

#endif // BITCOIN_CRYPTO_SHA256_IMPL_H
