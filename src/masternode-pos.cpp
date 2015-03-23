


#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "script.h"
#include "base58.h"
#include "protocol.h"
#include "activemasternode.h"
#include "masternodeman.h"
#include "spork.h"
#include <boost/lexical_cast.hpp>
#include "masternodeman.h"

using namespace std;
using namespace boost;

std::map<uint256, CMasternodeScanningError> mapMasternodeScanningErrors;
CMasternodeScanning mnscan;

/* 
    Masternode - Proof of Service 

    -- What it checks

    1.) Making sure Masternodes have their ports open
    2.) Are responding to requests made by the network

    -- How it works

    When a block comes in, DoMasternodePOS is executed if the client is a 
    masternode. Using the deterministic ranking algorithm up to 1% of the masternode 
    network is checked each block. 

    A port is opened from Masternode A to Masternode B, if successful then nothing happens. 
    If there is an error, a CMasternodeScanningError object is propagated with an error code.
    Errors are applied to the Masternodes and a score is incremented within the masternode object,
    after a threshold is met, the masternode goes into an error state. Each cycle the score is 
    decreased, so if the masternode comes back online it will return to the list. 

    Masternodes in a error state do not receive payment. 

    -- Future expansion

    We want to be able to prove the nodes have many qualities such as a specific CPU speed, bandwidth,
    and dedicated storage. E.g. We could require a full node be a computer running 2GHz with 10GB of space.

*/

void ProcessMessageMasternodePOS(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(fLiteMode) return; //disable all darksend/masternode related functionality
    if(!IsSporkActive(SPORK_7_MASTERNODE_SCANNING)) return;
    if(IsInitialBlockDownload()) return;

    if (strCommand == "mnse") //Masternode Scanning Error
    {

        CDataStream vMsg(vRecv);
        CMasternodeScanningError mnse;
        vRecv >> mnse;

        CInv inv(MSG_MASTERNODE_SCANNING_ERROR, mnse.GetHash());
        pfrom->AddInventoryKnown(inv);

        if(mapMasternodeScanningErrors.count(mnse.GetHash())){
            return;
        }
        mapMasternodeScanningErrors.insert(make_pair(mnse.GetHash(), mnse));

        if(!mnse.IsValid())
        {
            LogPrintf("MasternodePOS::mnse - Invalid object\n");   
            return;
        }

        CMasternode* pmnA = mnodeman.Find(mnse.vinMasternodeA);
        if(pmnA == NULL) return;
        if(pmnA->protocolVersion < MIN_MASTERNODE_POS_PROTO_VERSION) return;

        int nBlockHeight = chainActive.Tip()->nHeight;
        if(nBlockHeight - mnse.nBlockHeight > 10){
            LogPrintf("MasternodePOS::mnse - Too old\n");
            return;   
        }

        // Lowest masternodes in rank check the highest each block
        int a = mnodeman.GetMasternodeRank(mnse.vinMasternodeA, mnse.nBlockHeight, MIN_MASTERNODE_POS_PROTO_VERSION);
        if(a == -1 || a > GetCountScanningPerBlock())
        {
            if(a != -1) LogPrintf("MasternodePOS::mnse - MasternodeA ranking is too high\n");
            return;
        }

        int b = mnodeman.GetMasternodeRank(mnse.vinMasternodeB, mnse.nBlockHeight, MIN_MASTERNODE_POS_PROTO_VERSION, false);
        if(b == -1 || b < mnodeman.CountMasternodesAboveProtocol(MIN_MASTERNODE_POS_PROTO_VERSION)-GetCountScanningPerBlock())
        {
            if(b != -1) LogPrintf("MasternodePOS::mnse - MasternodeB ranking is too low\n");
            return;
        }

        if(!mnse.SignatureValid()){
            LogPrintf("MasternodePOS::mnse - Bad masternode message\n");
            return;
        }

        CMasternode* pmnB = mnodeman.Find(mnse.vinMasternodeB);
        if(pmnB == NULL) return;

        if(fDebug) LogPrintf("ProcessMessageMasternodePOS::mnse - nHeight %d MasternodeA %s MasternodeB %s\n", mnse.nBlockHeight, pmnA->addr.ToString().c_str(), pmnB->addr.ToString().c_str());

        pmnB->ApplyScanningError(mnse);
        mnse.Relay();
    }
}

// Returns how many masternodes are allowed to scan each block
int GetCountScanningPerBlock()
{
    return std::max(1, mnodeman.CountMasternodesAboveProtocol(MIN_MASTERNODE_POS_PROTO_VERSION)/100);
}


