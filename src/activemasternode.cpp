
#include "addrman.h"
#include "protocol.h"
#include "activemasternode.h"
#include "masternodeman.h"
#include "masternode.h"
#include "spork.h"

//
// Bootup the Masternode, look for a 10000 CRW input and register on the network
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
            if (pmn->IsEnabled() && pmn->protocolVersion == PROTOCOL_VERSION) {
                EnableHotColdMasterNode(pmn->vin, pmn->addr);
                if (!pmn->vchSignover.empty()) {
                    if (pmn->pubkey.Verify(pubKeyMasternode.GetHash(), pmn->vchSignover)) {
                        LogPrintf("%s: Verified pubkey2 signover for staking\n", __func__);
                        activeMasternode.vchSigSignover = pmn->vchSignover;
                    } else {
                        LogPrintf("%s: Failed to verify pubkey on signover!\n", __func__);
                    }
                } else {
                    LogPrintf("%s: NOT SIGNOVER!\n", __func__);
                }
            }
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
            if(service.GetPort() != 9340) {
                notCapableReason = strprintf("Invalid port: %u - only 9340 is supported on mainnet.", service.GetPort());
                LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
                return;
            }
        } else if(service.GetPort() == 9340) {
            notCapableReason = strprintf("Invalid port: %u - 9340 is only supported on mainnet.", service.GetPort());
            LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        LogPrintf("CActiveMasternode::ManageStatus() - Checking inbound connection to '%s'\n", service.ToString());

        if(!ConnectNode((CAddress)service, NULL, true)){
            notCapableReason = "Could not connect to " + service.ToString();
            LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        // Choose coins to use
        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;

        if(pwalletMain->GetMasternodeVinAndKeys(vin, pubKeyCollateralAddress, keyCollateralAddress)) {

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

            if(!legacySigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
            {
                notCapableReason = "Error upon calling SetKey: " + errorMessage;
                LogPrintf("Register::ManageStatus() - %s\n", notCapableReason);
                return;
            }

            CMasternodeBroadcast mnb;
            bool fSignOver = true;
            if(!CMasternodeBroadcast::Create(vin, service, keyCollateralAddress, pubKeyCollateralAddress, keyMasternode, pubKeyMasternode, fSignOver, errorMessage, mnb)) {
                notCapableReason = "Error on CreateBroadcast: " + errorMessage;
                LogPrintf("Register::ManageStatus() - %s\n", notCapableReason);
                return;
            }

            //update to masternode list
            LogPrintf("CActiveMasternode::ManageStatus() - Update Masternode List\n");
            mnodeman.UpdateMasternodeList(mnb);

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

std::string CActiveMasternode::GetStatus() const
{
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

    if(!legacySigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
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
        errorMessage = "Masternode List doesn't include our Masternode, shutting down Masternode pinging service! " + vin.ToString();
        status = ACTIVE_MASTERNODE_NOT_CAPABLE;
        notCapableReason = errorMessage;
        return false;
    }

}

// when starting a Masternode, this can enable to run as a hot wallet with no funds
bool CActiveMasternode::EnableHotColdMasterNode(const CTxIn& newVin, const CService& newService)
{
    if(!fMasterNode) return false;

    status = ACTIVE_MASTERNODE_STARTED;

    //The values below are needed for signing mnping messages going forward
    vin = newVin;
    service = newService;

    LogPrintf("CActiveMasternode::EnableHotColdMasterNode() - Enabled! You may shut down the cold daemon.\n");

    return true;
}
