


#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "protocol.h"
#include "spork.h"
#include "main.h"
#include "masternode-budget.h"
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

class CSporkMessage;
class CSporkManager;

CSporkManager sporkManager;

std::map<uint256, CSporkMessage> mapSporks;
std::map<int, CSporkMessage> mapSporksActive;


void ProcessSpork(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(fLiteMode) return; //disable all masternode related functionality

    if (strCommand == "spork")
    {
        //LogPrintf("ProcessSpork::spork\n");
        CDataStream vMsg(vRecv);
        CSporkMessage spork;
        vRecv >> spork;

        if(chainActive.Tip() == NULL) return;

        uint256 hash = spork.GetHash();
        if(mapSporksActive.count(spork.nSporkID)) {
            if(mapSporksActive[spork.nSporkID].nTimeSigned >= spork.nTimeSigned){
                if(fDebug) LogPrintf("spork - seen %s block %d \n", hash.ToString(), chainActive.Tip()->nHeight);
                return;
            } else {
                if(fDebug) LogPrintf("spork - got updated spork %s block %d \n", hash.ToString(), chainActive.Tip()->nHeight);
            }
        }

        LogPrintf("spork - new %s ID %d Time %d bestHeight %d\n", hash.ToString(), spork.nSporkID, spork.nValue, chainActive.Tip()->nHeight);

        if(!sporkManager.CheckSignature(spork)){
            LogPrintf("spork - invalid signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        mapSporks[hash] = spork;
        mapSporksActive[spork.nSporkID] = spork;
        sporkManager.Relay(spork);

        //does a task if needed
        ExecuteSpork(spork.nSporkID, spork.nValue);
    }
    if (strCommand == "getsporks")
    {
        std::map<int, CSporkMessage>::iterator it = mapSporksActive.begin();

        while(it != mapSporksActive.end()) {
            pfrom->PushMessage("spork", it->second);
            it++;
        }
    }

}

// grab the spork, otherwise say it's off
bool IsSporkActive(int nSporkID)
{
    int64_t r = -1;

    if(mapSporksActive.count(nSporkID)){
        r = mapSporksActive[nSporkID].nValue;
    } else {
        if(nSporkID == SPORK_2_INSTANTX) r = SPORK_2_INSTANTX_DEFAULT;
        if(nSporkID == SPORK_3_INSTANTX_BLOCK_FILTERING) r = SPORK_3_INSTANTX_BLOCK_FILTERING_DEFAULT;
        if(nSporkID == SPORK_4_ENABLE_MASTERNODE_PAYMENTS)
            r = Params().StartMasternodePayments() > 0 ? Params().StartMasternodePayments() : SPORK_4_ENABLE_MASTERNODE_PAYMENTS_DEFAULT;
        if(nSporkID == SPORK_5_MAX_VALUE) r = SPORK_5_MAX_VALUE_DEFAULT;
        if(nSporkID == SPORK_7_MASTERNODE_SCANNING) r = SPORK_7_MASTERNODE_SCANNING_DEFAULT;
        if(nSporkID == SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT) r = SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT_DEFAULT;
        if(nSporkID == SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT) r = SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT_DEFAULT;
        if(nSporkID == SPORK_10_MASTERNODE_DONT_PAY_OLD_NODES) r = SPORK_10_MASTERNODE_DONT_PAY_OLD_NODES_DEFAULT;
        if(nSporkID == SPORK_11_RESET_BUDGET) r = SPORK_11_RESET_BUDGET_DEFAULT;
        if(nSporkID == SPORK_12_RECONSIDER_BLOCKS) r = SPORK_12_RECONSIDER_BLOCKS_DEFAULT;
        if(nSporkID == SPORK_13_ENABLE_SUPERBLOCKS) r = SPORK_13_ENABLE_SUPERBLOCKS_DEFAULT;
        if(nSporkID == SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT) r = SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT_DEFAULT;
        if(nSporkID == SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES) r = SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES_DEFAULT;
        if(nSporkID == SPORK_16_DISCONNECT_OLD_NODES) r = SPORK_16_DISCONNECT_OLD_NODES_DEFAULT;
        if(nSporkID == SPORK_17_NFT_TX) r = SPORK_17_NFT_TX_DEFAULT;

        if(r == -1) LogPrintf("GetSpork::Unknown Spork %d\n", nSporkID);
    }
    if(r == -1) r = 4070908800; //return 2099-1-1 by default

    return r < GetTime();
}

// grab the value of the spork on the network, or the default
int64_t GetSporkValue(int nSporkID)
{
    int64_t r = -1;

    if(mapSporksActive.count(nSporkID)){
        r = mapSporksActive[nSporkID].nValue;
    } else {
        if(nSporkID == SPORK_2_INSTANTX) r = SPORK_2_INSTANTX_DEFAULT;
        if(nSporkID == SPORK_3_INSTANTX_BLOCK_FILTERING) r = SPORK_3_INSTANTX_BLOCK_FILTERING_DEFAULT;
        if(nSporkID == SPORK_4_ENABLE_MASTERNODE_PAYMENTS) r = SPORK_4_ENABLE_MASTERNODE_PAYMENTS_DEFAULT;
        if(nSporkID == SPORK_5_MAX_VALUE) r = SPORK_5_MAX_VALUE_DEFAULT;
        if(nSporkID == SPORK_7_MASTERNODE_SCANNING) r = SPORK_7_MASTERNODE_SCANNING_DEFAULT;
        if(nSporkID == SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT) r = SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT_DEFAULT;
        if(nSporkID == SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT) r = SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT_DEFAULT;
        if(nSporkID == SPORK_10_MASTERNODE_DONT_PAY_OLD_NODES) r = SPORK_10_MASTERNODE_DONT_PAY_OLD_NODES_DEFAULT;
        if(nSporkID == SPORK_11_RESET_BUDGET) r = SPORK_11_RESET_BUDGET_DEFAULT;
        if(nSporkID == SPORK_12_RECONSIDER_BLOCKS) r = SPORK_12_RECONSIDER_BLOCKS_DEFAULT;
        if(nSporkID == SPORK_13_ENABLE_SUPERBLOCKS) r = SPORK_13_ENABLE_SUPERBLOCKS_DEFAULT;
        if(nSporkID == SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT) r = SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT_DEFAULT;
        if(nSporkID == SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES) r = SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES_DEFAULT;
        if(nSporkID == SPORK_16_DISCONNECT_OLD_NODES) r = SPORK_16_DISCONNECT_OLD_NODES_DEFAULT;
        if(nSporkID == SPORK_17_NFT_TX) r = SPORK_17_NFT_TX_DEFAULT;

        if(r == -1) LogPrintf("GetSpork::Unknown Spork %d\n", nSporkID);
    }

    return r;
}

void ExecuteSpork(int nSporkID, int nValue)
{
    if (nSporkID == SPORK_11_RESET_BUDGET && nValue == 1)
    {
        budget.Clear();
    }
    else if (nSporkID == SPORK_12_RECONSIDER_BLOCKS && nValue > 0)
    {
        //correct fork via spork technology
        LogPrintf("Spork::ExecuteSpork -- Reconsider Last %d Blocks\n", nValue);
        ReprocessBlocks(nValue);
    }
    else if (nSporkID == SPORK_16_DISCONNECT_OLD_NODES && nValue == 1)
    {
        LOCK(cs_vNodes);

        for (std::vector<CNode*>::iterator i = vNodes.begin(); i != vNodes.end(); ++i)
        {
            if ((*i)->nVersion < MinPeerProtoVersion())
                (*i)->fDisconnect = true;
        }
    }
}

void ReprocessBlocks(int nBlocks) 
{   
    std::map<uint256, int64_t>::iterator it = mapRejectedBlocks.begin();
    while(it != mapRejectedBlocks.end()){
        //use a window twice as large as is usual for the nBlocks we want to reset
        if((*it).second  > GetTime() - (nBlocks*60*5)) {   
            BlockMap::iterator mi = mapBlockIndex.find((*it).first);
            if (mi != mapBlockIndex.end() && (*mi).second) {
                LOCK(cs_main);
                
                CBlockIndex* pindex = (*mi).second;
                LogPrintf("ReprocessBlocks - %s\n", (*it).first.ToString());

                CValidationState state;
                ReconsiderBlock(state, pindex);
            }
        }
        ++it;
    }

    CValidationState state;
    {
        LOCK(cs_main);
        DisconnectBlocksAndReprocess(nBlocks);
    }

    if (state.IsValid()) {
        ActivateBestChain(state);
    }
}


bool CSporkManager::CheckSignature(CSporkMessage& spork)
{
    //note: need to investigate why this is failing
    std::string strMessage = boost::lexical_cast<std::string>(spork.nSporkID) + boost::lexical_cast<std::string>(spork.nValue) + boost::lexical_cast<std::string>(spork.nTimeSigned);
    CPubKey pubkey(ParseHex(Params().SporkKey()));

    std::string errorMessage = "";
    if(!legacySigner.VerifyMessage(pubkey, spork.vchSig, strMessage, errorMessage)){
        return false;
    }

    return true;
}

bool CSporkManager::Sign(CSporkMessage& spork)
{
    std::string strMessage = boost::lexical_cast<std::string>(spork.nSporkID) + boost::lexical_cast<std::string>(spork.nValue) + boost::lexical_cast<std::string>(spork.nTimeSigned);

    CKey key2;
    CPubKey pubkey2;
    std::string errorMessage = "";

    if(!legacySigner.SetKey(strMasterPrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CMasternodePayments::Sign - ERROR: Invalid masternodeprivkey: '%s'\n", errorMessage);
        return false;
    }

    if(!legacySigner.SignMessage(strMessage, errorMessage, spork.vchSig, key2)) {
        LogPrintf("CMasternodePayments::Sign - Sign message failed");
        return false;
    }

    if(!legacySigner.VerifyMessage(pubkey2, spork.vchSig, strMessage, errorMessage)) {
        LogPrintf("CMasternodePayments::Sign - Verify message failed");
        return false;
    }

    return true;
}

bool CSporkManager::UpdateSpork(int nSporkID, int64_t nValue)
{

    CSporkMessage msg;
    msg.nSporkID = nSporkID;
    msg.nValue = nValue;
    msg.nTimeSigned = GetTime();

    if(Sign(msg)){
        Relay(msg);
        mapSporks[msg.GetHash()] = msg;
        mapSporksActive[nSporkID] = msg;
        return true;
    }

    return false;
}

void CSporkManager::Relay(CSporkMessage& msg)
{
    CInv inv(MSG_SPORK, msg.GetHash());
    RelayInv(inv, MIN_PEER_PROTO_VERSION_PREV); // to make sure SPORK_16_DISCONNECT_OLD_NODES will be relayed
}

bool CSporkManager::SetPrivKey(std::string strPrivKey)
{
    CSporkMessage msg;

    // Test signing successful, proceed
    strMasterPrivKey = strPrivKey;

    Sign(msg);

    if(CheckSignature(msg)){
        LogPrintf("CSporkManager::SetPrivKey - Successfully initialized as spork signer\n");
        return true;
    } else {
        return false;
    }
}

int CSporkManager::GetSporkIDByName(std::string strName)
{
    if(strName == "SPORK_2_INSTANTX") return SPORK_2_INSTANTX;
    if(strName == "SPORK_3_INSTANTX_BLOCK_FILTERING") return SPORK_3_INSTANTX_BLOCK_FILTERING;
    if(strName == "SPORK_4_ENABLE_MASTERNODE_PAYMENTS") return SPORK_4_ENABLE_MASTERNODE_PAYMENTS;
    if(strName == "SPORK_5_MAX_VALUE") return SPORK_5_MAX_VALUE;
    if(strName == "SPORK_7_MASTERNODE_SCANNING") return SPORK_7_MASTERNODE_SCANNING;
    if(strName == "SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT") return SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT;
    if(strName == "SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT") return SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT;
    if(strName == "SPORK_10_MASTERNODE_PAY_UPDATED_NODES") return SPORK_10_MASTERNODE_DONT_PAY_OLD_NODES;
    if(strName == "SPORK_11_RESET_BUDGET") return SPORK_11_RESET_BUDGET;
    if(strName == "SPORK_12_RECONSIDER_BLOCKS") return SPORK_12_RECONSIDER_BLOCKS;
    if(strName == "SPORK_13_ENABLE_SUPERBLOCKS") return SPORK_13_ENABLE_SUPERBLOCKS;
    if(strName == "SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT") return SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT;
    if(strName == "SPORK_15_SYSTEMNODE_PAY_UPDATED_NODES") return SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES;
    if(strName == "SPORK_16_DISCONNECT_OLD_NODES") return SPORK_16_DISCONNECT_OLD_NODES;
    if(strName == "SPORK_17_NFT_TX") return SPORK_17_NFT_TX;

    return -1;
}

std::string CSporkManager::GetSporkNameByID(int id)
{
    if(id == SPORK_2_INSTANTX) return "SPORK_2_INSTANTX";
    if(id == SPORK_3_INSTANTX_BLOCK_FILTERING) return "SPORK_3_INSTANTX_BLOCK_FILTERING";
    if(id == SPORK_4_ENABLE_MASTERNODE_PAYMENTS) return "SPORK_4_ENABLE_MASTERNODE_PAYMENTS";
    if(id == SPORK_5_MAX_VALUE) return "SPORK_5_MAX_VALUE";
    if(id == SPORK_7_MASTERNODE_SCANNING) return "SPORK_7_MASTERNODE_SCANNING";
    if(id == SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT) return "SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT";
    if(id == SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT) return "SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT";
    if(id == SPORK_10_MASTERNODE_DONT_PAY_OLD_NODES) return "SPORK_10_MASTERNODE_PAY_UPDATED_NODES";
    if(id == SPORK_11_RESET_BUDGET) return "SPORK_11_RESET_BUDGET";
    if(id == SPORK_12_RECONSIDER_BLOCKS) return "SPORK_12_RECONSIDER_BLOCKS";
    if(id == SPORK_13_ENABLE_SUPERBLOCKS) return "SPORK_13_ENABLE_SUPERBLOCKS";
    if(id == SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT) return "SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT";
    if(id == SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES) return "SPORK_15_SYSTEMNODE_PAY_UPDATED_NODES";
    if(id == SPORK_16_DISCONNECT_OLD_NODES) return "SPORK_16_DISCONNECT_OLD_NODES";
    if(id == SPORK_17_NFT_TX) return "SPORK_17_NFT_TX";

    return "Unknown";
}
