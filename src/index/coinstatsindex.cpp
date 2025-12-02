// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/coinstatsindex.h>

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <common/args.h>
#include <consensus/amount.h>
#include <crypto/muhash.h>
#include <dbwrapper.h>
#include <index/base.h>
#include <interfaces/chain.h>
#include <interfaces/types.h>
#include <kernel/coinstats.h>
#include <logging.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <serialize.h>
#include <uint256.h>
#include <undo.h>
#include <util/check.h>
#include <util/fs.h>
#include <validation.h>

#include <compare>
#include <ios>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

using kernel::ApplyCoinHash;
using kernel::CCoinsStats;
using kernel::GetBogoSize;
using kernel::RemoveCoinHash;

static constexpr uint8_t DB_BLOCK_HASH{'s'};
static constexpr uint8_t DB_BLOCK_HEIGHT{'t'};
static constexpr uint8_t DB_MUHASH{'M'};

namespace {

struct DBVal {
    uint256 muhash{uint256::ZERO};
    uint64_t transaction_output_count{0};
    uint64_t bogo_size{0};
    CAmount total_amount{0};
    CAmount total_subsidy{0};
    arith_uint256 total_prevout_spent_amount{0};
    arith_uint256 total_new_outputs_ex_coinbase_amount{0};
    arith_uint256 total_coinbase_amount{0};
    CAmount total_unspendables_genesis_block{0};
    CAmount total_unspendables_bip30{0};
    CAmount total_unspendables_scripts{0};
    CAmount total_unspendables_unclaimed_rewards{0};

    SERIALIZE_METHODS(DBVal, obj)
    {
        uint256 prevout_spent, new_outputs, coinbase;
        SER_WRITE(obj, prevout_spent = ArithToUint256(obj.total_prevout_spent_amount));
        SER_WRITE(obj, new_outputs = ArithToUint256(obj.total_new_outputs_ex_coinbase_amount));
        SER_WRITE(obj, coinbase = ArithToUint256(obj.total_coinbase_amount));

        READWRITE(obj.muhash);
        READWRITE(obj.transaction_output_count);
        READWRITE(obj.bogo_size);
        READWRITE(obj.total_amount);
        READWRITE(obj.total_subsidy);
        READWRITE(prevout_spent);
        READWRITE(new_outputs);
        READWRITE(coinbase);
        READWRITE(obj.total_unspendables_genesis_block);
        READWRITE(obj.total_unspendables_bip30);
        READWRITE(obj.total_unspendables_scripts);
        READWRITE(obj.total_unspendables_unclaimed_rewards);

        SER_READ(obj, obj.total_prevout_spent_amount = UintToArith256(prevout_spent));
        SER_READ(obj, obj.total_new_outputs_ex_coinbase_amount = UintToArith256(new_outputs));
        SER_READ(obj, obj.total_coinbase_amount = UintToArith256(coinbase));
    }
};

struct DBHeightKey {
    int height;

    explicit DBHeightKey(int height_in) : height(height_in) {}

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        ser_writedata8(s, DB_BLOCK_HEIGHT);
        ser_writedata32be(s, height);
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        const uint8_t prefix{ser_readdata8(s)};
        if (prefix != DB_BLOCK_HEIGHT) {
            throw std::ios_base::failure("Invalid format for coinstatsindex DB height key");
        }
        height = ser_readdata32be(s);
    }
};

struct DBHashKey {
    uint256 block_hash;

    explicit DBHashKey(const uint256& hash_in) : block_hash(hash_in) {}

    SERIALIZE_METHODS(DBHashKey, obj)
    {
        uint8_t prefix{DB_BLOCK_HASH};
        READWRITE(prefix);
        if (prefix != DB_BLOCK_HASH) {
            throw std::ios_base::failure("Invalid format for coinstatsindex DB hash key");
        }

        READWRITE(obj.block_hash);
    }
};

}; // namespace

std::unique_ptr<CoinStatsIndex> g_coin_stats_index;

CoinStatsIndex::CoinStatsIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex(std::move(chain), "coinstatsindex")
{
    // An earlier version of the index used "indexes/coinstats" but it contained
    // a bug and is superseded by a fixed version at "indexes/coinstatsindex".
    // The original index is kept around until the next release in case users
    // decide to downgrade their node.
    auto old_path = gArgs.GetDataDirNet() / "indexes" / "coinstats";
    if (fs::exists(old_path)) {
        // TODO: Change this to deleting the old index with v31.
        LogWarning("Old version of coinstatsindex found at %s. This folder can be safely deleted unless you " \
            "plan to downgrade your node to version 29 or lower.", fs::PathToString(old_path));
    }
    fs::path path{gArgs.GetDataDirNet() / "indexes" / "coinstatsindex"};
    fs::create_directories(path);

    m_db = std::make_unique<CoinStatsIndex::DB>(path / "db", n_cache_size, f_memory, f_wipe);
}

