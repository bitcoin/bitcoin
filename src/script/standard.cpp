// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/standard.h>

#include <key_io.h>
#include <crypto/sha256.h>
#include <pubkey.h>
#include <script/script.h>
#include <util/strencodings.h>

typedef std::vector<unsigned char> valtype;

bool fAcceptDatacarrier = DEFAULT_ACCEPT_DATACARRIER;
unsigned nMaxDatacarrierBytes = MAX_OP_RETURN_RELAY;

CScriptID::CScriptID(const CScript& in) : uint160(Hash160(in.begin(), in.end())) {}

ScriptHash::ScriptHash(const CScript& in) : uint160(Hash160(in.begin(), in.end())) {}

PKHash::PKHash(const CPubKey& pubkey) : uint160(pubkey.GetID()) {}

WitnessV0ScriptHash::WitnessV0ScriptHash(const CScript& in)
{
    CSHA256().Write(in.data(), in.size()).Finalize(begin());
}

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

static bool MatchPayToPubkey(const CScript& script, valtype& pubkey)
{
    if (script.size() == CPubKey::PUBLIC_KEY_SIZE + 2 && script[0] == CPubKey::PUBLIC_KEY_SIZE && script.back() == OP_CHECKSIG) {
        pubkey = valtype(script.begin() + 1, script.begin() + CPubKey::PUBLIC_KEY_SIZE + 1);
        return CPubKey::ValidSize(pubkey);
    }
    if (script.size() == CPubKey::COMPRESSED_PUBLIC_KEY_SIZE + 2 && script[0] == CPubKey::COMPRESSED_PUBLIC_KEY_SIZE && script.back() == OP_CHECKSIG) {
        pubkey = valtype(script.begin() + 1, script.begin() + CPubKey::COMPRESSED_PUBLIC_KEY_SIZE + 1);
        return CPubKey::ValidSize(pubkey);
    }
    return false;
}

static bool MatchPayToPubkeyHash(const CScript& script, valtype& pubkeyhash)
{
    if (script.size() == 25 && script[0] == OP_DUP && script[1] == OP_HASH160 && script[2] == 20 && script[23] == OP_EQUALVERIFY && script[24] == OP_CHECKSIG) {
        pubkeyhash = valtype(script.begin () + 3, script.begin() + 23);
        return true;
    }
    return false;
}

/** Test for "small positive integer" script opcodes - OP_1 through OP_16. */
static constexpr bool IsSmallInteger(opcodetype opcode)
{
    return opcode >= OP_1 && opcode <= OP_16;
}

static bool MatchMultisig(const CScript& script, unsigned int& required, std::vector<valtype>& pubkeys)
{
    opcodetype opcode;
    valtype data;
    CScript::const_iterator it = script.begin();
    if (script.size() < 1 || script.back() != OP_CHECKMULTISIG) return false;

    if (!script.GetOp(it, opcode, data) || !IsSmallInteger(opcode)) return false;
    required = CScript::DecodeOP_N(opcode);
    while (script.GetOp(it, opcode, data) && CPubKey::ValidSize(data)) {
        pubkeys.emplace_back(std::move(data));
    }
    if (!IsSmallInteger(opcode)) return false;
    unsigned int keys = CScript::DecodeOP_N(opcode);
    if (pubkeys.size() != keys || keys < required) return false;
    return (it + 1 == script.end());
}

