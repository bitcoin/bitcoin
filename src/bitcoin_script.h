// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef H_BITCOIN_BITCOIN_SCRIPT
#define H_BITCOIN_BITCOIN_SCRIPT

#include "key.h"
#include "util.h"

#include "script.h"

#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/variant.hpp>

class CKeyStore;
class Bitcoin_CTransaction;

bool Bitcoin_EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, const Bitcoin_CTransaction& txTo, unsigned int nIn, unsigned int flags, int nHashType);
bool Bitcoin_SignSignature(const CKeyStore& keystore, const CScript& fromPubKey, Bitcoin_CTransaction& txTo, unsigned int nIn, int nHashType=SIGHASH_ALL);
bool Bitcoin_SignSignature(const CKeyStore& keystore, const Bitcoin_CTransaction& txFrom, Bitcoin_CTransaction& txTo, unsigned int nIn, int nHashType=SIGHASH_ALL);
bool Bitcoin_VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const Bitcoin_CTransaction& txTo, unsigned int nIn, unsigned int flags, int nHashType);

// Given two sets of signatures for scriptPubKey, possibly with OP_0 placeholders,
// combine them intelligently and return the result.
CScript Bitcoin_CombineSignatures(CScript scriptPubKey, const Bitcoin_CTransaction& txTo, unsigned int nIn, const CScript& scriptSig1, const CScript& scriptSig2);

#endif
