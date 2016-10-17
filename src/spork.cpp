


<<<<<<< HEAD
=======
#include "bignum.h"
>>>>>>> origin/dirty-merge-dash-0.11.0
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
<<<<<<< HEAD
=======
#include "script.h"
>>>>>>> origin/dirty-merge-dash-0.11.0
#include "base58.h"
#include "protocol.h"
#include "spork.h"
#include "main.h"
<<<<<<< HEAD
#include "masternode-budget.h"
=======
>>>>>>> origin/dirty-merge-dash-0.11.0
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
<<<<<<< HEAD
    if(fLiteMode) return; //disable all darksend/masternode related functionality
=======
    if(fLiteMode) return; //disable all darksend/throne related functionality
>>>>>>> origin/dirty-merge-dash-0.11.0

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
<<<<<<< HEAD
                if(fDebug) LogPrintf("spork - seen %s block %d \n", hash.ToString(), chainActive.Tip()->nHeight);
                return;
            } else {
                if(fDebug) LogPrintf("spork - got updated spork %s block %d \n", hash.ToString(), chainActive.Tip()->nHeight);
            }
        }

        LogPrintf("spork - new %s ID %d Time %d bestHeight %d\n", hash.ToString(), spork.nSporkID, spork.nValue, chainActive.Tip()->nHeight);
=======
                if(fDebug) LogPrintf("spork - seen %s block %d \n", hash.ToString().c_str(), chainActive.Tip()->nHeight);
                return;
            } else {
                if(fDebug) LogPrintf("spork - got updated spork %s block %d \n", hash.ToString().c_str(), chainActive.Tip()->nHeight);
            }
        }

        LogPrintf("spork - new %s ID %d Time %d bestHeight %d\n", hash.ToString().c_str(), spork.nSporkID, spork.nValue, chainActive.Tip()->nHeight);
>>>>>>> origin/dirty-merge-dash-0.11.0

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
<<<<<<< HEAD
    int64_t r = -1;
=======
    int64_t r = 0;
>>>>>>> origin/dirty-merge-dash-0.11.0

    if(mapSporksActive.count(nSporkID)){
        r = mapSporksActive[nSporkID].nValue;
    } else {
<<<<<<< HEAD
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

        if(r == -1) LogPrintf("GetSpork::Unknown Spork %d\n", nSporkID);
    }
    if(r == -1) r = 4070908800; //return 2099-1-1 by default
=======
        if(nSporkID == SPORK_1_THRONE_PAYMENTS_ENFORCEMENT) r = SPORK_1_THRONE_PAYMENTS_ENFORCEMENT_DEFAULT;
        if(nSporkID == SPORK_2_INSTANTX) r = SPORK_2_INSTANTX_DEFAULT;
        if(nSporkID == SPORK_3_INSTANTX_BLOCK_FILTERING) r = SPORK_3_INSTANTX_BLOCK_FILTERING_DEFAULT;
        if(nSporkID == SPORK_5_MAX_VALUE) r = SPORK_5_MAX_VALUE_DEFAULT;
        if(nSporkID == SPORK_7_THRONE_SCANNING) r = SPORK_7_THRONE_SCANNING;

        if(r == 0) LogPrintf("GetSpork::Unknown Spork %d\n", nSporkID);
    }
    if(r == 0) r = 4070908800; //return 2099-1-1 by default
>>>>>>> origin/dirty-merge-dash-0.11.0

    return r < GetTime();
}

