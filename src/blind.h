// Copyright (c) 2017 The Particl developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef KEY_BLIND_H
#define KEY_BLIND_H

#include <secp256k1.h>
#include <inttypes.h>

extern secp256k1_context *secp256k1_ctx_blind;

int SelectRangeProofParameters(uint64_t nValueIn, int &exponent, uint64_t &minValue);

void ECC_Start_Blinding();
void ECC_Stop_Blinding();

#endif  // KEY_BLIND_H
