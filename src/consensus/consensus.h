// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAVEN_CONSENSUS_CONSENSUS_H
#define RAVEN_CONSENSUS_CONSENSUS_H

#include <stdlib.h>
#include <stdint.h>

/** The maximum allowed size for a serialized block, in bytes (only for buffer size limits) */
static const unsigned int MAX_BLOCK_SERIALIZED_SIZE = 4000000;
/** The maximum allowed weight for a block, see BIP 141 (network rule) */
static const unsigned int MAX_BLOCK_WEIGHT = 4000000;

/** The maximum allowed weight for a block, after RIP 2 (network rule) */
static const unsigned int MAX_BLOCK_WEIGHT_RIP2 = 8000000;

/** The maximum allowed size for a serialized block, in bytes after RIP 2(only for buffer size limits) */
static const unsigned int MAX_BLOCK_SERIALIZED_SIZE_RIP2 = 8000000;

/** The maximum allowed number of signature check operations in a block (network rule) */
static const int64_t MAX_BLOCK_SIGOPS_COST = 80000;
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 100;

static const int WITNESS_SCALE_FACTOR = 4;

static const size_t MIN_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 60; // 60 is the lower bound for the size of a valid serialized CTransaction
static const size_t MIN_SERIALIZABLE_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 10; // 10 is the lower bound for the size of a serialized CTransaction

#define UNUSED_VAR     __attribute__ ((unused))
//! This variable needs to in this class because undo.h uses it. However because it is in this class
//! it causes unused variable warnings when compiling. This UNUSED_VAR removes the unused warnings
UNUSED_VAR static bool fAssetsIsActive = false;

unsigned int GetMaxBlockWeight();
unsigned int GetMaxBlockSerializedSize();

/** Flags for nSequence and nLockTime locks */
enum {
    /* Interpret sequence numbers as relative lock-time constraints. */
    LOCKTIME_VERIFY_SEQUENCE = (1 << 0),

    /* Use GetMedianTimePast() instead of nTime for end point timestamp. */
    LOCKTIME_MEDIAN_TIME_PAST = (1 << 1),
};

#endif // RAVEN_CONSENSUS_CONSENSUS_H
