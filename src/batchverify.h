// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BATCHVERIFY_H
#define BITCOIN_BATCHVERIFY_H

#include <pubkey.h>

#include <array>

struct secp256k1_batch_struct;
typedef struct secp256k1_batch_struct secp256k1_batch;

struct SchnorrSignatureToVerify {
    std::array<unsigned char, 64> sig;
    XOnlyPubKey pubkey;
    uint256 sighash;
};

/** Collects Schnorr signatures and verifies them in a single batch.
 *
 *  Instances are not shared between threads and therefore need no locking: each
 *  script check worker thread owns one, as does the single-threaded
 *  ConnectBlock path.
 */
class BatchSchnorrVerifier {
private:
    secp256k1_batch* m_batch;

public:
    BatchSchnorrVerifier();
    ~BatchSchnorrVerifier();

    BatchSchnorrVerifier(const BatchSchnorrVerifier&) = delete;
    BatchSchnorrVerifier& operator=(const BatchSchnorrVerifier&) = delete;

    /** Add a signature to the batch.
     *
     *  Returns false if the pubkey could not be parsed. In that case the
     *  signature has *not* become part of the batch, so the caller must treat
     *  the check as failed instead of relying on a later Verify().
     */
    [[nodiscard]] bool Add(std::span<const unsigned char> sig, const XOnlyPubKey& pubkey, const uint256& sighash);
    bool Verify();
    void Reset();
};

#endif // BITCOIN_BATCHVERIFY_H
