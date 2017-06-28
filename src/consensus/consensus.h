// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CONSENSUS_H
#define BITCOIN_CONSENSUS_CONSENSUS_H

#include <stdint.h>

/** BIP102 block size increase - time until HF activated */
static const unsigned int BIP102_FORK_BUFFER = (144 * 90);

/** The maximum allowed size for a serialized block, in bytes (only for buffer size limits) */
static const unsigned int MAX_BLOCK_SERIALIZED_SIZE = (8 * 1000 * 1000);

/** The maximum allowed size for a block excluding witness data, in bytes (network rule) */
static inline bool BIP102active(bool fSegwitSeasoned)
{
    if (!fSegwitSeasoned)
        return false;

    return true;
}

static const unsigned int MAX_LEGACY_BLOCK_SIZE = (1 * 1000 * 1000);
inline unsigned int MaxBlockBaseSize(bool fSegwitSeasoned)
{
    if (!BIP102active(fSegwitSeasoned))
        return MAX_LEGACY_BLOCK_SIZE;

    return (2 * 1000 * 1000);
}

inline unsigned int MaxBlockBaseSize()
{
    return MaxBlockBaseSize(true);
}


/** The maximum allowed number of signature check operations in a block (network rule) */
static const uint64_t MAX_BLOCK_BASE_SIGOPS = 20000;
inline int64_t MaxBlockSigOpsCost(bool fSegwitSeasoned)
{
    if (!BIP102active(fSegwitSeasoned))
        return (MAX_BLOCK_BASE_SIGOPS * 4 /* WITNESS_SCALE_FACTOR */);

    return ((2 * MAX_BLOCK_BASE_SIGOPS) * 4 /* WITNESS_SCALE_FACTOR */);
}

inline int64_t MaxBlockSigOpsCost()
{
    return MaxBlockSigOpsCost(true);
}

/** The maximum allowed weight for a block, see BIP 141 (network rule) */
inline unsigned int MaxBlockWeight(bool fSegwitSeasoned)
{
    return (MaxBlockBaseSize(fSegwitSeasoned) * 4 /* WITNESS_SCALE_FACTOR */);
}

inline unsigned int MaxBlockWeight()
{
    return MaxBlockWeight(true);
}

/** The maximum allowed number of transactions per block */
static const unsigned int MAX_BLOCK_VTX_SIZE = 1000000;

/** The minimum allowed size for a transaction */
static const unsigned int MIN_TRANSACTION_BASE_SIZE = 10;
/** The maximum allowed size for a transaction, excluding witness data, in bytes */
static const unsigned int MAX_TX_BASE_SIZE = 1000000;
/** The maximum allowed number of transactions per block */
static const unsigned int MAX_BLOCK_VTX = (MaxBlockBaseSize() / MIN_TRANSACTION_BASE_SIZE);

/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 100;

/** Flags for nSequence and nLockTime locks */
enum {
    /* Interpret sequence numbers as relative lock-time constraints. */
    LOCKTIME_VERIFY_SEQUENCE = (1 << 0),

    /* Use GetMedianTimePast() instead of nTime for end point timestamp. */
    LOCKTIME_MEDIAN_TIME_PAST = (1 << 1),
};

#endif // BITCOIN_CONSENSUS_CONSENSUS_H
