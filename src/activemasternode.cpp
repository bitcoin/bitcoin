
#include "core.h"
#include "protocol.h"
#include "activemasternode.h"
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

//
// Bootup the masternode, look for a 1000DRK input and register on the network
//
void CActiveMasternode::RegisterAsMasterNode(bool stop)
{
    if(!fMasterNode) return;

    //need correct adjusted time to send ping
    bool fIsInitialDownload = IsInitialBlockDownload();
    if(fIsInitialDownload) {
        isCapableMasterNode = MASTERNODE_SYNC_IN_PROCESS;
        LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Sync in progress. Must wait until sync is complete to start masternode.\n");
        return;
    }

    std::string errorMessage;

    CKey key2;
    CPubKey pubkey2;

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Invalid masternodeprivkey: '%s'\n", errorMessage.c_str());
        exit(0);
    }

    if(isCapableMasterNode == MASTERNODE_INPUT_TOO_NEW || isCapableMasterNode == MASTERNODE_NOT_CAPABLE || isCapableMasterNode == MASTERNODE_SYNC_IN_PROCESS){
        isCapableMasterNode = MASTERNODE_NOT_PROCESSED;
    }

    if(isCapableMasterNode == MASTERNODE_NOT_PROCESSED) {
        if(strMasterNodeAddr.empty()) {
            if(!GetLocal(masterNodeSignAddr)) {
                LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Can't detect external address. Please use the masternodeaddr configuration option.\n");
                isCapableMasterNode = MASTERNODE_NOT_CAPABLE;
                return;
            }
        } else {
            masterNodeSignAddr = CService(strMasterNodeAddr);
        }

        if((Params().NetworkID() == CChainParams::TESTNET && masterNodeSignAddr.GetPort() != 19999) || (!(Params().NetworkID() == CChainParams::TESTNET) && masterNodeSignAddr.GetPort() != 9999)) {
            LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Invalid port\n");
            isCapableMasterNode = MASTERNODE_NOT_CAPABLE;
            return;
        }

        LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Checking inbound connection to '%s'\n", masterNodeSignAddr.ToString().c_str());

        if(ConnectNode((CAddress)masterNodeSignAddr, masterNodeSignAddr.ToString().c_str())){
            masternodePortOpen = MASTERNODE_PORT_OPEN;
        } else {
            masternodePortOpen = MASTERNODE_PORT_NOT_OPEN;
            isCapableMasterNode = MASTERNODE_NOT_CAPABLE;
            LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Port not open.\n");
            return;
        }

        if(pwalletMain->IsLocked()){
            isCapableMasterNode = MASTERNODE_NOT_CAPABLE;
            LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Not capable.\n");
            return;
        }

        isCapableMasterNode = MASTERNODE_NOT_CAPABLE;

        CKey SecretKey;
        // Choose coins to use
        if(GetMasterNodeVin(vinMasternode, pubkeyMasterNode, SecretKey)) {

            if(GetInputAge(vinMasternode) < MASTERNODE_MIN_CONFIRMATIONS){
                LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Input must have least %d confirmations - %d confirmations\n", MASTERNODE_MIN_CONFIRMATIONS, GetInputAge(vinMasternode));
                isCapableMasterNode = MASTERNODE_INPUT_TOO_NEW;
                return;
            }

            int protocolVersion = PROTOCOL_VERSION;

            masterNodeSignatureTime = GetAdjustedTime();

            std::string vchPubKey(pubkeyMasterNode.begin(), pubkeyMasterNode.end());
            std::string vchPubKey2(pubkey2.begin(), pubkey2.end());
            std::string strMessage = masterNodeSignAddr.ToString() + boost::lexical_cast<std::string>(masterNodeSignatureTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

            if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchMasterNodeSignature, SecretKey)) {
                LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Sign message failed\n");
                return;
            }

            if(!darkSendSigner.VerifyMessage(pubkeyMasterNode, vchMasterNodeSignature, strMessage, errorMessage)) {
                LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Verify message failed\n");
                return;
            }

            LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Is capable master node!\n");

            isCapableMasterNode = MASTERNODE_IS_CAPABLE;

            pwalletMain->LockCoin(vinMasternode.prevout);

            bool found = false;
            BOOST_FOREACH(CMasterNode& mn, darkSendMasterNodes)
                if(mn.vin == vinMasternode)
                    found = true;

            if(!found) {
                LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Adding myself to masternode list %s - %s\n", masterNodeSignAddr.ToString().c_str(), vinMasternode.ToString().c_str());
                CMasterNode mn(masterNodeSignAddr, vinMasternode, pubkeyMasterNode, vchMasterNodeSignature, masterNodeSignatureTime, pubkey2, PROTOCOL_VERSION);
                mn.UpdateLastSeen(masterNodeSignatureTime);
                darkSendMasterNodes.push_back(mn);
                LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Masternode input = %s\n", vinMasternode.ToString().c_str());
            }

            //relay to all
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
            {
                pnode->PushMessage("dsee", vinMasternode, masterNodeSignAddr, vchMasterNodeSignature, masterNodeSignatureTime, pubkeyMasterNode, pubkey2, -1, -1, masterNodeSignatureTime, protocolVersion);
            }

            return;
        }
    }

    if(isCapableMasterNode != MASTERNODE_IS_CAPABLE && isCapableMasterNode != MASTERNODE_REMOTELY_ENABLED)  return;

    masterNodeSignatureTime = GetAdjustedTime();

    std::string strMessage = masterNodeSignAddr.ToString() + boost::lexical_cast<std::string>(masterNodeSignatureTime) + boost::lexical_cast<std::string>(stop);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchMasterNodeSignature, key2)) {
        LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Sign message failed\n");
        return;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, vchMasterNodeSignature, strMessage, errorMessage)) {
        LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Verify message failed\n");
        return;
    }

    bool found = false;
    BOOST_FOREACH(CMasterNode& mn, darkSendMasterNodes) {
        //LogPrintf(" -- %s\n", mn.vin.ToString().c_str());

        if(mn.vin == vinMasternode) {
            found = true;
            mn.UpdateLastSeen();
        }
    }
    if(!found){
        LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Darksend Masternode List doesn't include our masternode, Shutting down masternode pinging service! %s\n", vinMasternode.ToString().c_str());
        isCapableMasterNode = MASTERNODE_STOPPED;
        return;
    }

    LogPrintf("CActiveMasternode::RegisterAsMasterNode() - Masternode input = %s\n", vinMasternode.ToString().c_str());

    if (stop) isCapableMasterNode = MASTERNODE_STOPPED;

    //relay to all peers
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        pnode->PushMessage("dseep", vinMasternode, vchMasterNodeSignature, masterNodeSignatureTime, stop);
    }
}

