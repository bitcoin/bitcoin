
#include "core.h"
#include "protocol.h"
#include "activethrone.h"
#include "throneman.h"
#include <boost/lexical_cast.hpp>

//
// Bootup the Throne, look for a 1000DRK input and register on the network
//
void CActiveThrone::ManageStatus()
{
    std::string errorMessage;

    if(!fThroNe) return;

    if (fDebug) LogPrintf("CActiveThrone::ManageStatus() - Begin\n");

    //need correct adjusted time to send ping
    bool fIsInitialDownload = IsInitialBlockDownload();
    if(fIsInitialDownload) {
        status = THRONE_SYNC_IN_PROCESS;
        LogPrintf("CActiveThrone::ManageStatus() - Sync in progress. Must wait until sync is complete to start Throne.\n");
        return;
    }

    if(status == THRONE_INPUT_TOO_NEW || status == THRONE_NOT_CAPABLE || status == THRONE_SYNC_IN_PROCESS){
        status = THRONE_NOT_PROCESSED;
    }

    if(status == THRONE_NOT_PROCESSED) {
        if(strThroNeAddr.empty()) {
            if(!GetLocal(service)) {
                notCapableReason = "Can't detect external address. Please use the Throneaddr configuration option.";
                status = THRONE_NOT_CAPABLE;
                LogPrintf("CActiveThrone::ManageStatus() - not capable: %s\n", notCapableReason.c_str());
                return;
            }
        } else {
            service = CService(strThroNeAddr);
        }

        LogPrintf("CActiveThrone::ManageStatus() - Checking inbound connection to '%s'\n", service.ToString().c_str());

        if(Params().NetworkID() == CChainParams::MAIN){
            if(service.GetPort() != 9341) {
                notCapableReason = "Invalid port: " + boost::lexical_cast<string>(service.GetPort()) + " - only 9999 is supported on mainnet.";
                status = THRONE_NOT_CAPABLE;
                LogPrintf("CActiveThrone::ManageStatus() - not capable: %s\n", notCapableReason.c_str());
                return;
            }
        } else if(service.GetPort() == 9341) {
            notCapableReason = "Invalid port: " + boost::lexical_cast<string>(service.GetPort()) + " - 9999 is only supported on mainnet.";
            status = THRONE_NOT_CAPABLE;
            LogPrintf("CActiveThrone::ManageStatus() - not capable: %s\n", notCapableReason.c_str());
            return;
        }

        if(!ConnectNode((CAddress)service, service.ToString().c_str())){
            notCapableReason = "Could not connect to " + service.ToString();
            status = THRONE_NOT_CAPABLE;
            LogPrintf("CActiveThrone::ManageStatus() - not capable: %s\n", notCapableReason.c_str());
            return;
        }

        if(pwalletMain->IsLocked()){
            notCapableReason = "Wallet is locked.";
            status = THRONE_NOT_CAPABLE;
            LogPrintf("CActiveThrone::ManageStatus() - not capable: %s\n", notCapableReason.c_str());
            return;
        }

        // Set defaults
        status = THRONE_NOT_CAPABLE;
        notCapableReason = "Unknown. Check debug.log for more information.";

        // Choose coins to use
        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;

        if(GetThroNeVin(vin, pubKeyCollateralAddress, keyCollateralAddress)) {

            if(GetInputAge(vin) < THRONE_MIN_CONFIRMATIONS){
                notCapableReason = "Input must have least " + boost::lexical_cast<string>(THRONE_MIN_CONFIRMATIONS) +
                        " confirmations - " + boost::lexical_cast<string>(GetInputAge(vin)) + " confirmations";
                LogPrintf("CActiveThrone::ManageStatus() - %s\n", notCapableReason.c_str());
                status = THRONE_INPUT_TOO_NEW;
                return;
            }

            LogPrintf("CActiveThrone::ManageStatus() - Is capable master node!\n");

            status = THRONE_IS_CAPABLE;
            notCapableReason = "";

            pwalletMain->LockCoin(vin.prevout);

            // send to all nodes
            CPubKey pubKeyThrone;
            CKey keyThrone;

            if(!darkSendSigner.SetKey(strThroNePrivKey, errorMessage, keyThrone, pubKeyThrone))
            {
                LogPrintf("Register::ManageStatus() - Error upon calling SetKey: %s\n", errorMessage.c_str());
                return;
            }

            /* donations are not supported in dash.conf */
            CScript donationAddress = CScript();
            int donationPercentage = 0;

            if(!Register(vin, service, keyCollateralAddress, pubKeyCollateralAddress, keyThrone, pubKeyThrone, donationAddress, donationPercentage, errorMessage)) {
                LogPrintf("CActiveThrone::ManageStatus() - Error on Register: %s\n", errorMessage.c_str());
            }

            return;
        } else {
            notCapableReason = "Could not find suitable coins!";
            LogPrintf("CActiveThrone::ManageStatus() - %s\n", notCapableReason.c_str());
        }
    }

    //send to all peers
    if(!Dseep(errorMessage)) {
        LogPrintf("CActiveThrone::ManageStatus() - Error on Ping: %s\n", errorMessage.c_str());
    }
}