void CMasternodeScanning::CleanMasternodeScanningErrors()
{
    if(chainActive.Tip() == NULL) return;

    std::map<uint256, CMasternodeScanningError>::iterator it = mapMasternodeScanningErrors.begin();

    while(it != mapMasternodeScanningErrors.end()) {
        if(GetTime() > it->second.nExpiration){ //keep them for an hour
            LogPrintf("Removing old masternode scanning error %s\n", it->second.GetHash().ToString().c_str());

            mapMasternodeScanningErrors.erase(it++);
        } else {
            it++;
        }
    }

}

// Check other masternodes to make sure they're running correctly
void CMasternodeScanning::DoMasternodePOSChecks()
{
    if(!fMasterNode) return;
    if(fLiteMode) return; //disable all darksend/masternode related functionality
    if(!IsSporkActive(SPORK_7_MASTERNODE_SCANNING)) return;
    if(IsInitialBlockDownload()) return;

    int nBlockHeight = chainActive.Tip()->nHeight-5;

    int a = mnodeman.GetMasternodeRank(activeMasternode.vin, nBlockHeight, MIN_MASTERNODE_POS_PROTO_VERSION);
    if(a == -1 || a > GetCountScanningPerBlock()){
        // we don't need to do anything this block
        return;
    }

    // The lowest ranking nodes (Masternode A) check the highest ranking nodes (Masternode B)
    CMasternode* pmn = mnodeman.GetMasternodeByRank(mnodeman.CountMasternodesAboveProtocol(MIN_MASTERNODE_POS_PROTO_VERSION)-a, nBlockHeight, MIN_MASTERNODE_POS_PROTO_VERSION, false);
    if(pmn == NULL) return;

    // -- first check : Port is open

    if(!ConnectNode((CAddress)pmn->addr, NULL, true)){
        // we couldn't connect to the node, let's send a scanning error
        CMasternodeScanningError mnse(activeMasternode.vin, pmn->vin, SCANNING_ERROR_NO_RESPONSE, nBlockHeight);
        mnse.Sign();
        mapMasternodeScanningErrors.insert(make_pair(mnse.GetHash(), mnse));
        mnse.Relay();
    }

    // success
    CMasternodeScanningError mnse(activeMasternode.vin, pmn->vin, SCANNING_SUCCESS, nBlockHeight);
    mnse.Sign();
    mapMasternodeScanningErrors.insert(make_pair(mnse.GetHash(), mnse));
    mnse.Relay();
}

bool CMasternodeScanningError::SignatureValid()
{
    std::string errorMessage;
    std::string strMessage = vinMasternodeA.ToString() + vinMasternodeB.ToString() + 
        boost::lexical_cast<std::string>(nBlockHeight) + boost::lexical_cast<std::string>(nErrorType);

    CMasternode* pmn = mnodeman.Find(vinMasternodeA);

    if(pmn == NULL)
    {
        LogPrintf("CMasternodeScanningError::SignatureValid() - Unknown Masternode\n");
        return false;
    }

    CScript pubkey;
    pubkey.SetDestination(pmn->pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CBitcoinAddress address2(address1);

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchMasterNodeSignature, strMessage, errorMessage)) {
        LogPrintf("CMasternodeScanningError::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

bool CMasternodeScanningError::Sign()
{
    std::string errorMessage;

    CKey key2;
    CPubKey pubkey2;
    std::string strMessage = vinMasternodeA.ToString() + vinMasternodeB.ToString() + 
        boost::lexical_cast<std::string>(nBlockHeight) + boost::lexical_cast<std::string>(nErrorType);

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CMasternodeScanningError::Sign() - ERROR: Invalid masternodeprivkey: '%s'\n", errorMessage.c_str());
        return false;
    }

    CScript pubkey;
    pubkey.SetDestination(pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CBitcoinAddress address2(address1);
    //LogPrintf("signing pubkey2 %s \n", address2.ToString().c_str());

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchMasterNodeSignature, key2)) {
        LogPrintf("CMasternodeScanningError::Sign() - Sign message failed");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, vchMasterNodeSignature, strMessage, errorMessage)) {
        LogPrintf("CMasternodeScanningError::Sign() - Verify message failed");
        return false;
    }

    return true;
}

void CMasternodeScanningError::Relay()
{
    CInv inv(MSG_MASTERNODE_SCANNING_ERROR, GetHash());

    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
}