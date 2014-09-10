// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef H_BITCOIN_SCRIPT_SIGCACHE
#define H_BITCOIN_SCRIPT_SIGCACHE

#include "script/interpreter.h"

#include <vector>

class CPubKey;

class CachingSignatureChecker : public SignatureChecker
{
public:
    CachingSignatureChecker(const CTransaction& txToIn, unsigned int nInIn) : SignatureChecker(txToIn, nInIn) {}

    bool VerifySignature(const std::vector<unsigned char>& vchSig, const CPubKey& vchPubKey, const uint256& sighash, int flags) const;
};

// Wrappers using a default SignatureChecker.
bool inline EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, const CTransaction& txTo, unsigned int nIn, unsigned int flags)
{
    return EvalScript(stack, script, flags, CachingSignatureChecker(txTo, nIn));
}

bool inline VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn, unsigned int flags)
{
    return VerifyScript(scriptSig, scriptPubKey, flags, CachingSignatureChecker(txTo, nIn));
}

#endif