//
// Bootup the masternode, look for a 1000DRK input and register on the network
// Takes 2 parameters to start a remote masternode
//
bool CActiveMasternode::RegisterAsMasterNodeRemoteOnly(std::string strMasterNodeAddr, std::string strMasterNodePrivKey)
{
    if(!fMasterNode) return false;

    LogPrintf("CActiveMasternode::RegisterAsMasterNodeRemoteOnly() - Address %s MasterNodePrivKey %s\n", strMasterNodeAddr.c_str(), strMasterNodePrivKey.c_str());

    std::string errorMessage;

    CKey key2;
    CPubKey pubkey2;


    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CActiveMasternode::RegisterAsMasterNodeRemoteOnly() - Invalid masternodeprivkey: '%s'\n", errorMessage.c_str());
        return false;
    }

    CService masterNodeSignAddr = CService(strMasterNodeAddr);
    BOOST_FOREACH(CMasterNode& mn, darkSendMasterNodes){
        if(mn.addr == masterNodeSignAddr){
            LogPrintf("CActiveMasternode::RegisterAsMasterNodeRemoteOnly() - Address in use\n");
            return false;
        }
    }

    if((Params().NetworkID() == CChainParams::TESTNET && masterNodeSignAddr.GetPort() != 19999) || (!(Params().NetworkID() == CChainParams::TESTNET) && masterNodeSignAddr.GetPort() != 9999)) {
        LogPrintf("CActiveMasternode::RegisterAsMasterNodeRemoteOnly() - Invalid port\n");
        return false;
    }

    LogPrintf("CActiveMasternode::RegisterAsMasterNodeRemoteOnly() - Checking inbound connection to '%s'\n", masterNodeSignAddr.ToString().c_str());

    if(!ConnectNode((CAddress)masterNodeSignAddr, masterNodeSignAddr.ToString().c_str())){
        LogPrintf("CActiveMasternode::RegisterAsMasterNodeRemoteOnly() - Error connecting to port\n");
        return false;
    }

    if(pwalletMain->IsLocked()){
        LogPrintf("CActiveMasternode::RegisterAsMasterNodeRemoteOnly() - Wallet is locked\n");
        return false;
    }

    CKey SecretKey;
    CTxIn vinMasternode;
    CPubKey pubkeyMasterNode;
    int masterNodeSignatureTime = 0;

    // Choose coins to use
    while (GetMasterNodeVin(vinMasternode, pubkeyMasterNode, SecretKey)) {
        // don't use a vin that's registered
        BOOST_FOREACH(CMasterNode& mn, darkSendMasterNodes)
            if(mn.vin == vinMasternode)
                continue;

        if(GetInputAge(vinMasternode) < MASTERNODE_MIN_CONFIRMATIONS)
            continue;

        masterNodeSignatureTime = GetAdjustedTime();

        std::string vchPubKey(pubkeyMasterNode.begin(), pubkeyMasterNode.end());
        std::string vchPubKey2(pubkey2.begin(), pubkey2.end());
        std::string strMessage = masterNodeSignAddr.ToString() + boost::lexical_cast<std::string>(masterNodeSignatureTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(PROTOCOL_VERSION);

        if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchMasterNodeSignature, SecretKey)) {
            LogPrintf("CActiveMasternode::RegisterAsMasterNodeRemoteOnly() - Sign message failed\n");
            return false;
        }

        if(!darkSendSigner.VerifyMessage(pubkeyMasterNode, vchMasterNodeSignature, strMessage, errorMessage)) {
            LogPrintf("CActiveMasternode::RegisterAsMasterNodeRemoteOnly() - Verify message failed\n");
            return false;
        }

        LogPrintf("CActiveMasternode::RegisterAsMasterNodeRemoteOnly() - Is capable master node!\n");

        pwalletMain->LockCoin(vinMasternode.prevout);

        int protocolVersion = PROTOCOL_VERSION;
        //relay to all
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
        {
            pnode->PushMessage("dsee", vinMasternode, masterNodeSignAddr, vchMasterNodeSignature, masterNodeSignatureTime, pubkeyMasterNode, pubkey2, -1, -1, masterNodeSignatureTime, protocolVersion);
        }

        return true;
    }

    LogPrintf("CActiveMasternode::RegisterAsMasterNodeRemoteOnly() - No sutable vin found\n");
    return false;
}


