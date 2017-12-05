// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//#define ENABLE_DASH_DEBUG

#include "activemasternode.h"
#include "governance.h"
#include "governance-vote.h"
#include "governance-classes.h"
#include "governance-validators.h"
#include "init.h"
#include "validation.h"
#include "masternode.h"
#include "masternode-sync.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "messagesigner.h"
#include "rpc/server.h"
#include "util.h"
#include "utilmoneystr.h"
#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif // ENABLE_WALLET

#include <boost/lexical_cast.hpp>

UniValue gobject(const UniValue& params, bool fHelp)
{
    std::string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (
#ifdef ENABLE_WALLET
         strCommand != "prepare" &&
#endif // ENABLE_WALLET
         strCommand != "vote-many" && strCommand != "vote-conf" && strCommand != "vote-alias" && strCommand != "submit" && strCommand != "count" &&
         strCommand != "deserialize" && strCommand != "get" && strCommand != "getvotes" && strCommand != "getcurrentvotes" && strCommand != "list" && strCommand != "diff" &&
         strCommand != "check" ))
        throw std::runtime_error(
                "gobject \"command\"...\n"
                "Manage governance objects\n"
                "\nAvailable commands:\n"
                "  check              - Validate governance object data (proposal only)\n"
#ifdef ENABLE_WALLET
                "  prepare            - Prepare governance object by signing and creating tx\n"
#endif // ENABLE_WALLET
                "  submit             - Submit governance object to network\n"
                "  deserialize        - Deserialize governance object from hex string to JSON\n"
                "  count              - Count governance objects and votes\n"
                "  get                - Get governance object by hash\n"
                "  getvotes           - Get all votes for a governance object hash (including old votes)\n"
                "  getcurrentvotes    - Get only current (tallying) votes for a governance object hash (does not include old votes)\n"
                "  list               - List governance objects (can be filtered by signal and/or object type)\n"
                "  diff               - List differences since last diff\n"
                "  vote-alias         - Vote on a governance object by masternode alias (using masternode.conf setup)\n"
                "  vote-conf          - Vote on a governance object by masternode configured in dash.conf\n"
                "  vote-many          - Vote on a governance object by all masternodes (using masternode.conf setup)\n"
                );


    if(strCommand == "count")
        return governance.ToString();
    /*
        ------ Example Governance Item ------

        gobject submit 6e622bb41bad1fb18e7f23ae96770aeb33129e18bd9efe790522488e580a0a03 0 1 1464292854 "beer-reimbursement" 5b5b22636f6e7472616374222c207b2270726f6a6563745f6e616d65223a20225c22626565722d7265696d62757273656d656e745c22222c20227061796d656e745f61646472657373223a20225c225879324c4b4a4a64655178657948726e34744744514238626a6876464564615576375c22222c2022656e645f64617465223a202231343936333030343030222c20226465736372697074696f6e5f75726c223a20225c227777772e646173687768616c652e6f72672f702f626565722d7265696d62757273656d656e745c22222c2022636f6e74726163745f75726c223a20225c22626565722d7265696d62757273656d656e742e636f6d2f3030312e7064665c22222c20227061796d656e745f616d6f756e74223a20223233342e323334323232222c2022676f7665726e616e63655f6f626a6563745f6964223a2037342c202273746172745f64617465223a202231343833323534303030227d5d5d1
    */

    // DEBUG : TEST DESERIALIZATION OF GOVERNANCE META DATA
    if(strCommand == "deserialize")
    {
        if (params.size() != 2) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'gobject deserialize <data-hex>'");
        }

        std::string strHex = params[1].get_str();

        std::vector<unsigned char> v = ParseHex(strHex);
        std::string s(v.begin(), v.end());

        UniValue u(UniValue::VOBJ);
        u.read(s);

        return u.write().c_str();
    }

    // VALIDATE A GOVERNANCE OBJECT PRIOR TO SUBMISSION
    if(strCommand == "check")
    {
        if (params.size() != 2) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'gobject check <data-hex>'");
        }

        // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

        uint256 hashParent;

        int nRevision = 1;

        int64_t nTime = GetAdjustedTime();
        std::string strData = params[1].get_str();

        CGovernanceObject govobj(hashParent, nRevision, nTime, uint256(), strData);

        if(govobj.GetObjectType() == GOVERNANCE_OBJECT_PROPOSAL) {
            CProposalValidator validator(strData);
            if(!validator.Validate())  {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid proposal data, error messages:" + validator.GetErrorMessages());
            }
        }
        else {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid object type, only proposals can be validated");
        }

        UniValue objResult(UniValue::VOBJ);

        objResult.push_back(Pair("Object status", "OK"));

        return objResult;
    }

