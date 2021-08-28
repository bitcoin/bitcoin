// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_NODE_COINSTATS_H
#define SYSCOIN_NODE_COINSTATS_H

#include <amount.h>
#include <chain.h>
#include <coins.h>
#include <streams.h>
#include <uint256.h>

#include <cstdint>
#include <functional>

class BlockManager;
class CCoinsView;

enum class CoinStatsHashType {
    HASH_SERIALIZED,
    MUHASH,
    NONE,
};

struct CCoinsStats
{
    CoinStatsHashType m_hash_type;
    int nHeight{0};
    uint256 hashBlock{};
    uint64_t nTransactions{0};
    uint64_t nTransactionOutputs{0};
    uint64_t nBogoSize{0};
    uint256 hashSerialized{};
    uint64_t nDiskSize{0};
    CAmount nTotalAmount{0};

    //! The number of coins contained.
    uint64_t coins_count{0};

    //! Signals if the coinstatsindex should be used (when available).
    bool index_requested{true};
    //! Signals if the coinstatsindex was used to retrieve the statistics.
    bool index_used{false};

    // Following values are only available from coinstats index

    //! Total cumulative amount of block subsidies up to and including this block
    CAmount total_subsidy{0};
    //! Total cumulative amount of unspendable coins up to and including this block
    CAmount total_unspendable_amount{0};
    //! Total cumulative amount of prevouts spent up to and including this block
    CAmount total_prevout_spent_amount{0};
    //! Total cumulative amount of outputs created up to and including this block
    CAmount total_new_outputs_ex_coinbase_amount{0};
    //! Total cumulative amount of coinbase outputs up to and including this block
    CAmount total_coinbase_amount{0};
    //! The unspendable coinbase amount from the genesis block
    CAmount total_unspendables_genesis_block{0};
    //! The two unspendable coinbase outputs total amount caused by BIP30
    CAmount total_unspendables_bip30{0};
    //! Total cumulative amount of outputs sent to unspendable scripts (OP_RETURN for example) up to and including this block
    CAmount total_unspendables_scripts{0};
    //! Total cumulative amount of coins lost due to unclaimed miner rewards up to and including this block
    CAmount total_unspendables_unclaimed_rewards{0};

    CCoinsStats(CoinStatsHashType hash_type) : m_hash_type(hash_type) {}
};

//! Calculate statistics about the unspent transaction output set
bool GetUTXOStats(CCoinsView* view, BlockManager& blockman, CCoinsStats& stats, const std::function<void()>& interruption_point = {}, const CBlockIndex* pindex = nullptr);

uint64_t GetBogoSize(const CScript& script_pub_key);

CDataStream TxOutSer(const COutPoint& outpoint, const Coin& coin);

#endif // SYSCOIN_NODE_COINSTATS_H
