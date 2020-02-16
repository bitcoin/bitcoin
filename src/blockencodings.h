// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BLOCKENCODINGS_H
#define BITCOIN_BLOCKENCODINGS_H

#include <primitives/block.h>


class CTxMemPool;

// Dumb helper to handle CTransaction compression at serialize-time
struct TransactionCompressor {
private:
    CTransactionRef& tx;
public:
    explicit TransactionCompressor(CTransactionRef& txIn) : tx(txIn) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(tx); //TODO: Compress tx encoding
    }
};

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

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(blockhash);
        uint64_t indexes_size = (uint64_t)indexes.size();
        READWRITE(COMPACTSIZE(indexes_size));
        if (ser_action.ForRead()) {
            size_t i = 0;
            while (indexes.size() < indexes_size) {
                indexes.resize(std::min((uint64_t)(1000 + indexes.size()), indexes_size));
                for (; i < indexes.size(); i++) {
                    uint64_t index = 0;
                    READWRITE(COMPACTSIZE(index));
                    if (index > std::numeric_limits<uint16_t>::max())
                        throw std::ios_base::failure("index overflowed 16 bits");
                    indexes[i] = index;
                }
            }

            int32_t offset = 0;
            for (size_t j = 0; j < indexes.size(); j++) {
                if (int32_t(indexes[j]) + offset > std::numeric_limits<uint16_t>::max())
                    throw std::ios_base::failure("indexes overflowed 16 bits");
                indexes[j] = indexes[j] + offset;
                offset = int32_t(indexes[j]) + 1;
            }
        } else {
            for (size_t i = 0; i < indexes.size(); i++) {
                uint64_t index = indexes[i] - (i == 0 ? 0 : (indexes[i - 1] + 1));
                READWRITE(COMPACTSIZE(index));
            }
        }
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

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(blockhash);
        uint64_t txn_size = (uint64_t)txn.size();
        READWRITE(COMPACTSIZE(txn_size));
        if (ser_action.ForRead()) {
            size_t i = 0;
            while (txn.size() < txn_size) {
                txn.resize(std::min((uint64_t)(1000 + txn.size()), txn_size));
                for (; i < txn.size(); i++)
                    READWRITE(TransactionCompressor(txn[i]));
            }
        } else {
            for (size_t i = 0; i < txn.size(); i++)
                READWRITE(TransactionCompressor(txn[i]));
        }
    }
};

// Dumb serialization/storage-helper for CBlockHeaderAndShortTxIDs and PartiallyDownloadedBlock
struct PrefilledTransaction {
    // Used as an offset since last prefilled tx in CBlockHeaderAndShortTxIDs,
    // as a proper transaction-in-block-index in PartiallyDownloadedBlock
    uint16_t index;
    CTransactionRef tx;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        uint64_t idx = index;
        READWRITE(COMPACTSIZE(idx));
        if (idx > std::numeric_limits<uint16_t>::max())
            throw std::ios_base::failure("index overflowed 16-bits");
        index = idx;
        READWRITE(TransactionCompressor(tx));
    }
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

    static const int SHORTTXIDS_LENGTH = 6;
protected:
    std::vector<uint64_t> shorttxids;
    std::vector<PrefilledTransaction> prefilledtxn;

public:
    CBlockHeader header;

    // Dummy for deserialization
    CBlockHeaderAndShortTxIDs() {}

    CBlockHeaderAndShortTxIDs(const CBlock& block, bool fUseWTXID);

    uint64_t GetShortID(const uint256& txhash) const;

    size_t BlockTxCount() const { return shorttxids.size() + prefilledtxn.size(); }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(header);
        READWRITE(nonce);

        uint64_t shorttxids_size = (uint64_t)shorttxids.size();
        READWRITE(COMPACTSIZE(shorttxids_size));
        if (ser_action.ForRead()) {
            size_t i = 0;
            while (shorttxids.size() < shorttxids_size) {
                shorttxids.resize(std::min((uint64_t)(1000 + shorttxids.size()), shorttxids_size));
                for (; i < shorttxids.size(); i++) {
                    uint32_t lsb = 0; uint16_t msb = 0;
                    READWRITE(lsb);
                    READWRITE(msb);
                    shorttxids[i] = (uint64_t(msb) << 32) | uint64_t(lsb);
                    static_assert(SHORTTXIDS_LENGTH == 6, "shorttxids serialization assumes 6-byte shorttxids");
                }
            }
        } else {
            for (size_t i = 0; i < shorttxids.size(); i++) {
                uint32_t lsb = shorttxids[i] & 0xffffffff;
                uint16_t msb = (shorttxids[i] >> 32) & 0xffff;
                READWRITE(lsb);
                READWRITE(msb);
            }
        }

        READWRITE(prefilledtxn);

        if (BlockTxCount() > std::numeric_limits<uint16_t>::max())
            throw std::ios_base::failure("indexes overflowed 16 bits");

        if (ser_action.ForRead())
            FillShortTxIDSelector();
    }
};

class PartiallyDownloadedBlock {
protected:
    std::vector<CTransactionRef> txn_available;
    size_t prefilled_count = 0, mempool_count = 0, extra_count = 0;
    CTxMemPool* pool;
public:
    CBlockHeader header;
    explicit PartiallyDownloadedBlock(CTxMemPool* poolIn) : pool(poolIn) {}

    // extra_txn is a list of extra transactions to look at, in <witness hash, reference> form
    ReadStatus InitData(const CBlockHeaderAndShortTxIDs& cmpctblock, const std::vector<std::pair<uint256, CTransactionRef>>& extra_txn);
    bool IsTxAvailable(size_t index) const;
    ReadStatus FillBlock(CBlock& block, const std::vector<CTransactionRef>& vtx_missing);
};

#endif // BITCOIN_BLOCKENCODINGS_H
