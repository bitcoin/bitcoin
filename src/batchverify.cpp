// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <batchverify.h>
#include <logging.h>
#include <pubkey.h>
#include <random.h>
#include <sync.h>

#include <secp256k1.h>
#include <secp256k1_batch.h>
#include <secp256k1_schnorrsig_batch.h>

BatchSchnorrVerifier::BatchSchnorrVerifier() {
    unsigned char rnd[16];
    GetRandBytes(rnd);
    // This is the maximum number of scalar-point pairs on the batch for which
    // Strauss' algorithm, which is used in the secp256k1 implementation, is
    // still efficient.
    const size_t max_batch_size{106};
    secp256k1_batch* batch{secp256k1_batch_create(secp256k1_context_static, max_batch_size, rnd)};
    m_batch = batch;
}

BatchSchnorrVerifier::~BatchSchnorrVerifier() {
    (void)secp256k1_batch_destroy(secp256k1_context_static, m_batch);
}

bool BatchSchnorrVerifier::Add(const Span<const unsigned char> sig, const XOnlyPubKey& pubkey, const uint256& sighash) {
    LOCK(m_batch_mutex);
    if (secp256k1_batch_usable(secp256k1_context_static, m_batch) == 0) {
        LogPrintf("ERROR: BatchSchnorrVerifier m_batch unusable\n");
        return false;
    }

    secp256k1_xonly_pubkey pubkey_parsed;
    (void)secp256k1_xonly_pubkey_parse(secp256k1_context_static, &pubkey_parsed, pubkey.data());

    return secp256k1_batch_add_schnorrsig(secp256k1_context_static, m_batch, sig.data(), sighash.begin(), 32, &pubkey_parsed);
}

bool BatchSchnorrVerifier::Verify() {
    LOCK(m_batch_mutex);
    return secp256k1_batch_verify(secp256k1_context_static, m_batch);
}
