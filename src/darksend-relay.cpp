
#include "darksend-relay.h"


CDarkSendRelay::CDarkSendRelay()
{
<<<<<<< HEAD
    vinMasternode = CTxIn();
=======
    vinThrone = CTxIn();
>>>>>>> origin/dirty-merge-dash-0.11.0
    nBlockHeight = 0;
    nRelayType = 0;
    in = CTxIn();
    out = CTxOut();
}

<<<<<<< HEAD
CDarkSendRelay::CDarkSendRelay(CTxIn& vinMasternodeIn, vector<unsigned char>& vchSigIn, int nBlockHeightIn, int nRelayTypeIn, CTxIn& in2, CTxOut& out2)
{
    vinMasternode = vinMasternodeIn;
=======
CDarkSendRelay::CDarkSendRelay(CTxIn& vinThroneIn, vector<unsigned char>& vchSigIn, int nBlockHeightIn, int nRelayTypeIn, CTxIn& in2, CTxOut& out2)
{
    vinThrone = vinThroneIn;
>>>>>>> origin/dirty-merge-dash-0.11.0
    vchSig = vchSigIn;
    nBlockHeight = nBlockHeightIn;
    nRelayType = nRelayTypeIn;
    in = in2;
    out = out2;
}

std::string CDarkSendRelay::ToString()
{
    std::ostringstream info;

<<<<<<< HEAD
    info << "vin: " << vinMasternode.ToString() <<
=======
    info << "vin: " << vinThrone.ToString() <<
>>>>>>> origin/dirty-merge-dash-0.11.0
        " nBlockHeight: " << (int)nBlockHeight <<
        " nRelayType: "  << (int)nRelayType <<
        " in " << in.ToString() <<
        " out " << out.ToString();
        
    return info.str();   
}

bool CDarkSendRelay::Sign(std::string strSharedKey)
{
    std::string strMessage = in.ToString() + out.ToString();

    CKey key2;
    CPubKey pubkey2;
    std::string errorMessage = "";

    if(!darkSendSigner.SetKey(strSharedKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CDarkSendRelay():Sign - ERROR: Invalid shared key: '%s'\n", errorMessage.c_str());
        return false;
    }

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig2, key2)) {
        LogPrintf("CDarkSendRelay():Sign - Sign message failed\n");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, vchSig2, strMessage, errorMessage)) {
        LogPrintf("CDarkSendRelay():Sign - Verify message failed\n");
        return false;
    }

    return true;
}

bool CDarkSendRelay::VerifyMessage(std::string strSharedKey)
{
    std::string strMessage = in.ToString() + out.ToString();

    CKey key2;
    CPubKey pubkey2;
    std::string errorMessage = "";

    if(!darkSendSigner.SetKey(strSharedKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CDarkSendRelay()::VerifyMessage - ERROR: Invalid shared key: '%s'\n", errorMessage.c_str());
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, vchSig2, strMessage, errorMessage)) {
        LogPrintf("CDarkSendRelay()::VerifyMessage - Verify message failed\n");
        return false;
    }

    return true;
}

void CDarkSendRelay::Relay()
{
<<<<<<< HEAD
    int nCount = std::min(mnodeman.CountEnabled(MIN_POOL_PEER_PROTO_VERSION), 20);
=======
    int nCount = std::min(mnodeman.CountEnabled(), 20);
>>>>>>> origin/dirty-merge-dash-0.11.0
    int nRank1 = (rand() % nCount)+1; 
    int nRank2 = (rand() % nCount)+1; 

    //keep picking another second number till we get one that doesn't match
    while(nRank1 == nRank2) nRank2 = (rand() % nCount)+1;

    //printf("rank 1 - rank2 %d %d \n", nRank1, nRank2);

    //relay this message through 2 separate nodes for redundancy
    RelayThroughNode(nRank1);
    RelayThroughNode(nRank2);
}

void CDarkSendRelay::RelayThroughNode(int nRank)
{
<<<<<<< HEAD
    CMasternode* pmn = mnodeman.GetMasternodeByRank(nRank, nBlockHeight, MIN_POOL_PEER_PROTO_VERSION);

    if(pmn != NULL){
        //printf("RelayThroughNode %s\n", pmn->addr.ToString().c_str());
        CNode* pnode = ConnectNode((CAddress)pmn->addr, NULL, false);
        if(pnode){
            //printf("Connected\n");
            pnode->PushMessage("dsr", (*this));
            pnode->Release();
            return;
=======
    CThrone* pmn = mnodeman.GetThroneByRank(nRank, nBlockHeight, MIN_POOL_PEER_PROTO_VERSION);

    if(pmn != NULL){
        //printf("RelayThroughNode %s\n", pmn->addr.ToString().c_str());
        if(ConnectNode((CAddress)pmn->addr, NULL, true)){
            //printf("Connected\n");
            CNode* pNode = FindNode(pmn->addr);
            if(pNode)
            {
                //printf("Found\n");
                pNode->PushMessage("dsr", (*this));
                return;
            }
>>>>>>> origin/dirty-merge-dash-0.11.0
        }
    } else {
        //printf("RelayThroughNode NULL\n");
    }
}
