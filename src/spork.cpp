// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <spork.h>

#include <base58.h>
#include <chainparams.h>
#include <validation.h>
#include <messagesigner.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <util/message.h>
#include <string>
#include <key_io.h>
const std::string CSporkManager::SERIALIZATION_VERSION_STRING = "CSporkManager-Version-2";

#define MAKE_SPORK_DEF(name, defaultValue) CSporkDef{name, defaultValue, #name}
std::vector<CSporkDef> sporkDefs = {
    MAKE_SPORK_DEF(SPORK_9_SUPERBLOCKS_ENABLED,            0), // ON
    MAKE_SPORK_DEF(SPORK_17_QUORUM_DKG_ENABLED,            4070908800ULL), // OFF
    MAKE_SPORK_DEF(SPORK_19_CHAINLOCKS_ENABLED,            4070908800ULL), // OFF
    MAKE_SPORK_DEF(SPORK_21_QUORUM_ALL_CONNECTED,          4070908800ULL), // OFF
    MAKE_SPORK_DEF(SPORK_23_QUORUM_POSE,                   4070908800ULL), // OFF
    MAKE_SPORK_DEF(SPORK_TEST,                             4070908800ULL), // OFF
    MAKE_SPORK_DEF(SPORK_TEST1,                            4070908800ULL), // OFF
};

CSporkManager sporkManager;

CSporkManager::CSporkManager()
{
    for (auto& sporkDef : sporkDefs) {
        sporkDefsById.try_emplace(sporkDef.sporkId, &sporkDef);
        sporkDefsByName.try_emplace(sporkDef.name, &sporkDef);
    }
}

bool CSporkManager::SporkValueIsActive(int32_t nSporkID, int64_t &nActiveValueRet) const
{
    LOCK(cs);

    if (!mapSporksActive.count(nSporkID)) return false;

    auto it = mapSporksCachedValues.find(nSporkID);
    if (it != mapSporksCachedValues.end()) {
        nActiveValueRet = it->second;
        return true;
    }

    // calc how many values we have and how many signers vote for every value
    std::unordered_map<int64_t, int> mapValueCounts;
    for (const auto& pair: mapSporksActive.at(nSporkID)) {
        mapValueCounts[pair.second.nValue]++;
        if (mapValueCounts.at(pair.second.nValue) >= nMinSporkKeys) {
            // nMinSporkKeys is always more than the half of the max spork keys number,
            // so there is only one such value and we can stop here
            nActiveValueRet = pair.second.nValue;
            mapSporksCachedValues[nSporkID] = nActiveValueRet;
            return true;
        }
    }

    return false;
}

void CSporkManager::Clear()
{
    LOCK(cs);
    mapSporksActive.clear();
    mapSporksByHash.clear();
    // sporkPubKeyID and sporkPrivKey should be set in init.cpp,
    // we should not alter them here.
}

void CSporkManager::CheckAndRemove()
{
    LOCK(cs);
    bool fSporkAddressIsSet = !setSporkPubKeyIDs.empty();
    assert(fSporkAddressIsSet);

    auto itActive = mapSporksActive.begin();
    while (itActive != mapSporksActive.end()) {
        auto itSignerPair = itActive->second.begin();
        while (itSignerPair != itActive->second.end()) {
            if (setSporkPubKeyIDs.find(itSignerPair->first) == setSporkPubKeyIDs.end()) {
                mapSporksByHash.erase(itSignerPair->second.GetHash());
                continue;
            }
            if (!itSignerPair->second.CheckSignature(itSignerPair->first)) {
                mapSporksByHash.erase(itSignerPair->second.GetHash());
                itActive->second.erase(itSignerPair++);
                continue;
            }
            ++itSignerPair;
        }
        if (itActive->second.empty()) {
            mapSporksActive.erase(itActive++);
            continue;
        }
        ++itActive;
    }

    auto itByHash = mapSporksByHash.begin();
    while (itByHash != mapSporksByHash.end()) {
        bool found = false;
        for (const auto& signer: setSporkPubKeyIDs) {
            if (itByHash->second.CheckSignature(signer)) {
                found = true;
                break;
            }
        }
        if (!found) {
            mapSporksByHash.erase(itByHash++);
            continue;
        }
        ++itByHash;
    }
}

