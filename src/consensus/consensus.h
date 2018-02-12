// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CONSENSUS_H
#define BITCOIN_CONSENSUS_CONSENSUS_H

/** The maximum allowed size for a serialized block, in bytes (network rule) */
static const unsigned int MAX_LEGACY_BLOCK_SIZE = 1000000;
static const unsigned int MAX_DIP0001_BLOCK_SIZE = 2000000;
inline unsigned int MaxBlockSize(bool fDIP0001Active /*= false */)
{
    return fDIP0001Active ? MAX_DIP0001_BLOCK_SIZE : MAX_LEGACY_BLOCK_SIZE;
}
/** The maximum allowed number of signature check operations in a block (network rule) */
inline unsigned int MaxBlockSigOps(bool fDIP0001Active /*= false */)
{
    return MaxBlockSize(fDIP0001Active) / 50;
}
/** The maximum allowed size of version 3 extra payload */
static const unsigned int MAX_TX_EXTRA_PAYLOAD = 10000;
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