bool CActiveMasternode::GetMasterNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey)
{
    int64_t nValueIn = 0;
    CScript pubScript;

    // try once before we try to denominate
    if (!pwalletMain->SelectCoinsMasternode(vin, nValueIn, pubScript))
    {
        if(fDebug) LogPrintf("CActiveMasternode::GetMasterNodeVin - I'm not a capable masternode\n");
        return false;
    }

    CTxDestination address1;
    ExtractDestination(pubScript, address1);
    CBitcoinAddress address2(address1);

    CKeyID keyID;
    if (!address2.GetKeyID(keyID)) {
        LogPrintf("CActiveMasternode::GetMasterNodeVin - Address does not refer to a key\n");
        return false;
    }

    if (!pwalletMain->GetKey(keyID, secretKey)) {
        LogPrintf ("CActiveMasternode::GetMasterNodeVin - Private key for address is not known\n");
        return false;
    }

    pubkey = secretKey.GetPubKey();
    return true;
}

// when starting a masternode, this can enable to run as a hot wallet with no funds
bool CActiveMasternode::EnableHotColdMasterNode(CTxIn& vin, int64_t sigTime, CService& addr)
{
    if(!fMasterNode) return false;

    isCapableMasterNode = MASTERNODE_REMOTELY_ENABLED;

    vinMasternode = vin;
    masterNodeSignatureTime = sigTime;
    masterNodeSignAddr = addr;

    LogPrintf("CActiveMasternode::EnableHotColdMasterNode() - Enabled! You may shut down the cold daemon.\n");

    return true;
}
