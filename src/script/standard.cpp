// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/standard.h>

#include <key/extkey.h>
#include <key/stealth.h>
#include <pubkey.h>
#include <script/script.h>
#include <util.h>
#include <utilstrencodings.h>


typedef std::vector<unsigned char> valtype;

bool fAcceptDatacarrier = DEFAULT_ACCEPT_DATACARRIER;
unsigned nMaxDatacarrierBytes = MAX_OP_RETURN_RELAY;

CScriptID::CScriptID(const CScript& in) : uint160(Hash160(in.begin(), in.end())) {}

bool CScriptID::Set(const uint256& in)
{
    CRIPEMD160().Write(in.begin(), 32).Finalize(this->begin());
    return true;
};

bool CScriptID256::Set(const CScript& in)
{
    *this = HashSha256(in.begin(), in.end());
    return true;
};

const char* GetTxnOutputType(txnouttype t)
{
    switch (t)
    {
    case TX_NONSTANDARD: return "nonstandard";
    case TX_PUBKEY: return "pubkey";
    case TX_PUBKEYHASH: return "pubkeyhash";
    case TX_SCRIPTHASH: return "scripthash";
    case TX_MULTISIG: return "multisig";

    case TX_NULL_DATA: return "nulldata";
    case TX_WITNESS_V0_KEYHASH: return "witness_v0_keyhash";
    case TX_WITNESS_V0_SCRIPTHASH: return "witness_v0_scripthash";
    case TX_WITNESS_UNKNOWN: return "witness_unknown";

    case TX_SCRIPTHASH256: return "scripthash256";
    case TX_PUBKEYHASH256: return "pubkeyhash256";
    case TX_TIMELOCKED_SCRIPTHASH: return "timelocked_scripthash";
    case TX_TIMELOCKED_PUBKEYHASH: return "timelocked_pubkeyhash";
    case TX_TIMELOCKED_MULTISIG: return "timelocked_multisig";
    case TX_CONDITIONAL_STAKE: return "conditional_stake";
    }
    return nullptr;
}

