// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BLOCK_ENCODINGS_H
#define BITCOIN_BLOCK_ENCODINGS_H

#include "primitives/block.h"

#include <memory>

class CTxMemPool;

// Dumb helper to handle CTransaction compression at serialize-time
struct TransactionCompressor {
private:
    CTransactionRef& tx;
public:
    TransactionCompressor(CTransactionRef& txIn) : tx(txIn) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(tx); //TODO: Compress tx encoding
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

            uint16_t offset = 0;
            for (size_t j = 0; j < indexes.size(); j++) {
                if (uint64_t(indexes[j]) + uint64_t(offset) > std::numeric_limits<uint16_t>::max())
                    throw std::ios_base::failure("indexes overflowed 16 bits");
                indexes[j] = indexes[j] + offset;
                offset = indexes[j] + 1;
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
    BlockTransactions(const BlockTransactionsRequest& req) :
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
                    READWRITE(REF(TransactionCompressor(txn[i])));
            }
        } else {
            for (size_t i = 0; i < txn.size(); i++)
                READWRITE(REF(TransactionCompressor(txn[i])));
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
        READWRITE(REF(TransactionCompressor(tx)));
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

    CBlockHeaderAndShortTxIDs(const CBlock& block);

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
    PartiallyDownloadedBlock(CTxMemPool* poolIn) : pool(poolIn) {}

    // extra_txn is a list of extra transactions to look at, in <hash, reference> form
    ReadStatus InitData(const CBlockHeaderAndShortTxIDs& cmpctblock, const std::vector<std::pair<uint256, CTransactionRef>>& extra_txn);
    bool IsTxAvailable(size_t index) const;
    ReadStatus FillBlock(CBlock& block, const std::vector<CTransactionRef>& vtx_missing);
};

#endif
