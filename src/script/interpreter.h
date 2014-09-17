// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef H_BITCOIN_SCRIPT_INTERPRETER
#define H_BITCOIN_SCRIPT_INTERPRETER

#include <vector>
#include <stdint.h>
#include <string>

class CScript;
class CTransaction;
class uint256;

/** Signature hash types/flags */
enum
{
    SIGHASH_ALL = 1,
    SIGHASH_NONE = 2,
    SIGHASH_SINGLE = 3,
    SIGHASH_ANYONECANPAY = 0x80,
};

/** Script verification flags */
enum
{
    SCRIPT_VERIFY_NONE      = 0,
    SCRIPT_VERIFY_P2SH      = (1U << 0), // evaluate P2SH (BIP16) subscripts
    SCRIPT_VERIFY_STRICTENC = (1U << 1), // enforce strict conformance to DER and SEC2 for signatures and pubkeys
    SCRIPT_VERIFY_LOW_S     = (1U << 2), // enforce low S values (<n/2) in signatures (depends on STRICTENC)
    SCRIPT_VERIFY_NOCACHE   = (1U << 3), // do not store results in signature cache (but do query it)
    SCRIPT_VERIFY_NULLDUMMY = (1U << 4), // verify dummy stack item consumed by CHECKMULTISIG is of zero-length
};

bool IsCanonicalPubKey(const std::vector<unsigned char> &vchPubKey, unsigned int flags);
bool IsCanonicalSignature(const std::vector<unsigned char> &vchSig, unsigned int flags);

uint256 SignatureHash(const CScript &scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType);
bool CheckSig(std::vector<unsigned char> vchSig, const std::vector<unsigned char> &vchPubKey, const CScript &scriptCode, const CTransaction& txTo, unsigned int nIn, int flags);
bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, const CTransaction& txTo, unsigned int nIn, unsigned int flags);
bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn, unsigned int flags);

#endif // H_BITCOIN_SCRIPT_INTERPRETER
