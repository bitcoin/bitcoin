// Copyright (c) 2023 The Navio developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVIO_BLSCT_WALLET_HELPERS_H
#define NAVIO_BLSCT_WALLET_HELPERS_H

#include <blsct/arith/mcl/mcl.h>
#include <blsct/public_key.h>
#include <blsct/wallet/address.h>
#include <hash.h>
#include <pubkey.h>

namespace blsct {
uint64_t CalculateViewTag(const MclG1Point& blindingKey, const MclScalar& viewKey);
CKeyID CalculateHashId(const MclG1Point& blindingKey, const MclG1Point& spendingKey, const MclScalar& viewKey);
MclScalar CalculatePrivateSpendingKey(const MclG1Point& blindingKey, const MclScalar& viewKey, const MclScalar& spendingKey, const int64_t& account, const uint64_t& address);
MclG1Point CalculateNonce(const MclG1Point& blindingKey, const MclScalar& viewKey);
SubAddress DeriveSubAddress(const PrivateKey& viewKey, const PublicKey& spendKey, const SubAddressIdentifier& subAddressId);
MclScalar FromSeedToChildKey(const MclScalar& seed);
MclScalar FromChildToTransactionKey(const MclScalar& seed);
MclScalar FromChildToBlindingKey(const MclScalar& seed);
MclScalar FromChildToTokenKey(const MclScalar& seed);
MclScalar FromTransactionToViewKey(const MclScalar& seed);
MclScalar FromTransactionToSpendKey(const MclScalar& seed);
MclScalar GenRandomSeed();
} // namespace blsct

#endif // NAVIO_BLSCT_WALLET_HELPERS_H
