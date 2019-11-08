// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <spork.h>

#include <chainparams.h>
#include <messagesigner.h>
#include <key_io.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <util/validation.h>
#include <util/init.h>
#include <validation.h>
#include <string>

CSporkManager sporkManager;

const std::string CSporkManager::SERIALIZATION_VERSION_STRING = "CSporkManager-Version-2";

std::map<int, int64_t> mapSporkDefaults = {
    {SPORK_1_SUPERBLOCKS_ENABLED,           4070908800ULL}, // OFF
    {SPORK_2_RECONSIDER_BLOCKS,             4070908800ULL}, // OFF
    {SPORK_3_QUORUM_DKG_ENABLED,            4070908800ULL}, // OFF
    {SPORK_4_CHAINLOCKS_ENABLED,            4070908800ULL}, // OFF
    {SPORK_5_INSTANTSEND_ENABLED,           0},             // ON
    {SPORK_6_INSTANTSEND_BLOCK_FILTERING,   0},             // ON
    {SPORK_7_INSTANTSEND_AUTOLOCKS,         4070908800ULL}, // OFF
};

bool CSporkManager::SporkValueIsActive(int nSporkID, int64_t &nActiveValueRet) const
{
    LOCK(cs);

    if (!mapSporksActive.count(nSporkID)) return false;

    // calc how many values we have and how many signers vote for every value
    std::unordered_map<int64_t, int> mapValueCounts;
    for (const auto& pair: mapSporksActive.at(nSporkID)) {
        mapValueCounts[pair.second.nValue]++;
        if (mapValueCounts.at(pair.second.nValue) >= nMinSporkKeys) {
            // nMinSporkKeys is always more than the half of the max spork keys number,
            // so there is only one such value and we can stop here
            nActiveValueRet = pair.second.nValue;
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
            if (!itSignerPair->second.CheckSignature(itSignerPair->first, false)) {
                if (!itSignerPair->second.CheckSignature(itSignerPair->first, true)) {
                    mapSporksByHash.erase(itSignerPair->second.GetHash());
                    itActive->second.erase(itSignerPair++);
                    continue;
                }
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
            if (itByHash->second.CheckSignature(signer, false) ||
                itByHash->second.CheckSignature(signer, true)) {
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

void CSporkManager::ProcessSpork(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    if(fLiteMode) return; // disable all BitGreen specific functionality

    if (strCommand == NetMsgType::SPORK) {
        CSporkMessage spork;
        vRecv >> spork;

        uint256 hash = spork.GetHash();

        std::string strLogMsg;
        {
            LOCK(cs_main);
            RemoveDataRequest(-1, CInv(MSG_SPORK, hash));
            if(!ChainActive().Tip()) return;
            strLogMsg = strprintf("SPORK -- hash: %s id: %d value: %10d bestHeight: %d peer=%d", hash.ToString(), spork.nSporkID, spork.nValue, ChainActive().Height(), pfrom->GetId());
        }

        if (spork.nTimeSigned > GetAdjustedTime() + 2 * 60 * 60) {
            LOCK(cs_main);
            LogPrint(BCLog::SPORK, "CSporkManager::%s -- ERROR: too far into the future\n", __func__);
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        CKeyID keyIDSigner;
        if (!spork.GetSignerKeyID(keyIDSigner, false) || !setSporkPubKeyIDs.count(keyIDSigner)) {
            LOCK(cs_main);
            LogPrint(BCLog::SPORK, "CSporkManager::%s -- ERROR: invalid signature\n", __func__);
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        {
            LOCK(cs); // make sure to not lock this together with cs_main
            if (mapSporksActive.count(spork.nSporkID)) {
                if (mapSporksActive[spork.nSporkID].count(keyIDSigner)) {
                    if (mapSporksActive[spork.nSporkID][keyIDSigner].nTimeSigned >= spork.nTimeSigned) {
                        LogPrint(BCLog::SPORK, "CSporkManager::%s -- %s seen\n", __func__, strLogMsg);
                        return;
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
        }
        spork.Relay();

        //does a task if needed
        int64_t nActiveValue = 0;
        if (SporkValueIsActive(spork.nSporkID, nActiveValue)) {
            ExecuteSpork(spork.nSporkID, nActiveValue);
        }

    } else if (strCommand == NetMsgType::GETSPORKS) {
        LOCK(cs); // make sure to not lock this together with cs_main
        for (const auto& pair : mapSporksActive) {
            for (const auto& signerSporkPair: pair.second) {
                g_connman->PushMessage(pfrom, CNetMsgMaker(pfrom->GetSendVersion()).Make(NetMsgType::SPORK, signerSporkPair.second));
            }
        }
    }

}

void CSporkManager::ExecuteSpork(int nSporkID, int nValue)
{
    //correct fork via spork technology
    if(nSporkID == SPORK_2_RECONSIDER_BLOCKS && nValue > 0) {
        // allow to reprocess 24h of blocks max, which should be enough to resolve any issues
        int64_t nMaxBlocks = 576;
        // this potentially can be a heavy operation, so only allow this to be executed once per 10 minutes
        int64_t nTimeout = 10 * 60;

        static int64_t nTimeExecuted = 0; // i.e. it was never executed before

        if(GetTime() - nTimeExecuted < nTimeout) {
            LogPrint(BCLog::SPORK, "CSporkManager::ExecuteSpork -- ERROR: Trying to reconsider blocks, too soon - %d/%d\n", GetTime() - nTimeExecuted, nTimeout);
            return;
        }

        if(nValue > nMaxBlocks) {
            LogPrint(BCLog::SPORK, "CSporkManager::ExecuteSpork -- ERROR: Trying to reconsider too many blocks %d/%d\n", nValue, nMaxBlocks);
            return;
        }


        LogPrint(BCLog::SPORK, "CSporkManager::ExecuteSpork -- Reconsider Last %d Blocks\n", nValue);

        ReprocessBlocks(nValue);
        nTimeExecuted = GetTime();
    }
}

bool CSporkManager::UpdateSpork(int nSporkID, int64_t nValue)
{
    CSporkMessage spork = CSporkMessage(nSporkID, nValue, GetAdjustedTime());

    if(spork.Sign(sporkPrivKey, false)) {
        CKeyID keyIDSigner;
        if (!spork.GetSignerKeyID(keyIDSigner, false) || !setSporkPubKeyIDs.count(keyIDSigner)) {
            LogPrint(BCLog::SPORK, "CSporkManager::UpdateSpork: failed to find keyid for private key\n");
            return false;
        }
        {
            LOCK(cs);
            mapSporksByHash[spork.GetHash()] = spork;
            mapSporksActive[nSporkID][keyIDSigner] = spork;
        }
        spork.Relay();
        return true;
    }

    return false;
}

bool CSporkManager::IsSporkActive(int nSporkID)
{
    LOCK(cs);
    int64_t nSporkValue = -1;

    if (SporkValueIsActive(nSporkID, nSporkValue)) {
        return nSporkValue < GetAdjustedTime();
    }

    if (mapSporkDefaults.count(nSporkID)) {
        return  mapSporkDefaults[nSporkID] < GetAdjustedTime();
    }

    LogPrint(BCLog::SPORK, "CSporkManager::IsSporkActive -- Unknown Spork ID %d\n", nSporkID);
    return false;
}

int64_t CSporkManager::GetSporkValue(int nSporkID)
{
    LOCK(cs);

    int64_t nSporkValue = -1;
    if (SporkValueIsActive(nSporkID, nSporkValue)) {
        return nSporkValue;
    }

    if (mapSporkDefaults.count(nSporkID)) {
        return mapSporkDefaults[nSporkID];
    }

    LogPrint(BCLog::SPORK, "CSporkManager::GetSporkValue -- Unknown Spork ID %d\n", nSporkID);
    return -1;
}

int CSporkManager::GetSporkIDByName(const std::string& strName)
{
    if (strName == "SPORK_1_SUPERBLOCKS_ENABLED")               return SPORK_1_SUPERBLOCKS_ENABLED;
    if (strName == "SPORK_2_RECONSIDER_BLOCKS")                 return SPORK_2_RECONSIDER_BLOCKS;
    if (strName == "SPORK_3_QUORUM_DKG_ENABLED")                return SPORK_3_QUORUM_DKG_ENABLED;
    if (strName == "SPORK_4_CHAINLOCKS_ENABLED")                return SPORK_4_CHAINLOCKS_ENABLED;
    if (strName == "SPORK_5_INSTANTSEND_ENABLED")               return SPORK_5_INSTANTSEND_ENABLED;
    if (strName == "SPORK_6_INSTANTSEND_BLOCK_FILTERING")       return SPORK_6_INSTANTSEND_BLOCK_FILTERING;
    if (strName == "SPORK_7_INSTANTSEND_AUTOLOCKS")             return SPORK_7_INSTANTSEND_AUTOLOCKS;

    LogPrint(BCLog::SPORK, "CSporkManager::GetSporkIDByName -- Unknown Spork name '%s'\n", strName);
    return -1;
}

std::string CSporkManager::GetSporkNameByID(int nSporkID)
{
    switch (nSporkID) {
        case SPORK_1_SUPERBLOCKS_ENABLED:               return "SPORK_1_SUPERBLOCKS_ENABLED";
        case SPORK_2_RECONSIDER_BLOCKS:                 return "SPORK_2_RECONSIDER_BLOCKS";
        case SPORK_3_QUORUM_DKG_ENABLED:                return "SPORK_3_QUORUM_DKG_ENABLED";
        case SPORK_4_CHAINLOCKS_ENABLED:                return "SPORK_4_CHAINLOCKS_ENABLED";
        case SPORK_5_INSTANTSEND_ENABLED:               return "SPORK_5_INSTANTSEND_ENABLED";
        case SPORK_6_INSTANTSEND_BLOCK_FILTERING:       return "SPORK_6_INSTANTSEND_BLOCK_FILTERING";
        case SPORK_7_INSTANTSEND_AUTOLOCKS:             return "SPORK_7_INSTANTSEND_AUTOLOCKS";
        default:
            LogPrint(BCLog::SPORK, "CSporkManager::GetSporkNameByID -- Unknown Spork ID %d\n", nSporkID);
            return "Unknown";
    }
}

bool CSporkManager::GetSporkByHash(const uint256& hash, CSporkMessage &sporkRet)
{
    LOCK(cs);

    const auto it = mapSporksByHash.find(hash);

    if (it == mapSporksByHash.end())
        return false;

    sporkRet = it->second;

    return true;
}

bool CSporkManager::SetSporkAddress(const std::string& strAddress)
{
    LOCK(cs);
    CTxDestination address = DecodeDestination(strAddress);
    auto *keyID = boost::get<PKHash>(&address);
    if (!IsValidDestination(address) || !keyID) {
        LogPrint(BCLog::SPORK, "CSporkManager::SetSporkAddress -- Failed to parse spork address\n");
        return false;
    }
    setSporkPubKeyIDs.insert(CKeyID(*keyID));
    return true;
}

bool CSporkManager::SetMinSporkKeys(int minSporkKeys)
{
    int maxKeysNumber = setSporkPubKeyIDs.size();
    if ((minSporkKeys <= maxKeysNumber / 2) || (minSporkKeys > maxKeysNumber)) {
        LogPrint(BCLog::SPORK, "CSporkManager::SetMinSporkKeys -- Invalid min spork signers number: %d\n", minSporkKeys);
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
        LogPrint(BCLog::SPORK, "CSporkManager::SetPrivKey -- Failed to parse private key\n");
        return false;
    }

    if (setSporkPubKeyIDs.find(pubKey.GetID()) == setSporkPubKeyIDs.end()) {
        LogPrint(BCLog::SPORK, "CSporkManager::SetPrivKey -- New private key does not belong to spork addresses\n");
        return false;
    }

    CSporkMessage spork;
    if (spork.Sign(key, false)) {
	    LOCK(cs);
        // Test signing successful, proceed
        LogPrint(BCLog::SPORK, "CSporkManager::SetPrivKey -- Successfully initialized as spork signer\n");

        sporkPrivKey = key;
        return true;
    } else {
        LogPrint(BCLog::SPORK, "CSporkManager::SetPrivKey -- Test signing failed\n");
        return false;
    }
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

bool CSporkMessage::Sign(const CKey& key, bool fSporkSixActive)
{
    if (!key.IsValid()) {
        LogPrintf("CSporkMessage::Sign -- signing key is not valid\n");
        return false;
    }

    CKeyID pubKeyId = key.GetPubKey().GetID();
    std::string strError = "";

    if (fSporkSixActive) {
        uint256 hash = GetSignatureHash();

        if(!CHashSigner::SignHash(hash, key, vchSig)) {
            LogPrintf("CSporkMessage::Sign -- SignHash() failed\n");
            return false;
        }

        if (!CHashSigner::VerifyHash(hash, pubKeyId, vchSig, strError)) {
            LogPrintf("CSporkMessage::Sign -- VerifyHash() failed, error: %s\n", strError);
            return false;
        }
    } else {
        std::string strMessage = std::to_string(nSporkID) + std::to_string(nValue) + std::to_string(nTimeSigned);

        if(!CMessageSigner::SignMessage(strMessage, vchSig, key)) {
            LogPrintf("CSporkMessage::Sign -- SignMessage() failed\n");
            return false;
        }

        if(!CMessageSigner::VerifyMessage(pubKeyId, vchSig, strMessage, strError)) {
            LogPrintf("CSporkMessage::Sign -- VerifyMessage() failed, error: %s\n", strError);
            return false;
        }
    }

    return true;
}

bool CSporkMessage::CheckSignature(const CKeyID& pubKeyId, bool fSporkSixActive) const
{
    std::string strError = "";

    if (fSporkSixActive) {
        uint256 hash = GetSignatureHash();

        if (!CHashSigner::VerifyHash(hash, pubKeyId, vchSig, strError)) {
            LogPrintf("CSporkMessage::CheckSignature -- VerifyHash() failed, error: %s\n", strError);
            return false;
        }
    } else {
        std::string strMessage = std::to_string(nSporkID) + std::to_string(nValue) + std::to_string(nTimeSigned);

        if (!CMessageSigner::VerifyMessage(pubKeyId, vchSig, strMessage, strError)){
            uint256 hash = GetSignatureHash();
            if (!CHashSigner::VerifyHash(hash, pubKeyId, vchSig, strError)) {
                LogPrintf("CSporkMessage::CheckSignature -- VerifyHash() failed, error: %s\n", strError);
                return false;
            }
        }
    }

    return true;
}

bool CSporkMessage::GetSignerKeyID(CKeyID &retKeyidSporkSigner, bool fSporkSixActive)
{
    CPubKey pubkeyFromSig;
    if (fSporkSixActive) {
        if (!pubkeyFromSig.RecoverCompact(GetSignatureHash(), vchSig)) {
            return false;
        }
    } else {
        std::string strMessage = std::to_string(nSporkID) + std::to_string(nValue) + std::to_string(nTimeSigned);
        CHashWriter ss(SER_GETHASH, 0);
        ss << strMessageMagic;
        ss << strMessage;
        if (!pubkeyFromSig.RecoverCompact(ss.GetHash(), vchSig)) {
            return false;
        }
    }

    retKeyidSporkSigner = pubkeyFromSig.GetID();
    return true;
}

void CSporkMessage::Relay()
{
    CInv inv(MSG_SPORK, GetHash());
    g_connman->RelayInv(inv);
}