void CSporkManager::ProcessSpork(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman, PeerManager& peerman)
{

    if (strCommand == NetMsgType::SPORK) {

        CSporkMessage spork;
        vRecv >> spork;

        const uint256 &hash = spork.GetHash();
        std::string strLogMsg;
        {
            LOCK(cs_main);
            pfrom->AddKnownTx(hash);
            peerman.ReceivedResponse(pfrom->GetId(), hash);
            if(!::ChainActive().Tip()) return;
            strLogMsg = strprintf("SPORK -- hash: %s id: %d value: %10d bestHeight: %d peer=%d", hash.ToString(), spork.nSporkID, spork.nValue, ::ChainActive().Height(), pfrom->GetId());
        }

        if (spork.nTimeSigned > GetAdjustedTime() + 2 * 60 * 60) {
            {
                LOCK(cs_main);
                peerman.ForgetTxHash(pfrom->GetId(), hash);
            }
            LogPrint(BCLog::SPORK, "CSporkManager::ProcessSpork -- ERROR: too far into the future\n");
            peerman.Misbehaving(pfrom->GetId(), 100, "spork too far into the future");
            return;
        }

        CKeyID keyIDSigner;

        if (!spork.GetSignerKeyID(keyIDSigner) || !setSporkPubKeyIDs.count(keyIDSigner)) {
            {
                LOCK(cs_main);
                peerman.ForgetTxHash(pfrom->GetId(), hash);
            }
            LogPrint(BCLog::SPORK, "CSporkManager::ProcessSpork -- ERROR: invalid signature\n");
            peerman.Misbehaving(pfrom->GetId(), 100, "invalid spork signature");
            return;
        }
        bool bSeen = false;
        {
            LOCK(cs); // make sure to not lock this together with cs_main
            if (mapSporksActive.count(spork.nSporkID)) {
                if (mapSporksActive[spork.nSporkID].count(keyIDSigner)) {
                    if (mapSporksActive[spork.nSporkID][keyIDSigner].nTimeSigned >= spork.nTimeSigned) {
                        LogPrint(BCLog::SPORK, "%s seen\n", strLogMsg);
                        bSeen = true;
                    } else {
                        LogPrintf("%s updated\n", strLogMsg);
                    }
                } else {
                    LogPrintf("%s new signer\n", strLogMsg);
                }
            } else {
                LogPrintf("%s new\n", strLogMsg);
            }
        }
        if(bSeen) {
            LOCK(cs_main);
            peerman.ForgetTxHash(pfrom->GetId(), hash);
            return;
        }


        {
            LOCK(cs); // make sure to not lock this together with cs_main
            mapSporksByHash[hash] = spork;
            mapSporksActive[spork.nSporkID][keyIDSigner] = spork;
            // Clear cached values on new spork being processed
            mapSporksCachedActive.erase(spork.nSporkID);
            mapSporksCachedValues.erase(spork.nSporkID);
        }
        spork.Relay(connman);
        {
            LOCK(cs_main);
            peerman.ForgetTxHash(pfrom->GetId(), hash);
        }
    } else if (strCommand == NetMsgType::GETSPORKS) {
        LOCK(cs); // make sure to not lock this together with cs_main
        for (const auto& pair : mapSporksActive) {
            for (const auto& signerSporkPair: pair.second) {
                connman.PushMessage(pfrom, CNetMsgMaker(pfrom->GetCommonVersion()).Make(NetMsgType::SPORK, signerSporkPair.second));
            }
        }
    }

}

bool CSporkManager::UpdateSpork(int32_t nSporkID, int64_t nValue, CConnman& connman)
{
    CSporkMessage spork = CSporkMessage(nSporkID, nValue, GetAdjustedTime());


    if (!spork.Sign(sporkPrivKey)) {
        LogPrintf("CSporkManager::%s -- ERROR: signing failed for spork %d\n", __func__, nSporkID);
        return false;
    }

    CKeyID keyIDSigner;
    if (!spork.GetSignerKeyID(keyIDSigner) || !setSporkPubKeyIDs.count(keyIDSigner)) {
        LogPrintf("CSporkManager::UpdateSpork: failed to find keyid for private key\n");
        return false;
    }

    LogPrintf("CSporkManager::%s -- signed %d %s\n", __func__, nSporkID, spork.GetHash().ToString());
    {
        LOCK(cs);
        mapSporksByHash[spork.GetHash()] = spork;
        mapSporksActive[nSporkID][keyIDSigner] = spork;
        // Clear cached values on new spork being processed
        mapSporksCachedActive.erase(spork.nSporkID);
        mapSporksCachedValues.erase(spork.nSporkID);
    }
    spork.Relay(connman);
    return true;
}

bool CSporkManager::IsSporkActive(int32_t nSporkID) const
{
    LOCK(cs);
    // If nSporkID is cached, and the cached value is true, then return early true
    auto it = mapSporksCachedActive.find(nSporkID);
    if (it != mapSporksCachedActive.end() && it->second) {
        return true;
    }

    int64_t nSporkValue = GetSporkValue(nSporkID);
    // Get time is somewhat costly it looks like
    bool ret = nSporkValue < GetAdjustedTime();
    // Only cache true values
    if (ret) {
        mapSporksCachedActive[nSporkID] = ret;
    }
    return ret;
}

int64_t CSporkManager::GetSporkValue(int32_t nSporkID) const
{
    LOCK(cs);

    int64_t nSporkValue = -1;
    if (SporkValueIsActive(nSporkID, nSporkValue)) {
        return nSporkValue;
    }

    auto it = sporkDefsById.find(nSporkID);
    if (it != sporkDefsById.end()) {
        return it->second->defaultValue;
    }

    LogPrint(BCLog::SPORK, "CSporkManager::GetSporkValue -- Unknown Spork ID %d\n", nSporkID);
    return -1;
}