// Send stop dseep to network for remote Throne
bool CActiveThrone::StopThroNe(std::string strService, std::string strKeyThrone, std::string& errorMessage) {
    CTxIn vin;
    CKey keyThrone;
    CPubKey pubKeyThrone;

    if(!darkSendSigner.SetKey(strKeyThrone, errorMessage, keyThrone, pubKeyThrone)) {
        LogPrintf("CActiveThrone::StopThroNe() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    return StopThroNe(vin, CService(strService), keyThrone, pubKeyThrone, errorMessage);
}

// Send stop dseep to network for main Throne
bool CActiveThrone::StopThroNe(std::string& errorMessage) {
    if(status != THRONE_IS_CAPABLE && status != THRONE_REMOTELY_ENABLED) {
        errorMessage = "Throne is not in a running status";
        LogPrintf("CActiveThrone::StopThroNe() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    status = THRONE_STOPPED;

    CPubKey pubKeyThrone;
    CKey keyThrone;

    if(!darkSendSigner.SetKey(strThroNePrivKey, errorMessage, keyThrone, pubKeyThrone))
    {
        LogPrintf("Register::ManageStatus() - Error upon calling SetKey: %s\n", errorMessage.c_str());
        return false;
    }

    return StopThroNe(vin, service, keyThrone, pubKeyThrone, errorMessage);
}

// Send stop dseep to network for any Throne
bool CActiveThrone::StopThroNe(CTxIn vin, CService service, CKey keyThrone, CPubKey pubKeyThrone, std::string& errorMessage) {
    pwalletMain->UnlockCoin(vin.prevout);
    return Dseep(vin, service, keyThrone, pubKeyThrone, errorMessage, true);
}

bool CActiveThrone::Dseep(std::string& errorMessage) {
    if(status != THRONE_IS_CAPABLE && status != THRONE_REMOTELY_ENABLED) {
        errorMessage = "Throne is not in a running status";
        LogPrintf("CActiveThrone::Dseep() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    CPubKey pubKeyThrone;
    CKey keyThrone;

    if(!darkSendSigner.SetKey(strThroNePrivKey, errorMessage, keyThrone, pubKeyThrone))
    {
        LogPrintf("CActiveThrone::Dseep() - Error upon calling SetKey: %s\n", errorMessage.c_str());
        return false;
    }

    return Dseep(vin, service, keyThrone, pubKeyThrone, errorMessage, false);
}

bool CActiveThrone::Dseep(CTxIn vin, CService service, CKey keyThrone, CPubKey pubKeyThrone, std::string &retErrorMessage, bool stop) {
    std::string errorMessage;
    std::vector<unsigned char> vchThroNeSignature;
    std::string strThroNeSignMessage;
    int64_t masterNodeSignatureTime = GetAdjustedTime();

    std::string strMessage = service.ToString() + boost::lexical_cast<std::string>(masterNodeSignatureTime) + boost::lexical_cast<std::string>(stop);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchThroNeSignature, keyThrone)) {
        retErrorMessage = "sign message failed: " + errorMessage;
        LogPrintf("CActiveThrone::Dseep() - Error: %s\n", retErrorMessage.c_str());
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyThrone, vchThroNeSignature, strMessage, errorMessage)) {
        retErrorMessage = "Verify message failed: " + errorMessage;
        LogPrintf("CActiveThrone::Dseep() - Error: %s\n", retErrorMessage.c_str());
        return false;
    }

    // Update Last Seen timestamp in Throne list
    CThrone* pmn = mnodeman.Find(vin);
    if(pmn != NULL)
    {
        if(stop)
            mnodeman.Remove(pmn->vin);
        else
            pmn->UpdateLastSeen();
    }
    else
    {
        // Seems like we are trying to send a ping while the Throne is not registered in the network
        retErrorMessage = "Darksend Throne List doesn't include our Throne, Shutting down Throne pinging service! " + vin.ToString();
        LogPrintf("CActiveThrone::Dseep() - Error: %s\n", retErrorMessage.c_str());
        status = THRONE_NOT_CAPABLE;
        notCapableReason = retErrorMessage;
        return false;
    }

    //send to all peers
    LogPrintf("CActiveThrone::Dseep() - RelayThroneEntryPing vin = %s\n", vin.ToString().c_str());
    mnodeman.RelayThroneEntryPing(vin, vchThroNeSignature, masterNodeSignatureTime, stop);

    return true;
}

bool CActiveThrone::Register(std::string strService, std::string strKeyThrone, std::string txHash, std::string strOutputIndex, std::string strDonationAddress, std::string strDonationPercentage, std::string& errorMessage) {
    CTxIn vin;
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;
    CPubKey pubKeyThrone;
    CKey keyThrone;
    CScript donationAddress = CScript();
    int donationPercentage = 0;

    if(!darkSendSigner.SetKey(strKeyThrone, errorMessage, keyThrone, pubKeyThrone))
    {
        LogPrintf("CActiveThrone::Register() - Error upon calling SetKey: %s\n", errorMessage.c_str());
        return false;
    }

    if(!GetThroNeVin(vin, pubKeyCollateralAddress, keyCollateralAddress, txHash, strOutputIndex)) {
        errorMessage = "could not allocate vin";
        LogPrintf("CActiveThrone::Register() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    CCrowncoinAddress address;
    if (strDonationAddress != "")
    {
        if(!address.SetString(strDonationAddress))
        {
            LogPrintf("CActiveThrone::Register - Invalid Donation Address\n");
            return false;
        }
        donationAddress.SetDestination(address.Get());

        try {
            donationPercentage = boost::lexical_cast<int>( strDonationPercentage );
        } catch( boost::bad_lexical_cast const& ) {
            LogPrintf("CActiveThrone::Register - Invalid Donation Percentage (Couldn't cast)\n");
            return false;
        }

        if(donationPercentage < 0 || donationPercentage > 100)
        {
            LogPrintf("CActiveThrone::Register - Donation Percentage Out Of Range\n");
            return false;
        }
    }

    return Register(vin, CService(strService), keyCollateralAddress, pubKeyCollateralAddress, keyThrone, pubKeyThrone, donationAddress, donationPercentage, errorMessage);
}

bool CActiveThrone::Register(CTxIn vin, CService service, CKey keyCollateralAddress, CPubKey pubKeyCollateralAddress, CKey keyThrone, CPubKey pubKeyThrone, CScript donationAddress, int donationPercentage, std::string &retErrorMessage) {
    std::string errorMessage;
    std::vector<unsigned char> vchThroNeSignature;
    std::string strThroNeSignMessage;
    int64_t masterNodeSignatureTime = GetAdjustedTime();

    std::string vchPubKey(pubKeyCollateralAddress.begin(), pubKeyCollateralAddress.end());
    std::string vchPubKey2(pubKeyThrone.begin(), pubKeyThrone.end());

    std::string strMessage = service.ToString() + boost::lexical_cast<std::string>(masterNodeSignatureTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(PROTOCOL_VERSION) + donationAddress.ToString() + boost::lexical_cast<std::string>(donationPercentage);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchThroNeSignature, keyCollateralAddress)) {
        retErrorMessage = "sign message failed: " + errorMessage;
        LogPrintf("CActiveThrone::Register() - Error: %s\n", retErrorMessage.c_str());
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyCollateralAddress, vchThroNeSignature, strMessage, errorMessage)) {
        retErrorMessage = "Verify message failed: " + errorMessage;
        LogPrintf("CActiveThrone::Register() - Error: %s\n", retErrorMessage.c_str());
        return false;
    }

    CThrone* pmn = mnodeman.Find(vin);
    if(pmn == NULL)
    {
        LogPrintf("CActiveThrone::Register() - Adding to Throne list service: %s - vin: %s\n", service.ToString().c_str(), vin.ToString().c_str());
        CThrone mn(service, vin, pubKeyCollateralAddress, vchThroNeSignature, masterNodeSignatureTime, pubKeyThrone, PROTOCOL_VERSION, donationAddress, donationPercentage);
        mn.UpdateLastSeen(masterNodeSignatureTime);
        mnodeman.Add(mn);
    }

    //send to all peers
    LogPrintf("CActiveThrone::Register() - RelayElectionEntry vin = %s\n", vin.ToString().c_str());
    mnodeman.RelayThroneEntry(vin, service, vchThroNeSignature, masterNodeSignatureTime, pubKeyCollateralAddress, pubKeyThrone, -1, -1, masterNodeSignatureTime, PROTOCOL_VERSION, donationAddress, donationPercentage);

    return true;
}

bool CActiveThrone::GetThroNeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey) {
    return GetThroNeVin(vin, pubkey, secretKey, "", "");
}

bool CActiveThrone::GetThroNeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex) {
    CScript pubScript;

    // Find possible candidates
    vector<COutput> possibleCoins = SelectCoinsThrone();
    COutput *selectedOutput;

    // Find the vin
    if(!strTxHash.empty()) {
        // Let's find it
        uint256 txHash(strTxHash);
        int outputIndex = boost::lexical_cast<int>(strOutputIndex);
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
            LogPrintf("CActiveThrone::GetThroNeVin - Could not locate valid vin\n");
            return false;
        }
    } else {
        // No output specified,  Select the first one
        if(possibleCoins.size() > 0) {
            selectedOutput = &possibleCoins[0];
        } else {
            LogPrintf("CActiveThrone::GetThroNeVin - Could not locate specified vin from possible list\n");
            return false;
        }
    }

    // At this point we have a selected output, retrieve the associated info
    return GetVinFromOutput(*selectedOutput, vin, pubkey, secretKey);
}


