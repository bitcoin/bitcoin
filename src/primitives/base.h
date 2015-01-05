// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BASE_H
#define BITCOIN_PRIMITIVES_BASE_H

#include "amount.h"
#include "script/script.h"
#include "serialize.h"
#include "uint256.h"

class COutPointBase
{
protected:
    static const uint32_t nNullValue = -1;
public:
    uint256 hash;
    uint32_t n;

    COutPointBase() : hash(0), n(nNullValue){}
    COutPointBase(uint256 hashIn, uint32_t nIn) : hash(hashIn), n(nIn){}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(FLATDATA(*this));
    }
};

template <typename OutPointType = COutPointBase>
class CTxInBase
{
public:
    OutPointType prevout;
    CScript scriptSig;
    uint32_t nSequence;

    CTxInBase() : nSequence(std::numeric_limits<unsigned int>::max()) {}
    CTxInBase(const OutPointType& prevoutIn, const CScript& scriptSigIn, uint32_t nSequenceIn) : prevout(prevoutIn), scriptSig(scriptSigIn), nSequence(nSequenceIn){}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(prevout);
        READWRITE(scriptSig);
        READWRITE(nSequence);
    }
};

class CTxOutBase
{
protected:
    static const CAmount nValueNull = -1;
public:
    CAmount nValue;
    CScript scriptPubKey;

    CTxOutBase() : nValue(nValueNull){}
    CTxOutBase(const CAmount& nValueIn, const CScript& scriptPubKeyIn) : nValue(nValueIn), scriptPubKey(scriptPubKeyIn){}

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nValue);
        READWRITE(scriptPubKey);
    }

};

template <typename TxInType, typename TxOutType>
class CMutableTransactionBase;

template <typename TxInType = CTxInBase<>, typename TxOutType = CTxOutBase>
class CTransactionBase
{
public:
    static const int32_t CURRENT_VERSION=1;
    // The local variables are made const to prevent unintended modification
    // without updating the cached hash value. However, CTransaction is not
    // actually immutable; deserialization and assignment are implemented,
    // and bypass the constness. This is safe, as they update the entire
    // structure, including the hash.
    const int32_t nVersion;
    const std::vector<TxInType> vin;
    const std::vector<TxOutType> vout;
    const uint32_t nLockTime;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(*const_cast<int32_t*>(&this->nVersion));
        READWRITE(*const_cast<std::vector<TxInType>*>(&vin));
        READWRITE(*const_cast<std::vector<TxOutType>*>(&vout));
        READWRITE(*const_cast<uint32_t*>(&nLockTime));
    }
    CTransactionBase() : nVersion(CURRENT_VERSION), vin(), vout(), nLockTime(0) { }
    CTransactionBase(int32_t nVersionIn, const std::vector<TxInType> vinIn, const std::vector<TxOutType> voutIn, uint32_t nLockTimeIn)
    : nVersion(nVersionIn), vin(vinIn), vout(voutIn), nLockTime(nLockTimeIn){}
    CTransactionBase(const CMutableTransactionBase<TxInType,TxOutType>& in);
};

template <typename TxInType = CTxInBase<>, typename TxOutType = CTxOutBase>
class CMutableTransactionBase
{
public:
    int32_t nVersion;
    std::vector<TxInType> vin;
    std::vector<TxOutType> vout;
    uint32_t nLockTime;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(vin);
        READWRITE(vout);
        READWRITE(nLockTime);
    }
    CMutableTransactionBase() : nVersion(CTransactionBase<TxInType,TxOutType>::CURRENT_VERSION), vin(), vout(), nLockTime(0) { }
    CMutableTransactionBase(int32_t nVersionIn, const std::vector<TxInType> vinIn, const std::vector<TxOutType> voutIn, uint32_t nLockTimeIn)
    : nVersion(nVersionIn), vin(vinIn), vout(voutIn), nLockTime(nLockTimeIn){}
    CMutableTransactionBase(const CTransactionBase<TxInType,TxOutType>& in)
    : nVersion(in.nVersion), vin(in.vin), vout(in.vout), nLockTime(in.nLockTime){}
};

template <typename TxInType, typename TxOutType>
CTransactionBase<TxInType,TxOutType>::CTransactionBase(const CMutableTransactionBase<TxInType,TxOutType>& in)
  : nVersion(in.nVersion), vin(in.vin), vout(in.vout), nLockTime(in.nLockTime){}


class CBlockHeaderBase
{
public:
    // header
    static const int32_t CURRENT_VERSION=2;
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;

    CBlockHeaderBase() : nVersion(CBlockHeaderBase::CURRENT_VERSION), hashPrevBlock(0), hashMerkleRoot(0), nTime(0), nBits(0), nNonce(0){}
    CBlockHeaderBase(int32_t nVersionIn, uint256 hashPrevBlockIn, uint256 hashMerkleRootIn, uint32_t nTimeIn, uint32_t nBitsIn, uint32_t nNonceIn)
    : nVersion(nVersionIn), hashPrevBlock(hashPrevBlockIn), hashMerkleRoot(hashMerkleRootIn), nTime(nTimeIn), nBits(nBitsIn), nNonce(nNonceIn){}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
    }
};

template <typename BlockHeaderType = CBlockHeaderBase, typename TransactionType = CTransactionBase<> >
class CBlockBase : public BlockHeaderType
{
public:
    // network and disk
    std::vector<TransactionType> vtx;

    CBlockBase(){}
    CBlockBase(const BlockHeaderType &header) : BlockHeaderType(header) {}
    CBlockBase(const BlockHeaderType &header, const std::vector<TransactionType>& vtxIn) : BlockHeaderType(header), vtx(vtxIn) {}
    CBlockBase(const std::vector<TransactionType>& vtxIn) : vtx(vtxIn) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(*(BlockHeaderType*)this);
        READWRITE(vtx);
    }
};

class CBlockLocatorBase
{
public:
    std::vector<uint256> vHave;

    CBlockLocatorBase() {}
    CBlockLocatorBase(const std::vector<uint256>& vHaveIn) : vHave(vHaveIn){}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    }
};

#endif // BITCOIN_PRIMITIVES_BASE_H
