// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>

#include <crypto/sha256.h>
#include <hash.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/solver.h>
#include <uint256.h>
#include <util/hash_type.h>

#include <cassert>
#include <vector>

typedef std::vector<unsigned char> valtype;

ScriptHash::ScriptHash(const CScript& in) : BaseHash(Hash160(in)) {}
ScriptHash::ScriptHash(const CScriptID& in) : BaseHash{in} {}

PKHash::PKHash(const CPubKey& pubkey) : BaseHash(pubkey.GetID()) {}
PKHash::PKHash(const CKeyID& pubkey_id) : BaseHash(pubkey_id) {}

WitnessV0KeyHash::WitnessV0KeyHash(const CPubKey& pubkey) : BaseHash(pubkey.GetID()) {}
WitnessV0KeyHash::WitnessV0KeyHash(const PKHash& pubkey_hash) : BaseHash{pubkey_hash} {}

CKeyID ToKeyID(const PKHash& key_hash)
{
    return CKeyID{uint160{key_hash}};
}

CKeyID ToKeyID(const WitnessV0KeyHash& key_hash)
{
    return CKeyID{uint160{key_hash}};
}

CScriptID ToScriptID(const ScriptHash& script_hash)
{
    return CScriptID{uint160{script_hash}};
}

WitnessV0ScriptHash::WitnessV0ScriptHash(const CScript& in)
{
    CSHA256().Write(in.data(), in.size()).Finalize(begin());
}

bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet)
{
    std::vector<valtype> vSolutions;
    TxoutType whichType = Solver(scriptPubKey, vSolutions);

    switch (whichType) {
    case TxoutType::PUBKEY: {
        CPubKey pubKey(vSolutions[0]);
        if (!pubKey.IsValid()) {
            addressRet = CNoDestination(scriptPubKey);
        } else {
            addressRet = PubKeyDestination(pubKey);
        }
        return false;
    }
    case TxoutType::PUBKEYHASH: {
        addressRet = PKHash(uint160(vSolutions[0]));
        return true;
    }
    case TxoutType::SCRIPTHASH: {
        addressRet = ScriptHash(uint160(vSolutions[0]));
        return true;
    }
    case TxoutType::WITNESS_V0_KEYHASH: {
        WitnessV0KeyHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    }
    case TxoutType::WITNESS_V0_SCRIPTHASH: {
        WitnessV0ScriptHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    }
    case TxoutType::WITNESS_V1_TAPROOT: {
        WitnessV1Taproot tap;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), tap.begin());
        addressRet = tap;
        return true;
    }
    case TxoutType::WITNESS_UNKNOWN: {
        addressRet = WitnessUnknown{vSolutions[0][0], vSolutions[1]};
        return true;
    }
    case TxoutType::MULTISIG:
    case TxoutType::NULL_DATA:
    case TxoutType::NONSTANDARD:
        addressRet = CNoDestination(scriptPubKey);
        return false;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

namespace {
class CScriptVisitor
{
public:
    CScript operator()(const V0SilentPaymentDestination& dest) const
    {
        return CScript();
    }
    CScript operator()(const CNoDestination& dest) const
    {
        return dest.GetScript();
    }

    CScript operator()(const PubKeyDestination& dest) const
    {
        return CScript() << ToByteVector(dest.GetPubKey()) << OP_CHECKSIG;
    }

    CScript operator()(const PKHash& keyID) const
    {
        return CScript() << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
    }

    CScript operator()(const ScriptHash& scriptID) const
    {
        return CScript() << OP_HASH160 << ToByteVector(scriptID) << OP_EQUAL;
    }

    CScript operator()(const WitnessV0KeyHash& id) const
    {
        return CScript() << OP_0 << ToByteVector(id);
    }

    CScript operator()(const WitnessV0ScriptHash& id) const
    {
        return CScript() << OP_0 << ToByteVector(id);
    }

    CScript operator()(const WitnessV1Taproot& tap) const
    {
        return CScript() << OP_1 << ToByteVector(tap);
    }

    CScript operator()(const WitnessUnknown& id) const
    {
        return CScript() << CScript::EncodeOP_N(id.GetWitnessVersion()) << id.GetWitnessProgram();
    }
};

class ValidDestinationVisitor
{
public:
    bool operator()(const CNoDestination& dest) const { return false; }
    bool operator()(const PubKeyDestination& dest) const { return false; }
    bool operator()(const PKHash& dest) const { return true; }
    bool operator()(const ScriptHash& dest) const { return true; }
    // silent payment addresses are not valid until sending support has been implemented
    // TODO: set this to true once sending is implemented
    bool operator()(const V0SilentPaymentDestination& dest) const { return false; }
    bool operator()(const WitnessV0KeyHash& dest) const { return true; }
    bool operator()(const WitnessV0ScriptHash& dest) const { return true; }
    bool operator()(const WitnessV1Taproot& dest) const { return true; }
    bool operator()(const WitnessUnknown& dest) const { return true; }
};
} // namespace

CScript GetScriptForDestination(const CTxDestination& dest)
{
    return std::visit(CScriptVisitor(), dest);
}

bool IsValidDestination(const CTxDestination& dest) {
    return std::visit(ValidDestinationVisitor(), dest);
}