// Extract Throne vin information from output
bool CActiveThrone::GetVinFromOutput(COutput out, CTxIn& vin, CPubKey& pubkey, CKey& secretKey) {

    CScript pubScript;

    vin = CTxIn(out.tx->GetHash(),out.i);
    pubScript = out.tx->vout[out.i].scriptPubKey; // the inputs PubKey

    CTxDestination address1;
    ExtractDestination(pubScript, address1);
    CCrowncoinAddress address2(address1);

    CKeyID keyID;
    if (!address2.GetKeyID(keyID)) {
        LogPrintf("CActiveThrone::GetThroNeVin - Address does not refer to a key\n");
        return false;
    }

    if (!pwalletMain->GetKey(keyID, secretKey)) {
        LogPrintf ("CActiveThrone::GetThroNeVin - Private key for address is not known\n");
        return false;
    }

    pubkey = secretKey.GetPubKey();
    return true;
}

// get all possible outputs for running Throne
vector<COutput> CActiveThrone::SelectCoinsThrone()
{
    vector<COutput> vCoins;
    vector<COutput> filteredCoins;

    // Retrieve all possible outputs
    pwalletMain->AvailableCoins(vCoins);

    // Filter
    BOOST_FOREACH(const COutput& out, vCoins)
    {
        if(out.tx->vout[out.i].nValue == 10000*COIN) { //exactly
            filteredCoins.push_back(out);
        }
    }
    return filteredCoins;
}

// when starting a Throne, this can enable to run as a hot wallet with no funds
bool CActiveThrone::EnableHotColdThroNe(CTxIn& newVin, CService& newService)
{
    if(!fThroNe) return false;

    status = THRONE_REMOTELY_ENABLED;

    //The values below are needed for signing dseep messages going forward
    this->vin = newVin;
    this->service = newService;

    LogPrintf("CActiveThrone::EnableHotColdThroNe() - Enabled! You may shut down the cold daemon.\n");

    return true;
}
