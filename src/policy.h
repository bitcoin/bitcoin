// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_H
#define BITCOIN_POLICY_H

class CCoinsViewCache;
class CTransaction;
class CTxMemPool;
class CTxMemPoolEntry;
class CValidationState;

class CNodePolicyBase
{
public:
    virtual bool AcceptTxPoolPreInputs(CTxMemPool&, CValidationState&, const CTransaction&) = 0;
    virtual bool AcceptTxWithInputs(CTxMemPool&, CValidationState&, const CTransaction&, CCoinsViewCache&) = 0;
    virtual bool AcceptMemPoolEntry(CTxMemPool&, CValidationState&, CTxMemPoolEntry&, CCoinsViewCache&, bool& fRateLimit) = 0;
    virtual bool RateLimitTx(CTxMemPool&, CValidationState&, CTxMemPoolEntry&, CCoinsViewCache&) = 0;
};

class CNodePolicy : CNodePolicyBase
{
public:
    bool fRequireStandardTx;

    CNodePolicy() : fRequireStandardTx(true) { };

    virtual bool AcceptTxPoolPreInputs(CTxMemPool&, CValidationState&, const CTransaction&);
    virtual bool AcceptTxWithInputs(CTxMemPool&, CValidationState&, const CTransaction&, CCoinsViewCache&);
    virtual bool AcceptMemPoolEntry(CTxMemPool&, CValidationState&, CTxMemPoolEntry&, CCoinsViewCache&, bool& fRateLimit);
    virtual bool RateLimitTx(CTxMemPool&, CValidationState&, CTxMemPoolEntry&, CCoinsViewCache&);
};

#endif // BITCOIN_POLICY_H
