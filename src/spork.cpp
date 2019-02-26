// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "spork.h"

#include "base58.h"
#include "chainparams.h"
#include "validation.h"
#include "messagesigner.h"
#include "net_processing.h"
#include "netmessagemaker.h"
#include <key_io.h>
#include <boost/lexical_cast.hpp>
extern void Misbehaving(NodeId nodeid, int howmuch, const std::string& message="") EXCLUSIVE_LOCKS_REQUIRED(cs_main);
CSporkManager sporkManager;

std::map<uint256, CSporkMessage> mapSporks;
std::map<int, int64_t> mapSporkDefaults = {
    {SPORK_6_NEW_SIGS,                       0}, // ON
    {SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT, 0}, // ON
    {SPORK_9_SUPERBLOCKS_ENABLED,            0}, // ON
    {SPORK_10_MASTERNODE_PAY_UPDATED_NODES,  0}, // ON
    {SPORK_12_RECONSIDER_BLOCKS,             0},             // 0 BLOCKS
    {SPORK_14_REQUIRE_SENTINEL_FLAG,         0}, // ON
};

void CSporkManager::ProcessSpork(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    if(fLiteMode) return; // disable all Syscoin specific functionality

    if (strCommand == NetMsgType::SPORK) {

        CSporkMessage spork;
        vRecv >> spork;

        uint256 hash = spork.GetHash();

        std::string strLogMsg;
        {
            LOCK(cs_main);
            pfrom->setAskFor.erase(hash);
            if(!chainActive.Tip()) return;
            strLogMsg = strprintf("SPORK -- hash: %s id: %d value: %10d bestHeight: %d peer=%d", hash.ToString(), spork.nSporkID, spork.nValue, chainActive.Height(), pfrom->GetId());
        }

        if(mapSporksActive.count(spork.nSporkID)) {
            if (mapSporksActive[spork.nSporkID].nTimeSigned >= spork.nTimeSigned) {
                LogPrint(BCLog::SPORK, "%s seen\n", strLogMsg);
                return;
            } else {
                LogPrint(BCLog::SPORK, "%s updated\n", strLogMsg);
            }
        } else {
            LogPrint(BCLog::SPORK, "%s new\n", strLogMsg);
        }

        if(!spork.CheckSignature(sporkPubKeyID)) {
            LOCK(cs_main);
            LogPrint(BCLog::SPORK, "CSporkManager::ProcessSpork -- ERROR: invalid signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        mapSporks[hash] = spork;
        mapSporksActive[spork.nSporkID] = spork;
        spork.Relay(connman);

        //does a task if needed
        ExecuteSpork(spork.nSporkID, spork.nValue);

    } else if (strCommand == NetMsgType::GETSPORKS) {
        for (const auto& pair : mapSporksActive) {
            connman.PushMessage(pfrom, CNetMsgMaker(pfrom->GetSendVersion()).Make(NetMsgType::SPORK, pair.second));
        }
    }

}

void CSporkManager::ExecuteSpork(int nSporkID, int nValue)
{
    //correct fork via spork technology
    if(nSporkID == SPORK_12_RECONSIDER_BLOCKS && nValue > 0) {
        // allow to reprocess 24h of blocks max, which should be enough to resolve any issues
        int64_t nMaxBlocks = 1440;
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

bool CSporkManager::UpdateSpork(int nSporkID, int64_t nValue, CConnman& connman)
{

    CSporkMessage spork = CSporkMessage(nSporkID, nValue, GetAdjustedTime());

    if(spork.Sign(sporkPrivKey)) {
        spork.Relay(connman);
        mapSporks[spork.GetHash()] = spork;
        mapSporksActive[nSporkID] = spork;
        return true;
    }

    return false;
}

// grab the spork, otherwise say it's off
bool CSporkManager::IsSporkActive(int nSporkID)
{
    int64_t r = -1;

    if(mapSporksActive.count(nSporkID)){
        r = mapSporksActive[nSporkID].nValue;
    } else if (mapSporkDefaults.count(nSporkID)) {
        r = mapSporkDefaults[nSporkID];
    } else {
        LogPrint(BCLog::SPORK, "CSporkManager::IsSporkActive -- Unknown Spork ID %d\n", nSporkID);
        r = 4070908800ULL; // 2099-1-1 i.e. off by default
    }

    return r < GetAdjustedTime();
}

// grab the value of the spork on the network, or the default
int64_t CSporkManager::GetSporkValue(int nSporkID)
{
    if (mapSporksActive.count(nSporkID))
        return mapSporksActive[nSporkID].nValue;

    if (mapSporkDefaults.count(nSporkID)) {
        return mapSporkDefaults[nSporkID];
    }

    LogPrint(BCLog::SPORK, "CSporkManager::GetSporkValue -- Unknown Spork ID %d\n", nSporkID);
    return -1;
}

int CSporkManager::GetSporkIDByName(const std::string& strName)
{
    if (strName == "SPORK_6_NEW_SIGS")                          return SPORK_6_NEW_SIGS;
    if (strName == "SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT")    return SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT;
    if (strName == "SPORK_9_SUPERBLOCKS_ENABLED")               return SPORK_9_SUPERBLOCKS_ENABLED;
    if (strName == "SPORK_10_MASTERNODE_PAY_UPDATED_NODES")     return SPORK_10_MASTERNODE_PAY_UPDATED_NODES;
    if (strName == "SPORK_12_RECONSIDER_BLOCKS")                return SPORK_12_RECONSIDER_BLOCKS;
    if (strName == "SPORK_14_REQUIRE_SENTINEL_FLAG")            return SPORK_14_REQUIRE_SENTINEL_FLAG;

    LogPrint(BCLog::SPORK, "CSporkManager::GetSporkIDByName -- Unknown Spork name '%s'\n", strName);
    return -1;
}

std::string CSporkManager::GetSporkNameByID(int nSporkID)
{
    switch (nSporkID) {
        case SPORK_6_NEW_SIGS:                          return "SPORK_6_NEW_SIGS";
        case SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT:    return "SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT";
        case SPORK_9_SUPERBLOCKS_ENABLED:               return "SPORK_9_SUPERBLOCKS_ENABLED";
        case SPORK_10_MASTERNODE_PAY_UPDATED_NODES:     return "SPORK_10_MASTERNODE_PAY_UPDATED_NODES";
        case SPORK_12_RECONSIDER_BLOCKS:                return "SPORK_12_RECONSIDER_BLOCKS";
        case SPORK_14_REQUIRE_SENTINEL_FLAG:            return "SPORK_14_REQUIRE_SENTINEL_FLAG";
        default:
            LogPrint(BCLog::SPORK, "CSporkManager::GetSporkNameByID -- Unknown Spork ID %d\n", nSporkID);
            return "Unknown";
    }
}

bool CSporkManager::SetSporkAddress(const std::string& strAddress) {
    CTxDestination address = DecodeDestination(strAddress);
    const CKeyID *keyID = boost::get<CKeyID>(&address);
    if (!IsValidDestination(address) || !keyID) {
        LogPrint(BCLog::SPORK, "CSporkManager::SetSporkAddress -- Failed to parse spork address\n");
        return false;
    }
    sporkPubKeyID = *keyID;
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

    if (pubKey.GetID() != sporkPubKeyID) {
        LogPrint(BCLog::SPORK, "CSporkManager::SetPrivKey -- New private key does not belong to spork address\n");
        return false;
    }

    CSporkMessage spork;
    if (spork.Sign(key)) {
        // Test signing successful, proceed
        LogPrint(BCLog::SPORK, "CSporkManager::SetPrivKey -- Successfully initialized as spork signer\n");

        sporkPrivKey = key;
        return true;
    } else {
        LogPrint(BCLog::SPORK, "CSporkManager::SetPrivKey -- Test signing failed\n");
        return false;
    }
}

uint256 CSporkMessage::GetHash() const
{
    return SerializeHash(*this);
}

uint256 CSporkMessage::GetSignatureHash() const
{
    return GetHash();
}

bool CSporkMessage::Sign(const CKey& key)
{
    if (!key.IsValid()) {
        LogPrint(BCLog::SPORK, "CSporkMessage::Sign -- signing key is not valid\n");
        return false;
    }

    CKeyID pubKeyId = key.GetPubKey().GetID();
    std::string strError = "";

    if (sporkManager.IsSporkActive(SPORK_6_NEW_SIGS)) {
        uint256 hash = GetSignatureHash();

        if(!CHashSigner::SignHash(hash, key, vchSig)) {
            LogPrint(BCLog::SPORK, "CSporkMessage::Sign -- SignHash() failed\n");
            return false;
        }

        if (!CHashSigner::VerifyHash(hash, pubKeyId, vchSig, strError)) {
            LogPrint(BCLog::SPORK, "CSporkMessage::Sign -- VerifyHash() failed, error: %s\n", strError);
            return false;
        }
    } 

    return true;
}

bool CSporkMessage::CheckSignature(const CKeyID& pubKeyId) const
{
    std::string strError = "";

	if (sporkManager.IsSporkActive(SPORK_6_NEW_SIGS)) {
		uint256 hash = GetSignatureHash();

		if (!CHashSigner::VerifyHash(hash, pubKeyId, vchSig, strError)) {
			// Note: unlike for many other messages when SPORK_6_NEW_SIGS is ON sporks with sigs in old format
			// and newer timestamps should not be accepted, so if we failed here - that's it
			LogPrint(BCLog::SPORK, "CSporkMessage::CheckSignature -- VerifyHash() failed, error: %s\n", strError);
			return false;
		}
	}

    return true;
}

void CSporkMessage::Relay(CConnman& connman)
{
    CInv inv(MSG_SPORK, GetHash());
    g_connman->RelayInv(inv);
}
