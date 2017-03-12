#pragma once

#include <boost/variant.hpp>
#include "script/interpreter.h"
#include "pubkey.h" // CKeyID
#include "ScriptID.h"

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
*  A CTxDestination is the internal data type encoded in a CBitcoinAddress
*/
class CTxDestination : public boost::variant<CNoDestination, CKeyID, CScriptID> {
public:
    typedef boost::variant<CNoDestination, CKeyID, CScriptID> Super;
public:
    CTxDestination() {}
    CTxDestination(const CNoDestination& rhs) : Super(rhs) {}
    CTxDestination(const CKeyID& rhs) : Super(rhs) {}
    CTxDestination(const CScriptID& rhs) : Super(rhs) {}

    bool operator <  (const CTxDestination& _rhs) const {
        const CTxDestination& lhs = *this;
        const CTxDestination& rhs = _rhs;
        return lhs < rhs;
    }
    bool operator == (const CTxDestination& _rhs) const {
        const CTxDestination& lhs = *this;
        const CTxDestination& rhs = _rhs;
        return lhs == rhs;
    }
public:
    base58string GetBase58address33() const;
};
