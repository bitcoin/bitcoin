// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <spork.h>

#include <chainparams.h>
#include <consensus/params.h>
#include <flat-database.h>
#include <key_io.h>
#include <logging.h>
#include <messagesigner.h>
#include <net.h>
#include <netmessagemaker.h>
#include <primitives/block.h>
#include <protocol.h>
#include <script/standard.h>
#include <timedata.h>
#include <util/message.h> // for MESSAGE_MAGIC
#include <util/ranges.h>
#include <util/string.h>
#include <validation.h>

#include <string>

std::unique_ptr<CSporkManager> sporkManager;

const std::string SporkStore::SERIALIZATION_VERSION_STRING = "CSporkManager-Version-2";

std::optional<SporkValue> CSporkManager::SporkValueIfActive(SporkId nSporkID) const
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
    std::unordered_map<SporkValue, int> mapValueCounts;
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

PeerMsgRet CSporkManager::ProcessMessage(CNode& peer, CConnman& connman, std::string_view msg_type, CDataStream& vRecv)
{
    if (msg_type == NetMsgType::SPORK) {
        return ProcessSpork(peer, connman, vRecv);
    } else if (msg_type == NetMsgType::GETSPORKS) {
        ProcessGetSporks(peer, connman);
    }
    return {};
}

PeerMsgRet CSporkManager::ProcessSpork(const CNode& peer, CConnman& connman, CDataStream& vRecv)
{
    CSporkMessage spork;
    vRecv >> spork;

    uint256 hash = spork.GetHash();

    std::string strLogMsg;
    {
        LOCK(cs_main);
        EraseObjectRequest(peer.GetId(), CInv(MSG_SPORK, hash));
        if (!::ChainActive().Tip()) return {};
        strLogMsg = strprintf("SPORK -- hash: %s id: %d value: %10d bestHeight: %d peer=%d", hash.ToString(), spork.nSporkID, spork.nValue, ::ChainActive().Height(), peer.GetId());
    }

    if (spork.nTimeSigned > GetAdjustedTime() + 2 * 60 * 60) {
        LogPrint(BCLog::SPORK, "CSporkManager::ProcessSpork -- ERROR: too far into the future\n");
        return tl::unexpected{100};
    }

    auto opt_keyIDSigner = spork.GetSignerKeyID();

    if (opt_keyIDSigner == std::nullopt || WITH_LOCK(cs, return !setSporkPubKeyIDs.count(*opt_keyIDSigner))) {
        LogPrint(BCLog::SPORK, "CSporkManager::ProcessSpork -- ERROR: invalid signature\n");
        return tl::unexpected{100};
    }

    auto keyIDSigner = *opt_keyIDSigner;

    {
        LOCK(cs); // make sure to not lock this together with cs_main
        if (mapSporksActive.count(spork.nSporkID)) {
            if (mapSporksActive[spork.nSporkID].count(keyIDSigner)) {
                if (mapSporksActive[spork.nSporkID][keyIDSigner].nTimeSigned >= spork.nTimeSigned) {
                    LogPrint(BCLog::SPORK, "%s seen\n", strLogMsg);
                    return {};
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


    {
        LOCK(cs); // make sure to not lock this together with cs_main
        mapSporksByHash[hash] = spork;
        mapSporksActive[spork.nSporkID][keyIDSigner] = spork;
        // Clear cached values on new spork being processed
        WITH_LOCK(cs_mapSporksCachedActive, mapSporksCachedActive.erase(spork.nSporkID));
        WITH_LOCK(cs_mapSporksCachedValues, mapSporksCachedValues.erase(spork.nSporkID));
    }
    spork.Relay(connman);
    return {};
}

void CSporkManager::ProcessGetSporks(CNode& peer, CConnman& connman)
{
    LOCK(cs); // make sure to not lock this together with cs_main
    for (const auto& pair : mapSporksActive) {
        for (const auto& signerSporkPair : pair.second) {
            connman.PushMessage(&peer, CNetMsgMaker(peer.GetCommonVersion()).Make(NetMsgType::SPORK, signerSporkPair.second));
        }
    }
}


bool CSporkManager::UpdateSpork(SporkId nSporkID, SporkValue nValue, CConnman& connman)
{
    CSporkMessage spork(nSporkID, nValue, GetAdjustedTime());

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

    spork.Relay(connman);
    return true;
}

bool CSporkManager::IsSporkActive(SporkId nSporkID) const
{
    // If nSporkID is cached, and the cached value is true, then return early true
    {
        LOCK(cs_mapSporksCachedActive);
        if (auto it = mapSporksCachedActive.find(nSporkID); it != mapSporksCachedActive.end() && it->second) {
            return true;
        }
    }

    SporkValue nSporkValue = GetSporkValue(nSporkID);
    // Get time is somewhat costly it looks like
    bool ret = nSporkValue < GetAdjustedTime();
    // Only cache true values
    if (ret) {
        LOCK(cs_mapSporksCachedActive);
        mapSporksCachedActive[nSporkID] = ret;
    }
    return ret;
}

SporkValue CSporkManager::GetSporkValue(SporkId nSporkID) const
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

SporkId CSporkManager::GetSporkIDByName(std::string_view strName)
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
    const PKHash* pkhash = std::get_if<PKHash>(&dest);
    if (!pkhash) {
        LogPrintf("CSporkManager::SetSporkAddress -- Failed to parse spork address\n");
        return false;
    }
    setSporkPubKeyIDs.insert(ToKeyID(*pkhash));
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

    // Harden Spork6 so that it is active on testnet and no other networks
    if (std::string strError; Params().NetworkIDString() == CBaseChainParams::TESTNET) {
        uint256 hash = GetSignatureHash();

        if (!CHashSigner::SignHash(hash, key, vchSig)) {
            LogPrintf("CSporkMessage::Sign -- SignHash() failed\n");
            return false;
        }

        if (!CHashSigner::VerifyHash(hash, pubKeyId, vchSig, strError)) {
            LogPrintf("CSporkMessage::Sign -- VerifyHash() failed, error: %s\n", strError);
            return false;
        }
    } else {
        std::string strMessage = ToString(nSporkID) + ToString(nValue) + ToString(nTimeSigned);

        if (!CMessageSigner::SignMessage(strMessage, vchSig, key)) {
            LogPrintf("CSporkMessage::Sign -- SignMessage() failed\n");
            return false;
        }

        if (!CMessageSigner::VerifyMessage(pubKeyId, vchSig, strMessage, strError)) {
            LogPrintf("CSporkMessage::Sign -- VerifyMessage() failed, error: %s\n", strError);
            return false;
        }
    }

    return true;
}

bool CSporkMessage::CheckSignature(const CKeyID& pubKeyId) const
{
    // Harden Spork6 so that it is active on testnet and no other networks
    if (std::string strError; Params().NetworkIDString() == CBaseChainParams::TESTNET) {
        uint256 hash = GetSignatureHash();

        if (!CHashSigner::VerifyHash(hash, pubKeyId, vchSig, strError)) {
            LogPrint(BCLog::SPORK, "CSporkMessage::CheckSignature -- VerifyHash() failed, error: %s\n", strError);
            return false;
        }
    } else {
        std::string strMessage = ToString(nSporkID) + ToString(nValue) + ToString(nTimeSigned);

        if (!CMessageSigner::VerifyMessage(pubKeyId, vchSig, strMessage, strError)) {
            LogPrint(BCLog::SPORK, "CSporkMessage::CheckSignature -- VerifyMessage() failed, error: %s\n", strError);
            return false;
        }
    }

    return true;
}

std::optional<CKeyID> CSporkMessage::GetSignerKeyID() const
{
    CPubKey pubkeyFromSig;
    // Harden Spork6 so that it is active on testnet and no other networks
    if (Params().NetworkIDString() == CBaseChainParams::TESTNET) {
        if (!pubkeyFromSig.RecoverCompact(GetSignatureHash(), vchSig)) {
            return std::nullopt;
        }
    } else {
        std::string strMessage = ToString(nSporkID) + ToString(nValue) + ToString(nTimeSigned);
        CHashWriter ss(SER_GETHASH, 0);
        ss << MESSAGE_MAGIC;
        ss << strMessage;
        if (!pubkeyFromSig.RecoverCompact(ss.GetHash(), vchSig)) {
            return std::nullopt;
        }
    }

    return {pubkeyFromSig.GetID()};
}

void CSporkMessage::Relay(CConnman& connman) const
{
    CInv inv(MSG_SPORK, GetHash());
    connman.RelayInv(inv);
}
