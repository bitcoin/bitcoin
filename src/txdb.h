// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXDB_H
#define BITCOIN_TXDB_H

#include <coins.h>
#include <dbwrapper.h>
#include <chain.h>
#include <primitives/block.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class CBlockIndex;
class uint256;

//! No need to periodic flush if at least this much space still available.
static constexpr int MAX_BLOCK_COINSDB_USAGE = 10;
//! -dbcache default (MiB)
static const int64_t nDefaultDbCache = 450;
//! -dbbatchsize default (bytes)
static const int64_t nDefaultDbBatchSize = 16 << 20;
//! max. -dbcache (MiB)
static const int64_t nMaxDbCache = sizeof(void*) > 4 ? 16384 : 1024;
//! min. -dbcache (MiB)
static const int64_t nMinDbCache = 4;
//! Max memory allocated to block tree DB specific cache, if no -txindex (MiB)
static const int64_t nMaxBlockDBCache = 2;
//! Max memory allocated to block tree DB specific cache, if -txindex (MiB)
// Unlike for the UTXO database, for the txindex scenario the leveldb cache make
// a meaningful difference: https://github.com/bitcoin/bitcoin/pull/8273#issuecomment-229601991
static const int64_t nMaxTxIndexCache = 1024;
//! Max memory allocated to all block filter index caches combined in MiB.
static const int64_t max_filter_index_cache = 1024;
//! Max memory allocated to coin DB specific cache (MiB)
static const int64_t nMaxCoinsDBCache = 8;

/** CCoinsView backed by the coin database (chainstate/) */
class CCoinsViewDB final : public CCoinsView
{
protected:
    mutable CDBWrapper db;
public:
    /**
     * @param[in] ldb_path    Location in the filesystem where leveldb data will be stored.
     */
    explicit CCoinsViewDB(fs::path ldb_path, size_t nCacheSize, bool fMemory, bool fWipe);

    bool GetCoin(const COutPoint &outpoint, Coin &coin) const override;
    bool HaveCoin(const COutPoint &outpoint) const override;
    uint256 GetBestBlock() const override;
    std::vector<uint256> GetHeadBlocks() const override;
    bool BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock) override;
    CCoinsViewCursorRef Cursor() const override;
    CCoinsViewCursorRef Cursor(const CAccountID &accountID) const override;
    CCoinsViewCursorRef PointSendCursor(const CAccountID &accountID) const override;
    CCoinsViewCursorRef PointReceiveCursor(const CAccountID &accountID) const override;
    CCoinsViewCursorRef StakingSendCursor(const CAccountID &accountID) const override;
    CCoinsViewCursorRef StakingReceiveCursor(const CAccountID &accountID) const override;

    //! Attempt to update from an older database format. Returns whether an error occurred.
    bool Upgrade(bool &fUpgraded);
    size_t EstimateSize() const override;

    CAmount GetAccountBalance(const CAccountID &accountID, CAmount *balanceBindPlotter, CAmount balancePoint[2], CAmount balanceStaking[2], const CCoinsMap &mapModifiedCoins = {}) const override;
    CBindPlotterCoinsMap GetAccountBindPlotterEntries(const CAccountID &accountID, const uint64_t &plotterId = 0) const override;
    CBindPlotterCoinsMap GetBindPlotterEntries(const uint64_t &plotterId) const override;
    CAccountBalanceList GetTopStakingAccounts(int n, const CCoinsMap &mapModifiedCoins = {}) const override;
};

/** Access to the block database (blocks/index/) */
class CBlockTreeDB : public CDBWrapper
{
public:
    explicit CBlockTreeDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool WriteBatchSync(const std::vector<std::pair<int, const CBlockFileInfo*> >& fileInfo, int nLastFile, const std::vector<const CBlockIndex*>& blockinfo);
    bool ReadBlockFileInfo(int nFile, CBlockFileInfo &info);
    bool ReadLastBlockFile(int &nFile);
    bool WriteReindexing(bool fReindexing);
    void ReadReindexing(bool &fReindexing);
    bool WriteFlag(const std::string &name, bool fValue);
    bool ReadFlag(const std::string &name, bool &fValue);
    bool LoadBlockIndexGuts(const Consensus::Params& consensusParams, std::function<CBlockIndex*(const uint256&)> insertBlockIndex);
};

#endif // BITCOIN_TXDB_H
