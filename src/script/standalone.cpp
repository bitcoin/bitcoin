// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "script/interpreter.h"
#include "script/script.h"
#include "script/transaction_serializer.hpp"
#include "uint256.h"
#include "util.h"

namespace {

class COutPoint
{
public:
    uint256 hash;
    uint32_t n;
    COutPoint() { hash = 0; n = (uint32_t) -1; }
    IMPLEMENT_SERIALIZE( READWRITE(FLATDATA(*this)); )
};

class COutPoint;

class CTxIn
{
public:
    COutPoint prevout;
    uint32_t nSequence;
    CTxIn() {}
};

class CTxOut
{
public:
    int64_t nValue;
    CScript scriptPubKey;

    CTxOut()
    {
        nValue = -1;
        scriptPubKey.clear();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(nValue);
        READWRITE(scriptPubKey);
    )
};

class CTransaction
{
public:
    static const int32_t CURRENT_VERSION=1;
    const int32_t nVersion;
    const std::vector<CTxIn> vin;
    const std::vector<CTxOut> vout;
    const uint32_t nLockTime;
    CTransaction() : nVersion(CTransaction::CURRENT_VERSION), vin(), vout(), nLockTime(0) { }
};

class CTxSignatureSerializer : public CSignatureSerializer 
{
private:
    const CTransaction& txTo;  // reference to the spending transaction (the one being serialized)
    const unsigned int nIn;    // input index of txTo being signed
public:
    CTxSignatureSerializer(const CTransaction& txToIn, unsigned int nInIn) :
        txTo(txToIn), nIn(nInIn) {}
    virtual uint256 SignatureHash(const CScript& scriptCode, int nHashType) const;
};

uint256 CTxSignatureSerializer::SignatureHash(const CScript& scriptCode, int nHashType) const
{
    if (nIn >= txTo.vin.size()) {
        LogPrintf("ERROR: SignatureHash() : nIn=%d out of range\n", nIn);
        return 1;
    }

    // Check for invalid use of SIGHASH_SINGLE
    if ((nHashType & 0x1f) == SIGHASH_SINGLE) {
        if (nIn >= txTo.vout.size()) {
            LogPrintf("ERROR: SignatureHash() : nOut=%d out of range\n", nIn);
            return 1;
        }
    }

    // Wrapper to serialize only the necessary parts of the transaction being signed
    CTransactionSignatureSerializer<CTransaction, CTxOut> txTmp(txTo, scriptCode, nIn, nHashType);

    // Serialize and hash
    CHashWriter ss(SER_GETHASH, 0);
    ss << txTmp << nHashType;
    return ss.GetHash();
}

} // anon namespace

bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn, unsigned int flags)
{
    CTxSignatureSerializer tx(txTo, nIn);
    return VerifyScript(scriptSig, scriptPubKey, tx, flags);
}
