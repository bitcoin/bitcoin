// Copyright (c) 2016-2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WIDECOIN_BLOCKENCODINGS_H
#define WIDECOIN_BLOCKENCODINGS_H

#include <primitives/block.h>


class CTxMemPool;

// Transaction compression schemes for compact block relay can be introduced by writing
// an actual formatter here.
using TransactionCompression = DefaultFormatter;

class DifferenceFormatter
{
    uint64_t m_shift = 0;

public:
    template<typename Stream, typename I>
    void Ser(Stream& s, I v)
    {
        if (v < m_shift || v >= std::numeric_limits<uint64_t>::max()) throw std::ios_base::failure("differential value overflow");
        WriteCompactSize(s, v - m_shift);
        m_shift = uint64_t(v) + 1;
    }
    template<typename Stream, typename I>
    void Unser(Stream& s, I& v)
    {
        uint64_t n = ReadCompactSize(s);
        m_shift += n;
        if (m_shift < n || m_shift >= std::numeric_limits<uint64_t>::max() || m_shift < std::numeric_limits<I>::min() || m_shift > std::numeric_limits<I>::max()) throw std::ios_base::failure("differential value overflow");
        v = I(m_shift++);
    }
};

class BlockTransactionsRequest {
public:
    // A BlockTransactionsRequest message
    uint256 blockhash;
    std::vector<uint16_t> indexes;

    SERIALIZE_METHODS(BlockTransactionsRequest, obj)
    {
        READWRITE(obj.blockhash, Using<VectorFormatter<DifferenceFormatter>>(obj.indexes));
    }
};

class BlockTransactions {
public:
    // A BlockTransactions message
    uint256 blockhash;
    std::vector<CTransactionRef> txn;

    BlockTransactions() {}
    explicit BlockTransactions(const BlockTransactionsRequest& req) :
        blockhash(req.blockhash), txn(req.indexes.size()) {}

    SERIALIZE_METHODS(BlockTransactions, obj)
    {
        READWRITE(obj.blockhash, Using<VectorFormatter<TransactionCompression>>(obj.txn));
    }
};

// Dumb serialization/storage-helper for CBlockHeaderAndShortTxIDs and PartiallyDownloadedBlock
struct PrefilledTransaction {
    // Used as an offset since last prefilled tx in CBlockHeaderAndShortTxIDs,
    // as a proper transaction-in-block-index in PartiallyDownloadedBlock
    uint16_t index;
    CTransactionRef tx;

    SERIALIZE_METHODS(PrefilledTransaction, obj) { READWRITE(COMPACTSIZE(obj.index), Using<TransactionCompression>(obj.tx)); }
};

typedef enum ReadStatus_t
{
    READ_STATUS_OK,
    READ_STATUS_INVALID, // Invalid object, peer is sending bogus crap
    READ_STATUS_FAILED, // Failed to process object
    READ_STATUS_CHECKBLOCK_FAILED, // Used only by FillBlock to indicate a
                                   // failure in CheckBlock.
} ReadStatus;

class CBlockHeaderAndShortTxIDs {
private:
    mutable uint64_t shorttxidk0, shorttxidk1;
    uint64_t nonce;

    void FillShortTxIDSelector() const;

    friend class PartiallyDownloadedBlock;

protected:
    std::vector<uint64_t> shorttxids;
    std::vector<PrefilledTransaction> prefilledtxn;

public:
    static constexpr int SHORTTXIDS_LENGTH = 6;

    CBlockHeader header;

    // Dummy for deserialization
    CBlockHeaderAndShortTxIDs() {}

    CBlockHeaderAndShortTxIDs(const CBlock& block, bool fUseWTXID);

    uint64_t GetShortID(const uint256& txhash) const;

    size_t BlockTxCount() const { return shorttxids.size() + prefilledtxn.size(); }

    SERIALIZE_METHODS(CBlockHeaderAndShortTxIDs, obj)
    {
        READWRITE(obj.header, obj.nonce, Using<VectorFormatter<CustomUintFormatter<SHORTTXIDS_LENGTH>>>(obj.shorttxids), obj.prefilledtxn);
        if (ser_action.ForRead()) {
            if (obj.BlockTxCount() > std::numeric_limits<uint16_t>::max()) {
                throw std::ios_base::failure("indexes overflowed 16 bits");
            }
            obj.FillShortTxIDSelector();
        }
    }
};

class PartiallyDownloadedBlock {
protected:
    std::vector<CTransactionRef> txn_available;
    size_t prefilled_count = 0, mempool_count = 0, extra_count = 0;
    const CTxMemPool* pool;
public:
    CBlockHeader header;
    explicit PartiallyDownloadedBlock(CTxMemPool* poolIn) : pool(poolIn) {}

    // extra_txn is a list of extra transactions to look at, in <witness hash, reference> form
    ReadStatus InitData(const CBlockHeaderAndShortTxIDs& cmpctblock, const std::vector<std::pair<uint256, CTransactionRef>>& extra_txn);
    bool IsTxAvailable(size_t index) const;
    ReadStatus FillBlock(CBlock& block, const std::vector<CTransactionRef>& vtx_missing);
};

#endif // WIDECOIN_BLOCKENCODINGS_H
