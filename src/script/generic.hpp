// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef H_BITCOIN_SCRIPT_GENERIC
#define H_BITCOIN_SCRIPT_GENERIC

#include "hash.h"
#include "interpreter.h"
#include "keystore.h"
#include "pubkey.h"
#include "sign.h"

class SimpleSignatureChecker : public BaseSignatureChecker
{
public:
    uint256 hash;

    SimpleSignatureChecker(const uint256& hashIn) : hash(hashIn) {};
    bool CheckSig(const std::vector<unsigned char>& vchSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const
    {
        CPubKey pubkey(vchPubKey);
        if (!pubkey.IsValid())
            return false;
        if (vchSig.empty())
            return false;
        return pubkey.Verify(hash, vchSig);
    }
};

class SimpleSignatureCreator : public BaseSignatureCreator
{
    SimpleSignatureChecker checker;

public:
    SimpleSignatureCreator(const CKeyStore* keystoreIn, const uint256& hashIn) : BaseSignatureCreator(keystoreIn), checker(hashIn) {};
    virtual ~SimpleSignatureCreator() {};
    virtual const BaseSignatureChecker& Checker() const { return checker; }
    virtual bool CreateSig(std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const
    {
        CKey key;
        if (!keystore->GetKey(keyid, key))
            return false;
        return key.Sign(checker.hash, vchSig);
    }
};

template<typename T>
bool GenericVerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, unsigned int flags, const T& data)
{
    return VerifyScript(scriptSig, scriptPubKey, NULL, flags, SimpleSignatureChecker(SerializeHash(data)));
}

template<typename T>
bool GenericSignScript(const CKeyStore& keystore, const T& data, const CScript& scriptPubKey, SignatureData& scriptSig)
{
    return ProduceSignature(SimpleSignatureCreator(&keystore, SerializeHash(data)), scriptPubKey, scriptSig);
}

template<typename T>
SignatureData GenericCombineSignatures(const CScript& scriptPubKey, const T& data, const SignatureData& scriptSig1, const SignatureData& scriptSig2)
{
    return CombineSignatures(scriptPubKey, SimpleSignatureChecker(SerializeHash(data)), scriptSig1, scriptSig2);
}

#endif // H_BITCOIN_SCRIPT_GENERIC
