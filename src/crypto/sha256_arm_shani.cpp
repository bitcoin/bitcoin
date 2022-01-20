// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Based on https://github.com/noloader/SHA-Intrinsics/blob/master/sha256-arm.c,
// Written and placed in public domain by Jeffrey Walton.
// Based on code from ARM, and by Johannes Schneiders, Skip Hovsmith and
// Barry O'Rourke for the mbedTLS project.

#ifdef ENABLE_ARM_SHANI

#include <cstdint>
#include <cstddef>

namespace sha256_arm_shani {
void Transform(uint32_t* s, const unsigned char* chunk, size_t blocks)
{

}
}

#endif
