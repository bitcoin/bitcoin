
#include "addrman.h"
#include "protocol.h"
#include "activesystemnode.h"
#include "systemnodeman.h"
#include "systemnode.h"
#include "spork.h"

CActiveSystemnode activeSystemnode;

//
// Bootup the Systemnode, look for a 10000 CRW input and register on the network
//
void CActiveSystemnode::ManageStatus()
{    
    std::string errorMessage;

    if(!fSystemNode) return;

    if (fDebug) LogPrintf("CActiveSystemnode::ManageStatus() - Begin\n");

    //need correct blocks to send ping
    if(Params().NetworkID() != CBaseChainParams::REGTEST && !systemnodeSync.IsBlockchainSynced()) {
        status = ACTIVE_SYSTEMNODE_SYNC_IN_PROCESS;
        LogPrintf("CActiveSystemnode::ManageStatus() - %s\n", GetStatus());
        return;
    }

    if(status == ACTIVE_SYSTEMNODE_SYNC_IN_PROCESS) status = ACTIVE_SYSTEMNODE_INITIAL;

    if(status == ACTIVE_SYSTEMNODE_INITIAL) {
        CSystemnode *pmn;
        pmn = snodeman.Find(pubKeySystemnode);
        if(pmn != NULL) {
            pmn->Check();
            if(pmn->IsEnabled() && pmn->protocolVersion == PROTOCOL_VERSION)  EnableHotColdSystemNode(pmn->vin, pmn->addr);
        }
    }

    if(status != ACTIVE_SYSTEMNODE_STARTED) {

        // Set defaults
        status = ACTIVE_SYSTEMNODE_NOT_CAPABLE;
        notCapableReason = "";

        if(pwalletMain->IsLocked()){
            notCapableReason = "Wallet is locked.";
            LogPrintf("CActiveSystemnode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        if(pwalletMain->GetBalance() == 0){
            notCapableReason = "Hot node, waiting for remote activation.";
            LogPrintf("CActiveSystemnode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        if(strSystemNodeAddr.empty()) {
            if(!GetLocal(service)) {
                notCapableReason = "Can't detect external address. Please use the systemnodeaddr configuration option.";
                LogPrintf("CActiveSystemnode::ManageStatus() - not capable: %s\n", notCapableReason);
                return;
            }
        } else {
            service = CService(strSystemNodeAddr);
        }

        if(Params().NetworkID() == CBaseChainParams::MAIN) {
            if(service.GetPort() != 9340) {
                notCapableReason = strprintf("Invalid port: %u - only 9340 is supported on mainnet.", service.GetPort());
                LogPrintf("CActiveSystemnode::ManageStatus() - not capable: %s\n", notCapableReason);
                return;
            }
        } else if(service.GetPort() == 9340) {
            notCapableReason = strprintf("Invalid port: %u - 9340 is only supported on mainnet.", service.GetPort());
            LogPrintf("CActiveSystemnode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        LogPrintf("CActiveSystemnode::ManageStatus() - Checking inbound connection to '%s'\n", service.ToString());

        if(!ConnectNode((CAddress)service, NULL, false, true)){
            notCapableReason = "Could not connect to " + service.ToString();
            LogPrintf("CActiveSystemnode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        // Choose coins to use
        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;

        if(pwalletMain->GetSystemnodeVinAndKeys(vin, pubKeyCollateralAddress, keyCollateralAddress)) {

            if(GetInputAge(vin) < SYSTEMNODE_MIN_CONFIRMATIONS){
                status = ACTIVE_SYSTEMNODE_INPUT_TOO_NEW;
                notCapableReason = strprintf("%s - %d confirmations", GetStatus(), GetInputAge(vin));
                LogPrintf("CActiveSystemnode::ManageStatus() - %s\n", notCapableReason);
                return;
            }

            LOCK(pwalletMain->cs_wallet);
            pwalletMain->LockCoin(vin.prevout);

            // send to all nodes
            CPubKey pubKeySystemnode;
            CKey keySystemnode;

            if(!legacySigner.SetKey(strSystemNodePrivKey, errorMessage, keySystemnode, pubKeySystemnode))
            {
                notCapableReason = "Error upon calling SetKey: " + errorMessage;
                LogPrintf("Register::ManageStatus() - %s\n", notCapableReason);
                return;
            }

            CSystemnodeBroadcast mnb;
            if(!CSystemnodeBroadcast::Create(vin, service, keyCollateralAddress, pubKeyCollateralAddress, keySystemnode, pubKeySystemnode, errorMessage, mnb)) {
                notCapableReason = "Error on CreateBroadcast: " + errorMessage;
                LogPrintf("Register::ManageStatus() - %s\n", notCapableReason);
                return;
            }

            //update to masternode list
            LogPrintf("CActiveSystemnode::ManageStatus() - Update Systemnode List\n");
            snodeman.UpdateSystemnodeList(mnb);

            //send to all peers
            LogPrintf("CActiveSystemnode::ManageStatus() - Relay broadcast vin = %s\n", vin.ToString());
            mnb.Relay();

            LogPrintf("CActiveSystemnode::ManageStatus() - Is capable master node!\n");
            status = ACTIVE_SYSTEMNODE_STARTED;

            return;
        } else {
            notCapableReason = "Could not find suitable coins!";
            LogPrintf("CActiveSystemnode::ManageStatus() - %s\n", notCapableReason);
            return;
        }
    }

    //send to all peers
    if(!SendSystemnodePing(errorMessage)) {
        LogPrintf("CActiveSystemnode::ManageStatus() - Error on Ping: %s\n", errorMessage);
    }
}

std::string CActiveSystemnode::GetStatus() {
    switch (status) {
    case ACTIVE_SYSTEMNODE_INITIAL: return "Node just started, not yet activated";
    case ACTIVE_SYSTEMNODE_SYNC_IN_PROCESS: return "Sync in progress. Must wait until sync is complete to start Systemnode";
    case ACTIVE_SYSTEMNODE_INPUT_TOO_NEW: return strprintf("Systemnode input must have at least %d confirmations", SYSTEMNODE_MIN_CONFIRMATIONS);
    case ACTIVE_SYSTEMNODE_NOT_CAPABLE: return "Not capable systemnode: " + notCapableReason;
    case ACTIVE_SYSTEMNODE_STARTED: return "Systemnode successfully started";
    default: return "unknown";
    }
}

bool CActiveSystemnode::SendSystemnodePing(std::string& errorMessage) {
    if(status != ACTIVE_SYSTEMNODE_STARTED) {
        errorMessage = "Systemnode is not in a running status";
        return false;
    }

    CPubKey pubKeySystemnode;
    CKey keySystemnode;

    if(!legacySigner.SetKey(strSystemNodePrivKey, errorMessage, keySystemnode, pubKeySystemnode))
    {
        errorMessage = strprintf("Error upon calling SetKey: %s\n", errorMessage);
        return false;
    }

    LogPrintf("CActiveSystemnode::SendSystemnodePing() - Relay Systemnode Ping vin = %s\n", vin.ToString());
    
    CSystemnodePing mnp(vin);
    if(!mnp.Sign(keySystemnode, pubKeySystemnode))
    {
        errorMessage = "Couldn't sign Systemnode Ping";
        return false;
    }

    // Update lastPing for our systemnode in Systemnode list
    CSystemnode* pmn = snodeman.Find(vin);
    if(pmn != NULL)
    {
        if(pmn->IsPingedWithin(SYSTEMNODE_PING_SECONDS, mnp.sigTime)){
            errorMessage = "Too early to send Systemnode Ping";
            return false;
        }

        pmn->lastPing = mnp;
        snodeman.mapSeenSystemnodePing.insert(make_pair(mnp.GetHash(), mnp));

        //snodeman.mapSeenSystemnodeBroadcast.lastPing is probably outdated, so we'll update it
        CSystemnodeBroadcast mnb(*pmn);
        uint256 hash = mnb.GetHash();
        if(snodeman.mapSeenSystemnodeBroadcast.count(hash)) snodeman.mapSeenSystemnodeBroadcast[hash].lastPing = mnp;

        mnp.Relay();

        return true;
    }
    else
    {
        // Seems like we are trying to send a ping while the Systemnode is not registered in the network
        errorMessage = "Systemnode List doesn't include our Systemnode, shutting down Systemnode pinging service! " + vin.ToString();
        status = ACTIVE_SYSTEMNODE_NOT_CAPABLE;
        notCapableReason = errorMessage;
        return false;
    }

}

// when starting a Systemnode, this can enable to run as a hot wallet with no funds
bool CActiveSystemnode::EnableHotColdSystemNode(const CTxIn& newVin, const CService& newService)
{
    if(!fSystemNode) return false;

    status = ACTIVE_SYSTEMNODE_STARTED;

    //The values below are needed for signing mnping messages going forward
    vin = newVin;
    service = newService;

    LogPrintf("CActiveSystemnode::EnableHotColdSystemNode() - Enabled! You may shut down the cold daemon.\n");

    return true;
}
