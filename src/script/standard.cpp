// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/standard.h>

#include <base58.h>
#include <pubkey.h>
#include <script/script.h>
#include <util.h>
#include <utilstrencodings.h>


typedef std::vector<unsigned char> valtype;

bool fAcceptDatacarrier = DEFAULT_ACCEPT_DATACARRIER;
unsigned nMaxDatacarrierBytes = MAX_OP_RETURN_RELAY;

CScriptID::CScriptID(const CScript& in) : uint160(Hash160(in.begin(), in.end())) {}

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
    }

    vSolutionsRet.clear();

    // Shortcut for pay-to-script-hash, which are more constrained than the other types:
    // it is always OP_HASH160 20 [20 byte hash] OP_EQUAL
    if (scriptPubKey.IsPayToScriptHash())
    {
        typeRet = TX_SCRIPTHASH;
        std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
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
        CScript::const_iterator pc1 = script1.begin();
        CScript::const_iterator pc2 = script2.begin();
        while (true)
        {
            if (pc1 == script1.end() && pc2 == script2.end())
            {
                // Found a match
                typeRet = tplate.first;
                if (typeRet == TX_MULTISIG)
                {
                    // Additional checks for TX_MULTISIG:
                    unsigned char m = vSolutionsRet.front()[0];
                    unsigned char n = vSolutionsRet.back()[0];
                    if (m < 1 || n < 1 || m > n || vSolutionsRet.size()-2 != n)
                        return false;
                }
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
    else if (whichType == TX_PUBKEYHASH)
    {
        addressRet = CKeyID(uint160(vSolutions[0]));
        return true;
    }
    else if (whichType == TX_SCRIPTHASH)
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
    // Multisig txns have more than one address...
    return false;
}

CTxDestination ExtractDestination(const CScript& scriptPubKey)
{
    CTxDestination addressRet;
    if (ExtractDestination(scriptPubKey, addressRet))
        return addressRet;

    return CTxDestination();
}

bool ExtractDestinations(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<CTxDestination>& addressRet, int& nRequiredRet)
{
    addressRet.clear();
    typeRet = TX_NONSTANDARD;
    std::vector<valtype> vSolutions;
    if (!Solver(scriptPubKey, typeRet, vSolutions))
        return false;
    if (typeRet == TX_NULL_DATA) {
        // This is data, not addresses
        return false;
    }

    if (typeRet == TX_MULTISIG)
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

CAccountID ExtractAccountID(const CPubKey& pubkey)
{
    if (!pubkey.IsValid() || !pubkey.IsCompressed())
        return CAccountID();

    // P2SH-Segwit
    CKeyID keyid = pubkey.GetID();
    CTxDestination segwit = WitnessV0KeyHash(keyid);
    CTxDestination p2sh = CScriptID(GetScriptForDestination(segwit));
    return ExtractAccountID(p2sh);
}

CAccountID ExtractAccountID(const CScript& scriptPubKey)
{
    return ExtractAccountID(ExtractDestination(scriptPubKey));
}

CAccountID ExtractAccountID(const CTxDestination& dest)
{
    const CScriptID *scriptID = boost::get<CScriptID>(&dest);
    if (scriptID != nullptr) {
        return CAccountID(*scriptID);
    } else {
        return CAccountID();
    }
}

namespace {

std::vector<unsigned char> ToByteVector(uint32_t v) {
    return {
        (unsigned char)((v >> 0)&0xff),
        (unsigned char)((v >> 8)&0xff),
        (unsigned char)((v >> 16)&0xff),
        (unsigned char)((v >> 24)&0xff),
    };
}

std::vector<unsigned char> ToByteVector(DatacarrierType type) {
    return ToByteVector((uint32_t) type);
}

}

bool IsValidPassphrase(const std::string& passphrase) {
    return !passphrase.empty();
}

bool IsValidPlotterID(const std::string& strPlotterId, uint64_t *id) {
    if (strPlotterId.empty() || strPlotterId.size() > 20 || strPlotterId[0] == '0')
        return false;

    if (strPlotterId.find_first_not_of("0123456789") != std::string::npos)
        return false;

    if (id) *id = static_cast<uint64_t>(std::stoull(strPlotterId));
    return true;
}

CScript GetBindPlotterScriptForDestination(const CTxDestination& dest, const std::string& passphrase, int lastActiveHeight) {
    CScript script;

    if (lastActiveHeight <= 0 || !IsValidPassphrase(passphrase))
        return script;

    // Check destination type is P2SH
    const CScriptID *scriptID = boost::get<CScriptID>(&dest);
    if (scriptID == nullptr)
        return script;

    unsigned char data[32], signature[64], publicKey[32];
    CSHA256().Write(scriptID->begin(), CScriptID::WIDTH).Write((const unsigned char*)&lastActiveHeight, 4).Finalize(data);
    if (!PocLegacy::Sign(passphrase, data, signature, publicKey))
        return script;
    assert(PocLegacy::Verify(publicKey, data, signature));
    if (PocLegacy::ToPlotterId(publicKey) == 0)
        return script;

    script << OP_RETURN;
    script << ToByteVector(DATACARRIER_TYPE_BINDPLOTTER);
    script << ToByteVector((uint32_t) lastActiveHeight);
    script << std::vector<unsigned char>(publicKey, publicKey+32);
    script << std::vector<unsigned char>(signature, signature+64);

    assert(script.size() == PROTOCOL_BINDPLOTTER_SCRIPTSIZE);

    return script;
}

uint64_t GetBindPlotterIdFromScript(const CScript &script)
{
    if (script.size() != PROTOCOL_BINDPLOTTER_SCRIPTSIZE)
        return 0;

    return PocLegacy::ToPlotterId(&script[12]);
}

CScript GetRentalScriptForDestination(const CTxDestination& dest) {
    CScript script;

    const CScriptID *scriptID = boost::get<CScriptID>(&dest);
    if (scriptID != nullptr) {
        script << OP_RETURN;
        script << ToByteVector(DATACARRIER_TYPE_RENTAL);
        script << ToByteVector(*scriptID);
    }

    assert(script.empty() || script.size() == PROTOCOL_RENTAL_SCRIPTSIZE);
    return script;
}

CScript GetTextScript(const std::string& text) {
    CScript script;
    if (text.size() <= PROTOCOL_TEXT_MAXSIZE) {
        script << OP_RETURN;
        script << ToByteVector(DATACARRIER_TYPE_TEXT);
        script << ToByteVector(text);
    }
    return script;
}

static CDatacarrierPayloadRef ExtractDatacarrier(const CTransaction& tx, int nHeight, bool fAllData, bool *pReject, int *pLastActiveHeight) {
    // OP_RETURN 0x04 <Protocol> <...>
    const CScript &scriptPubKey = tx.vout.back().scriptPubKey;
    if (scriptPubKey.size() < 6 || scriptPubKey[0] != OP_RETURN || scriptPubKey[1] != 0x04)
        return nullptr;
    CScript::const_iterator pc = scriptPubKey.begin() + 1;
    opcodetype opcode;
    std::vector<unsigned char> vData;

    // Get data type
    if (!scriptPubKey.GetOp(pc, opcode, vData) || opcode != 0x04)
        return nullptr;
    unsigned int type = (vData[0] << 0) | (vData[1] << 8) | (vData[2] << 16) | (vData[3] << 24);
    if (type == DATACARRIER_TYPE_BINDPLOTTER) {
        // Bind plotter transaction
        if (tx.nVersion != CTransaction::UNIFORM_VERSION || tx.vout.size() < 2 || tx.vout.size() > 3 || tx.vout[0].scriptPubKey.IsUnspendable())
            return nullptr;
        if (scriptPubKey.size() != PROTOCOL_BINDPLOTTER_SCRIPTSIZE || tx.vout[0].nValue != PROTOCOL_BINDPLOTTER_LOCKAMOUNT)
            return nullptr;
        // Check destination
        CTxDestination dest;
        if (!ExtractDestination(tx.vout[0].scriptPubKey, dest))
            return nullptr;
        const CScriptID *scriptID = boost::get<CScriptID>(&dest);
        if (scriptID == nullptr)
            return nullptr;

        // Check last active height
        if (!scriptPubKey.GetOp(pc, opcode, vData) || opcode != sizeof(uint32_t))
            return nullptr;
        uint32_t lastActiveHeight = (((uint32_t)vData[0]) >> 0) | (((uint32_t)vData[1]) << 8) | (((uint32_t)vData[2]) << 16) | (((uint32_t)vData[3]) << 24);
        if (nHeight != 0 && (nHeight > (int)lastActiveHeight || nHeight + PROTOCOL_BINDPLOTTER_MAXALIVE < (int)lastActiveHeight))
            return nullptr;
        if (pLastActiveHeight) *pLastActiveHeight = (int) lastActiveHeight;

        // Verify signature
        unsigned char data[32];
        std::vector<unsigned char> vPublicKey, vSignature;
        if (!scriptPubKey.GetOp(pc, opcode, vPublicKey) || opcode != 0x20)
            return nullptr;
        if (!scriptPubKey.GetOp(pc, opcode, vSignature) || opcode != 0x40)
            return nullptr;
        CSHA256().Write(scriptID->begin(), CScriptID::WIDTH).Write((const unsigned char*)&lastActiveHeight, 4).Finalize(data);
        if (!PocLegacy::Verify(&vPublicKey[0], data, &vSignature[0])) {
            if (pReject) *pReject = true;
            return nullptr;
        }

        uint64_t plotterId = PocLegacy::ToPlotterId(&vPublicKey[0]);
        if (plotterId == 0)
            return nullptr;

        std::shared_ptr<BindPlotterPayload> payload = std::make_shared<BindPlotterPayload>();
        payload->id = plotterId;
        return payload;
    } else if (type == DATACARRIER_TYPE_RENTAL) {
        // Plege transaction
        if (tx.nVersion != CTransaction::UNIFORM_VERSION || tx.vout.size() < 2 || tx.vout.size() > 3 || tx.vout[0].scriptPubKey.IsUnspendable())
            return nullptr;
        if (tx.vout[0].nValue < PROTOCOL_RENTAL_AMOUNT_MIN || scriptPubKey.size() != PROTOCOL_RENTAL_SCRIPTSIZE)
            return nullptr;

        // Debit account
        if (!scriptPubKey.GetOp(pc, opcode, vData) || opcode != CScriptID::WIDTH)
            return nullptr;

        std::shared_ptr<RentalPayload> payload = std::make_shared<RentalPayload>();
        payload->borrowerAccountID = uint160(vData);
        if (payload->GetBorrowerAccountID().IsNull())
            return nullptr;
        return payload;
    } else if (type == DATACARRIER_TYPE_TEXT && fAllData) {
        if (scriptPubKey.size() > MAX_OP_RETURN_RELAY)
            return nullptr;
        if (!scriptPubKey.GetOp(pc, opcode, vData))
            return nullptr;
        std::shared_ptr<TextPayload> payload = std::make_shared<TextPayload>();
        payload->text = std::string(vData.cbegin(), vData.cend());
        return payload;
    }

    return nullptr;
}

CDatacarrierPayloadRef ExtractTransactionDatacarrier(const CTransaction& tx, int nHeight) {
    return ExtractDatacarrier(tx, nHeight, false, nullptr, nullptr);
}

CDatacarrierPayloadRef ExtractTransactionDatacarrier(const CTransaction& tx, int nHeight, bool& fReject, int& lastActiveHeight) {
    return ExtractDatacarrier(tx, nHeight, false, &fReject, &lastActiveHeight);
}

CDatacarrierPayloadRef ExtractTransactionDatacarrierUnlimit(const CTransaction& tx, int nHeight) {
    return ExtractDatacarrier(tx, nHeight, true, nullptr, nullptr);
}