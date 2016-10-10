


#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "script.h"
#include "base58.h"
#include "protocol.h"
#include "activethrone.h"
#include "throneman.h"
#include "spork.h"
#include <boost/lexical_cast.hpp>
#include "throneman.h"

using namespace std;
using namespace boost;

std::map<uint256, CThroneScanningError> mapThroneScanningErrors;
CThroneScanning mnscan;

/* 
    Throne - Proof of Service 

    -- What it checks

    1.) Making sure Thrones have their ports open
    2.) Are responding to requests made by the network

    -- How it works

    When a block comes in, DoThronePOS is executed if the client is a 
    throne. Using the deterministic ranking algorithm up to 1% of the throne 
    network is checked each block. 

    A port is opened from Throne A to Throne B, if successful then nothing happens. 
    If there is an error, a CThroneScanningError object is propagated with an error code.
    Errors are applied to the Thrones and a score is incremented within the throne object,
    after a threshold is met, the throne goes into an error state. Each cycle the score is 
    decreased, so if the throne comes back online it will return to the list. 

    Thrones in a error state do not receive payment. 

    -- Future expansion

    We want to be able to prove the nodes have many qualities such as a specific CPU speed, bandwidth,
    and dedicated storage. E.g. We could require a full node be a computer running 2GHz with 10GB of space.

*/

void ProcessMessageThronePOS(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(fLiteMode) return; //disable all darksend/throne related functionality
    if(!IsSporkActive(SPORK_7_THRONE_SCANNING)) return;
    if(IsInitialBlockDownload()) return;

    if (strCommand == "mnse") //Throne Scanning Error
    {

        CDataStream vMsg(vRecv);
        CThroneScanningError mnse;
        vRecv >> mnse;

        CInv inv(MSG_THRONE_SCANNING_ERROR, mnse.GetHash());
        pfrom->AddInventoryKnown(inv);

        if(mapThroneScanningErrors.count(mnse.GetHash())){
            return;
        }
        mapThroneScanningErrors.insert(make_pair(mnse.GetHash(), mnse));

        if(!mnse.IsValid())
        {
            LogPrintf("ThronePOS::mnse - Invalid object\n");   
            return;
        }

        CThrone* pmnA = mnodeman.Find(mnse.vinThroneA);
        if(pmnA == NULL) return;
        if(pmnA->protocolVersion < MIN_THRONE_POS_PROTO_VERSION) return;

        int nBlockHeight = chainActive.Tip()->nHeight;
        if(nBlockHeight - mnse.nBlockHeight > 10){
            LogPrintf("ThronePOS::mnse - Too old\n");
            return;   
        }

        // Lowest thrones in rank check the highest each block
        int a = mnodeman.GetThroneRank(mnse.vinThroneA, mnse.nBlockHeight, MIN_THRONE_POS_PROTO_VERSION);
        if(a == -1 || a > GetCountScanningPerBlock())
        {
            if(a != -1) LogPrintf("ThronePOS::mnse - ThroneA ranking is too high\n");
            return;
        }

        int b = mnodeman.GetThroneRank(mnse.vinThroneB, mnse.nBlockHeight, MIN_THRONE_POS_PROTO_VERSION, false);
        if(b == -1 || b < mnodeman.CountThronesAboveProtocol(MIN_THRONE_POS_PROTO_VERSION)-GetCountScanningPerBlock())
        {
            if(b != -1) LogPrintf("ThronePOS::mnse - ThroneB ranking is too low\n");
            return;
        }

        if(!mnse.SignatureValid()){
            LogPrintf("ThronePOS::mnse - Bad throne message\n");
            return;
        }

        CThrone* pmnB = mnodeman.Find(mnse.vinThroneB);
        if(pmnB == NULL) return;

        if(fDebug) LogPrintf("ProcessMessageThronePOS::mnse - nHeight %d ThroneA %s ThroneB %s\n", mnse.nBlockHeight, pmnA->addr.ToString().c_str(), pmnB->addr.ToString().c_str());

        pmnB->ApplyScanningError(mnse);
        mnse.Relay();
    }
}

// Returns how many thrones are allowed to scan each block
int GetCountScanningPerBlock()
{
    return std::max(1, mnodeman.CountThronesAboveProtocol(MIN_THRONE_POS_PROTO_VERSION)/100);
}