bool CoinStatsIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    const CAmount block_subsidy{GetBlockSubsidy(block.height, Params().GetConsensus())};
    m_total_subsidy += block_subsidy;

    // Ignore genesis block
    if (block.height > 0) {
        uint256 expected_block_hash{*Assert(block.prev_hash)};
        if (m_current_block_hash != expected_block_hash) {
            LogError("previous block header belongs to unexpected block %s; expected %s",
                      m_current_block_hash.ToString(), expected_block_hash.ToString());
            return false;
        }

        // Add the new utxos created from the block
        assert(block.data);
        for (size_t i = 0; i < block.data->vtx.size(); ++i) {
            const auto& tx{block.data->vtx.at(i)};
            const bool is_coinbase{tx->IsCoinBase()};

            // Skip duplicate txid coinbase transactions (BIP30).
            if (is_coinbase && IsBIP30Unspendable(block.hash, block.height)) {
                m_total_unspendables_bip30 += block_subsidy;
                continue;
            }

            for (uint32_t j = 0; j < tx->vout.size(); ++j) {
                const CTxOut& out{tx->vout[j]};
                const Coin coin{out, block.height, is_coinbase};
                const COutPoint outpoint{tx->GetHash(), j};

                // Skip unspendable coins
                if (coin.out.scriptPubKey.IsUnspendable()) {
                    m_total_unspendables_scripts += coin.out.nValue;
                    continue;
                }

                ApplyCoinHash(m_muhash, outpoint, coin);

                if (is_coinbase) {
                    m_total_coinbase_amount += coin.out.nValue;
                } else {
                    m_total_new_outputs_ex_coinbase_amount += coin.out.nValue;
                }

                ++m_transaction_output_count;
                m_total_amount += coin.out.nValue;
                m_bogo_size += GetBogoSize(coin.out.scriptPubKey);
            }

            // The coinbase tx has no undo data since no former output is spent
            if (!is_coinbase) {
                const auto& tx_undo{Assert(block.undo_data)->vtxundo.at(i - 1)};

                for (size_t j = 0; j < tx_undo.vprevout.size(); ++j) {
                    const Coin& coin{tx_undo.vprevout[j]};
                    const COutPoint outpoint{tx->vin[j].prevout.hash, tx->vin[j].prevout.n};

                    RemoveCoinHash(m_muhash, outpoint, coin);

                    m_total_prevout_spent_amount += coin.out.nValue;

                    --m_transaction_output_count;
                    m_total_amount -= coin.out.nValue;
                    m_bogo_size -= GetBogoSize(coin.out.scriptPubKey);
                }
            }
        }
    } else {
        // genesis block
        m_total_unspendables_genesis_block += block_subsidy;
    }

    // If spent prevouts + block subsidy are still a higher amount than
    // new outputs + coinbase + current unspendable amount this means
    // the miner did not claim the full block reward. Unclaimed block
    // rewards are also unspendable.
    const CAmount temp_total_unspendable_amount{m_total_unspendables_genesis_block + m_total_unspendables_bip30 + m_total_unspendables_scripts + m_total_unspendables_unclaimed_rewards};
    const arith_uint256 unclaimed_rewards{(m_total_prevout_spent_amount + m_total_subsidy) - (m_total_new_outputs_ex_coinbase_amount + m_total_coinbase_amount + temp_total_unspendable_amount)};
    assert(unclaimed_rewards <= arith_uint256(std::numeric_limits<CAmount>::max()));
    m_total_unspendables_unclaimed_rewards += static_cast<CAmount>(unclaimed_rewards.GetLow64());

    std::pair<uint256, DBVal> value;
    value.first = block.hash;
    value.second.transaction_output_count = m_transaction_output_count;
    value.second.bogo_size = m_bogo_size;
    value.second.total_amount = m_total_amount;
    value.second.total_subsidy = m_total_subsidy;
    value.second.total_prevout_spent_amount = m_total_prevout_spent_amount;
    value.second.total_new_outputs_ex_coinbase_amount = m_total_new_outputs_ex_coinbase_amount;
    value.second.total_coinbase_amount = m_total_coinbase_amount;
    value.second.total_unspendables_genesis_block = m_total_unspendables_genesis_block;
    value.second.total_unspendables_bip30 = m_total_unspendables_bip30;
    value.second.total_unspendables_scripts = m_total_unspendables_scripts;
    value.second.total_unspendables_unclaimed_rewards = m_total_unspendables_unclaimed_rewards;

    uint256 out;
    m_muhash.Finalize(out);
    value.second.muhash = out;

    m_current_block_hash = block.hash;

    // Intentionally do not update DB_MUHASH here so it stays in sync with
    // DB_BEST_BLOCK, and the index is not corrupted if there is an unclean shutdown.
    m_db->Write(DBHeightKey(block.height), value);
    return true;
}