// grab the value of the spork on the network, or the default
<<<<<<< HEAD
int64_t GetSporkValue(int nSporkID)
{
    int64_t r = -1;
=======
int GetSporkValue(int nSporkID)
{
    int r = 0;
>>>>>>> origin/dirty-merge-dash-0.11.0

    if(mapSporksActive.count(nSporkID)){
        r = mapSporksActive[nSporkID].nValue;
    } else {
<<<<<<< HEAD
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

        if(r == -1) LogPrintf("GetSpork::Unknown Spork %d\n", nSporkID);
=======
        if(nSporkID == SPORK_1_THRONE_PAYMENTS_ENFORCEMENT) r = SPORK_1_THRONE_PAYMENTS_ENFORCEMENT_DEFAULT;
        if(nSporkID == SPORK_2_INSTANTX) r = SPORK_2_INSTANTX_DEFAULT;
        if(nSporkID == SPORK_3_INSTANTX_BLOCK_FILTERING) r = SPORK_3_INSTANTX_BLOCK_FILTERING_DEFAULT;
        if(nSporkID == SPORK_5_MAX_VALUE) r = SPORK_5_MAX_VALUE_DEFAULT;
        if(nSporkID == SPORK_7_THRONE_SCANNING) r = SPORK_7_THRONE_SCANNING;

        if(r == 0) LogPrintf("GetSpork::Unknown Spork %d\n", nSporkID);
>>>>>>> origin/dirty-merge-dash-0.11.0
    }

    return r;
}

void ExecuteSpork(int nSporkID, int nValue)
{
<<<<<<< HEAD
    if(nSporkID == SPORK_11_RESET_BUDGET && nValue == 1){
        budget.Clear();
    }

    //correct fork via spork technology
    if(nSporkID == SPORK_12_RECONSIDER_BLOCKS && nValue > 0) {
        LogPrintf("Spork::ExecuteSpork -- Reconsider Last %d Blocks\n", nValue);

        ReprocessBlocks(nValue);
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
=======
>>>>>>> origin/dirty-merge-dash-0.11.0
}


bool CSporkManager::CheckSignature(CSporkMessage& spork)
{
    //note: need to investigate why this is failing
    std::string strMessage = boost::lexical_cast<std::string>(spork.nSporkID) + boost::lexical_cast<std::string>(spork.nValue) + boost::lexical_cast<std::string>(spork.nTimeSigned);
<<<<<<< HEAD
    CPubKey pubkey(ParseHex(Params().SporkKey()));
=======
    std::string strPubKey = (Params().NetworkID() == CChainParams::MAIN) ? strMainPubKey : strTestPubKey;
    CPubKey pubkey(ParseHex(strPubKey));
>>>>>>> origin/dirty-merge-dash-0.11.0

    std::string errorMessage = "";
    if(!darkSendSigner.VerifyMessage(pubkey, spork.vchSig, strMessage, errorMessage)){
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

    if(!darkSendSigner.SetKey(strMasterPrivKey, errorMessage, key2, pubkey2))
    {
<<<<<<< HEAD
        LogPrintf("CMasternodePayments::Sign - ERROR: Invalid masternodeprivkey: '%s'\n", errorMessage);
=======
        LogPrintf("CThronePayments::Sign - ERROR: Invalid throneprivkey: '%s'\n", errorMessage.c_str());
>>>>>>> origin/dirty-merge-dash-0.11.0
        return false;
    }

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, spork.vchSig, key2)) {
<<<<<<< HEAD
        LogPrintf("CMasternodePayments::Sign - Sign message failed");
=======
        LogPrintf("CThronePayments::Sign - Sign message failed");
>>>>>>> origin/dirty-merge-dash-0.11.0
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, spork.vchSig, strMessage, errorMessage)) {
<<<<<<< HEAD
        LogPrintf("CMasternodePayments::Sign - Verify message failed");
=======
        LogPrintf("CThronePayments::Sign - Verify message failed");
>>>>>>> origin/dirty-merge-dash-0.11.0
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
<<<<<<< HEAD
    RelayInv(inv);
=======

    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
>>>>>>> origin/dirty-merge-dash-0.11.0
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
<<<<<<< HEAD
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
=======
    if(strName == "SPORK_1_THRONE_PAYMENTS_ENFORCEMENT") return SPORK_1_THRONE_PAYMENTS_ENFORCEMENT;
    if(strName == "SPORK_2_INSTANTX") return SPORK_2_INSTANTX;
    if(strName == "SPORK_3_INSTANTX_BLOCK_FILTERING") return SPORK_3_INSTANTX_BLOCK_FILTERING;
    if(strName == "SPORK_5_MAX_VALUE") return SPORK_5_MAX_VALUE;
    if(strName == "SPORK_7_THRONE_SCANNING") return SPORK_7_THRONE_SCANNING;
>>>>>>> origin/dirty-merge-dash-0.11.0

    return -1;
}

std::string CSporkManager::GetSporkNameByID(int id)
{
<<<<<<< HEAD
    if(id == SPORK_2_INSTANTX) return "SPORK_2_INSTANTX";
    if(id == SPORK_3_INSTANTX_BLOCK_FILTERING) return "SPORK_3_INSTANTX_BLOCK_FILTERING";
    if(id == SPORK_5_MAX_VALUE) return "SPORK_5_MAX_VALUE";
    if(id == SPORK_7_MASTERNODE_SCANNING) return "SPORK_7_MASTERNODE_SCANNING";
    if(id == SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT) return "SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT";
    if(id == SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT) return "SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT";
    if(id == SPORK_10_MASTERNODE_PAY_UPDATED_NODES) return "SPORK_10_MASTERNODE_PAY_UPDATED_NODES";
    if(id == SPORK_11_RESET_BUDGET) return "SPORK_11_RESET_BUDGET";
    if(id == SPORK_12_RECONSIDER_BLOCKS) return "SPORK_12_RECONSIDER_BLOCKS";
    if(id == SPORK_13_ENABLE_SUPERBLOCKS) return "SPORK_13_ENABLE_SUPERBLOCKS";
=======
    if(id == SPORK_1_THRONE_PAYMENTS_ENFORCEMENT) return "SPORK_1_THRONE_PAYMENTS_ENFORCEMENT";
    if(id == SPORK_2_INSTANTX) return "SPORK_2_INSTANTX";
    if(id == SPORK_3_INSTANTX_BLOCK_FILTERING) return "SPORK_3_INSTANTX_BLOCK_FILTERING";
    if(id == SPORK_5_MAX_VALUE) return "SPORK_5_MAX_VALUE";
    if(id == SPORK_7_THRONE_SCANNING) return "SPORK_7_THRONE_SCANNING";
>>>>>>> origin/dirty-merge-dash-0.11.0

    return "Unknown";
}
