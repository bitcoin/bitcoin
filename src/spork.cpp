// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <spork.h>

#include <chainparams.h>
#include <flat-database.h>
#include <key_io.h>
#include <logging.h>
#include <messagesigner.h>
#include <protocol.h>
#include <script/standard.h>
#include <timedata.h>
#include <util/helpers.h>
#include <util/string.h>

#include <string>

const std::string SporkStore::SERIALIZATION_VERSION_STRING = "CSporkManager-Version-3";

std::optional<SporkValue> CSporkManager::SporkValueIfActive(SporkId nSporkID) const
{
    AssertLockHeld(cs);

    if (auto it = mapSporksActive.find(nSporkID); it != mapSporksActive.end()) {
        return {it->second.nValue};
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

    if (sporkPubKeyID.IsNull()) return;

    for (auto itActive = mapSporksActive.begin(); itActive != mapSporksActive.end();) {
        if (!itActive->second.CheckSignature(sporkPubKeyID)) {
            mapSporksByHash.erase(itActive->second.GetHash());
            mapSporksActive.erase(itActive++);
            continue;
        }
        ++itActive;
    }

    for (auto itByHash = mapSporksByHash.begin(); itByHash != mapSporksByHash.end();) {
        if (!itByHash->second.CheckSignature(sporkPubKeyID)) {
            mapSporksByHash.erase(itByHash++);
            continue;
        }
        ++itByHash;
    }
}

bool CSporkManager::IsValidSpork(const CSporkMessage& spork) const
{
    const auto max_time{std::chrono::time_point_cast<std::chrono::seconds>(GetAdjustedTime() + 2h)};
    if (spork.TimeSigned() > max_time) {
        LogPrint(BCLog::SPORK, "CSporkManager::%s -- ERROR: too far into the future\n", __func__);
        return false;
    }

    // Copy the key out instead of verifying under `cs`: signature verification is expensive and
    // this runs for every spork message received from any peer, before it is scored as misbehaving.
    const CKeyID pubKeyId{WITH_LOCK(cs, return sporkPubKeyID)};
    if (pubKeyId.IsNull() || !spork.CheckSignature(pubKeyId)) {
        LogPrint(BCLog::SPORK, "CSporkManager::%s -- ERROR: invalid signature\n", __func__);
        return false;
    }
    return true;
}

bool CSporkManager::ProcessSpork(const CSporkMessage& spork, std::string_view peer_log_suffix)
{
    uint256 hash = spork.GetHash();
    std::string strLogMsg{strprintf("SPORK -- hash: %s id: %d value: %10d%s", hash.ToString(), spork.nSporkID,
                                    spork.nValue, peer_log_suffix)};

    LOCK(cs); // make sure to not lock this together with cs_main
    if (auto it = mapSporksActive.find(spork.nSporkID); it != mapSporksActive.end()) {
        if (it->second.nTimeSigned >= spork.nTimeSigned) {
            LogPrint(BCLog::SPORK, "%s seen\n", strLogMsg);
            return false;
        }
        LogPrintf("%s updated\n", strLogMsg);
    } else {
        LogPrintf("%s new\n", strLogMsg);
    }

    mapSporksByHash[hash] = spork;
    mapSporksActive[spork.nSporkID] = spork;

    return true;
}

std::vector<CSporkMessage> CSporkManager::ActiveSporks() const
{
    LOCK(cs); // make sure to not lock this together with cs_main
    std::vector<CSporkMessage> sporks;
    sporks.reserve(mapSporksActive.size());
    for (const auto& [_, spork] : mapSporksActive) {
        sporks.push_back(spork);
    }
    return sporks;
}

std::optional<CInv> CSporkManager::UpdateSpork(SporkId nSporkID, SporkValue nValue)
{
    LOCK(cs);

    const auto now = GetAdjustedTime();
    auto spork_time = now;
    if (const auto it = mapSporksActive.find(nSporkID); it != mapSporksActive.end()) {
        const auto min_next_time = it->second.TimeSigned() + 1s;
        if (spork_time < min_next_time) {
            spork_time = min_next_time;
        }
    }
    if (spork_time > now + 2h) {
        LogPrintf("CSporkManager::%s -- ERROR: spork %d needs a timestamp too far into the future for peers\n", __func__, nSporkID);
        return std::nullopt;
    }
    CSporkMessage spork(nSporkID, nValue, spork_time);

    if (!spork.Sign(sporkPrivKey)) {
        LogPrintf("CSporkManager::%s -- ERROR: signing failed for spork %d\n", __func__, nSporkID);
        return std::nullopt;
    }

    if (!spork.CheckSignature(sporkPubKeyID)) {
        LogPrintf("CSporkManager::UpdateSpork: private key does not match spork address\n");
        return std::nullopt;
    }

    LogPrintf("CSporkManager::%s -- signed %d %s\n", __func__, nSporkID, spork.GetHash().ToString());

    mapSporksByHash[spork.GetHash()] = spork;
    mapSporksActive[nSporkID] = spork;

    CInv inv(MSG_SPORK, spork.GetHash());
    return inv;
}

bool CSporkManager::IsSporkActive(SporkId nSporkID) const
{
    const SporkValue nSporkValue = GetSporkValue(nSporkID);
    const NodeSeconds activation_time{std::chrono::seconds{nSporkValue}};
    const auto now{std::chrono::time_point_cast<std::chrono::seconds>(GetAdjustedTime())};
    return activation_time < now;
}

SporkValue CSporkManager::GetSporkValue(SporkId nSporkID) const
{
    // Harden all sporks on Mainnet
    if (!Params().IsTestChain()) {
        switch (nSporkID) {
            case SPORK_21_QUORUM_ALL_CONNECTED:
                return 1;
            default:
                return 0;
        }
    }

    LOCK(cs);

    if (auto opt_sporkValue = SporkValueIfActive(nSporkID)) {
        return *opt_sporkValue;
    }

    if (auto optSpork = util::find_if_opt(sporkDefs,
                                          [&nSporkID](const auto& sporkDef) { return sporkDef.sporkId == nSporkID; })) {
        return optSpork->defaultValue;
    } else {
        LogPrint(BCLog::SPORK, "CSporkManager::GetSporkValue -- Unknown Spork ID %d\n", nSporkID);
        return -1;
    }
}

SporkId CSporkManager::GetSporkIDByName(std::string_view strName)
{
    if (auto optSpork = util::find_if_opt(sporkDefs,
                                          [&strName](const auto& sporkDef) { return sporkDef.name == strName; })) {
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
    sporkPubKeyID = ToKeyID(*pkhash);
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
    if (pubKey.GetID() != sporkPubKeyID) {
        LogPrintf("CSporkManager::SetPrivKey -- New private key does not belong to spork address\n");
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
