// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "darksend.h"
#include "main.h"
#include "spork.h"

#include <boost/lexical_cast.hpp>

class CSporkMessage;
class CSporkManager;

CSporkManager sporkManager;

std::map<uint256, CSporkMessage> mapSporks;

void CSporkManager::ProcessSpork(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(fLiteMode) return; // disable all Dash specific functionality

    if (strCommand == NetMsgType::SPORK)
    {
        // LogPrintf("CSporkManager::ProcessSpork\n");
        CDataStream vMsg(vRecv);
        CSporkMessage spork;
        vRecv >> spork;

        LOCK(cs_main);
        if(chainActive.Tip() == NULL) return;

        uint256 hash = spork.GetHash();
        if(mapSporksActive.count(spork.nSporkID)) {
            if(mapSporksActive[spork.nSporkID].nTimeSigned >= spork.nTimeSigned){
                if(fDebug) LogPrintf("CSporkManager::ProcessSpork -- seen %s block %d \n", hash.ToString(), chainActive.Tip()->nHeight);
                return;
            } else {
                if(fDebug) LogPrintf("CSporkManager::ProcessSpork -- got updated spork %s block %d \n", hash.ToString(), chainActive.Tip()->nHeight);
            }
        }

        LogPrintf("spork -- new %s ID %d Time %d bestHeight %d\n", hash.ToString(), spork.nSporkID, spork.nValue, chainActive.Tip()->nHeight);

        if(!spork.CheckSignature()){
            LogPrintf("CSporkManager::ProcessSpork -- invalid signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        mapSporks[hash] = spork;
        mapSporksActive[spork.nSporkID] = spork;
        spork.Relay();

        //does a task if needed
        ExecuteSpork(spork.nSporkID, spork.nValue);
    }
    if (strCommand == NetMsgType::GETSPORKS)
    {
        std::map<int, CSporkMessage>::iterator it = mapSporksActive.begin();

        while(it != mapSporksActive.end()) {
            pfrom->PushMessage(NetMsgType::SPORK, it->second);
            it++;
        }
    }

}

void CSporkManager::ExecuteSpork(int nSporkID, int nValue)
{
    // if(nSporkID == SPORK_11_RESET_BUDGET && nValue == 1){
    //     budget.Clear();
    // }

    //correct fork via spork technology
    if(nSporkID == SPORK_12_RECONSIDER_BLOCKS && nValue > 0) {
        LogPrintf("CSporkManager::ExecuteSpork -- Reconsider Last %d Blocks\n", nValue);

        ReprocessBlocks(nValue);
    }
}

bool CSporkManager::UpdateSpork(int nSporkID, int64_t nValue)
{

    CSporkMessage spork = CSporkMessage(nSporkID, nValue, GetTime());

    if(spork.Sign(strMasterPrivKey)){
        spork.Relay();
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
    } else {
        if(nSporkID == SPORK_2_INSTANTX) r = SPORK_2_INSTANTX_DEFAULT;
        if(nSporkID == SPORK_3_INSTANTX_BLOCK_FILTERING) r = SPORK_3_INSTANTX_BLOCK_FILTERING_DEFAULT;
        if(nSporkID == SPORK_5_MAX_VALUE) r = SPORK_5_MAX_VALUE_DEFAULT;
        if(nSporkID == SPORK_7_MASTERNODE_SCANNING) r = SPORK_7_MASTERNODE_SCANNING_DEFAULT;
        if(nSporkID == SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT) r = SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT_DEFAULT;
        if(nSporkID == SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT) r = SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT_DEFAULT;
        if(nSporkID == SPORK_10_MASTERNODE_PAY_UPDATED_NODES) r = SPORK_10_MASTERNODE_PAY_UPDATED_NODES_DEFAULT;
        if(nSporkID == SPORK_11_RESET_BUDGET) r = SPORK_11_RESET_BUDGET_DEFAULT;
        if(nSporkID == SPORK_12_RECONSIDER_BLOCKS) r = SPORK_12_RECONSIDER_BLOCKS_DEFAULT;
        if(nSporkID == SPORK_13_ENABLE_SUPERBLOCKS) r = SPORK_13_ENABLE_SUPERBLOCKS_DEFAULT;

        if(r == -1) LogPrintf("CSporkManager::IsSporkActive -- Unknown Spork %d\n", nSporkID);
    }
    if(r == -1) r = 4070908800; //return 2099-1-1 by default

    return r < GetTime();
}

// grab the value of the spork on the network, or the default
int64_t CSporkManager::GetSporkValue(int nSporkID)
{
    int64_t r = -1;

    if(mapSporksActive.count(nSporkID)){
        r = mapSporksActive[nSporkID].nValue;
    } else {
        if(nSporkID == SPORK_2_INSTANTX) r = SPORK_2_INSTANTX_DEFAULT;
        if(nSporkID == SPORK_3_INSTANTX_BLOCK_FILTERING) r = SPORK_3_INSTANTX_BLOCK_FILTERING_DEFAULT;
        if(nSporkID == SPORK_5_MAX_VALUE) r = SPORK_5_MAX_VALUE_DEFAULT;
        if(nSporkID == SPORK_7_MASTERNODE_SCANNING) r = SPORK_7_MASTERNODE_SCANNING_DEFAULT;
        if(nSporkID == SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT) r = SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT_DEFAULT;
        if(nSporkID == SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT) r = SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT_DEFAULT;
        if(nSporkID == SPORK_10_MASTERNODE_PAY_UPDATED_NODES) r = SPORK_10_MASTERNODE_PAY_UPDATED_NODES_DEFAULT;
        if(nSporkID == SPORK_11_RESET_BUDGET) r = SPORK_11_RESET_BUDGET_DEFAULT;
        if(nSporkID == SPORK_12_RECONSIDER_BLOCKS) r = SPORK_12_RECONSIDER_BLOCKS_DEFAULT;
        if(nSporkID == SPORK_13_ENABLE_SUPERBLOCKS) r = SPORK_13_ENABLE_SUPERBLOCKS_DEFAULT;

        if(r == -1) LogPrintf("CSporkManager::GetSporkValue -- Unknown Spork %d\n", nSporkID);
    }

    return r;
}

int CSporkManager::GetSporkIDByName(std::string strName)
{
    if(strName == "SPORK_2_INSTANTX") return SPORK_2_INSTANTX;
    if(strName == "SPORK_3_INSTANTX_BLOCK_FILTERING") return SPORK_3_INSTANTX_BLOCK_FILTERING;
    if(strName == "SPORK_5_MAX_VALUE") return SPORK_5_MAX_VALUE;
    if(strName == "SPORK_7_MASTERNODE_SCANNING") return SPORK_7_MASTERNODE_SCANNING;
    if(strName == "SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT") return SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT;
    if(strName == "SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT") return SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT;
    if(strName == "SPORK_10_MASTERNODE_PAY_UPDATED_NODES") return SPORK_10_MASTERNODE_PAY_UPDATED_NODES;
    if(strName == "SPORK_11_RESET_BUDGET") return SPORK_11_RESET_BUDGET;
    if(strName == "SPORK_12_RECONSIDER_BLOCKS") return SPORK_12_RECONSIDER_BLOCKS;
    if(strName == "SPORK_13_ENABLE_SUPERBLOCKS") return SPORK_13_ENABLE_SUPERBLOCKS;

    return -1;
}

std::string CSporkManager::GetSporkNameByID(int nSporkID)
{
    if(nSporkID == SPORK_2_INSTANTX) return "SPORK_2_INSTANTX";
    if(nSporkID == SPORK_3_INSTANTX_BLOCK_FILTERING) return "SPORK_3_INSTANTX_BLOCK_FILTERING";
    if(nSporkID == SPORK_5_MAX_VALUE) return "SPORK_5_MAX_VALUE";
    if(nSporkID == SPORK_7_MASTERNODE_SCANNING) return "SPORK_7_MASTERNODE_SCANNING";
    if(nSporkID == SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT) return "SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT";
    if(nSporkID == SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT) return "SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT";
    if(nSporkID == SPORK_10_MASTERNODE_PAY_UPDATED_NODES) return "SPORK_10_MASTERNODE_PAY_UPDATED_NODES";
    if(nSporkID == SPORK_11_RESET_BUDGET) return "SPORK_11_RESET_BUDGET";
    if(nSporkID == SPORK_12_RECONSIDER_BLOCKS) return "SPORK_12_RECONSIDER_BLOCKS";
    if(nSporkID == SPORK_13_ENABLE_SUPERBLOCKS) return "SPORK_13_ENABLE_SUPERBLOCKS";

    return "Unknown";
}

bool CSporkManager::SetPrivKey(std::string strPrivKey)
{
    CSporkMessage spork;

    spork.Sign(strMasterPrivKey);

    if(spork.CheckSignature()){
        // Test signing successful, proceed
        LogPrintf("CSporkManager::SetPrivKey -- Successfully initialized as spork signer\n");
        strMasterPrivKey = strPrivKey;
        return true;
    } else {
        return false;
    }
}

bool CSporkMessage::Sign(std::string strSignKey)
{
    std::string strMessage = boost::lexical_cast<std::string>(nSporkID) + boost::lexical_cast<std::string>(nValue) + boost::lexical_cast<std::string>(nTimeSigned);

    CKey key;
    CPubKey pubkey;
    std::string errorMessage = "";

    if(!darkSendSigner.GetKeysFromSecret(strSignKey, errorMessage, key, pubkey)) {
        LogPrintf("CSporkMessage::Sign -- ERROR: '%s'\n", errorMessage);
        return false;
    }

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, key)) {
        LogPrintf("CSporkMessage::Sign -- SignMessage() failed");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey, vchSig, strMessage, errorMessage)) {
        LogPrintf("CSporkMessage::Sign -- VerifyMessage() failed");
        return false;
    }

    return true;
}

bool CSporkMessage::CheckSignature()
{
    //note: need to investigate why this is failing
    std::string strMessage = boost::lexical_cast<std::string>(nSporkID) + boost::lexical_cast<std::string>(nValue) + boost::lexical_cast<std::string>(nTimeSigned);
    CPubKey pubkey(ParseHex(Params().SporkPubKey()));

    std::string errorMessage = "";
    if(!darkSendSigner.VerifyMessage(pubkey, vchSig, strMessage, errorMessage)){
        LogPrintf("CSporkMessage::CheckSignature -- VerifyMessage() failed");
        return false;
    }

    return true;
}

void CSporkMessage::Relay()
{
    CInv inv(MSG_SPORK, GetHash());
    RelayInv(inv);
}
