// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_MINING_H
#define BITCOIN_NODE_MINING_H

#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <uint256.h>
#include <util/time.h>

#include <cstddef>
#include <optional>
#include <vector>

namespace node {

/**
 * Block template creation options. These override node defaults, but can't
 * exceed node limits (e.g. block_reserved_weight can't exceed max block weight).
 */
struct BlockCreateOptions {
    /**
     * Set false to omit mempool transactions
     */
    bool use_mempool{true};
    /**
     * The default reserved weight for the fixed-size block header,
     * transaction count and coinbase transaction. Minimum: 2000 weight units
     * (MINIMUM_BLOCK_RESERVED_WEIGHT).
     *
     * Cap'n Proto IPC clients do not currently have a way of leaving this field
     * unset and will always provide a value.
     */
    std::optional<size_t> block_reserved_weight{};
    /**
     * The maximum additional sigops which the pool will add in coinbase
     * transaction outputs.
     */
    size_t coinbase_output_max_additional_sigops{400};
    /**
     * Script to put in the coinbase transaction. The default is an
     * anyone-can-spend dummy.
     *
     * Should only be used for tests, when the default doesn't suffice.
     *
     * Note that higher level code like the getblocktemplate RPC may omit the
     * coinbase transaction entirely. It's instead constructed by pool software
     * using fields like coinbasevalue, coinbaseaux and default_witness_commitment.
     * This software typically also controls the payout outputs, even for solo
     * mining.
     *
     * The size and sigops are not checked against
     * coinbase_max_additional_weight and coinbase_output_max_additional_sigops.
     */
    CScript coinbase_output_script{CScript() << OP_TRUE};
    /**
     * Whether to call TestBlockValidity() at the end of CreateNewBlock().
     * Should only be used for tests / benchmarks.
     */
    bool test_block_validity{true};
    /**
     * Maximum block weight, defaults to -maxblockweight
     *
     * block_reserved_weight can safely exceed block_max_weight, but the rest of
     * the block template will be empty.
     */
    std::optional<size_t> block_max_weight{};
};

struct BlockWaitOptions {
    /**
     * How long to wait before returning nullptr instead of a new template.
     * Default is to wait forever.
     */
    MillisecondsDouble timeout{MillisecondsDouble::max()};

    /**
     * The wait method will not return a new template unless it has fees at
     * least fee_threshold sats higher than the current template, or unless
     * the chain tip changes and the previous template is no longer valid.
     *
     * A caller may not be interested in templates with higher fees, and
     * determining whether fee_threshold is reached is also expensive. So as
     * an optimization, when fee_threshold is set to MAX_MONEY (default), the
     * implementation is able to be much more efficient, skipping expensive
     * checks and only returning new templates when the chain tip changes.
     */
    CAmount fee_threshold{MAX_MONEY};
};

struct BlockCheckOptions {
    /**
     * Set false to omit the merkle root check
     */
    bool check_merkle_root{true};

    /**
     * Set false to omit the proof-of-work check
     */
    bool check_pow{true};
};

/**
 * Template containing all coinbase transaction fields that are set by our
 * miner code. Clients are expected to add their own outputs and typically
 * also expand the scriptSig.
 */
struct CoinbaseTx {
    /* nVersion */
    uint32_t version;
    /* nSequence for the only coinbase transaction input */
    uint32_t sequence;
    /**
     * Prefix which needs to be placed at the beginning of the scriptSig.
     * Clients may append extra data to this as long as the overall scriptSig
     * size is 100 bytes or less, to avoid the block being rejected with
     * "bad-cb-length" error.
     *
     * Currently with BIP 34, the prefix is guaranteed to be less than 8 bytes,
     * but future soft forks could require longer prefixes.
     */
    CScript script_sig_prefix;
    /**
     * The first (and only) witness stack element of the coinbase input.
     *
     * Omitted for block templates without witness data.
     *
     * This is currently the BIP 141 witness reserved value, and can be chosen
     * arbitrarily by the node, but future soft forks may constrain it.
     */
    std::optional<uint256> witness;
    /**
     * Block subsidy plus fees, minus any non-zero required_outputs.
     *
     * Currently there are no non-zero required_outputs, so block_reward_remaining
     * is the entire block reward. See also required_outputs.
     */
    CAmount block_reward_remaining;
    /*
     * To be included as the last outputs in the coinbase transaction.
     * Currently this is only the witness commitment OP_RETURN, but future
     * softforks or a custom mining patch could add more.
     *
     * The dummy output that spends the full reward is excluded.
     */
    std::vector<CTxOut> required_outputs;
    uint32_t lock_time;
};

} // namespace node

#endif // BITCOIN_NODE_MINING_H
