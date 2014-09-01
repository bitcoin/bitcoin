// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "script/interpreter_core.h"

#include "core.h"
#include "script/script.h"
#include "script/transaction_serializer.hpp"
#include "uint256.h"
#include "util.h"

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
    CTransactionSignatureSerializer<CTransaction> txTmp(txTo, scriptCode, nIn, nHashType);

    // Serialize and hash
    CHashWriter ss(SER_GETHASH, 0);
    ss << txTmp << nHashType;
    return ss.GetHash();
}

uint256 SignatureHash(const CScript& scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType)
{
    CTxSignatureSerializer tx(txTo, nIn);
    return tx.SignatureHash(scriptCode, nHashType);
}

bool CheckSig(std::vector<unsigned char> vchSig, const std::vector<unsigned char>& vchPubKey, const CScript &scriptCode, const CTransaction& txTo, unsigned int nIn, int flags)
{
    CTxSignatureSerializer tx(txTo, nIn);
    return CheckSig(vchSig, vchPubKey, scriptCode, tx, flags);
}

bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, const CTransaction& txTo, unsigned int nIn, unsigned int flags)
{
    CTxSignatureSerializer tx(txTo, nIn);
    return EvalScript(stack, script, tx, flags);
}

bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn, unsigned int flags)
{
    CTxSignatureSerializer tx(txTo, nIn);
    return VerifyScript(scriptSig, scriptPubKey, tx, flags);
}
