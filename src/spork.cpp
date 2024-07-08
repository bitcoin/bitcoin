// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <spork.h>
#include <validation.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <flatdatabase.h>
#include <key_io.h>
#include <logging.h>
#include <messagesigner.h>
#include <net.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <primitives/block.h>
#include <protocol.h>
#include <timedata.h>
#include <util/message.h> // for MESSAGE_MAGIC
#include <util/ranges.h>
#include <util/string.h>
#include <validation.h>
#include <key_io.h>
#include <string>

std::unique_ptr<CSporkManager> sporkManager;

const std::string SporkStore::SERIALIZATION_VERSION_STRING = "CSporkManager-Version-2";

std::optional<int64_t> CSporkManager::SporkValueIfActive(int32_t nSporkID) const
{
    AssertLockHeld(cs);

    if (!mapSporksActive.count(nSporkID)) return std::nullopt;

    {
        LOCK(cs_mapSporksCachedValues);
        if (auto it = mapSporksCachedValues.find(nSporkID); it != mapSporksCachedValues.end()) {
            return {it->second};
        }
    }

    // calc how many values we have and how many signers vote for every value
    std::unordered_map<int64_t, int> mapValueCounts;
    for (const auto& [_, spork] : mapSporksActive.at(nSporkID)) {
        mapValueCounts[spork.nValue]++;
        if (mapValueCounts.at(spork.nValue) >= nMinSporkKeys) {
            // nMinSporkKeys is always more than the half of the max spork keys number,
            // so there is only one such value and we can stop here
            {
                LOCK(cs_mapSporksCachedValues);
                mapSporksCachedValues[nSporkID] = spork.nValue;
            }
            return {spork.nValue};
        }
    }

    return std::nullopt;
}

void SporkStore::Clear()
{
    LOCK(cs);
    mapSporksActive.clear();
    mapSporksByHash.clear();
    // sporkPubKeyID and sporkPrivKey should be set in init.cpp,
    // we should not alter them here.
}

CSporkManager::CSporkManager() :
    m_db{std::make_unique<db_type>("sporks.dat", "magicSporkCache")}
{
}

CSporkManager::~CSporkManager()
{
    if (!is_valid) return;
    m_db->Store(*this);
}

bool CSporkManager::LoadCache()
{
    assert(m_db != nullptr);
    is_valid = m_db->Load(*this);
    if (is_valid) {
        CheckAndRemove();
    }
    return is_valid;
}

