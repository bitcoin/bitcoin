// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_WALLETFILTERINDEX_H
#define BITCOIN_INDEX_WALLETFILTERINDEX_H

#include <attributes.h>
#include <blockfilter.h>
#include <chain.h>
#include <flatfile.h>
#include <index/base.h>
#include <util/hasher.h>

static constexpr bool DEFAULT_WALLETFILTERINDEX{false};

class WalletFilterIndex final : public BaseIndex
{
private:
    std::unique_ptr<BaseIndex::DB> m_db;

    FlatFilePos m_next_filter_pos;
    std::unique_ptr<FlatFileSeq> m_filter_fileseq;

    GCSFilter::Params m_params;

    bool ReadFilterFromDisk(const FlatFilePos& pos, GCSFilter& filter) const;
    size_t WriteFilterToDisk(FlatFilePos& pos, const GCSFilter& filter);

    bool AllowPrune() const override { return true; }

protected:
    bool CustomInit(const std::optional<interfaces::BlockKey>& block) override;

    bool CustomCommit(CDBBatch& batch) override;

    bool CustomAppend(const interfaces::BlockInfo& block) override;

    BaseIndex::DB& GetDB() const LIFETIMEBOUND override { return *m_db; }

public:
    /** Constructs the index, which becomes available to be queried. */
    explicit WalletFilterIndex(std::unique_ptr<interfaces::Chain> chain,
                              size_t n_cache_size, bool f_memory = false,
                              bool f_wipe = false);

    /** Get a single filter by block. */
    bool LookupFilter(const CBlockIndex* block_index, GCSFilter& filter_out) const;
};

/// The global wallet filter index. May be null.
extern std::unique_ptr<WalletFilterIndex> g_wallet_filter_index;

#endif // BITCOIN_INDEX_WALLETFILTERINDEX_H