[[nodiscard]] static bool CopyHeightIndexToHashIndex(CDBIterator& db_it, CDBBatch& batch,
                                                     const std::string& index_name, int height)
{
    DBHeightKey key{height};
    db_it.Seek(key);

    if (!db_it.GetKey(key) || key.height != height) {
        LogError("unexpected key in %s: expected (%c, %d)",
                 index_name, DB_BLOCK_HEIGHT, height);
        return false;
    }

    std::pair<uint256, DBVal> value;
    if (!db_it.GetValue(value)) {
        LogError("unable to read value in %s at key (%c, %d)",
                 index_name, DB_BLOCK_HEIGHT, height);
        return false;
    }

    batch.Write(DBHashKey(value.first), value.second);
    return true;
}

bool CoinStatsIndex::CustomRemove(const interfaces::BlockInfo& block)
{
    CDBBatch batch(*m_db);
    std::unique_ptr<CDBIterator> db_it(m_db->NewIterator());

    // During a reorg, copy the block's hash digest from the height index to the hash index,
    // ensuring it's still accessible after the height index entry is overwritten.
    if (!CopyHeightIndexToHashIndex(*db_it, batch, m_name, block.height)) {
        return false;
    }

    m_db->WriteBatch(batch);

    if (!RevertBlock(block)) {
        return false; // failure cause logged internally
    }

    return true;
}

static bool LookUpOne(const CDBWrapper& db, const interfaces::BlockRef& block, DBVal& result)
{
    // First check if the result is stored under the height index and the value
    // there matches the block hash. This should be the case if the block is on
    // the active chain.
    std::pair<uint256, DBVal> read_out;
    if (!db.Read(DBHeightKey(block.height), read_out)) {
        return false;
    }
    if (read_out.first == block.hash) {
        result = std::move(read_out.second);
        return true;
    }

    // If value at the height index corresponds to an different block, the
    // result will be stored in the hash index.
    return db.Read(DBHashKey(block.hash), result);
}

std::optional<CCoinsStats> CoinStatsIndex::LookUpStats(const CBlockIndex& block_index) const
{
    CCoinsStats stats{block_index.nHeight, block_index.GetBlockHash()};
    stats.index_used = true;

    DBVal entry;
    if (!LookUpOne(*m_db, {block_index.GetBlockHash(), block_index.nHeight}, entry)) {
        return std::nullopt;
    }

    stats.hashSerialized = entry.muhash;
    stats.nTransactionOutputs = entry.transaction_output_count;
    stats.nBogoSize = entry.bogo_size;
    stats.total_amount = entry.total_amount;
    stats.total_subsidy = entry.total_subsidy;
    stats.total_prevout_spent_amount = entry.total_prevout_spent_amount;
    stats.total_new_outputs_ex_coinbase_amount = entry.total_new_outputs_ex_coinbase_amount;
    stats.total_coinbase_amount = entry.total_coinbase_amount;
    stats.total_unspendables_genesis_block = entry.total_unspendables_genesis_block;
    stats.total_unspendables_bip30 = entry.total_unspendables_bip30;
    stats.total_unspendables_scripts = entry.total_unspendables_scripts;
    stats.total_unspendables_unclaimed_rewards = entry.total_unspendables_unclaimed_rewards;

    return stats;
}

bool CoinStatsIndex::CustomInit(const std::optional<interfaces::BlockRef>& block)
{
    if (!m_db->Read(DB_MUHASH, m_muhash)) {
        // Check that the cause of the read failure is that the key does not
        // exist. Any other errors indicate database corruption or a disk
        // failure, and starting the index would cause further corruption.
        if (m_db->Exists(DB_MUHASH)) {
            LogError("Cannot read current %s state; index may be corrupted",
                      GetName());
            return false;
        }
    }

    if (block) {
        DBVal entry;
        if (!LookUpOne(*m_db, *block, entry)) {
            LogError("Cannot read current %s state; index may be corrupted",
                      GetName());
            return false;
        }

        uint256 out;
        m_muhash.Finalize(out);
        if (entry.muhash != out) {
            LogError("Cannot read current %s state; index may be corrupted",
                      GetName());
            return false;
        }

        m_transaction_output_count = entry.transaction_output_count;
        m_bogo_size = entry.bogo_size;
        m_total_amount = entry.total_amount;
        m_total_subsidy = entry.total_subsidy;
        m_total_prevout_spent_amount = entry.total_prevout_spent_amount;
        m_total_new_outputs_ex_coinbase_amount = entry.total_new_outputs_ex_coinbase_amount;
        m_total_coinbase_amount = entry.total_coinbase_amount;
        m_total_unspendables_genesis_block = entry.total_unspendables_genesis_block;
        m_total_unspendables_bip30 = entry.total_unspendables_bip30;
        m_total_unspendables_scripts = entry.total_unspendables_scripts;
        m_total_unspendables_unclaimed_rewards = entry.total_unspendables_unclaimed_rewards;
        m_current_block_hash = block->hash;
    }

    return true;
}

