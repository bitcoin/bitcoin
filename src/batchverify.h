// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BATCHVERIFY_H
#define BITCOIN_BATCHVERIFY_H

#include <pubkey.h>
#include <sync.h>

#include <secp256k1_batch.h>

class BatchSchnorrVerifier {
private:
    secp256k1_batch* m_batch GUARDED_BY(m_batch_mutex);
    mutable Mutex m_batch_mutex;

public:
    BatchSchnorrVerifier();
    ~BatchSchnorrVerifier();

    bool Add(const Span<const unsigned char> sig, const XOnlyPubKey& pubkey, const uint256& sighash) EXCLUSIVE_LOCKS_REQUIRED(!m_batch_mutex);
    bool Verify() EXCLUSIVE_LOCKS_REQUIRED(!m_batch_mutex);
};

#endif // BITCOIN_BATCHVERIFY_H
