// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_SIGN_H
#define BITCOIN_SCRIPT_SIGN_H

#include <script/interpreter.h>

class CKey;
class CKeyID;
class CScript;
class CScriptID;
class CTransaction;

struct CMutableTransaction;

/** An interface to be implemented by keystores that support signing. */
class SigningProvider
{
public:
    virtual ~SigningProvider() {}
    virtual bool GetCScript(const CScriptID &scriptid, CScript& script) const =0;
    virtual bool GetPubKey(const CKeyID &address, CPubKey& pubkey) const =0;
    virtual bool GetKey(const CKeyID &address, CKey& key) const =0;
};

/** Virtual base class for signature creators. */
class BaseSignatureCreator {
protected:
    const SigningProvider* m_provider;

public:
    explicit BaseSignatureCreator(const SigningProvider* provider) : m_provider(provider) {}
    const SigningProvider& Provider() const { return *m_provider; }
    virtual ~BaseSignatureCreator() {}
    virtual const BaseSignatureChecker& Checker() const =0;

    /** Create a singular (non-script) signature. */
    virtual bool CreateSig(std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const =0;
};

/** A signature creator for transactions. */
class TransactionSignatureCreator : public BaseSignatureCreator {
    const CTransaction* txTo;
    unsigned int nIn;
    int nHashType;
    CAmount amount;
    const TransactionSignatureChecker checker;

public:
    TransactionSignatureCreator(const SigningProvider* provider, const CTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, int nHashTypeIn=SIGHASH_ALL);
    const BaseSignatureChecker& Checker() const override { return checker; }
    bool CreateSig(std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const override;
};

class MutableTransactionSignatureCreator : public TransactionSignatureCreator {
    CTransaction tx;

public:
    MutableTransactionSignatureCreator(const SigningProvider* provider, const CMutableTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, int nHashTypeIn) : TransactionSignatureCreator(provider, &tx, nInIn, amountIn, nHashTypeIn), tx(*txToIn) {}
};

/** A signature creator that just produces 72-byte empty signatures. */
class DummySignatureCreator : public BaseSignatureCreator {
public:
    explicit DummySignatureCreator(const SigningProvider* provider) : BaseSignatureCreator(provider) {}
    const BaseSignatureChecker& Checker() const override;
    bool CreateSig(std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const override;
};

struct SignatureData {
    CScript scriptSig;
    CScriptWitness scriptWitness;

    SignatureData() {}
    explicit SignatureData(const CScript& script) : scriptSig(script) {}
};

/** Produce a script signature using a generic signature creator. */
bool ProduceSignature(const BaseSignatureCreator& creator, const CScript& scriptPubKey, SignatureData& sigdata);

/** Produce a script signature for a transaction. */
bool SignSignature(const SigningProvider &provider, const CScript& fromPubKey, CMutableTransaction& txTo, unsigned int nIn, const CAmount& amount, int nHashType);
bool SignSignature(const SigningProvider &provider, const CTransaction& txFrom, CMutableTransaction& txTo, unsigned int nIn, int nHashType);

/** Combine two script signatures using a generic signature checker, intelligently, possibly with OP_0 placeholders. */
SignatureData CombineSignatures(const CScript& scriptPubKey, const BaseSignatureChecker& checker, const SignatureData& scriptSig1, const SignatureData& scriptSig2);

/** Extract signature data from a transaction, and insert it. */
SignatureData DataFromTransaction(const CMutableTransaction& tx, unsigned int nIn);
void UpdateTransaction(CMutableTransaction& tx, unsigned int nIn, const SignatureData& data);
void UpdateInput(CTxIn& input, const SignatureData& data);

/* Check whether we know how to sign for an output like this, assuming we
 * have all private keys. While this function does not need private keys, the passed
 * provider is used to look up public keys and redeemscripts by hash.
 * Solvability is unrelated to whether we consider this output to be ours. */
bool IsSolvable(const SigningProvider& provider, const CScript& script);

#endif // BITCOIN_SCRIPT_SIGN_H