bool CoinStatsIndex::CustomCommit(CDBBatch& batch)
{
    // DB_MUHASH should always be committed in a batch together with DB_BEST_BLOCK
    // to prevent an inconsistent state of the DB.
    batch.Write(DB_MUHASH, m_muhash);
    return true;
}

interfaces::Chain::NotifyOptions CoinStatsIndex::CustomOptions()
{
    interfaces::Chain::NotifyOptions options;
    options.connect_undo_data = true;
    options.disconnect_data = true;
    options.disconnect_undo_data = true;
    return options;
}

// Revert a single block as part of a reorg
bool CoinStatsIndex::RevertBlock(const interfaces::BlockInfo& block)
{
    std::pair<uint256, DBVal> read_out;

    // Ignore genesis block
    if (block.height > 0) {
        if (!m_db->Read(DBHeightKey(block.height - 1), read_out)) {
            return false;
        }

        uint256 expected_block_hash{*block.prev_hash};
        if (read_out.first != expected_block_hash) {
            LogWarning("previous block header belongs to unexpected block %s; expected %s",
                      read_out.first.ToString(), expected_block_hash.ToString());

            if (!m_db->Read(DBHashKey(expected_block_hash), read_out)) {
                LogError("previous block header not found; expected %s",
                          expected_block_hash.ToString());
                return false;
            }
        }
    }

    // Roll back muhash by removing the new UTXOs that were created by the
    // block and reapplying the old UTXOs that were spent by the block
    assert(block.data);
    assert(block.undo_data);
    for (size_t i = 0; i < block.data->vtx.size(); ++i) {
        const auto& tx{block.data->vtx.at(i)};
        const bool is_coinbase{tx->IsCoinBase()};

        if (is_coinbase && IsBIP30Unspendable(block.hash, block.height)) {
            continue;
        }

        for (uint32_t j = 0; j < tx->vout.size(); ++j) {
            const CTxOut& out{tx->vout[j]};
            const COutPoint outpoint{tx->GetHash(), j};
            const Coin coin{out, block.height, is_coinbase};

            if (!coin.out.scriptPubKey.IsUnspendable()) {
                RemoveCoinHash(m_muhash, outpoint, coin);
            }
        }

        // The coinbase tx has no undo data since no former output is spent
        if (!is_coinbase) {
            const auto& tx_undo{block.undo_data->vtxundo.at(i - 1)};

            for (size_t j = 0; j < tx_undo.vprevout.size(); ++j) {
                const Coin& coin{tx_undo.vprevout[j]};
                const COutPoint outpoint{tx->vin[j].prevout.hash, tx->vin[j].prevout.n};
                ApplyCoinHash(m_muhash, outpoint, coin);
            }
        }
    }

    // Check that the rolled back muhash is consistent with the DB read out
    uint256 out;
    m_muhash.Finalize(out);
    Assert(read_out.second.muhash == out);

    // Apply the other values from the DB to the member variables
    m_transaction_output_count = read_out.second.transaction_output_count;
    m_total_amount = read_out.second.total_amount;
    m_bogo_size = read_out.second.bogo_size;
    m_total_subsidy = read_out.second.total_subsidy;
    m_total_prevout_spent_amount = read_out.second.total_prevout_spent_amount;
    m_total_new_outputs_ex_coinbase_amount = read_out.second.total_new_outputs_ex_coinbase_amount;
    m_total_coinbase_amount = read_out.second.total_coinbase_amount;
    m_total_unspendables_genesis_block = read_out.second.total_unspendables_genesis_block;
    m_total_unspendables_bip30 = read_out.second.total_unspendables_bip30;
    m_total_unspendables_scripts = read_out.second.total_unspendables_scripts;
    m_total_unspendables_unclaimed_rewards = read_out.second.total_unspendables_unclaimed_rewards;
    m_current_block_hash = *block.prev_hash;

    return true;
}