bool Solver(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<std::vector<unsigned char> >& vSolutionsRet)
{
    // Templates
    static std::multimap<txnouttype, CScript> mTemplates;
    if (mTemplates.empty())
    {
        // Standard tx, sender provides pubkey, receiver adds signature
        mTemplates.insert(std::make_pair(TX_PUBKEY, CScript() << OP_PUBKEY << OP_CHECKSIG));

        // Bitcoin address tx, sender provides hash of pubkey, receiver provides signature and pubkey
        mTemplates.insert(std::make_pair(TX_PUBKEYHASH, CScript() << OP_DUP << OP_HASH160 << OP_PUBKEYHASH << OP_EQUALVERIFY << OP_CHECKSIG));

        // Sender provides N pubkeys, receivers provides M signatures
        mTemplates.insert(std::make_pair(TX_MULTISIG, CScript() << OP_SMALLINTEGER << OP_PUBKEYS << OP_SMALLINTEGER << OP_CHECKMULTISIG));

        //mTemplates.insert(make_pair(TX_TIMELOCKED_PUBKEYHASH, CScript() << OP_INTEGER << OP_CHECKLOCKTIMEVERIFY << OP_DROP << OP_DUP << OP_HASH160 << OP_PUBKEYHASH << OP_EQUALVERIFY << OP_CHECKSIG));

        mTemplates.insert(std::make_pair(TX_PUBKEYHASH256, CScript() << OP_DUP << OP_SHA256 << OP_PUBKEYHASH256 << OP_EQUALVERIFY << OP_CHECKSIG));
    }

    vSolutionsRet.clear();

    opcodetype opcode;
    std::vector<unsigned char> vch1;
    CScript::const_iterator pc1 = scriptPubKey.begin();
    size_t k;
    for (k = 0; k < 3; ++k)
    {
        if (!scriptPubKey.GetOp(pc1, opcode, vch1))
            break;

        if (k == 0)
        {
            if (0 <= opcode && opcode <= OP_PUSHDATA4)
            {
            } else
            {
                break;
            };
        } else
        if (k == 1)
        {
            if (opcode != OP_CHECKLOCKTIMEVERIFY
                && opcode != OP_CHECKSEQUENCEVERIFY)
                break;
        } else
        if (k == 2)
        {
             if (opcode != OP_DROP)
                break;
        };
    };
    bool fIsTimeLocked = k == 3;
    CScript::const_iterator tli = pc1;

    if (fIsTimeLocked) // Check further for TX_TIMELOCKED_SCRIPTHASH
    for (k = 0; k < 3; ++k)
    {
        if (!scriptPubKey.GetOp(pc1, opcode, vch1))
            break;

        if (k == 0)
        {
            if (opcode != OP_HASH160)
                break;
        }  else
        if (k == 1)
        {
            if (vch1.size() != sizeof(uint160))
                break;
            vSolutionsRet.push_back(vch1);
        } else
        if (k == 2)
        {
            if (opcode != OP_EQUAL)
                break;
            typeRet = TX_TIMELOCKED_SCRIPTHASH;
            return true;
        }
    };

    vSolutionsRet.clear();


    if (!fIsTimeLocked)
    {
        // Shortcut for pay-to-script-hash, which are more constrained than the other types:
        // it is always OP_HASH160 20 [20 byte hash] OP_EQUAL
        if (scriptPubKey.IsPayToScriptHash())
        {
            typeRet = TX_SCRIPTHASH;
            std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
            vSolutionsRet.push_back(hashBytes);
            return true;
        }

        if (scriptPubKey.IsPayToScriptHash256())
        {
            typeRet = TX_SCRIPTHASH256;
            std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+34);
            vSolutionsRet.push_back(hashBytes);
            return true;
        }

        int witnessversion;
        std::vector<unsigned char> witnessprogram;
        if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
            if (witnessversion == 0 && witnessprogram.size() == 20) {
                typeRet = TX_WITNESS_V0_KEYHASH;
                vSolutionsRet.push_back(witnessprogram);
                return true;
            }
            if (witnessversion == 0 && witnessprogram.size() == 32) {
                typeRet = TX_WITNESS_V0_SCRIPTHASH;
                vSolutionsRet.push_back(witnessprogram);
                return true;
            }
            if (witnessversion != 0) {
                typeRet = TX_WITNESS_UNKNOWN;
                vSolutionsRet.push_back(std::vector<unsigned char>{(unsigned char)witnessversion});
                vSolutionsRet.push_back(std::move(witnessprogram));
                return true;
            }
            return false;
        }
    }

    // Provably prunable, data-carrying output
    //
    // So long as script passes the IsUnspendable() test and all but the first
    // byte passes the IsPushOnly() test we don't care what exactly is in the
    // script.
    if (scriptPubKey.size() >= 1 && scriptPubKey[0] == OP_RETURN && scriptPubKey.IsPushOnly(scriptPubKey.begin()+1)) {
        typeRet = TX_NULL_DATA;
        return true;
    }

    // Scan templates
    const CScript& script1 = scriptPubKey;
    for (const std::pair<txnouttype, CScript>& tplate : mTemplates)
    {
        const CScript& script2 = tplate.second;
        vSolutionsRet.clear();

        opcodetype opcode1, opcode2;
        std::vector<unsigned char> vch1, vch2;

        // Compare
        CScript::const_iterator pc1 = fIsTimeLocked ? tli : script1.begin();
        CScript::const_iterator pc2 = script2.begin();
        while (true)
        {
            if (pc1 == script1.end() && pc2 == script2.end())
            {
                // Found a match
                typeRet = tplate.first;
                if (typeRet == TX_MULTISIG || typeRet == TX_TIMELOCKED_MULTISIG)
                {
                    // Additional checks for TX_MULTISIG:
                    unsigned char m = vSolutionsRet.front()[0];
                    unsigned char n = vSolutionsRet.back()[0];
                    if (m < 1 || n < 1 || m > n || vSolutionsRet.size()-2 != n)
                        return false;
                }

                if (fIsTimeLocked)
                switch (typeRet)
                {
                    case TX_SCRIPTHASH: typeRet = TX_TIMELOCKED_SCRIPTHASH; break;
                    case TX_PUBKEYHASH: typeRet = TX_TIMELOCKED_PUBKEYHASH; break;
                    case TX_MULTISIG:   typeRet = TX_TIMELOCKED_MULTISIG;   break;
                    default: break;
                };

                return true;
            }
            if (!script1.GetOp(pc1, opcode1, vch1))
                break;
            if (!script2.GetOp(pc2, opcode2, vch2))
                break;

            // Template matching opcodes:
            if (opcode2 == OP_PUBKEYS)
            {
                while (vch1.size() >= 33 && vch1.size() <= 65)
                {
                    vSolutionsRet.push_back(vch1);
                    if (!script1.GetOp(pc1, opcode1, vch1))
                        break;
                }
                if (!script2.GetOp(pc2, opcode2, vch2))
                    break;
                // Normal situation is to fall through
                // to other if/else statements
            }

            if (opcode2 == OP_PUBKEY)
            {
                if (vch1.size() < 33 || vch1.size() > 65)
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_PUBKEYHASH)
            {
                if (vch1.size() != sizeof(uint160))
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_PUBKEYHASH256)
            {
                if (vch1.size() != sizeof(uint256))
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_SMALLINTEGER)
            {   // Single-byte small integer pushed onto vSolutions
                if (opcode1 == OP_0 ||
                    (opcode1 >= OP_1 && opcode1 <= OP_16))
                {
                    char n = (char)CScript::DecodeOP_N(opcode1);
                    vSolutionsRet.push_back(valtype(1, n));
                }
                else
                    break;
            }
            else if (opcode1 != opcode2 || vch1 != vch2)
            {
                // Others must match exactly
                break;
            }
        }
    }

    vSolutionsRet.clear();
    typeRet = TX_NONSTANDARD;
    return false;
}

bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet)
{
    std::vector<valtype> vSolutions;
    txnouttype whichType;

    if (HasIsCoinstakeOp(scriptPubKey))
    {
        CScript scriptB;
        if (!GetNonCoinstakeScriptPath(scriptPubKey, scriptB))
            return false;

        // Return only the spending address
        return ExtractDestination(scriptB, addressRet);
    };

    if (!Solver(scriptPubKey, whichType, vSolutions))
        return false;

    if (whichType == TX_PUBKEY)
    {
        CPubKey pubKey(vSolutions[0]);
        if (!pubKey.IsValid())
            return false;

        addressRet = pubKey.GetID();
        return true;
    }
    else if (whichType == TX_PUBKEYHASH || whichType == TX_TIMELOCKED_PUBKEYHASH)
    {
        addressRet = CKeyID(uint160(vSolutions[0]));
        return true;
    }
    else if (whichType == TX_SCRIPTHASH || whichType == TX_TIMELOCKED_SCRIPTHASH)
    {
        addressRet = CScriptID(uint160(vSolutions[0]));
        return true;
    } else if (whichType == TX_WITNESS_V0_KEYHASH) {
        WitnessV0KeyHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    } else if (whichType == TX_WITNESS_V0_SCRIPTHASH) {
        WitnessV0ScriptHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    } else if (whichType == TX_WITNESS_UNKNOWN) {
        WitnessUnknown unk;
        unk.version = vSolutions[0][0];
        std::copy(vSolutions[1].begin(), vSolutions[1].end(), unk.program);
        unk.length = vSolutions[1].size();
        addressRet = unk;
        return true;
    }
    else if (whichType == TX_PUBKEYHASH256)
    {
        addressRet = CKeyID256(uint256(vSolutions[0]));
        return true;
    }
    else if (whichType == TX_SCRIPTHASH256)
    {
        addressRet = CScriptID256(uint256(vSolutions[0]));
        return true;
    }
    // Multisig txns have more than one address...
    return false;
}

bool ExtractDestinations(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<CTxDestination>& addressRet, int& nRequiredRet)
{
    addressRet.clear();
    typeRet = TX_NONSTANDARD;
    std::vector<valtype> vSolutions;

    if (HasIsCoinstakeOp(scriptPubKey))
    {
        CScript scriptB;
        if (!GetNonCoinstakeScriptPath(scriptPubKey, scriptB))
            return false;

        // Return only the spending address to keep insight working
        ExtractDestinations(scriptB, typeRet, addressRet, nRequiredRet);

        return true;
    };

    if (!Solver(scriptPubKey, typeRet, vSolutions))
        return false;

    if (typeRet == TX_NULL_DATA){
        // This is data, not addresses
        return false;
    }

    if (typeRet == TX_MULTISIG || typeRet == TX_TIMELOCKED_MULTISIG)
    {
        nRequiredRet = vSolutions.front()[0];
        for (unsigned int i = 1; i < vSolutions.size()-1; i++)
        {
            CPubKey pubKey(vSolutions[i]);
            if (!pubKey.IsValid())
                continue;

            CTxDestination address = pubKey.GetID();
            addressRet.push_back(address);
        }

        if (addressRet.empty())
            return false;
    }
    else
    {
        nRequiredRet = 1;
        CTxDestination address;
        if (!ExtractDestination(scriptPubKey, address))
           return false;
        addressRet.push_back(address);
    }

    return true;
}

