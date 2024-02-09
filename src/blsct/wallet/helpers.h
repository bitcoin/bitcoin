// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_WALLET_HELPERS_H
#define NAVCOIN_BLSCT_WALLET_HELPERS_H

#include <blsct/arith/mcl/mcl.h>
#include <blsct/public_key.h>
#include <blsct/wallet/address.h>
#include <hash.h>
#include <pubkey.h>

namespace blsct {
uint64_t CalculateViewTag(const MclG1Point& blindingKey, const MclScalar& viewKey);
CKeyID CalculateHashId(const MclG1Point& blindingKey, const MclG1Point& spendingKEy, const MclScalar& viewKey);
MclScalar CalculatePrivateSpendingKey(const MclG1Point& blindingKey, const MclScalar& viewKey, const MclScalar& spendingKey, const int64_t& account, const uint64_t& address);
MclG1Point CalculateNonce(const MclG1Point& blindingKey, const MclScalar& viewKey);
} // namespace blsct

#endif // NAVCOIN_BLSCT_WALLET_HELPERS_H
