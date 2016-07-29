
#include "addrman.h"
#include "protocol.h"
#include "activemasternode.h"
#include "masternodeman.h"
#include "masternode.h"
#include "masternodeconfig.h"
#include "spork.h"

//
// Bootup the Masternode, look for a 1000DRK input and register on the network
//
void CActiveMasternode::ManageStatus()
{    
    std::string errorMessage;

    if(!fMasterNode) return;

    if (fDebug) LogPrintf("CActiveMasternode::ManageStatus() - Begin\n");

    //need correct blocks to send ping
    if(Params().NetworkID() != CBaseChainParams::REGTEST && !masternodeSync.IsBlockchainSynced()) {
        status = ACTIVE_MASTERNODE_SYNC_IN_PROCESS;
        LogPrintf("CActiveMasternode::ManageStatus() - %s\n", GetStatus());
        return;
    }

    if(status == ACTIVE_MASTERNODE_SYNC_IN_PROCESS) status = ACTIVE_MASTERNODE_INITIAL;

    if(status == ACTIVE_MASTERNODE_INITIAL) {
        CMasternode *pmn;
        pmn = mnodeman.Find(pubKeyMasternode);
        if(pmn != NULL) {
            pmn->Check();
            if(pmn->IsEnabled() && pmn->protocolVersion == PROTOCOL_VERSION) EnableHotColdMasterNode(pmn->vin, pmn->addr);
        }
    }

    if(status != ACTIVE_MASTERNODE_STARTED) {

        // Set defaults
        status = ACTIVE_MASTERNODE_NOT_CAPABLE;
        notCapableReason = "";

        if(pwalletMain->IsLocked()){
            notCapableReason = "Wallet is locked.";
            LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        if(pwalletMain->GetBalance() == 0){
            notCapableReason = "Hot node, waiting for remote activation.";
            LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        if(strMasterNodeAddr.empty()) {
            if(!GetLocal(service)) {
                notCapableReason = "Can't detect external address. Please use the masternodeaddr configuration option.";
                LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
                return;
            }
        } else {
            service = CService(strMasterNodeAddr);
        }

        if(Params().NetworkID() == CBaseChainParams::MAIN) {
            if(service.GetPort() != 9999) {
                notCapableReason = strprintf("Invalid port: %u - only 9999 is supported on mainnet.", service.GetPort());
                LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
                return;
            }
        } else if(service.GetPort() == 9999) {
            notCapableReason = strprintf("Invalid port: %u - 9999 is only supported on mainnet.", service.GetPort());
            LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        LogPrintf("CActiveMasternode::ManageStatus() - Checking inbound connection to '%s'\n", service.ToString());

        CNode *pnode = ConnectNode((CAddress)service, NULL, false);
        if(!pnode){
            notCapableReason = "Could not connect to " + service.ToString();
            LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }
        pnode->Release();

        // Choose coins to use
        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;

        if(GetMasterNodeVin(vin, pubKeyCollateralAddress, keyCollateralAddress)) {

            if(GetInputAge(vin) < MASTERNODE_MIN_CONFIRMATIONS){
                status = ACTIVE_MASTERNODE_INPUT_TOO_NEW;
                notCapableReason = strprintf("%s - %d confirmations", GetStatus(), GetInputAge(vin));
                LogPrintf("CActiveMasternode::ManageStatus() - %s\n", notCapableReason);
                return;
            }

            LOCK(pwalletMain->cs_wallet);
            pwalletMain->LockCoin(vin.prevout);

            // send to all nodes
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
            {
                notCapableReason = "Error upon calling SetKey: " + errorMessage;
                LogPrintf("Register::ManageStatus() - %s\n", notCapableReason);
                return;
            }

            CMasternodeBroadcast mnb;
            if(!CreateBroadcast(vin, service, keyCollateralAddress, pubKeyCollateralAddress, keyMasternode, pubKeyMasternode, errorMessage, mnb)) {
                notCapableReason = "Error on CreateBroadcast: " + errorMessage;
                LogPrintf("Register::ManageStatus() - %s\n", notCapableReason);
                return;
            }

            //send to all peers
            LogPrintf("CActiveMasternode::ManageStatus() - Relay broadcast vin = %s\n", vin.ToString());
            mnb.Relay();

            LogPrintf("CActiveMasternode::ManageStatus() - Is capable master node!\n");
            status = ACTIVE_MASTERNODE_STARTED;

            return;
        } else {
            notCapableReason = "Could not find suitable coins!";
            LogPrintf("CActiveMasternode::ManageStatus() - %s\n", notCapableReason);
            return;
        }
    }

    //send to all peers
    if(!SendMasternodePing(errorMessage)) {
        LogPrintf("CActiveMasternode::ManageStatus() - Error on Ping: %s\n", errorMessage);
    }
}

std::string CActiveMasternode::GetStatus() {
    switch (status) {
    case ACTIVE_MASTERNODE_INITIAL: return "Node just started, not yet activated";
    case ACTIVE_MASTERNODE_SYNC_IN_PROCESS: return "Sync in progress. Must wait until sync is complete to start Masternode";
    case ACTIVE_MASTERNODE_INPUT_TOO_NEW: return strprintf("Masternode input must have at least %d confirmations", MASTERNODE_MIN_CONFIRMATIONS);
    case ACTIVE_MASTERNODE_NOT_CAPABLE: return "Not capable masternode: " + notCapableReason;
    case ACTIVE_MASTERNODE_STARTED: return "Masternode successfully started";
    default: return "unknown";
    }
}

bool CActiveMasternode::SendMasternodePing(std::string& errorMessage) {
    if(status != ACTIVE_MASTERNODE_STARTED) {
        errorMessage = "Masternode is not in a running status";
        return false;
    }

    CPubKey pubKeyMasternode;
    CKey keyMasternode;

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
    {
        errorMessage = strprintf("Error upon calling SetKey: %s\n", errorMessage);
        return false;
    }

    LogPrintf("CActiveMasternode::SendMasternodePing() - Relay Masternode Ping vin = %s\n", vin.ToString());
    
    CMasternodePing mnp(vin);
    if(!mnp.Sign(keyMasternode, pubKeyMasternode))
    {
        errorMessage = "Couldn't sign Masternode Ping";
        return false;
    }

    // Update lastPing for our masternode in Masternode list
    CMasternode* pmn = mnodeman.Find(vin);
    if(pmn != NULL)
    {
        if(pmn->IsPingedWithin(MASTERNODE_PING_SECONDS, mnp.sigTime)){
            errorMessage = "Too early to send Masternode Ping";
            return false;
        }

        pmn->lastPing = mnp;
        mnodeman.mapSeenMasternodePing.insert(make_pair(mnp.GetHash(), mnp));

        //mnodeman.mapSeenMasternodeBroadcast.lastPing is probably outdated, so we'll update it
        CMasternodeBroadcast mnb(*pmn);
        uint256 hash = mnb.GetHash();
        if(mnodeman.mapSeenMasternodeBroadcast.count(hash)) mnodeman.mapSeenMasternodeBroadcast[hash].lastPing = mnp;

        mnp.Relay();

        return true;
    }
    else
    {
        // Seems like we are trying to send a ping while the Masternode is not registered in the network
        errorMessage = "Darksend Masternode List doesn't include our Masternode, shutting down Masternode pinging service! " + vin.ToString();
        status = ACTIVE_MASTERNODE_NOT_CAPABLE;
        notCapableReason = errorMessage;
        return false;
    }

}

bool CActiveMasternode::CreateBroadcast(std::string strService, std::string strKeyMasternode, std::string strTxHash, std::string strOutputIndex, std::string& errorMessage, CMasternodeBroadcast &mnb, bool fOffline) {
    CTxIn vin;
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;
    CPubKey pubKeyMasternode;
    CKey keyMasternode;

    //need correct blocks to send ping
    if(!fOffline && !masternodeSync.IsBlockchainSynced()) {
        errorMessage = "Sync in progress. Must wait until sync is complete to start Masternode";
        LogPrintf("CActiveMasternode::CreateBroadcast() - %s\n", errorMessage);
        return false;
    }

    if(!darkSendSigner.SetKey(strKeyMasternode, errorMessage, keyMasternode, pubKeyMasternode))
    {
        errorMessage = strprintf("Can't find keys for masternode %s - %s", strService, errorMessage);
        LogPrintf("CActiveMasternode::CreateBroadcast() - %s\n", errorMessage);
        return false;
    }

    if(!GetMasterNodeVin(vin, pubKeyCollateralAddress, keyCollateralAddress, strTxHash, strOutputIndex)) {
        errorMessage = strprintf("Could not allocate vin %s:%s for masternode %s", strTxHash, strOutputIndex, strService);
        LogPrintf("CActiveMasternode::CreateBroadcast() - %s\n", errorMessage);
        return false;
    }

    CService service = CService(strService);
    if(Params().NetworkID() == CBaseChainParams::MAIN) {
        if(service.GetPort() != 9999) {
            errorMessage = strprintf("Invalid port %u for masternode %s - only 9999 is supported on mainnet.", service.GetPort(), strService);
            LogPrintf("CActiveMasternode::CreateBroadcast() - %s\n", errorMessage);
            return false;
        }
    } else if(service.GetPort() == 9999) {
        errorMessage = strprintf("Invalid port %u for masternode %s - 9999 is only supported on mainnet.", service.GetPort(), strService);
        LogPrintf("CActiveMasternode::CreateBroadcast() - %s\n", errorMessage);
        return false;
    }

    addrman.Add(CAddress(service), CNetAddr("127.0.0.1"), 2*60*60);

    return CreateBroadcast(vin, CService(strService), keyCollateralAddress, pubKeyCollateralAddress, keyMasternode, pubKeyMasternode, errorMessage, mnb);
}

bool CActiveMasternode::CreateBroadcast(CTxIn vin, CService service, CKey keyCollateralAddress, CPubKey pubKeyCollateralAddress, CKey keyMasternode, CPubKey pubKeyMasternode, std::string &errorMessage, CMasternodeBroadcast &mnb) {
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    CMasternodePing mnp(vin);
    if(!mnp.Sign(keyMasternode, pubKeyMasternode)){
        errorMessage = strprintf("Failed to sign ping, vin: %s", vin.ToString());
        LogPrintf("CActiveMasternode::CreateBroadcast() -  %s\n", errorMessage);
        mnb = CMasternodeBroadcast();
        return false;
    }

    mnb = CMasternodeBroadcast(service, vin, pubKeyCollateralAddress, pubKeyMasternode, PROTOCOL_VERSION);
    mnb.lastPing = mnp;
    if(!mnb.Sign(keyCollateralAddress)){
        errorMessage = strprintf("Failed to sign broadcast, vin: %s", vin.ToString());
        LogPrintf("CActiveMasternode::CreateBroadcast() - %s\n", errorMessage);
        mnb = CMasternodeBroadcast();
        return false;
    }

    return true;
}

bool CActiveMasternode::GetMasterNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey) {
    return GetMasterNodeVin(vin, pubkey, secretKey, "", "");
}

bool CActiveMasternode::GetMasterNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex) {
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    // Find possible candidates
    TRY_LOCK(pwalletMain->cs_wallet, fWallet);
    if(!fWallet) return false;

    vector<COutput> possibleCoins = SelectCoinsMasternode();
    COutput *selectedOutput;

    // Find the vin
    if(!strTxHash.empty()) {
        // Let's find it
        uint256 txHash(strTxHash);
        int outputIndex = atoi(strOutputIndex.c_str());
        bool found = false;
        BOOST_FOREACH(COutput& out, possibleCoins) {
            if(out.tx->GetHash() == txHash && out.i == outputIndex)
            {
                selectedOutput = &out;
                found = true;
                break;
            }
        }
        if(!found) {
            LogPrintf("CActiveMasternode::GetMasterNodeVin - Could not locate valid vin\n");
            return false;
        }
    } else {
        // No output specified,  Select the first one
        if(possibleCoins.size() > 0) {
            selectedOutput = &possibleCoins[0];
        } else {
            LogPrintf("CActiveMasternode::GetMasterNodeVin - Could not locate specified vin from possible list\n");
            return false;
        }
    }

    // At this point we have a selected output, retrieve the associated info
    return GetVinFromOutput(*selectedOutput, vin, pubkey, secretKey);
}


// Extract Masternode vin information from output
bool CActiveMasternode::GetVinFromOutput(COutput out, CTxIn& vin, CPubKey& pubkey, CKey& secretKey) {
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    CScript pubScript;

    vin = CTxIn(out.tx->GetHash(),out.i);
    pubScript = out.tx->vout[out.i].scriptPubKey; // the inputs PubKey

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

// get all possible outputs for running Masternode
vector<COutput> CActiveMasternode::SelectCoinsMasternode()
{
    vector<COutput> vCoins;
    vector<COutput> filteredCoins;
    vector<COutPoint> confLockedCoins;

    // Temporary unlock MN coins from masternode.conf
    if(GetBoolArg("-mnconflock", true)) {
        uint256 mnTxHash;
        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            mnTxHash.SetHex(mne.getTxHash());
            COutPoint outpoint = COutPoint(mnTxHash, atoi(mne.getOutputIndex().c_str()));
            confLockedCoins.push_back(outpoint);
            pwalletMain->UnlockCoin(outpoint);
        }
    }

    // Retrieve all possible outputs
    pwalletMain->AvailableCoins(vCoins);

    // Lock MN coins from masternode.conf back if they where temporary unlocked
    if(!confLockedCoins.empty()) {
        BOOST_FOREACH(COutPoint outpoint, confLockedCoins)
            pwalletMain->LockCoin(outpoint);
    }

    // Filter
    BOOST_FOREACH(const COutput& out, vCoins)
    {
        if(out.tx->vout[out.i].nValue == 1000*COIN) { //exactly
            filteredCoins.push_back(out);
        }
    }
    return filteredCoins;
}

// when starting a Masternode, this can enable to run as a hot wallet with no funds
bool CActiveMasternode::EnableHotColdMasterNode(CTxIn& newVin, CService& newService)
{
    if(!fMasterNode) return false;

    status = ACTIVE_MASTERNODE_STARTED;

    //The values below are needed for signing mnping messages going forward
    vin = newVin;
    service = newService;

    LogPrintf("CActiveMasternode::EnableHotColdMasterNode() - Enabled! You may shut down the cold daemon.\n");

    return true;
}