bool ExtractStakingKeyID(const CScript &scriptPubKey, CKeyID &keyID)
{
    if (scriptPubKey.IsPayToPublicKeyHash())
    {
        keyID = CKeyID(uint160(&scriptPubKey[3], 20));
        return true;
    };

    if (scriptPubKey.IsPayToPublicKeyHash256())
    {
        keyID = CKeyID(uint256(&scriptPubKey[3], 32));
        return true;
    };

    if (scriptPubKey.IsPayToPublicKeyHash256_CS()
        || scriptPubKey.IsPayToScriptHash256_CS()
        || scriptPubKey.IsPayToScriptHash_CS()
        )
    {
        keyID = CKeyID(uint160(&scriptPubKey[5], 20));
        return true;
    };

    return false;
};

namespace
{
class CScriptVisitor : public boost::static_visitor<bool>
{
private:
    CScript *script;
public:
    explicit CScriptVisitor(CScript *scriptin) { script = scriptin; }

    bool operator()(const CNoDestination &dest) const {
        script->clear();
        return false;
    }

    bool operator()(const CKeyID &keyID) const {
        script->clear();
        *script << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
        return true;
    }

    bool operator()(const CScriptID &scriptID) const {
        script->clear();
        *script << OP_HASH160 << ToByteVector(scriptID) << OP_EQUAL;
        return true;
    }

    bool operator()(const WitnessV0KeyHash& id) const
    {
        script->clear();
        *script << OP_0 << ToByteVector(id);
        return true;
    }

    bool operator()(const WitnessV0ScriptHash& id) const
    {
        script->clear();
        *script << OP_0 << ToByteVector(id);
        return true;
    }

    bool operator()(const WitnessUnknown& id) const
    {
        script->clear();
        *script << CScript::EncodeOP_N(id.version) << std::vector<unsigned char>(id.program, id.program + id.length);
        return true;
    }

    bool operator()(const CStealthAddress &ek) const {
        script->clear();
        LogPrintf("CScriptVisitor(CStealthAddress) TODO\n");
        return false;
    }

    bool operator()(const CExtKeyPair &ek) const {
        script->clear();
        LogPrintf("CScriptVisitor(CExtKeyPair) TODO\n");
        return false;
    }

    bool operator()(const CKeyID256 &keyID) const {
        script->clear();
        *script << OP_DUP << OP_SHA256 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
        return true;
    }

    bool operator()(const CScriptID256 &scriptID) const {
        script->clear();
        *script << OP_SHA256 << ToByteVector(scriptID) << OP_EQUAL;
        return true;
    }
};
} // namespace

CScript GetScriptForDestination(const CTxDestination& dest)
{
    CScript script;

    boost::apply_visitor(CScriptVisitor(&script), dest);
    return script;
}

CScript GetScriptForRawPubKey(const CPubKey& pubKey)
{
    return CScript() << std::vector<unsigned char>(pubKey.begin(), pubKey.end()) << OP_CHECKSIG;
}

CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys)
{
    CScript script;

    script << CScript::EncodeOP_N(nRequired);
    for (const CPubKey& key : keys)
        script << ToByteVector(key);
    script << CScript::EncodeOP_N(keys.size()) << OP_CHECKMULTISIG;
    return script;
}

CScript GetScriptForWitness(const CScript& redeemscript)
{
    CScript ret;

    txnouttype typ;
    std::vector<std::vector<unsigned char> > vSolutions;
    if (Solver(redeemscript, typ, vSolutions)) {
        if (typ == TX_PUBKEY) {
            return GetScriptForDestination(WitnessV0KeyHash(Hash160(vSolutions[0].begin(), vSolutions[0].end())));
        } else if (typ == TX_PUBKEYHASH) {
            return GetScriptForDestination(WitnessV0KeyHash(vSolutions[0]));
        }
    }
    uint256 hash;
    CSHA256().Write(&redeemscript[0], redeemscript.size()).Finalize(hash.begin());
    return GetScriptForDestination(WitnessV0ScriptHash(hash));
}

bool IsValidDestination(const CTxDestination& dest) {
    return dest.which() != 0;
}