void CThroneScanning::CleanThroneScanningErrors()
{
    if(chainActive.Tip() == NULL) return;

    std::map<uint256, CThroneScanningError>::iterator it = mapThroneScanningErrors.begin();

    while(it != mapThroneScanningErrors.end()) {
        if(GetTime() > it->second.nExpiration){ //keep them for an hour
            LogPrintf("Removing old throne scanning error %s\n", it->second.GetHash().ToString().c_str());

            mapThroneScanningErrors.erase(it++);
        } else {
            it++;
        }
    }

}

// Check other thrones to make sure they're running correctly
void CThroneScanning::DoThronePOSChecks()
{
    if(!fThroNe) return;
    if(fLiteMode) return; //disable all darksend/throne related functionality
    if(!IsSporkActive(SPORK_7_THRONE_SCANNING)) return;
    if(IsInitialBlockDownload()) return;

    int nBlockHeight = chainActive.Tip()->nHeight-5;

    int a = mnodeman.GetThroneRank(activeThrone.vin, nBlockHeight, MIN_THRONE_POS_PROTO_VERSION);
    if(a == -1 || a > GetCountScanningPerBlock()){
        // we don't need to do anything this block
        return;
    }

    // The lowest ranking nodes (Throne A) check the highest ranking nodes (Throne B)
    CThrone* pmn = mnodeman.GetThroneByRank(mnodeman.CountThronesAboveProtocol(MIN_THRONE_POS_PROTO_VERSION)-a, nBlockHeight, MIN_THRONE_POS_PROTO_VERSION, false);
    if(pmn == NULL) return;

    // -- first check : Port is open

    if(!ConnectNode((CAddress)pmn->addr, NULL, true)){
        // we couldn't connect to the node, let's send a scanning error
        CThroneScanningError mnse(activeThrone.vin, pmn->vin, SCANNING_ERROR_NO_RESPONSE, nBlockHeight);
        mnse.Sign();
        mapThroneScanningErrors.insert(make_pair(mnse.GetHash(), mnse));
        mnse.Relay();
    }

    // success
    CThroneScanningError mnse(activeThrone.vin, pmn->vin, SCANNING_SUCCESS, nBlockHeight);
    mnse.Sign();
    mapThroneScanningErrors.insert(make_pair(mnse.GetHash(), mnse));
    mnse.Relay();
}

bool CThroneScanningError::SignatureValid()
{
    std::string errorMessage;
    std::string strMessage = vinThroneA.ToString() + vinThroneB.ToString() + 
        boost::lexical_cast<std::string>(nBlockHeight) + boost::lexical_cast<std::string>(nErrorType);

    CThrone* pmn = mnodeman.Find(vinThroneA);

    if(pmn == NULL)
    {
        LogPrintf("CThroneScanningError::SignatureValid() - Unknown Throne\n");
        return false;
    }

    CScript pubkey;
    pubkey.SetDestination(pmn->pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CCrowncoinAddress address2(address1);

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchThroNeSignature, strMessage, errorMessage)) {
        LogPrintf("CThroneScanningError::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

bool CThroneScanningError::Sign()
{
    std::string errorMessage;

    CKey key2;
    CPubKey pubkey2;
    std::string strMessage = vinThroneA.ToString() + vinThroneB.ToString() + 
        boost::lexical_cast<std::string>(nBlockHeight) + boost::lexical_cast<std::string>(nErrorType);

    if(!darkSendSigner.SetKey(strThroNePrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CThroneScanningError::Sign() - ERROR: Invalid throneprivkey: '%s'\n", errorMessage.c_str());
        return false;
    }

    CScript pubkey;
    pubkey.SetDestination(pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CCrowncoinAddress address2(address1);
    //LogPrintf("signing pubkey2 %s \n", address2.ToString().c_str());

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchThroNeSignature, key2)) {
        LogPrintf("CThroneScanningError::Sign() - Sign message failed");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, vchThroNeSignature, strMessage, errorMessage)) {
        LogPrintf("CThroneScanningError::Sign() - Verify message failed");
        return false;
    }

    return true;
}

void CThroneScanningError::Relay()
{
    CInv inv(MSG_THRONE_SCANNING_ERROR, GetHash());

    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
}