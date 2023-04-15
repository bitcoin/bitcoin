// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_BUILDING_BLOCK_FIAT_SHAMIR_H
#define NAVCOIN_BLSCT_BUILDING_BLOCK_FIAT_SHAMIR_H

#define GEN_FIAT_SHAMIR_VAR(var, fiat_shamir, retry) \
    Scalar var = fiat_shamir.GetHash(); \
    if (var == 0) goto retry; \
    fiat_shamir << var;

#endif // NAVCOIN_BLSCT_BUILDING_BLOCK_FIAT_SHAMIR_H