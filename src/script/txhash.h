// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_TXHASH_H
#define BITCOIN_SCRIPT_TXHASH_H

#include <attributes.h>
#include <crypto/common.h>
#include <hash.h>
#include <primitives/transaction.h>
#include <span.h>
#include <sync.h>
#include <uint256.h>

#include <stdint.h>
#include <vector>

class CSHA256;
class CTxOut;
class uint256;

static const unsigned char TXFS_VERSION = 1 << 0;
static const unsigned char TXFS_LOCKTIME = 1 << 1;
static const unsigned char TXFS_CURRENT_INPUT_IDX = 1 << 2;
static const unsigned char TXFS_CURRENT_INPUT_SPENTSCRIPT = 1 << 3;
static const unsigned char TXFS_CURRENT_INPUT_CONTROL_BLOCK = 1 << 4;
static const unsigned char TXFS_CURRENT_INPUT_LAST_CODESEPARATOR_POS = 1 << 5;
// bit 7 unused
static const unsigned char TXFS_CONTROL = 1 << 7;

static const unsigned char TXFS_ALL = TXFS_VERSION
    | TXFS_LOCKTIME
    | TXFS_CURRENT_INPUT_IDX
    | TXFS_CURRENT_INPUT_SPENTSCRIPT
    | TXFS_CURRENT_INPUT_CONTROL_BLOCK
    | TXFS_CURRENT_INPUT_LAST_CODESEPARATOR_POS
    | TXFS_CONTROL;

static const unsigned char TXFS_INPUTS_PREVOUTS = 1 << 0;
static const unsigned char TXFS_INPUTS_SEQUENCES = 1 << 1;
static const unsigned char TXFS_INPUTS_SCRIPTSIGS = 1 << 2;
static const unsigned char TXFS_INPUTS_PREVOUT_SCRIPTPUBKEYS = 1 << 3;
static const unsigned char TXFS_INPUTS_PREVOUT_VALUES = 1 << 4;
static const unsigned char TXFS_INPUTS_TAPROOT_ANNEXES = 1 << 5;
static const unsigned char TXFS_OUTPUTS_SCRIPTPUBKEYS = 1 << 6;
static const unsigned char TXFS_OUTPUTS_VALUES = 1 << 7;

static const unsigned char TXFS_INPUTS_ALL = TXFS_INPUTS_PREVOUTS
    | TXFS_INPUTS_SEQUENCES
    | TXFS_INPUTS_SCRIPTSIGS
    | TXFS_INPUTS_PREVOUT_SCRIPTPUBKEYS
    | TXFS_INPUTS_PREVOUT_VALUES
    | TXFS_INPUTS_TAPROOT_ANNEXES;
static const unsigned char TXFS_INPUTS_TEMPLATE = TXFS_INPUTS_SEQUENCES
    | TXFS_INPUTS_SCRIPTSIGS
    | TXFS_INPUTS_PREVOUT_VALUES
    | TXFS_INPUTS_TAPROOT_ANNEXES;
static const unsigned char TXFS_OUTPUTS_ALL = TXFS_OUTPUTS_SCRIPTPUBKEYS | TXFS_OUTPUTS_VALUES;

static const unsigned char TXFS_INOUT_NUMBER = 1 << 7;
static const unsigned char TXFS_INOUT_SELECTION_NONE = 0x00;
static const unsigned char TXFS_INOUT_SELECTION_CURRENT = 0x40;
static const unsigned char TXFS_INOUT_SELECTION_ALL = 0x3f;
static const unsigned char TXFS_INOUT_SELECTION_MODE = 1 << 6;
static const unsigned char TXFS_INOUT_LEADING_SIZE = 1 << 5;
static const unsigned char TXFS_INOUT_INDIVIDUAL_MODE = 1 << 5;
static const unsigned char TXFS_INOUT_SELECTION_MASK = 0xff ^ (1 << 7) ^ (1 << 6) ^ (1 << 5 );


static const std::vector<unsigned char> TXFS_SPECIAL_ALL = {
    TXFS_ALL,
    TXFS_INPUTS_ALL | TXFS_OUTPUTS_ALL,
    TXFS_INOUT_NUMBER | TXFS_INOUT_SELECTION_ALL,
    TXFS_INOUT_NUMBER | TXFS_INOUT_SELECTION_ALL,
};
static const std::vector<unsigned char> TXFS_SPECIAL_TEMPLATE = {
    TXFS_ALL,
    TXFS_INPUTS_TEMPLATE | TXFS_OUTPUTS_ALL,
    TXFS_INOUT_NUMBER | TXFS_INOUT_SELECTION_ALL,
    TXFS_INOUT_NUMBER | TXFS_INOUT_SELECTION_ALL,
};

static const unsigned int LEADING_CACHE_INTERVAL = 10;

struct TxHashCache
{
    //! Mutex to guard access to the TxHashCache.
    RecursiveMutex mtx;

    //! Individual hashes for all input fields that can be of variable size.
    std::vector<uint256> hashed_script_sigs;
    std::vector<uint256> hashed_prevout_spks;
    std::vector<uint256> hashed_annexes;
    //! Individual hashes for all output fields that can be of variable size.
    std::vector<uint256> hashed_script_pubkeys;
    //! Cached hash engines for txhash "leading" hashes at fixed intervals for outputs.
    std::vector<CSHA256> leading_prevouts;
    std::vector<CSHA256> leading_sequences;
    std::vector<CSHA256> leading_script_sigs;
    std::vector<CSHA256> leading_prevout_spks;
    std::vector<CSHA256> leading_prevout_amounts;
    std::vector<CSHA256> leading_annexes;
    //! Cached hash engines for txhash "leading" hashes at fixed intervals for inputs.
    std::vector<CSHA256> leading_script_pubkeys;
    std::vector<CSHA256> leading_amounts;
    //! Hash of all hashed items of input fields.
    uint256 all_prevouts;
    uint256 all_sequences;
    uint256 all_script_sigs;
    uint256 all_prevout_spks;
    uint256 all_prevout_amounts;
    uint256 all_annexes;
    //! Hash of all hashed items of output fields.
    uint256 all_script_pubkeys;
    uint256 all_amounts;

    // Special cases that are cached but need no all or leading cache.
    std::vector<uint256> hashed_spentscripts;
    std::vector<uint256> hashed_control_blocks;

    TxHashCache() = default;
};

template <class T>
bool calculate_txhash(
    uint256& hash_out,
    Span<const unsigned char> field_selector,
    TxHashCache& cache,
    const T& tx,
    const std::vector<CTxOut>& prevout_outputs,
    uint32_t codeseparator_pos,
    uint32_t in_pos
);

#endif // BITCOIN_SCRIPT_TXHASH_H
