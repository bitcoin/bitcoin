// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BLOCKENCODINGS_H
#define BITCOIN_BLOCKENCODINGS_H

#include <primitives/block.h>


class CTxMemPool;

// Dumb helper to handle CTransaction compression at serialize-time
struct TransactionCompression
{
    template<typename T>
    struct Wrapper {
    private:
        T& tx;
    public:
        explicit Wrapper(T& txIn) : tx(txIn) {}
        SERIALIZE_METHODS(Wrapper, obj) { READWRITE(obj.tx); }
    };
};

struct Uint48
{
    template<typename I>
    struct Wrapper {
    private:
        I& m_int;
    public:
        explicit Wrapper(I& i) : m_int(i)
        {
            static_assert(std::is_unsigned<I>::value, "Uint48 needs an unsigned integer");
            static_assert(sizeof(I) >= 6, "Uint48 needs a 48+ bit type");
        }

        template <typename Stream> void Serialize(Stream& s) const
        {
            uint32_t lsb = m_int & 0xffffffff;
            uint16_t msb = (m_int >> 32) & 0xffff;
            s << lsb << msb;
        }

        template <typename Stream> void Unserialize(Stream& s)
        {
            uint32_t lsb;
            uint16_t msb;
            s >> lsb >> msb;
            m_int = (uint64_t(msb) << 32) | uint64_t(lsb);
        }
    };
};

/** Vector-wrapper (compatible with VectorApply) to differentially encode values. */
struct Differential
{
    template<typename V>
    struct Wrapper {
    private:
        V& m_v;

    public:
        typedef typename V::value_type value_type;

        explicit Wrapper(V& v) : m_v(v) {}
        size_t size() const { return m_v.size(); }
        void clear() { m_v.clear(); }
        void reserve(size_t size) { m_v.reserve(size); }

        value_type operator[](size_t pos) const
        {
            if (pos == 0) return m_v[0];
            return m_v[pos] - (m_v[pos - 1] + 1);
        }

        void push_back(value_type val)
        {
            if (m_v.size() == 0) {
                m_v.push_back(val);
            } else {
                value_type add = val + (m_v.back() + 1);
                if (add <= val) throw std::ios_base::failure("differential value overflow");
                m_v.push_back(add);
            }
        }
    };
};

class BlockTransactionsRequest {
public:
    // A BlockTransactionsRequest message
    uint256 blockhash;
    std::vector<uint16_t> indexes;

    SERIALIZE_METHODS(BlockTransactionsRequest, obj)
    {
        READWRITE(obj.blockhash, Wrap<VectorApply<CompactSize>>(Wrap<Differential>(obj.indexes)));
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
        READWRITE(obj.blockhash, Wrap<VectorApply<TransactionCompression>>(obj.txn));
    }
};

// Dumb serialization/storage-helper for CBlockHeaderAndShortTxIDs and PartiallyDownloadedBlock
struct PrefilledTransaction {
    // Used as an offset since last prefilled tx in CBlockHeaderAndShortTxIDs,
    // as a proper transaction-in-block-index in PartiallyDownloadedBlock
    uint16_t index;
    CTransactionRef tx;

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        s << COMPACTSIZE(index) << Wrap<TransactionCompression>(tx);
    }

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        uint64_t idx;
        s >> COMPACTSIZE(idx);
        if (idx > std::numeric_limits<uint16_t>::max())
            throw std::ios_base::failure("index overflowed 16-bits");
        index = idx;
        s >> Wrap<TransactionCompression>(tx);
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

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << header << nonce << Wrap<VectorApply<Uint48>>(shorttxids) << prefilledtxn;
    }

    template <typename Stream>
    inline void Unserialize(Stream& s)
    {
        static_assert(SHORTTXIDS_LENGTH == 6, "shorttxids serialization assumes 6-byte shorttxids");
        s >> header >> nonce >> Wrap<VectorApply<Uint48>>(shorttxids) >> prefilledtxn;
        if (BlockTxCount() > std::numeric_limits<uint16_t>::max()) {
            throw std::ios_base::failure("indexes overflowed 16 bits");
        }
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
