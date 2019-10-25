// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Talkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TALKCOIN_PRIMITIVES_BLOCK_H
#define TALKCOIN_PRIMITIVES_BLOCK_H

#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>

static const int SER_WITHOUT_SIGNATURE = 1 << 3;

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
#ifdef ENABLE_MOMENTUM_HASH_ALGO
    uint32_t nBirthdayA;
    uint32_t nBirthdayB;
#endif
    COutPoint prevoutStake;
    std::vector<unsigned char> vchBlockSig;

    virtual ~CBlockHeader() = default;

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
#ifdef ENABLE_MOMENTUM_HASH_ALGO
        READWRITE(nBirthdayA);
        READWRITE(nBirthdayB);
#endif
        READWRITE(prevoutStake);
        if (!(s.GetType() & SER_WITHOUT_SIGNATURE))
            READWRITE(vchBlockSig);

    }

    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
#ifdef ENABLE_MOMENTUM_HASH_ALGO
        nBirthdayA = 0;
		nBirthdayB = 0;
#endif
        vchBlockSig.clear();
        prevoutStake.SetNull();
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }
    uint256 GetSHA256() const;
    uint256 GetMemHash() const;
    uint256 GetMomentumHash() const;
    uint256 GetVerifiedHash() const;
    uint256 CalculateBestBirthdayHash();
    uint256 GetMidHash() const;
    uint256 GetGroestlHash() const;
    uint256 GetHash() const;

    uint256 GetHashWithoutSign() const;

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
    std::string ToString() const;

    // ppcoin: two types of block: proof-of-work or proof-of-stake
    virtual bool IsProofOfStake() const 
    {
        return !prevoutStake.IsNull();
    }

    virtual bool IsProofOfWork() const
    {
        return !IsProofOfStake();
    }
    
    virtual uint32_t StakeTime() const
    {
        uint32_t ret = 0;
        if(IsProofOfStake())
        {
            ret = nTime;
        }
        return ret;
    }

    CBlockHeader& operator=(const CBlockHeader& other) 
    {
        if (this != &other)
        {
            this->nVersion       = other.nVersion;
            this->hashPrevBlock  = other.hashPrevBlock;
            this->hashMerkleRoot = other.hashMerkleRoot;
            this->nTime          = other.nTime;
            this->nBits          = other.nBits;
            this->nNonce         = other.nNonce;
#ifdef ENABLE_MOMENTUM_HASH_ALGO
            this->nBirthdayA = other.nBirthdayA;
		    this->nBirthdayB = other.nBirthdayB;
#endif
            this->vchBlockSig    = other.vchBlockSig;
            this->prevoutStake   = other.prevoutStake;
        }
        return *this;
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

    std::pair<COutPoint, unsigned int> GetProofOfStake() const 
    {
        return IsProofOfStake()? std::make_pair(prevoutStake, nTime) : std::make_pair(COutPoint(), (unsigned int)0);
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
#ifdef ENABLE_MOMENTUM_HASH_ALGO
		block.nBirthdayA     = nBirthdayA;
        block.nBirthdayB     = nBirthdayB;
#endif
        block.vchBlockSig    = vchBlockSig;
        block.prevoutStake   = prevoutStake;

        return block;
    }

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

#endif // TALKCOIN_PRIMITIVES_BLOCK_H