#ifdef ENABLE_WALLET
    // PREPARE THE GOVERNANCE OBJECT BY CREATING A COLLATERAL TRANSACTION
    if(strCommand == "prepare")
    {
        if (params.size() != 5) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'gobject prepare <parent-hash> <revision> <time> <data-hex>'");
        }

        // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

        uint256 hashParent;

        // -- attach to root node (root node doesn't really exist, but has a hash of zero)
        if(params[1].get_str() == "0") {
            hashParent = uint256();
        } else {
            hashParent = ParseHashV(params[1], "fee-txid, parameter 1");
        }

        std::string strRevision = params[2].get_str();
        std::string strTime = params[3].get_str();
        int nRevision = boost::lexical_cast<int>(strRevision);
        int nTime = boost::lexical_cast<int>(strTime);
        std::string strData = params[4].get_str();

        // CREATE A NEW COLLATERAL TRANSACTION FOR THIS SPECIFIC OBJECT

        CGovernanceObject govobj(hashParent, nRevision, nTime, uint256(), strData);

        if(govobj.GetObjectType() == GOVERNANCE_OBJECT_PROPOSAL) {
            CProposalValidator validator(strData);
            if(!validator.Validate())  {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid proposal data, error messages:" + validator.GetErrorMessages());
            }
        }

        if((govobj.GetObjectType() == GOVERNANCE_OBJECT_TRIGGER) ||
           (govobj.GetObjectType() == GOVERNANCE_OBJECT_WATCHDOG)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Trigger and watchdog objects need not be prepared (however only masternodes can create them)");
        }

        {
            LOCK(cs_main);
            std::string strError = "";
            if(!govobj.IsValidLocally(strError, false))
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Governance object is not valid - " + govobj.GetHash().ToString() + " - " + strError);
        }

        CWalletTx wtx;
        if(!pwalletMain->GetBudgetSystemCollateralTX(wtx, govobj.GetHash(), govobj.GetMinCollateralFee(), false)) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Error making collateral transaction for governance object. Please check your wallet balance and make sure your wallet is unlocked.");
        }

        // -- make our change address
        CReserveKey reservekey(pwalletMain);
        // -- send the tx to the network
        pwalletMain->CommitTransaction(wtx, reservekey, g_connman.get(), NetMsgType::TX);

        DBG( cout << "gobject: prepare "
             << " strData = " << govobj.GetDataAsString()
             << ", hash = " << govobj.GetHash().GetHex()
             << ", txidFee = " << wtx.GetHash().GetHex()
             << endl; );

        return wtx.GetHash().ToString();
    }
