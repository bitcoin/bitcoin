// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef H_BITCOIN_SCRIPT_INTERPRETER_CORE
#define H_BITCOIN_SCRIPT_INTERPRETER_CORE

#include "script/interpreter.h"

#include <vector>

class uint256;
class CScript;
class CTransaction;

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

uint256 SignatureHash(const CScript& scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType);
bool CheckSig(std::vector<unsigned char> vchSig, const std::vector<unsigned char>& vchPubKey, const CScript &scriptCode, const CTransaction& txTo, unsigned int nIn, int flags);
bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, const CTransaction& txTo, unsigned int nIn, unsigned int flags);
bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn, unsigned int flags);

#endif