int32_t CSporkManager::GetSporkIDByName(const std::string& strName) const
{
    auto it = sporkDefsByName.find(strName);
    if (it == sporkDefsByName.end()) {
        LogPrint(BCLog::SPORK, "CSporkManager::GetSporkIDByName -- Unknown Spork name '%s'\n", strName);
        return SPORK_INVALID;
    }
    return it->second->sporkId;
}

std::string CSporkManager::GetSporkNameByID(int32_t nSporkID) const
{
    auto it = sporkDefsById.find(nSporkID);
    if (it == sporkDefsById.end()) {
        LogPrint(BCLog::SPORK, "CSporkManager::GetSporkNameByID -- Unknown Spork ID %d\n", nSporkID);
        return "Unknown";
    }
    return it->second->name;
}

bool CSporkManager::GetSporkByHash(const uint256& hash, CSporkMessage &sporkRet) const
{
    LOCK(cs);

    const auto it = mapSporksByHash.find(hash);

    if (it == mapSporksByHash.end())
        return false;

    sporkRet = it->second;

    return true;
}

bool CSporkManager::SetSporkAddress(const std::string& strAddress) {
    LOCK(cs);
    CTxDestination dest = DecodeDestination(strAddress);
    CKeyID keyID;
    if (auto witness_id = std::get_if<WitnessV0KeyHash>(&dest)) {	
        keyID = ToKeyID(*witness_id);
    }	
    else if (auto key_id = std::get_if<PKHash>(&dest)) {	
        keyID = ToKeyID(*key_id);
    }	
    if (keyID.IsNull()) {
        LogPrintf("CSporkManager::SetSporkAddress -- Failed to parse spork address\n");
        return false;
    }
    setSporkPubKeyIDs.insert(keyID);
    return true;
}

bool CSporkManager::SetMinSporkKeys(int minSporkKeys)
{
    int maxKeysNumber = setSporkPubKeyIDs.size();
    if ((minSporkKeys <= maxKeysNumber / 2) || (minSporkKeys > maxKeysNumber)) {
        LogPrintf("CSporkManager::SetMinSporkKeys -- Invalid min spork signers number: %d\n", minSporkKeys);
        return false;
    }
    nMinSporkKeys = minSporkKeys;
    return true;
}

bool CSporkManager::SetPrivKey(const std::string& strPrivKey)
{
    CKey key;
    CPubKey pubKey;
    if(!CMessageSigner::GetKeysFromSecret(strPrivKey, key, pubKey)) {
        LogPrintf("CSporkManager::SetPrivKey -- Failed to parse private key\n");
        return false;
    }

    if (setSporkPubKeyIDs.find(pubKey.GetID()) == setSporkPubKeyIDs.end()) {
        LogPrintf("CSporkManager::SetPrivKey -- New private key does not belong to spork addresses\n");
        return false;
    }

    CSporkMessage spork;
    if (!spork.Sign(key)) {
        LogPrintf("CSporkManager::SetPrivKey -- Test signing failed\n");
        return false;
    }

    // Test signing successful, proceed
    LOCK(cs);
    LogPrintf("CSporkManager::SetPrivKey -- Successfully initialized as spork signer\n");
    sporkPrivKey = key;
    return true;
}

std::string CSporkManager::ToString() const
{
    LOCK(cs);
    return strprintf("Sporks: %llu", mapSporksActive.size());
}

uint256 CSporkMessage::GetHash() const
{
    return SerializeHash(*this);
}

uint256 CSporkMessage::GetSignatureHash() const
{
    CHashWriter s(SER_GETHASH, 0);
    s << nSporkID;
    s << nValue;
    s << nTimeSigned;
    return s.GetHash();
}

bool CSporkMessage::Sign(const CKey& key)
{
    if (!key.IsValid()) {
        LogPrintf("CSporkMessage::Sign -- signing key is not valid\n");
        return false;
    }

    CKeyID pubKeyId = key.GetPubKey().GetID();


    uint256 hash = GetSignatureHash();

    if(!CHashSigner::SignHash(hash, key, vchSig)) {
        LogPrintf("CSporkMessage::Sign -- SignHash() failed\n");
        return false;
    }

    if (!CHashSigner::VerifyHash(hash, pubKeyId, vchSig)) {
        LogPrintf("CSporkMessage::Sign -- VerifyHash() failed\n");
        return false;
    }
  

    return true;
}

bool CSporkMessage::CheckSignature(const CKeyID& pubKeyId) const
{
    uint256 hash = GetSignatureHash();

    if (!CHashSigner::VerifyHash(hash, pubKeyId, vchSig)) {
        LogPrint(BCLog::SPORK, "CSporkMessage::CheckSignature -- VerifyHash() failed\n");
        return false;
    }
    return true;
}

bool CSporkMessage::GetSignerKeyID(CKeyID &retKeyidSporkSigner) const
{
    CPubKey pubkeyFromSig;
    if (!pubkeyFromSig.RecoverCompact(GetSignatureHash(), vchSig)) {
        return false;
    }

    retKeyidSporkSigner = pubkeyFromSig.GetID();
    return true;
}

void CSporkMessage::Relay(CConnman& connman) const
{
    CInv inv(MSG_SPORK, GetHash());
    connman.RelayOtherInv(inv);
}
