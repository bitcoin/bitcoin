// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_COINSTATS_H
#define BITCOIN_KERNEL_COINSTATS_H

#include <arith_uint256.h>
#include <consensus/amount.h>
#include <crypto/muhash.h>
#include <streams.h>
#include <uint256.h>

#include <cstdint>
#include <functional>
#include <optional>

class CCoinsView;
class Coin;
class COutPoint;
class CScript;
namespace node {
class BlockManager;
} // namespace node

namespace kernel {
enum class CoinStatsHashType {
    HASH_SERIALIZED,
    MUHASH,
    NONE,
};

struct CCoinsStats {
    int nHeight{0};
    uint256 hashBlock{};
    uint64_t nTransactions{0};
    uint64_t nTransactionOutputs{0};
    uint64_t nBogoSize{0};
    uint256 hashSerialized{};
    uint64_t nDiskSize{0};
    //! The total amount, or nullopt if an overflow occurred calculating it
    std::optional<CAmount> total_amount{0};

    //! The number of coins contained.
    uint64_t coins_count{0};

    //! Signals if the coinstatsindex was used to retrieve the statistics.
    bool index_used{false};

    // Following values are only available from coinstats index

    //! Total cumulative amount of block subsidies up to and including this block
    CAmount total_subsidy{0};
    //! The unspendable coinbase amount from the genesis block
    CAmount total_unspendables_genesis_block{0};
    //! The two unspendable coinbase outputs total amount caused by BIP30
    CAmount total_unspendables_bip30{0};
    //! Total cumulative amount of outputs sent to unspendable scripts (OP_RETURN for example) up to and including this block
    CAmount total_unspendables_scripts{0};
    //! Total cumulative amount of coins lost due to unclaimed miner rewards up to and including this block
    CAmount total_unspendables_unclaimed_rewards{0};

    // Despite containing amounts the following values use a uint256 type to prevent overflowing

    //! Total cumulative amount of prevouts spent up to and including this block
    arith_uint256 total_prevout_spent_amount{0};
    //! Total cumulative amount of outputs created up to and including this block
    arith_uint256 total_new_outputs_ex_coinbase_amount{0};
    //! Total cumulative amount of coinbase outputs up to and including this block
    arith_uint256 total_coinbase_amount{0};

    CCoinsStats() = default;
    CCoinsStats(int block_height, const uint256& block_hash);
};

uint64_t GetBogoSize(const CScript& script_pub_key);

void ApplyCoinHash(MuHash3072& muhash, const COutPoint& outpoint, const Coin& coin);
void RemoveCoinHash(MuHash3072& muhash, const COutPoint& outpoint, const Coin& coin);

std::optional<CCoinsStats> ComputeUTXOStats(CoinStatsHashType hash_type, CCoinsView* view, node::BlockManager& blockman, const std::function<void()>& interruption_point = {});
} // namespace kernel

#endif // BITCOIN_KERNEL_COINSTATS_H