void CSporkManager::CheckAndRemove()
{
    LOCK(cs);

    if (setSporkPubKeyIDs.empty()) return;

    for (auto itActive = mapSporksActive.begin(); itActive != mapSporksActive.end();) {
        auto itSignerPair = itActive->second.begin();
        while (itSignerPair != itActive->second.end()) {
            bool fHasValidSig = setSporkPubKeyIDs.find(itSignerPair->first) != setSporkPubKeyIDs.end() &&
                                itSignerPair->second.CheckSignature(itSignerPair->first);
            if (!fHasValidSig) {
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

    for (auto itByHash = mapSporksByHash.begin(); itByHash != mapSporksByHash.end();) {
        bool found = false;
        for (const auto& signer : setSporkPubKeyIDs) {
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

void CSporkManager::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman, PeerManager& peerman)
{
    if (strCommand == NetMsgType::SPORK) {
        ProcessSpork(pfrom, vRecv, peerman);
    } else if (strCommand == NetMsgType::GETSPORKS) {
        ProcessGetSporks(pfrom, connman);
    }
}

void CSporkManager::ProcessSpork(CNode* pfrom, CDataStream& vRecv, PeerManager& peerman) {
    CSporkMessage spork;
    vRecv >> spork;

    const uint256 &hash = spork.GetHash();

    std::string strLogMsg;
    PeerRef peer = peerman.GetPeerRef(pfrom->GetId());
    if (peer)
        peerman.AddKnownTx(*peer, hash);
    {
        LOCK(cs_main);
        peerman.ReceivedResponse(pfrom->GetId(), hash);
        strLogMsg = strprintf("SPORK -- hash: %s id: %d value: %10d peer=%d", hash.ToString(), spork.nSporkID, spork.nValue, pfrom->GetId());
    }

    if (spork.nTimeSigned > TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime()) + 2 * 60 * 60) {
        {
            LOCK(cs_main);
            peerman.ForgetTxHash(pfrom->GetId(), hash);
        }
        LogPrint(BCLog::SPORK, "CSporkManager::ProcessSpork -- ERROR: too far into the future\n");
        if(peer)
            peerman.Misbehaving(*peer, 100, "spork too far into the future");
        return;
    }

    auto opt_keyIDSigner = spork.GetSignerKeyID();

    if (opt_keyIDSigner == std::nullopt || WITH_LOCK(cs, return !setSporkPubKeyIDs.count(*opt_keyIDSigner))) {
        {
            LOCK(cs_main);
            peerman.ForgetTxHash(pfrom->GetId(), hash);
        }
        LogPrint(BCLog::SPORK, "CSporkManager::ProcessSpork -- ERROR: invalid signature\n");
        if(peer)
            peerman.Misbehaving(*peer, 100, "invalid spork signature");
        return;
    }

    auto keyIDSigner = *opt_keyIDSigner;
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
        WITH_LOCK(cs_mapSporksCachedActive, mapSporksCachedActive.erase(spork.nSporkID));
        WITH_LOCK(cs_mapSporksCachedValues, mapSporksCachedValues.erase(spork.nSporkID));
    }
    spork.Relay(peerman);
    {
        LOCK(cs_main);
        peerman.ForgetTxHash(pfrom->GetId(), hash);
    }
}

void CSporkManager::ProcessGetSporks(CNode* pfrom, CConnman& connman)
{
    LOCK(cs); // make sure to not lock this together with cs_main
    for (const auto& pair : mapSporksActive) {
        for (const auto& signerSporkPair : pair.second) {
            connman.PushMessage(pfrom, CNetMsgMaker(pfrom->GetCommonVersion()).Make(NetMsgType::SPORK, signerSporkPair.second));
        }
    }
}


bool CSporkManager::UpdateSpork(int32_t nSporkID, int64_t nValue, PeerManager& peerman)
{
    CSporkMessage spork(nSporkID, nValue, TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime()));

    {
        LOCK(cs);

        if (!spork.Sign(sporkPrivKey)) {
            LogPrintf("CSporkManager::%s -- ERROR: signing failed for spork %d\n", __func__, nSporkID);
            return false;
        }

        auto opt_keyIDSigner = spork.GetSignerKeyID();
        if (opt_keyIDSigner == std::nullopt || !setSporkPubKeyIDs.count(*opt_keyIDSigner)) {
            LogPrintf("CSporkManager::UpdateSpork: failed to find keyid for private key\n");
            return false;
        }

        LogPrintf("CSporkManager::%s -- signed %d %s\n", __func__, nSporkID, spork.GetHash().ToString());

        mapSporksByHash[spork.GetHash()] = spork;
        mapSporksActive[nSporkID][*opt_keyIDSigner] = spork;
        // Clear cached values on new spork being processed
        WITH_LOCK(cs_mapSporksCachedActive, mapSporksCachedActive.erase(spork.nSporkID));
        WITH_LOCK(cs_mapSporksCachedValues, mapSporksCachedValues.erase(spork.nSporkID));
    }

    spork.Relay(peerman);
    return true;
}

bool CSporkManager::IsSporkActive(int32_t nSporkID) const
{
    // If nSporkID is cached, and the cached value is true, then return early true
    {
        LOCK(cs_mapSporksCachedActive);
        if (auto it = mapSporksCachedActive.find(nSporkID); it != mapSporksCachedActive.end() && it->second) {
            return true;
        }
    }

    int64_t nSporkValue = GetSporkValue(nSporkID);
    // Get time is somewhat costly it looks like
    bool ret = nSporkValue < TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime());
    // Only cache true values
    if (ret) {
        LOCK(cs_mapSporksCachedActive);
        mapSporksCachedActive[nSporkID] = ret;
    }
    return ret;
}

int64_t CSporkManager::GetSporkValue(int32_t nSporkID) const
{
    LOCK(cs);

    if (auto opt_sporkValue = SporkValueIfActive(nSporkID)) {
        return *opt_sporkValue;
    }


    if (auto optSpork = ranges::find_if_opt(sporkDefs,
                                            [&nSporkID](const auto& sporkDef){return sporkDef.sporkId == nSporkID;})) {
        return optSpork->defaultValue;
    } else {
        LogPrint(BCLog::SPORK, "CSporkManager::GetSporkValue -- Unknown Spork ID %d\n", nSporkID);
        return -1;
    }
}

int32_t CSporkManager::GetSporkIDByName(std::string_view strName)
{
    if (auto optSpork = ranges::find_if_opt(sporkDefs,
                                            [&strName](const auto& sporkDef){return sporkDef.name == strName;})) {
        return optSpork->sporkId;
    }

    LogPrint(BCLog::SPORK, "CSporkManager::GetSporkIDByName -- Unknown Spork name '%s'\n", strName);
    return SPORK_INVALID;
}

std::optional<CSporkMessage> CSporkManager::GetSporkByHash(const uint256& hash) const
{
    LOCK(cs);

    if (const auto it = mapSporksByHash.find(hash); it != mapSporksByHash.end())
        return {it->second};

    return std::nullopt;
}

bool CSporkManager::SetSporkAddress(const std::string& strAddress)
{
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
    LOCK(cs);
    if (int maxKeysNumber = setSporkPubKeyIDs.size(); (minSporkKeys <= maxKeysNumber / 2) || (minSporkKeys > maxKeysNumber)) {
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
    if (!CMessageSigner::GetKeysFromSecret(strPrivKey, key, pubKey)) {
        LogPrintf("CSporkManager::SetPrivKey -- Failed to parse private key\n");
        return false;
    }

    LOCK(cs);
    if (setSporkPubKeyIDs.find(pubKey.GetID()) == setSporkPubKeyIDs.end()) {
        LogPrintf("CSporkManager::SetPrivKey -- New private key does not belong to spork addresses\n");
        return false;
    }

    if (!CSporkMessage().Sign(key)) {
        LogPrintf("CSporkManager::SetPrivKey -- Test signing failed\n");
        return false;
    }

    // Test signing successful, proceed
    LogPrintf("CSporkManager::SetPrivKey -- Successfully initialized as spork signer\n");
    sporkPrivKey = key;
    return true;
}

std::string SporkStore::ToString() const
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
    const uint256 &hash = GetSignatureHash();

    if (!CHashSigner::SignHash(hash, key, vchSig)) {
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

    const uint256 &hash = GetSignatureHash();

    if (!CHashSigner::VerifyHash(hash, pubKeyId, vchSig)) {
        LogPrint(BCLog::SPORK, "CSporkMessage::CheckSignature -- VerifyHash() failed\n");
        return false;
    }
    return true;
}

std::optional<CKeyID> CSporkMessage::GetSignerKeyID() const
{
    CPubKey pubkeyFromSig;

    if (!pubkeyFromSig.RecoverCompact(GetSignatureHash(), vchSig)) {
        return std::nullopt;
    }

    return {pubkeyFromSig.GetID()};
}

void CSporkMessage::Relay(PeerManager& peerman) const
{
    CInv inv(MSG_SPORK, GetHash());
    peerman.RelayTransactionOther(inv);
}