txnouttype Solver(const CScript& scriptPubKey, std::vector<std::vector<unsigned char>>& vSolutionsRet)
{
    vSolutionsRet.clear();

    // Shortcut for pay-to-script-hash, which are more constrained than the other types:
    // it is always OP_HASH160 20 [20 byte hash] OP_EQUAL
    if (scriptPubKey.IsPayToScriptHash())
    {
        std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
        vSolutionsRet.push_back(hashBytes);
        return TX_SCRIPTHASH;
    }

    int witnessversion;
    std::vector<unsigned char> witnessprogram;
    if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_KEYHASH_SIZE) {
            vSolutionsRet.push_back(witnessprogram);
            return TX_WITNESS_V0_KEYHASH;
        }
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
            vSolutionsRet.push_back(witnessprogram);
            return TX_WITNESS_V0_SCRIPTHASH;
        }
        if (witnessversion != 0) {
            vSolutionsRet.push_back(std::vector<unsigned char>{(unsigned char)witnessversion});
            vSolutionsRet.push_back(std::move(witnessprogram));
            return TX_WITNESS_UNKNOWN;
        }
        return TX_NONSTANDARD;
    }

    // Provably prunable, data-carrying output
    //
    // So long as script passes the IsUnspendable() test and all but the first
    // byte passes the IsPushOnly() test we don't care what exactly is in the
    // script.
    if (scriptPubKey.size() >= 1 && scriptPubKey[0] == OP_RETURN && scriptPubKey.IsPushOnly(scriptPubKey.begin()+1)) {
        return TX_NULL_DATA;
    }

    std::vector<unsigned char> data;
    if (MatchPayToPubkey(scriptPubKey, data)) {
        vSolutionsRet.push_back(std::move(data));
        return TX_PUBKEY;
    }

    if (MatchPayToPubkeyHash(scriptPubKey, data)) {
        vSolutionsRet.push_back(std::move(data));
        return TX_PUBKEYHASH;
    }

    unsigned int required;
    std::vector<std::vector<unsigned char>> keys;
    if (MatchMultisig(scriptPubKey, required, keys)) {
        vSolutionsRet.push_back({static_cast<unsigned char>(required)}); // safe as required is in range 1..16
        vSolutionsRet.insert(vSolutionsRet.end(), keys.begin(), keys.end());
        vSolutionsRet.push_back({static_cast<unsigned char>(keys.size())}); // safe as size is in range 1..16
        return TX_MULTISIG;
    }

    vSolutionsRet.clear();
    return TX_NONSTANDARD;
}

bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet)
{
    std::vector<valtype> vSolutions;
    txnouttype whichType = Solver(scriptPubKey, vSolutions);

    if (whichType == TX_PUBKEY) {
        CPubKey pubKey(vSolutions[0]);
        if (!pubKey.IsValid())
            return false;

        addressRet = PKHash(pubKey);
        return true;
    }
    else if (whichType == TX_PUBKEYHASH)
    {
        addressRet = PKHash(uint160(vSolutions[0]));
        return true;
    }
    else if (whichType == TX_SCRIPTHASH)
    {
        addressRet = ScriptHash(uint160(vSolutions[0]));
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
    std::vector<valtype> vSolutions;
    typeRet = Solver(scriptPubKey, vSolutions);
    if (typeRet == TX_NONSTANDARD) {
        return false;
    } else if (typeRet == TX_NULL_DATA) {
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

            CTxDestination address = PKHash(pubKey);
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

    bool operator()(const PKHash &keyID) const {
        script->clear();
        *script << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
        return true;
    }

    bool operator()(const ScriptHash &scriptID) const {
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
    std::vector<std::vector<unsigned char> > vSolutions;
    txnouttype typ = Solver(redeemscript, vSolutions);
    if (typ == TX_PUBKEY) {
        return GetScriptForDestination(WitnessV0KeyHash(Hash160(vSolutions[0].begin(), vSolutions[0].end())));
    } else if (typ == TX_PUBKEYHASH) {
        return GetScriptForDestination(WitnessV0KeyHash(vSolutions[0]));
    }
    return GetScriptForDestination(WitnessV0ScriptHash(redeemscript));
}

CScript GetScriptForPubKey(const CPubKey& pubkey)
{
    if (!pubkey.IsValid() || !pubkey.IsCompressed())
        return CScript();

    // P2WPKH
    CKeyID keyid = pubkey.GetID();
    CTxDestination segwit = WitnessV0KeyHash(keyid);
    CTxDestination p2sh = ScriptHash(GetScriptForDestination(segwit));
    return GetScriptForDestination(p2sh);
}

CScript GetScriptForAccountID(const CAccountID& accountID)
{
    return GetScriptForDestination(ExtractDestination(accountID));
}

bool IsValidDestination(const CTxDestination& dest) {
    return dest.which() != 0;
}

CAccountID ExtractAccountID(const CPubKey& pubkey)
{
    return ExtractAccountID(GetScriptForPubKey(pubkey));
}

CAccountID ExtractAccountID(const CScript& scriptPubKey)
{
    return ExtractAccountID(ExtractDestination(scriptPubKey));
}

CAccountID ExtractAccountID(const CTxDestination& dest)
{
    const ScriptHash *scriptID = boost::get<ScriptHash>(&dest);
    if (scriptID != nullptr) {
        return CAccountID(*scriptID);
    } else {
        return CAccountID();
    }
}

CTxDestination ExtractDestination(const CAccountID& accountID)
{
    if (accountID.IsNull())
        return CTxDestination();
    return CTxDestination(ScriptHash(accountID));
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

std::vector<unsigned char> ToByteVector(TxOutType type) {
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

// Make sure signature not from other similar COIN
static const unsigned char bindPlotterSalt[] = {'Q', 'I', 'T', 'C', 'O', 'I', 'N'};

CScript GetBindPlotterScriptForDestination(const CTxDestination& dest, const std::string& passphrase, int lastActiveHeight) {
    CScript script;

    if (lastActiveHeight <= 0 || !IsValidPassphrase(passphrase))
        return script;

    // Check destination type is P2SH
    const ScriptHash *scriptID = boost::get<ScriptHash>(&dest);
    if (scriptID == nullptr)
        return script;

    unsigned char data[32], signature[64], publicKey[32];
    CSHA256().
        Write(ToByteVector((uint32_t) lastActiveHeight).data(), 4).
        Write(scriptID->begin(), CScriptID::WIDTH).
        Write(bindPlotterSalt, sizeof(bindPlotterSalt)).
        Finalize(data);
    if (!PocLegacy::Sign(passphrase, data, signature, publicKey))
        return script;
    assert(PocLegacy::Verify(publicKey, data, signature));
    if (PocLegacy::ToPlotterId(publicKey) == 0)
        return script;

    script << ToByteVector(TXOUT_TYPE_BINDPLOTTER);
    script << ToByteVector((uint32_t) lastActiveHeight);
    script << std::vector<unsigned char>(publicKey, publicKey+32);
    script << std::vector<unsigned char>(signature, signature+64);

    assert(script.size() == PROTOCOL_BINDPLOTTER_SCRIPTSIZE);
    return script;
}

bool IsBindPlotterScript(const CScript &script)
{
    if (script.size() != PROTOCOL_BINDPLOTTER_SCRIPTSIZE)
        return false;

    opcodetype opcode;
    std::vector<unsigned char> vData;

    CScript::const_iterator pc = script.begin();
    // Get data type
    if (!script.GetOp(pc, opcode, vData) || opcode != 0x04)
        return false;
    unsigned int type = (vData[0] << 0) | (vData[1] << 8) | (vData[2] << 16) | (vData[3] << 24);
    if (type != TXOUT_TYPE_BINDPLOTTER)
        return false;

    return true;
}

CScript SignBindPlotterScript(const CScript &script, const CKey &key)
{
    if (script.size() != PROTOCOL_BINDPLOTTER_SCRIPTSIZE)
        return CScript();

    if (!key.IsCompressed())
        return CScript();
    CPubKey pubkey = key.GetPubKey();
    CTxDestination segwit = WitnessV0KeyHash(pubkey.GetID());
    CTxDestination dest = ScriptHash(GetScriptForDestination(segwit));
    const ScriptHash *scriptID = boost::get<ScriptHash>(&dest);
    if (scriptID == nullptr) {
        printf("%s: %d\n", __FUNCTION__, __LINE__);
        return CScript();
    }

    opcodetype opcode;
    std::vector<unsigned char> vData;

    CScript::const_iterator pc = script.begin();
    // Get data type
    if (!script.GetOp(pc, opcode, vData) || opcode != 0x04)
        return CScript();
    unsigned int type = (vData[0] << 0) | (vData[1] << 8) | (vData[2] << 16) | (vData[3] << 24);
    if (type != TXOUT_TYPE_BINDPLOTTER)
        return CScript();

    // Check last active height
    if (!script.GetOp(pc, opcode, vData) || opcode != sizeof(uint32_t))
        return CScript();
    int lastActiveHeight = (((uint32_t)vData[0]) >> 0) | (((uint32_t)vData[1]) << 8) | (((uint32_t)vData[2]) << 16) | (((uint32_t)vData[3]) << 24);

    uint256 unsignDataHash;
    CSHA256().
        Write(ToByteVector((uint32_t) lastActiveHeight).data(), 4).
        Write(scriptID->begin(), CScriptID::WIDTH).
        Write(bindPlotterSalt, sizeof(bindPlotterSalt)).
        Finalize(unsignDataHash.begin());

    // sign
    std::vector<unsigned char> vchSig;
    if (!key.Sign(unsignDataHash, vchSig))
        return CScript();

    CScript signScript = script;
    signScript << std::vector<unsigned char>(pubkey.begin(), pubkey.end());
    signScript << vchSig;
    return signScript;
}

bool DecodeBindPlotterScript(const CScript &script, uint64_t& plotterIdOut, std::string& pubkeyHexOut, std::string& signatureHexOut, int& lastActiveHeightOut)
{
    if (script.size() != PROTOCOL_BINDPLOTTER_SCRIPTSIZE)
        return false;

    opcodetype opcode;
    std::vector<unsigned char> vData;

    CScript::const_iterator pc = script.begin();
    // Get data type
    if (!script.GetOp(pc, opcode, vData) || opcode != 0x04)
        return false;
    unsigned int type = (vData[0] << 0) | (vData[1] << 8) | (vData[2] << 16) | (vData[3] << 24);
    if (type != TXOUT_TYPE_BINDPLOTTER)
        return false;

    // Check last active height
    if (!script.GetOp(pc, opcode, vData) || opcode != sizeof(uint32_t))
        return false;
    int lastActiveHeight = (((uint32_t)vData[0]) >> 0) | (((uint32_t)vData[1]) << 8) | (((uint32_t)vData[2]) << 16) | (((uint32_t)vData[3]) << 24);

    // Signature
    std::vector<unsigned char> vPublicKey, vSignature;
    if (!script.GetOp(pc, opcode, vPublicKey) || opcode != 0x20)
        return false;
    if (!script.GetOp(pc, opcode, vSignature) || opcode != 0x40)
        return false;

    // success
    plotterIdOut = PocLegacy::ToPlotterId(&vPublicKey[0]);
    pubkeyHexOut = HexStr(vPublicKey);
    signatureHexOut = HexStr(vSignature);
    lastActiveHeightOut = lastActiveHeight;
    return true;
}

uint64_t GetBindPlotterIdFromScript(const CScript &script)
{
    if (script.size() != PROTOCOL_BINDPLOTTER_SCRIPTSIZE)
        return 0;

    return PocLegacy::ToPlotterId(&script[12]);
}

CScript GetPointScriptForDestination(const CTxDestination& dest, int lockBlocks) {
    CScript script;

    const ScriptHash *scriptID = boost::get<ScriptHash>(&dest);
    if (scriptID != nullptr) {
        script << ToByteVector(TXOUT_TYPE_POINT);
        script << ToByteVector(*scriptID);
        script << ToByteVector((uint32_t) lockBlocks);
    }

    assert(script.empty() || script.size() == PROTOCOL_POINT_SCRIPTSIZE);
    return script;
}

CAmount GetPointAmount(CAmount amount, int lockBlocks)
{
    if (lockBlocks == 360 * 480) {
        return amount / 2;
    }
    if (lockBlocks == 540 * 480) {
        return amount;
    }

    return 0;
}

CScript GetStakingScriptForDestination(const CTxDestination& dest, int lockBlocks) {
    CScript script;

    const ScriptHash *scriptID = boost::get<ScriptHash>(&dest);
    if (scriptID != nullptr) {
        script << ToByteVector(TXOUT_TYPE_STAKING);
        script << ToByteVector(*scriptID);
        script << ToByteVector((uint32_t) lockBlocks);
    }

    assert(script.empty() || script.size() == PROTOCOL_STAKING_SCRIPTSIZE);
    return script;
}

CAmount GetStakingAmount(CAmount amount, int lockBlocks)
{
    if (lockBlocks == 360 * 480) {
        return amount / 2;
    }
    if (lockBlocks == 540 * 480) {
        return amount;
    }

    return 0;
}

CTxOutPayloadRef ExtractTxoutPayload(const CTxOut& txout, int nHeight, const std::set<TxOutType> &filters, bool for_test)
{
    // 0x04 <Protocol> <...>
    if (txout.payload.size() < 6 || txout.payload[0] != 0x04)
        return nullptr;

    CScript::const_iterator pc = txout.payload.begin();
    opcodetype opcode;
    std::vector<unsigned char> vData;

    // Get data type
    if (!txout.payload.GetOp(pc, opcode, vData) || opcode != 0x04)
        return nullptr;
    unsigned int type = (vData[0] << 0) | (vData[1] << 8) | (vData[2] << 16) | (vData[3] << 24);
    if (!filters.empty() && !filters.count((TxOutType) type))
        return nullptr;

    if (type == TXOUT_TYPE_BINDPLOTTER) {
        // Bind plotter
        if (txout.nValue < PROTOCOL_BINDPLOTTER_LOCKAMOUNT)
            return nullptr;
        if ((for_test && txout.payload.size() != PROTOCOL_BINDPLOTTER_SCRIPTSIZE) ||
                (!for_test && txout.payload.size() <= PROTOCOL_BINDPLOTTER_SCRIPTSIZE))
            return nullptr;

        // Check destination
        CTxDestination dest;
        if (!ExtractDestination(txout.scriptPubKey, dest))
            return nullptr;
        const ScriptHash *scriptID = boost::get<ScriptHash>(&dest);
        if (scriptID == nullptr)
            return nullptr;

        // Check last active height
        if (!txout.payload.GetOp(pc, opcode, vData) || opcode != sizeof(uint32_t))
            return nullptr;
        uint32_t lastActiveHeight = (((uint32_t)vData[0]) >> 0) | (((uint32_t)vData[1]) << 8) | (((uint32_t)vData[2]) << 16) | (((uint32_t)vData[3]) << 24);
        if (nHeight != 0 && (nHeight > (int)lastActiveHeight || nHeight + PROTOCOL_BINDPLOTTER_MAXALIVE < (int)lastActiveHeight))
            return nullptr;

        // Verify signature: want binding
        uint64_t plotterId;
        uint256 unsignDataHash;
        {
            std::vector<unsigned char> vPlotterPublicKey, vPlotterSignature;
            if (!txout.payload.GetOp(pc, opcode, vPlotterPublicKey) || opcode != 0x20)
                return nullptr;
            if (!txout.payload.GetOp(pc, opcode, vPlotterSignature) || opcode != 0x40)
                return nullptr;
            CSHA256().
                Write(ToByteVector((uint32_t) lastActiveHeight).data(), 4).
                Write(scriptID->begin(), CScriptID::WIDTH).
                Write(bindPlotterSalt, sizeof(bindPlotterSalt)).
                Finalize(unsignDataHash.begin());
            if (!PocLegacy::Verify(&vPlotterPublicKey[0], unsignDataHash.begin(), &vPlotterSignature[0]))
                return nullptr;

            plotterId = PocLegacy::ToPlotterId(&vPlotterPublicKey[0]);
            if (plotterId == 0)
                return nullptr;
        }

        // Verify signature for binding target: accept binding
        if (!for_test) {
            std::vector<unsigned char> vPublicKey, vSignature;
            if (!txout.payload.GetOp(pc, opcode, vPublicKey))
                return nullptr;
            if (!txout.payload.GetOp(pc, opcode, vSignature))
                return nullptr;
            CPubKey pubkey(vPublicKey);
            if (!pubkey.IsValid())
                return nullptr;
            if (ExtractAccountID(pubkey) != *scriptID)
                return nullptr;
            if (!pubkey.Verify(unsignDataHash, vSignature))
                return nullptr;
        }

        std::shared_ptr<BindPlotterPayload> payload = std::make_shared<BindPlotterPayload>();
        payload->id = plotterId;
        return payload;
    } else if (type == TXOUT_TYPE_POINT) {
        // Point
        if (txout.nValue < PROTOCOL_POINT_AMOUNT_MIN || txout.payload.size() != PROTOCOL_POINT_SCRIPTSIZE)
            return nullptr;

        // Receiver account
        std::vector<unsigned char> receiverID;
        if (!txout.payload.GetOp(pc, opcode, receiverID) || opcode != CScriptID::WIDTH)
            return nullptr;

        // Lock blocks
        uint32_t lockBlocks;
        if (!txout.payload.GetOp(pc, opcode, vData) || opcode != 4)
            return nullptr;
        lockBlocks = (((uint32_t)vData[0]) >> 0) | (((uint32_t)vData[1]) << 8) | (((uint32_t)vData[2]) << 16) | (((uint32_t)vData[3]) << 24);

        std::shared_ptr<PointPayload> payload = std::make_shared<PointPayload>();
        payload->receiverID = CAccountID(receiverID);
        payload->lockBlocks = lockBlocks;
        payload->amount = GetPointAmount(txout.nValue, (int) lockBlocks);
        if (payload->GetReceiverID().IsNull() || payload->amount == 0)
            return nullptr;
        return payload;
    } else if (type == TXOUT_TYPE_STAKING) {
        // Staking
        if (txout.nValue < PROTOCOL_STAKING_AMOUNT_MIN || txout.payload.size() != PROTOCOL_STAKING_SCRIPTSIZE)
            return nullptr;

        // Receiver account
        std::vector<unsigned char> receiverID;
        if (!txout.payload.GetOp(pc, opcode, receiverID) || opcode != CScriptID::WIDTH)
            return nullptr;

        // Lock blocks
        uint32_t lockBlocks;
        if (!txout.payload.GetOp(pc, opcode, vData) || opcode != 4)
            return nullptr;
        lockBlocks = (((uint32_t)vData[0]) >> 0) | (((uint32_t)vData[1]) << 8) | (((uint32_t)vData[2]) << 16) | (((uint32_t)vData[3]) << 24);

        std::shared_ptr<StakingPayload> payload = std::make_shared<StakingPayload>();
        payload->receiverID = CAccountID(receiverID);
        payload->lockBlocks = lockBlocks;
        payload->amount = GetStakingAmount(txout.nValue, (int) lockBlocks);
        if (payload->GetReceiverID().IsNull() || payload->amount == 0)
            return nullptr;
        return payload;
    }

    return nullptr;
}