#endif // ENABLE_WALLET

    // AFTER COLLATERAL TRANSACTION HAS MATURED USER CAN SUBMIT GOVERNANCE OBJECT TO PROPAGATE NETWORK
    if(strCommand == "submit")
    {
        if ((params.size() < 5) || (params.size() > 6))  {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'gobject submit <parent-hash> <revision> <time> <data-hex> <fee-txid>'");
        }

        if(!masternodeSync.IsBlockchainSynced()) {
            throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Must wait for client to sync with masternode network. Try again in a minute or so.");
        }

        bool fMnFound = mnodeman.Has(activeMasternode.outpoint);

        DBG( cout << "gobject: submit activeMasternode.pubKeyMasternode = " << activeMasternode.pubKeyMasternode.GetHash().ToString()
             << ", outpoint = " << activeMasternode.outpoint.ToStringShort()
             << ", params.size() = " << params.size()
             << ", fMnFound = " << fMnFound << endl; );

        // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

        uint256 txidFee;

        if(params.size() == 6) {
            txidFee = ParseHashV(params[5], "fee-txid, parameter 6");
        }
        uint256 hashParent;
        if(params[1].get_str() == "0") { // attach to root node (root node doesn't really exist, but has a hash of zero)
            hashParent = uint256();
        } else {
            hashParent = ParseHashV(params[1], "parent object hash, parameter 2");
        }

        // GET THE PARAMETERS FROM USER

        std::string strRevision = params[2].get_str();
        std::string strTime = params[3].get_str();
        int nRevision = boost::lexical_cast<int>(strRevision);
        int nTime = boost::lexical_cast<int>(strTime);
        std::string strData = params[4].get_str();

        CGovernanceObject govobj(hashParent, nRevision, nTime, txidFee, strData);

        DBG( cout << "gobject: submit "
             << " strData = " << govobj.GetDataAsString()
             << ", hash = " << govobj.GetHash().GetHex()
             << ", txidFee = " << txidFee.GetHex()
             << endl; );

        if(govobj.GetObjectType() == GOVERNANCE_OBJECT_PROPOSAL) {
            CProposalValidator validator(strData);
            if(!validator.Validate())  {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid proposal data, error messages:" + validator.GetErrorMessages());
            }
        }

        // Attempt to sign triggers if we are a MN
        if((govobj.GetObjectType() == GOVERNANCE_OBJECT_TRIGGER) ||
           (govobj.GetObjectType() == GOVERNANCE_OBJECT_WATCHDOG)) {
            if(fMnFound) {
                govobj.SetMasternodeVin(activeMasternode.outpoint);
                govobj.Sign(activeMasternode.keyMasternode, activeMasternode.pubKeyMasternode);
            }
            else {
                LogPrintf("gobject(submit) -- Object submission rejected because node is not a masternode\n");
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Only valid masternodes can submit this type of object");
            }
        }
        else {
            if(params.size() != 6) {
                LogPrintf("gobject(submit) -- Object submission rejected because fee tx not provided\n");
                throw JSONRPCError(RPC_INVALID_PARAMETER, "The fee-txid parameter must be included to submit this type of object");
            }
        }

        std::string strHash = govobj.GetHash().ToString();

        std::string strError = "";
        bool fMissingMasternode;
        bool fMissingConfirmations;
        {
            LOCK(cs_main);
            if(!govobj.IsValidLocally(strError, fMissingMasternode, fMissingConfirmations, true) && !fMissingConfirmations) {
                LogPrintf("gobject(submit) -- Object submission rejected because object is not valid - hash = %s, strError = %s\n", strHash, strError);
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Governance object is not valid - " + strHash + " - " + strError);
            }
        }

        // RELAY THIS OBJECT
        // Reject if rate check fails but don't update buffer
        if(!governance.MasternodeRateCheck(govobj)) {
            LogPrintf("gobject(submit) -- Object submission rejected because of rate check failure - hash = %s\n", strHash);
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Object creation rate limit exceeded");
        }

        LogPrintf("gobject(submit) -- Adding locally created governance object - %s\n", strHash);

        if(fMissingConfirmations) {
            governance.AddPostponedObject(govobj);
            govobj.Relay(*g_connman);
        } else {
            governance.AddGovernanceObject(govobj, *g_connman);
        }

        return govobj.GetHash().ToString();
    }

    if(strCommand == "vote-conf")
    {
        if(params.size() != 4)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'gobject vote-conf <governance-hash> [funding|valid|delete] [yes|no|abstain]'");

        uint256 hash;
        std::string strVote;

        hash = ParseHashV(params[1], "Object hash");
        std::string strVoteSignal = params[2].get_str();
        std::string strVoteOutcome = params[3].get_str();

        vote_signal_enum_t eVoteSignal = CGovernanceVoting::ConvertVoteSignal(strVoteSignal);
        if(eVoteSignal == VOTE_SIGNAL_NONE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               "Invalid vote signal. Please using one of the following: "
                               "(funding|valid|delete|endorsed) OR `custom sentinel code` ");
        }

        vote_outcome_enum_t eVoteOutcome = CGovernanceVoting::ConvertVoteOutcome(strVoteOutcome);
        if(eVoteOutcome == VOTE_OUTCOME_NONE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote outcome. Please use one of the following: 'yes', 'no' or 'abstain'");
        }

        int nSuccessful = 0;
        int nFailed = 0;

        UniValue resultsObj(UniValue::VOBJ);

        std::vector<unsigned char> vchMasterNodeSignature;
        std::string strMasterNodeSignMessage;

        UniValue statusObj(UniValue::VOBJ);
        UniValue returnObj(UniValue::VOBJ);

        CMasternode mn;
        bool fMnFound = mnodeman.Get(activeMasternode.outpoint, mn);

        if(!fMnFound) {
            nFailed++;
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "Can't find masternode by collateral output"));
            resultsObj.push_back(Pair("dash.conf", statusObj));
            returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed)));
            returnObj.push_back(Pair("detail", resultsObj));
            return returnObj;
        }

        CGovernanceVote vote(mn.vin.prevout, hash, eVoteSignal, eVoteOutcome);
        if(!vote.Sign(activeMasternode.keyMasternode, activeMasternode.pubKeyMasternode)) {
            nFailed++;
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "Failure to sign."));
            resultsObj.push_back(Pair("dash.conf", statusObj));
            returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed)));
            returnObj.push_back(Pair("detail", resultsObj));
            return returnObj;
        }

        CGovernanceException exception;
        if(governance.ProcessVoteAndRelay(vote, exception, *g_connman)) {
            nSuccessful++;
            statusObj.push_back(Pair("result", "success"));
        }
        else {
            nFailed++;
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", exception.GetMessage()));
        }

        resultsObj.push_back(Pair("dash.conf", statusObj));

        returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed)));
        returnObj.push_back(Pair("detail", resultsObj));

        return returnObj;
    }

    if(strCommand == "vote-many")
    {
        if(params.size() != 4)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'gobject vote-many <governance-hash> [funding|valid|delete] [yes|no|abstain]'");

        uint256 hash;
        std::string strVote;

        hash = ParseHashV(params[1], "Object hash");
        std::string strVoteSignal = params[2].get_str();
        std::string strVoteOutcome = params[3].get_str();


        vote_signal_enum_t eVoteSignal = CGovernanceVoting::ConvertVoteSignal(strVoteSignal);
        if(eVoteSignal == VOTE_SIGNAL_NONE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               "Invalid vote signal. Please using one of the following: "
                               "(funding|valid|delete|endorsed) OR `custom sentinel code` ");
        }

        vote_outcome_enum_t eVoteOutcome = CGovernanceVoting::ConvertVoteOutcome(strVoteOutcome);
        if(eVoteOutcome == VOTE_OUTCOME_NONE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote outcome. Please use one of the following: 'yes', 'no' or 'abstain'");
        }

        int nSuccessful = 0;
        int nFailed = 0;

        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        UniValue resultsObj(UniValue::VOBJ);

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            std::string strError;
            std::vector<unsigned char> vchMasterNodeSignature;
            std::string strMasterNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            UniValue statusObj(UniValue::VOBJ);

            if(!CMessageSigner::GetKeysFromSecret(mne.getPrivKey(), keyMasternode, pubKeyMasternode)){
                nFailed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Masternode signing error, could not set key correctly"));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }

            uint256 nTxHash;
            nTxHash.SetHex(mne.getTxHash());

            int nOutputIndex = 0;
            if(!ParseInt32(mne.getOutputIndex(), &nOutputIndex)) {
                continue;
            }

            COutPoint outpoint(nTxHash, nOutputIndex);

            CMasternode mn;
            bool fMnFound = mnodeman.Get(outpoint, mn);

            if(!fMnFound) {
                nFailed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Can't find masternode by collateral output"));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }

            CGovernanceVote vote(mn.vin.prevout, hash, eVoteSignal, eVoteOutcome);
            if(!vote.Sign(keyMasternode, pubKeyMasternode)){
                nFailed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Failure to sign."));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }

            CGovernanceException exception;
            if(governance.ProcessVoteAndRelay(vote, exception, *g_connman)) {
                nSuccessful++;
                statusObj.push_back(Pair("result", "success"));
            }
            else {
                nFailed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", exception.GetMessage()));
            }

            resultsObj.push_back(Pair(mne.getAlias(), statusObj));
        }

        UniValue returnObj(UniValue::VOBJ);
        returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed)));
        returnObj.push_back(Pair("detail", resultsObj));

        return returnObj;
    }


    // MASTERNODES CAN VOTE ON GOVERNANCE OBJECTS ON THE NETWORK FOR VARIOUS SIGNALS AND OUTCOMES
    if(strCommand == "vote-alias")
    {
        if(params.size() != 5)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'gobject vote-alias <governance-hash> [funding|valid|delete] [yes|no|abstain] <alias-name>'");

        uint256 hash;
        std::string strVote;

        // COLLECT NEEDED PARAMETRS FROM USER

        hash = ParseHashV(params[1], "Object hash");
        std::string strVoteSignal = params[2].get_str();
        std::string strVoteOutcome = params[3].get_str();
        std::string strAlias = params[4].get_str();

        // CONVERT NAMED SIGNAL/ACTION AND CONVERT

        vote_signal_enum_t eVoteSignal = CGovernanceVoting::ConvertVoteSignal(strVoteSignal);
        if(eVoteSignal == VOTE_SIGNAL_NONE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               "Invalid vote signal. Please using one of the following: "
                               "(funding|valid|delete|endorsed) OR `custom sentinel code` ");
        }

        vote_outcome_enum_t eVoteOutcome = CGovernanceVoting::ConvertVoteOutcome(strVoteOutcome);
        if(eVoteOutcome == VOTE_OUTCOME_NONE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote outcome. Please use one of the following: 'yes', 'no' or 'abstain'");
        }

        // EXECUTE VOTE FOR EACH MASTERNODE, COUNT SUCCESSES VS FAILURES

        int nSuccessful = 0;
        int nFailed = 0;

        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        UniValue resultsObj(UniValue::VOBJ);

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries())
        {
            // IF WE HAVE A SPECIFIC NODE REQUESTED TO VOTE, DO THAT
            if(strAlias != mne.getAlias()) continue;

            // INIT OUR NEEDED VARIABLES TO EXECUTE THE VOTE
            std::string strError;
            std::vector<unsigned char> vchMasterNodeSignature;
            std::string strMasterNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            // SETUP THE SIGNING KEY FROM MASTERNODE.CONF ENTRY

            UniValue statusObj(UniValue::VOBJ);

            if(!CMessageSigner::GetKeysFromSecret(mne.getPrivKey(), keyMasternode, pubKeyMasternode)) {
                nFailed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", strprintf("Invalid masternode key %s.", mne.getPrivKey())));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }

            // SEARCH FOR THIS MASTERNODE ON THE NETWORK, THE NODE MUST BE ACTIVE TO VOTE

            uint256 nTxHash;
            nTxHash.SetHex(mne.getTxHash());

            int nOutputIndex = 0;
            if(!ParseInt32(mne.getOutputIndex(), &nOutputIndex)) {
                continue;
            }

            COutPoint outpoint(nTxHash, nOutputIndex);

            CMasternode mn;
            bool fMnFound = mnodeman.Get(outpoint, mn);

            if(!fMnFound) {
                nFailed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Masternode must be publically available on network to vote. Masternode not found."));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }

            // CREATE NEW GOVERNANCE OBJECT VOTE WITH OUTCOME/SIGNAL

            CGovernanceVote vote(outpoint, hash, eVoteSignal, eVoteOutcome);
            if(!vote.Sign(keyMasternode, pubKeyMasternode)) {
                nFailed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Failure to sign."));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }

            // UPDATE LOCAL DATABASE WITH NEW OBJECT SETTINGS

            CGovernanceException exception;
            if(governance.ProcessVoteAndRelay(vote, exception, *g_connman)) {
                nSuccessful++;
                statusObj.push_back(Pair("result", "success"));
            }
            else {
                nFailed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", exception.GetMessage()));
            }

            resultsObj.push_back(Pair(mne.getAlias(), statusObj));
        }

        // REPORT STATS TO THE USER

        UniValue returnObj(UniValue::VOBJ);
        returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed)));
        returnObj.push_back(Pair("detail", resultsObj));

        return returnObj;
    }

    // USERS CAN QUERY THE SYSTEM FOR A LIST OF VARIOUS GOVERNANCE ITEMS
    if(strCommand == "list" || strCommand == "diff")
    {
        if (params.size() > 3)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'gobject [list|diff] ( signal type )'");

        // GET MAIN PARAMETER FOR THIS MODE, VALID OR ALL?

        std::string strCachedSignal = "valid";
        if (params.size() >= 2) strCachedSignal = params[1].get_str();
        if (strCachedSignal != "valid" && strCachedSignal != "funding" && strCachedSignal != "delete" && strCachedSignal != "endorsed" && strCachedSignal != "all")
            return "Invalid signal, should be 'valid', 'funding', 'delete', 'endorsed' or 'all'";

        std::string strType = "all";
        if (params.size() == 3) strType = params[2].get_str();
        if (strType != "proposals" && strType != "triggers" && strType != "watchdogs" && strType != "all")
            return "Invalid type, should be 'proposals', 'triggers', 'watchdogs' or 'all'";

        // GET STARTING TIME TO QUERY SYSTEM WITH

        int nStartTime = 0; //list
        if(strCommand == "diff") nStartTime = governance.GetLastDiffTime();

        // SETUP BLOCK INDEX VARIABLE / RESULTS VARIABLE

        UniValue objResult(UniValue::VOBJ);

        // GET MATCHING GOVERNANCE OBJECTS

        LOCK2(cs_main, governance.cs);

        std::vector<CGovernanceObject*> objs = governance.GetAllNewerThan(nStartTime);
        governance.UpdateLastDiffTime(GetTime());

        // CREATE RESULTS FOR USER

        BOOST_FOREACH(CGovernanceObject* pGovObj, objs)
        {
            if(strCachedSignal == "valid" && !pGovObj->IsSetCachedValid()) continue;
            if(strCachedSignal == "funding" && !pGovObj->IsSetCachedFunding()) continue;
            if(strCachedSignal == "delete" && !pGovObj->IsSetCachedDelete()) continue;
            if(strCachedSignal == "endorsed" && !pGovObj->IsSetCachedEndorsed()) continue;

            if(strType == "proposals" && pGovObj->GetObjectType() != GOVERNANCE_OBJECT_PROPOSAL) continue;
            if(strType == "triggers" && pGovObj->GetObjectType() != GOVERNANCE_OBJECT_TRIGGER) continue;
            if(strType == "watchdogs" && pGovObj->GetObjectType() != GOVERNANCE_OBJECT_WATCHDOG) continue;

            UniValue bObj(UniValue::VOBJ);
            bObj.push_back(Pair("DataHex",  pGovObj->GetDataAsHex()));
            bObj.push_back(Pair("DataString",  pGovObj->GetDataAsString()));
            bObj.push_back(Pair("Hash",  pGovObj->GetHash().ToString()));
            bObj.push_back(Pair("CollateralHash",  pGovObj->GetCollateralHash().ToString()));
            bObj.push_back(Pair("ObjectType", pGovObj->GetObjectType()));
            bObj.push_back(Pair("CreationTime", pGovObj->GetCreationTime()));
            const CTxIn& masternodeVin = pGovObj->GetMasternodeVin();
            if(masternodeVin != CTxIn()) {
                bObj.push_back(Pair("SigningMasternode", masternodeVin.prevout.ToStringShort()));
            }

            // REPORT STATUS FOR FUNDING VOTES SPECIFICALLY
            bObj.push_back(Pair("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING)));
            bObj.push_back(Pair("YesCount",  pGovObj->GetYesCount(VOTE_SIGNAL_FUNDING)));
            bObj.push_back(Pair("NoCount",  pGovObj->GetNoCount(VOTE_SIGNAL_FUNDING)));
            bObj.push_back(Pair("AbstainCount",  pGovObj->GetAbstainCount(VOTE_SIGNAL_FUNDING)));

            // REPORT VALIDITY AND CACHING FLAGS FOR VARIOUS SETTINGS
            std::string strError = "";
            bObj.push_back(Pair("fBlockchainValidity",  pGovObj->IsValidLocally(strError, false)));
            bObj.push_back(Pair("IsValidReason",  strError.c_str()));
            bObj.push_back(Pair("fCachedValid",  pGovObj->IsSetCachedValid()));
            bObj.push_back(Pair("fCachedFunding",  pGovObj->IsSetCachedFunding()));
            bObj.push_back(Pair("fCachedDelete",  pGovObj->IsSetCachedDelete()));
            bObj.push_back(Pair("fCachedEndorsed",  pGovObj->IsSetCachedEndorsed()));

            objResult.push_back(Pair(pGovObj->GetHash().ToString(), bObj));
        }

        return objResult;
    }

    // GET SPECIFIC GOVERNANCE ENTRY
    if(strCommand == "get")
    {
        if (params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'gobject get <governance-hash>'");

        // COLLECT VARIABLES FROM OUR USER
        uint256 hash = ParseHashV(params[1], "GovObj hash");

        LOCK2(cs_main, governance.cs);

        // FIND THE GOVERNANCE OBJECT THE USER IS LOOKING FOR
        CGovernanceObject* pGovObj = governance.FindGovernanceObject(hash);

        if(pGovObj == NULL)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown governance object");

        // REPORT BASIC OBJECT STATS

        UniValue objResult(UniValue::VOBJ);
        objResult.push_back(Pair("DataHex",  pGovObj->GetDataAsHex()));
        objResult.push_back(Pair("DataString",  pGovObj->GetDataAsString()));
        objResult.push_back(Pair("Hash",  pGovObj->GetHash().ToString()));
        objResult.push_back(Pair("CollateralHash",  pGovObj->GetCollateralHash().ToString()));
        objResult.push_back(Pair("ObjectType", pGovObj->GetObjectType()));
        objResult.push_back(Pair("CreationTime", pGovObj->GetCreationTime()));
        const CTxIn& masternodeVin = pGovObj->GetMasternodeVin();
        if(masternodeVin != CTxIn()) {
            objResult.push_back(Pair("SigningMasternode", masternodeVin.prevout.ToStringShort()));
        }

        // SHOW (MUCH MORE) INFORMATION ABOUT VOTES FOR GOVERNANCE OBJECT (THAN LIST/DIFF ABOVE)
        // -- FUNDING VOTING RESULTS

        UniValue objFundingResult(UniValue::VOBJ);
        objFundingResult.push_back(Pair("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING)));
        objFundingResult.push_back(Pair("YesCount",  pGovObj->GetYesCount(VOTE_SIGNAL_FUNDING)));
        objFundingResult.push_back(Pair("NoCount",  pGovObj->GetNoCount(VOTE_SIGNAL_FUNDING)));
        objFundingResult.push_back(Pair("AbstainCount",  pGovObj->GetAbstainCount(VOTE_SIGNAL_FUNDING)));
        objResult.push_back(Pair("FundingResult", objFundingResult));

        // -- VALIDITY VOTING RESULTS
        UniValue objValid(UniValue::VOBJ);
        objValid.push_back(Pair("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_VALID)));
        objValid.push_back(Pair("YesCount",  pGovObj->GetYesCount(VOTE_SIGNAL_VALID)));
        objValid.push_back(Pair("NoCount",  pGovObj->GetNoCount(VOTE_SIGNAL_VALID)));
        objValid.push_back(Pair("AbstainCount",  pGovObj->GetAbstainCount(VOTE_SIGNAL_VALID)));
        objResult.push_back(Pair("ValidResult", objValid));

        // -- DELETION CRITERION VOTING RESULTS
        UniValue objDelete(UniValue::VOBJ);
        objDelete.push_back(Pair("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_DELETE)));
        objDelete.push_back(Pair("YesCount",  pGovObj->GetYesCount(VOTE_SIGNAL_DELETE)));
        objDelete.push_back(Pair("NoCount",  pGovObj->GetNoCount(VOTE_SIGNAL_DELETE)));
        objDelete.push_back(Pair("AbstainCount",  pGovObj->GetAbstainCount(VOTE_SIGNAL_DELETE)));
        objResult.push_back(Pair("DeleteResult", objDelete));

        // -- ENDORSED VIA MASTERNODE-ELECTED BOARD
        UniValue objEndorsed(UniValue::VOBJ);
        objEndorsed.push_back(Pair("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_ENDORSED)));
        objEndorsed.push_back(Pair("YesCount",  pGovObj->GetYesCount(VOTE_SIGNAL_ENDORSED)));
        objEndorsed.push_back(Pair("NoCount",  pGovObj->GetNoCount(VOTE_SIGNAL_ENDORSED)));
        objEndorsed.push_back(Pair("AbstainCount",  pGovObj->GetAbstainCount(VOTE_SIGNAL_ENDORSED)));
        objResult.push_back(Pair("EndorsedResult", objEndorsed));

        // --
        std::string strError = "";
        objResult.push_back(Pair("fLocalValidity",  pGovObj->IsValidLocally(strError, false)));
        objResult.push_back(Pair("IsValidReason",  strError.c_str()));
        objResult.push_back(Pair("fCachedValid",  pGovObj->IsSetCachedValid()));
        objResult.push_back(Pair("fCachedFunding",  pGovObj->IsSetCachedFunding()));
        objResult.push_back(Pair("fCachedDelete",  pGovObj->IsSetCachedDelete()));
        objResult.push_back(Pair("fCachedEndorsed",  pGovObj->IsSetCachedEndorsed()));
        return objResult;
    }

    // GETVOTES FOR SPECIFIC GOVERNANCE OBJECT
    if(strCommand == "getvotes")
    {
        if (params.size() != 2)
            throw std::runtime_error(
                "Correct usage is 'gobject getvotes <governance-hash>'"
                );

        // COLLECT PARAMETERS FROM USER

        uint256 hash = ParseHashV(params[1], "Governance hash");

        // FIND OBJECT USER IS LOOKING FOR

        LOCK(governance.cs);

        CGovernanceObject* pGovObj = governance.FindGovernanceObject(hash);

        if(pGovObj == NULL) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown governance-hash");
        }

        // REPORT RESULTS TO USER

        UniValue bResult(UniValue::VOBJ);

        // GET MATCHING VOTES BY HASH, THEN SHOW USERS VOTE INFORMATION

        std::vector<CGovernanceVote> vecVotes = governance.GetMatchingVotes(hash);
        BOOST_FOREACH(CGovernanceVote vote, vecVotes) {
            bResult.push_back(Pair(vote.GetHash().ToString(),  vote.ToString()));
        }

        return bResult;
    }

    // GETVOTES FOR SPECIFIC GOVERNANCE OBJECT
    if(strCommand == "getcurrentvotes")
    {
        if (params.size() != 2 && params.size() != 4)
            throw std::runtime_error(
                "Correct usage is 'gobject getcurrentvotes <governance-hash> [txid vout_index]'"
                );

        // COLLECT PARAMETERS FROM USER

        uint256 hash = ParseHashV(params[1], "Governance hash");

        COutPoint mnCollateralOutpoint;
        if (params.size() == 4) {
            uint256 txid = ParseHashV(params[2], "Masternode Collateral hash");
            std::string strVout = params[3].get_str();
            uint32_t vout = boost::lexical_cast<uint32_t>(strVout);
            mnCollateralOutpoint = COutPoint(txid, vout);
        }

        // FIND OBJECT USER IS LOOKING FOR

        LOCK(governance.cs);

        CGovernanceObject* pGovObj = governance.FindGovernanceObject(hash);

        if(pGovObj == NULL) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown governance-hash");
        }

        // REPORT RESULTS TO USER

        UniValue bResult(UniValue::VOBJ);

        // GET MATCHING VOTES BY HASH, THEN SHOW USERS VOTE INFORMATION

        std::vector<CGovernanceVote> vecVotes = governance.GetCurrentVotes(hash, mnCollateralOutpoint);
        BOOST_FOREACH(CGovernanceVote vote, vecVotes) {
            bResult.push_back(Pair(vote.GetHash().ToString(),  vote.ToString()));
        }

        return bResult;
    }

    return NullUniValue;
}

UniValue voteraw(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 7)
        throw std::runtime_error(
                "voteraw <masternode-tx-hash> <masternode-tx-index> <governance-hash> <vote-signal> [yes|no|abstain] <time> <vote-sig>\n"
                "Compile and relay a governance vote with provided external signature instead of signing vote internally\n"
                );

    uint256 hashMnTx = ParseHashV(params[0], "mn tx hash");
    int nMnTxIndex = params[1].get_int();
    COutPoint outpoint = COutPoint(hashMnTx, nMnTxIndex);

    uint256 hashGovObj = ParseHashV(params[2], "Governance hash");
    std::string strVoteSignal = params[3].get_str();
    std::string strVoteOutcome = params[4].get_str();

    vote_signal_enum_t eVoteSignal = CGovernanceVoting::ConvertVoteSignal(strVoteSignal);
    if(eVoteSignal == VOTE_SIGNAL_NONE)  {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "Invalid vote signal. Please using one of the following: "
                           "(funding|valid|delete|endorsed) OR `custom sentinel code` ");
    }

    vote_outcome_enum_t eVoteOutcome = CGovernanceVoting::ConvertVoteOutcome(strVoteOutcome);
    if(eVoteOutcome == VOTE_OUTCOME_NONE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote outcome. Please use one of the following: 'yes', 'no' or 'abstain'");
    }

    int64_t nTime = params[5].get_int64();
    std::string strSig = params[6].get_str();
    bool fInvalid = false;
    std::vector<unsigned char> vchSig = DecodeBase64(strSig.c_str(), &fInvalid);

    if (fInvalid) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");
    }

    CMasternode mn;
    bool fMnFound = mnodeman.Get(outpoint, mn);

    if(!fMnFound) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failure to find masternode in list : " + outpoint.ToStringShort());
    }

    CGovernanceVote vote(outpoint, hashGovObj, eVoteSignal, eVoteOutcome);
    vote.SetTime(nTime);
    vote.SetSignature(vchSig);

    if(!vote.IsValid(true)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failure to verify vote.");
    }

    CGovernanceException exception;
    if(governance.ProcessVoteAndRelay(vote, exception, *g_connman)) {
        return "Voted successfully";
    }
    else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Error voting : " + exception.GetMessage());
    }
}

UniValue getgovernanceinfo(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0) {
        throw std::runtime_error(
            "getgovernanceinfo\n"
            "Returns an object containing governance parameters.\n"
            "\nResult:\n"
            "{\n"
            "  \"governanceminquorum\": xxxxx,           (numeric) the absolute minimum number of votes needed to trigger a governance action\n"
            "  \"masternodewatchdogmaxseconds\": xxxxx,  (numeric) sentinel watchdog expiration time in seconds\n"
            "  \"proposalfee\": xxx.xx,                  (numeric) the collateral transaction fee which must be paid to create a proposal in " + CURRENCY_UNIT + "\n"
            "  \"superblockcycle\": xxxxx,               (numeric) the number of blocks between superblocks\n"
            "  \"lastsuperblock\": xxxxx,                (numeric) the block number of the last superblock\n"
            "  \"nextsuperblock\": xxxxx,                (numeric) the block number of the next superblock\n"
            "  \"maxgovobjdatasize\": xxxxx,             (numeric) maximum governance object data size in bytes\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getgovernanceinfo", "")
            + HelpExampleRpc("getgovernanceinfo", "")
            );
    }

    // Compute last/next superblock
    int nLastSuperblock, nNextSuperblock;

    // Get current block height
    int nBlockHeight = 0;
    {
        LOCK(cs_main);
        nBlockHeight = (int)chainActive.Height();
    }

    // Get chain parameters
    int nSuperblockStartBlock = Params().GetConsensus().nSuperblockStartBlock;
    int nSuperblockCycle = Params().GetConsensus().nSuperblockCycle;

    // Get first superblock
    int nFirstSuperblockOffset = (nSuperblockCycle - nSuperblockStartBlock % nSuperblockCycle) % nSuperblockCycle;
    int nFirstSuperblock = nSuperblockStartBlock + nFirstSuperblockOffset;

    if(nBlockHeight < nFirstSuperblock){
        nLastSuperblock = 0;
        nNextSuperblock = nFirstSuperblock;
    } else {
        nLastSuperblock = nBlockHeight - nBlockHeight % nSuperblockCycle;
        nNextSuperblock = nLastSuperblock + nSuperblockCycle;
    }

    UniValue obj(UniValue::VOBJ);
    obj.push_back(Pair("governanceminquorum", Params().GetConsensus().nGovernanceMinQuorum));
    obj.push_back(Pair("masternodewatchdogmaxseconds", MASTERNODE_WATCHDOG_MAX_SECONDS));
    obj.push_back(Pair("proposalfee", ValueFromAmount(GOVERNANCE_PROPOSAL_FEE_TX)));
    obj.push_back(Pair("superblockcycle", Params().GetConsensus().nSuperblockCycle));
    obj.push_back(Pair("lastsuperblock", nLastSuperblock));
    obj.push_back(Pair("nextsuperblock", nNextSuperblock));
    obj.push_back(Pair("maxgovobjdatasize", MAX_GOVERNANCE_OBJECT_DATA_SIZE));

    return obj;
}

UniValue getsuperblockbudget(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1) {
        throw std::runtime_error(
            "getsuperblockbudget index\n"
            "\nReturns the absolute maximum sum of superblock payments allowed.\n"
            "\nArguments:\n"
            "1. index         (numeric, required) The block index\n"
            "\nResult:\n"
            "n                (numeric) The absolute maximum sum of superblock payments allowed, in " + CURRENCY_UNIT + "\n"
            "\nExamples:\n"
            + HelpExampleCli("getsuperblockbudget", "1000")
            + HelpExampleRpc("getsuperblockbudget", "1000")
        );
    }

    int nBlockHeight = params[0].get_int();
    if (nBlockHeight < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
    }

    CAmount nBudget = CSuperblock::GetPaymentsLimit(nBlockHeight);
    std::string strBudget = FormatMoney(nBudget);

    return strBudget;
}

