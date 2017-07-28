// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_STANDARD_H
#define BITCOIN_SCRIPT_STANDARD_H

#include "script/interpreter.h"
#include "uint256.h"
#include "pubkey.h"

#include <boost/variant.hpp>

#include <stdint.h>

static const bool DEFAULT_ACCEPT_DATACARRIER = true;

class CWitKeyID;
class CScript;

/** A reference to a CScript: the Hash160 of its serialization (see script.h) */
class CScriptID : public uint160
{
public:
    CScriptID() : uint160() {}
    CScriptID(const CScript& in);
    CScriptID(const uint160& in) : uint160(in) {}
};

/** A reference to a CScript: the SHA256 of its serialization (see script.h) */
class CWitScriptID : public uint256
{
public:
    CWitScriptID() : uint256() {}
    CWitScriptID(const uint256& in) : uint256(in) {}

    CScriptID ToP2SH() const
    {
        return CScriptID(CScript() << OP_0 << std::vector<unsigned char>(this->begin(), this->end()));
    }

    CScriptID ToScriptID() const
    {
        uint160 hash;
        auto value = std::vector<unsigned char>(this->begin(), this->end());
        CRIPEMD160().Write(&value[0], value.size()).Finalize(hash.begin());
        return CScriptID(hash);
    }
};

/** A reference to a CKey: the Hash160 of its serialized witness public key */
class CWitKeyID : public uint160
{
public:
    CWitKeyID() : uint160() {}
    CWitKeyID(const uint160& in) : uint160(in) {}

    /** keystore is indexing keys of p2wpkh by CScriptID, so we need a way to transform this CWitKeyID into CKeyID */
    CKeyID ToKeyID() const
    {
        return CKeyID(uint160(std::vector<unsigned char>(this->begin(), this->end())));
    }
    CScriptID ToP2SH() const
    {
        return CScriptID(CScript() << OP_0 << std::vector<unsigned char>(this->begin(), this->end()));
    }
};

static const unsigned int MAX_OP_RETURN_RELAY = 83; //!< bytes (+1 for OP_RETURN, +2 for the pushdata opcodes)
extern bool fAcceptDatacarrier;
extern unsigned nMaxDatacarrierBytes;

/**
 * Mandatory script verification flags that all new blocks must comply with for
 * them to be valid. (but old blocks may not comply with) Currently just P2SH,
 * but in the future other flags may be added, such as a soft-fork to enforce
 * strict DER encoding.
 * 
 * Failing one of these tests may trigger a DoS ban - see CheckInputs() for
 * details.
 */
static const unsigned int MANDATORY_SCRIPT_VERIFY_FLAGS = SCRIPT_VERIFY_P2SH;

enum txnouttype
{
    TX_NONSTANDARD,
    // 'standard' transaction types:
    TX_PUBKEY,
    TX_PUBKEYHASH,
    TX_SCRIPTHASH,
    TX_MULTISIG,
    TX_NULL_DATA,
    TX_WITNESS_V0_SCRIPTHASH,
    TX_WITNESS_V0_KEYHASH,
};

class CNoDestination {
public:
    friend bool operator==(const CNoDestination &a, const CNoDestination &b) { return true; }
    friend bool operator<(const CNoDestination &a, const CNoDestination &b) { return true; }
};

/** 
 * A txout script template with a specific destination. It is either:
 *  * CNoDestination: no destination set
 *  * CKeyID: TX_PUBKEYHASH destination
 *  * CScriptID: TX_SCRIPTHASH destination
 *  * CWitKeyID: TX_WITNESS_V0_KEYHASH destination
 *  * CWitScriptID: TX_WITNESS_V0_SCRIPTHASH destination
 *  A CTxDestination is the internal data type encoded in a CBitcoinAddress
 */
typedef boost::variant<CNoDestination, CKeyID, CScriptID, CWitKeyID, CWitScriptID> CTxDestination;

const char* GetTxnOutputType(txnouttype t);

bool Solver(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<std::vector<unsigned char> >& vSolutionsRet);
bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet);
bool ExtractDestinations(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<CTxDestination>& addressRet, int& nRequiredRet);

CScript GetScriptForDestination(const CTxDestination& dest);
CScript GetScriptForRawPubKey(const CPubKey& pubkey);
CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys);
CScript GetScriptForWitness(const CScript& redeemscript);

#endif // BITCOIN_SCRIPT_STANDARD_H
