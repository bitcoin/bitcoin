// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_PUBLIC_KEYS_H
#define NAVCOIN_BLSCT_PUBLIC_KEYS_H

#include <blsct/public_key.h>

#include <vector>

namespace blsct {

class PublicKeys
{
public:
    PublicKeys(const std::vector<PublicKey>& pks): m_pks(pks) {}

    PublicKey Aggregate() const;

    // Basic scheme
    bool VerifyBalanceBatch(const Signature& sig) const;

    // Message augmentation scheme
    bool VerifyBatch(const std::vector<PublicKey::Message>& msgs, const Signature& sig, const bool& fVerifyTx = false) const;

private:
    // Core operations
    bool CoreAggregateVerify(const std::vector<PublicKey::Message>& msgs, const Signature& sig) const;

    std::vector<PublicKey> m_pks;
};

}

#endif  // NAVCOIN_BLSCT_PUBLICS_KEYS_H
