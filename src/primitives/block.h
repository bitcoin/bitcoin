// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader
{
public:
    // header
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;

    CBlockHeader()
    {
        SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(this->nVersion);
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
    }

    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    uint256 GetHash() const;

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
};


class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransactionRef> vtx;

    // memory only
    mutable bool fChecked;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *(static_cast<CBlockHeader*>(this)) = header;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITEAS(CBlockHeader, *this);
        READWRITE(vtx);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        return block;
    }

    /**
     * Get the vout index of the segwit commitment in the coinbase transaction of this block.
     *
     * Returns -1 if no witness commitment was found.
     */
    inline int GetWitnessCommitmentIndex() const
    {
        return vtx.empty() ? -1 : GetWitnessCommitmentIndex(*vtx.at(0));
    }

    /**
     * Get the vout index of the segwit commitment in the given coinbase transaction.
     *
     * Returns -1 if no witness commitment was found.
     */
    template<typename T> static inline int GetWitnessCommitmentIndex(const T& coinbase) {
        for (int64_t o = coinbase.vout.size() - 1; o > -1; --o) {
            auto vospk = coinbase.vout[o].scriptPubKey;
            if (vospk.size() >= 38 && vospk[0] == OP_RETURN && vospk[1] == 0x24 && vospk[2] == 0xaa && vospk[3] == 0x21 && vospk[4] == 0xa9 && vospk[5] == 0xed) {
                return o;
            }
        }
        return -1;
    }

    /**
     * Attempt to get the data for the section with the given header in the witness commitment of this block.
     *
     * Returns false if header was not found. The data (excluding the 4 byte header) is written into result if found.
     */
    bool GetWitnessCommitmentSection(const uint8_t header[4], std::vector<uint8_t>& result) const;

    /**
     * Attempt to add or update the data for the section with the given header in the witness commitment of this block.
     *
     * This operation may fail and return false, if no witness commitment exists upon call time. Returns true on success.
     */
    bool SetWitnessCommitmentSection(const uint8_t header[4], const std::vector<uint8_t>& data);

    /**
     * The tx based equivalent of the above.
     */
    static bool SetWitnessCommitmentSection(CMutableTransaction& tx, const uint8_t header[4], const std::vector<uint8_t>& data);

    std::string ToString() const;
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    explicit CBlockLocator(const std::vector<uint256>& vHaveIn) : vHave(vHaveIn) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

#endif // BITCOIN_PRIMITIVES_BLOCK_H
