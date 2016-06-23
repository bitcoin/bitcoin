// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CONSENSUS_H
#define BITCOIN_CONSENSUS_CONSENSUS_H

/** Block size limit, post-2MB fork */
static const unsigned int MAX_BLOCK_SIZE = 2000000;
/** The old block size limit */
static const unsigned int OLD_MAX_BLOCK_SIZE = 1000000;
/** limit on signature operations in a block */
static const unsigned int MAX_BLOCK_SIGOPS = OLD_MAX_BLOCK_SIZE/50;
/** limit on number of bytes hashed to compute signatures in a block */
static const unsigned int MAX_BLOCK_SIGHASH = 1300 * 1000 * 1000; // 1.3 gigabytes
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
