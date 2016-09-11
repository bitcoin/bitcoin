// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//#define ENABLE_DASH_DEBUG

#include "util.h"
#include "main.h"
#include "db.h"
#include "init.h"
#include "activemasternode.h"
#include "darksend.h"
#include "governance.h"
#include "darksend.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "rpcserver.h"
#include "utilmoneystr.h"
#include "governance-vote.h"
#include "governance-classes.h"
#include <boost/lexical_cast.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

UniValue gobject(const UniValue& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "vote-many" && strCommand != "vote-conf" && strCommand != "vote-alias" && strCommand != "prepare" && strCommand != "submit" &&
         strCommand != "vote" && strCommand != "get" && strCommand != "getvotes" && strCommand != "list" && strCommand != "diff" && strCommand != "deserialize"))
        throw runtime_error(
                "gobject \"command\"...\n"
                "Manage governance objects\n"
                "\nAvailable commands:\n"
                "  prepare            - Prepare govobj by signing and creating tx\n"
                "  submit             - Submit govobj to network\n"
                "  get                - Get govobj by hash\n"
                "  getvotes           - Get votes for a govobj hash\n"
                "  list               - List all govobjs\n"
                "  diff               - List differences since last diff\n"
                "  vote-many          - Vote on a governance object by all masternodes (using masternode.conf setup)\n"
                "  vote-conf          - Vote on a governance object by masternode configured in dash.conf\n"
                "  vote-alias         - Vote on a governance object by masternode alias\n"
                );
    

    /*
        ------ Example Governance Item ------ 
        
        gobject submit 6e622bb41bad1fb18e7f23ae96770aeb33129e18bd9efe790522488e580a0a03 0 1 1464292854 "beer-reimbursement" 5b5b22636f6e7472616374222c207b2270726f6a6563745f6e616d65223a20225c22626565722d7265696d62757273656d656e745c22222c20227061796d656e745f61646472657373223a20225c225879324c4b4a4a64655178657948726e34744744514238626a6876464564615576375c22222c2022656e645f64617465223a202231343936333030343030222c20226465736372697074696f6e5f75726c223a20225c227777772e646173687768616c652e6f72672f702f626565722d7265696d62757273656d656e745c22222c2022636f6e74726163745f75726c223a20225c22626565722d7265696d62757273656d656e742e636f6d2f3030312e7064665c22222c20227061796d656e745f616d6f756e74223a20223233342e323334323232222c2022676f7665726e616e63655f6f626a6563745f6964223a2037342c202273746172745f64617465223a202231343833323534303030227d5d5d1
    */

    // DEBUG : TEST DESERIALIZATION OF GOVERNANCE META DATA
    if(strCommand == "deserialize")
    {
        std::string strHex = params[1].get_str();

        vector<unsigned char> v = ParseHex(strHex);
        string s(v.begin(), v.end());

        UniValue u(UniValue::VOBJ);
        u.read(s);

        return u.write().c_str();
    }

    // PREPARE THE GOVERNANCE OBJECT BY CREATING A COLLATERAL TRANSACTION
    if(strCommand == "prepare")
    {
        if (params.size() != 6)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'gobject prepare <parent-hash> <revision> <time> <name> <data-hex>'");

        // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

        LOCK(cs_main);
        CBlockIndex* pindex = chainActive.Tip();

        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        uint256 hashParent;

        // -- attach to root node (root node doesn't really exist, but has a hash of zero)
        if(params[1].get_str() == "0") { 
            hashParent = uint256();
        } else {
            hashParent = ParseHashV(params[1], "fee-tx hash, parameter 1");
        }

        std::string strRevision = params[2].get_str();
        std::string strTime = params[3].get_str();
        int nRevision = boost::lexical_cast<int>(strRevision);
        int nTime = boost::lexical_cast<int>(strTime);
        std::string strName = SanitizeString(params[4].get_str());
        std::string strData = params[5].get_str();
        
        // CREATE A NEW COLLATERAL TRANSACTION FOR THIS SPECIFIC OBJECT

        CGovernanceObject govobj(hashParent, nRevision, strName, nTime, uint256(), strData);

        std::string strError = "";
        if(!govobj.IsValidLocally(pindex, strError, false))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Governance object is not valid - " + govobj.GetHash().ToString() + " - " + strError);

        CWalletTx wtx;
        if(!pwalletMain->GetBudgetSystemCollateralTX(wtx, govobj.GetHash(), govobj.GetMinCollateralFee(), false)) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Error making collateral transaction for govobj. Please check your wallet balance and make sure your wallet is unlocked.");
        }
        
        // -- make our change address
        CReserveKey reservekey(pwalletMain);
        // -- send the tx to the network
        pwalletMain->CommitTransaction(wtx, reservekey, NetMsgType::TX);

        DBG( cout << "gobject: prepare strName = " << strName
             << ", strData = " << govobj.GetDataAsString()
             << ", hash = " << govobj.GetHash().GetHex()
             << ", fee_tx = " << wtx.GetHash().GetHex()
             << endl; );

        return wtx.GetHash().ToString();
    }

    // AFTER COLLATERAL TRANSACTION HAS MATURED USER CAN SUBMIT GOVERNANCE OBJECT TO PROPAGATE NETWORK
    if(strCommand == "submit")
    {
        if ((params.size() < 6) || (params.size() > 7))  {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'gobject submit <parent-hash> <revision> <time> <name> <data-hex> <fee-tx>'");
        }

        if(!masternodeSync.IsBlockchainSynced()) {
            throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Must wait for client to sync with masternode network. Try again in a minute or so.");
        }

        CMasternode mn;
        bool mnFound = mnodeman.Get(activeMasternode.pubKeyMasternode, mn);

        DBG( cout << "gobject: submit activeMasternode.pubKeyMasternode = " << activeMasternode.pubKeyMasternode.GetHash().ToString()
             << ", params.size() = " << params.size()
             << ", mnFound = " << mnFound << endl; );

        if((params.size() == 6) && (!mnFound))  {
            throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Non-masternodes must include fee-tx parameter.");
        }

        // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

        LOCK(cs_main);
        CBlockIndex* pindex = chainActive.Tip();

        uint256 fee_tx;

        if(params.size() == 7) {
            fee_tx = ParseHashV(params[6], "fee-tx hash, parameter 6");
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
        std::string strName = SanitizeString(params[4].get_str());
        std::string strData = params[5].get_str();

        CGovernanceObject govobj(hashParent, nRevision, strName, nTime, fee_tx, strData);

        DBG( cout << "gobject: submit strName = " << strName
             << ", strData = " << govobj.GetDataAsString()
             << ", hash = " << govobj.GetHash().GetHex()
             << ", fee_tx = " << fee_tx.GetHex()
             << endl; );

        // Attempt to sign triggers if we are a MN
        if(govobj.GetObjectType() == GOVERNANCE_OBJECT_TRIGGER) {
            if(mnFound) {
                govobj.SetMasternodeInfo(mn.vin, activeMasternode.pubKeyMasternode);
                govobj.Sign(activeMasternode.keyMasternode);
            }
            else {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Only valid masternodes can submit this type of object");
            }
        }

        std::string strError = "";
        if(!govobj.IsValidLocally(pindex, strError, true)) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Governance object is not valid - " + govobj.GetHash().ToString() + " - " + strError);
        }

        // RELAY THIS OBJECT
        governance.AddSeenGovernanceObject(govobj.GetHash(), SEEN_OBJECT_IS_VALID);
        govobj.Relay();
        governance.AddGovernanceObject(govobj);

        return govobj.GetHash().ToString();

    }

    if(strCommand == "vote-conf")
    {
        if(params.size() != 4)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'mngovernance vote-conf <governance-hash> [funding|valid|delete] [yes|no|abstain]'");

        uint256 hash;
        std::string strVote;

        hash = ParseHashV(params[1], "Object hash");
        std::string strVoteAction = params[2].get_str();
        std::string strVoteOutcome = params[3].get_str();
        
        vote_signal_enum_t eVoteSignal = CGovernanceVoting::ConvertVoteSignal(strVoteAction);
        if(eVoteSignal == VOTE_SIGNAL_NONE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, 
                               "Invalid vote signal. Please using one of the following: "
                               "(funding|valid|delete|endorsed) OR `custom sentinel code` "); 
        }

        vote_outcome_enum_t eVoteOutcome = CGovernanceVoting::ConvertVoteOutcome(strVoteOutcome);
        if(eVoteOutcome == VOTE_OUTCOME_NONE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote outcome. Please use one of the following: 'yes', 'no' or 'abstain'");
        }

        int success = 0;
        int failed = 0;

        UniValue resultsObj(UniValue::VOBJ);

        std::string errorMessage;
        std::vector<unsigned char> vchMasterNodeSignature;
        std::string strMasterNodeSignMessage;

        UniValue statusObj(UniValue::VOBJ);

        CMasternode* pmn = mnodeman.Find(activeMasternode.pubKeyMasternode);
        if(pmn == NULL)
        {
            failed++;
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "Can't find masternode by pubkey"));
            resultsObj.push_back(Pair("dash.conf", statusObj));
            return "Can't find masternode by pubkey";
        }

        CGovernanceVote vote(pmn->vin, hash, eVoteSignal, eVoteOutcome);
        if(!vote.Sign(activeMasternode.keyMasternode, activeMasternode.pubKeyMasternode)){
            failed++;
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "Failure to sign."));
            resultsObj.push_back(Pair("dash.conf", statusObj));
            return "Failure to sign.";
        }

        std::string strError = "";
        if(governance.AddOrUpdateVote(vote, NULL, strError)) {
            governance.AddSeenVote(vote.GetHash(), SEEN_OBJECT_IS_VALID);
            vote.Relay();
            success++;
            statusObj.push_back(Pair("result", "success"));
        } else {
            failed++;
            statusObj.push_back(Pair("result", strError.c_str()));
        }

        resultsObj.push_back(Pair("dash.conf", statusObj));

        UniValue returnObj(UniValue::VOBJ);
        returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed)));
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
        std::string strVoteAction = params[2].get_str();
        std::string strVoteOutcome = params[3].get_str();


        vote_signal_enum_t eVoteSignal = CGovernanceVoting::ConvertVoteSignal(strVoteAction);
        if(eVoteSignal == VOTE_SIGNAL_NONE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               "Invalid vote signal. Please using one of the following: "
                               "(funding|valid|delete|endorsed) OR `custom sentinel code` ");
        }

        vote_outcome_enum_t eVoteOutcome = CGovernanceVoting::ConvertVoteOutcome(strVoteOutcome);
        if(eVoteOutcome == VOTE_OUTCOME_NONE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote outcome. Please use one of the following: 'yes', 'no' or 'abstain'");
        }

        int success = 0;
        int failed = 0;

        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        UniValue resultsObj(UniValue::VOBJ);

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            std::string errorMessage;
            std::vector<unsigned char> vchMasterNodeSignature;
            std::string strMasterNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            UniValue statusObj(UniValue::VOBJ);

            if(!darkSendSigner.GetKeysFromSecret(mne.getPrivKey(), keyMasternode, pubKeyMasternode)){
                failed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Masternode signing error, could not set key correctly: " + errorMessage));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }

            CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
            if(pmn == NULL)
            {
                failed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Can't find masternode by pubkey"));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }

            CGovernanceVote vote(pmn->vin, hash, eVoteSignal, eVoteOutcome);
            if(!vote.Sign(keyMasternode, pubKeyMasternode)){
                failed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Failure to sign."));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }

            std::string strError = "";
            if(governance.AddOrUpdateVote(vote, NULL, strError)) {
                governance.AddSeenVote(vote.GetHash(), SEEN_OBJECT_IS_VALID);
                vote.Relay();
                success++;
                statusObj.push_back(Pair("result", "success"));
            } else {
                failed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", strError.c_str()));
            }

            resultsObj.push_back(Pair(mne.getAlias(), statusObj));
        }

        UniValue returnObj(UniValue::VOBJ);
        returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed)));
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
        std::string strVoteAction = params[2].get_str();
        std::string strVoteOutcome = params[3].get_str();
        std::string strAlias = params[4].get_str();
       
        // CONVERT NAMED SIGNAL/ACTION AND CONVERT

        vote_signal_enum_t eVoteSignal = CGovernanceVoting::ConvertVoteSignal(strVoteAction);
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

        int success = 0;
        int failed = 0;

        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        UniValue resultsObj(UniValue::VOBJ);

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries())
        {
            // IF WE HAVE A SPECIFIC NODE REQUESTED TO VOTE, DO THAT
            if( strAlias != mne.getAlias()) continue;

            // INIT OUR NEEDED VARIABLES TO EXECUTE THE VOTE
            std::string errorMessage;
            std::vector<unsigned char> vchMasterNodeSignature;
            std::string strMasterNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            // SETUP THE SIGNING KEY FROM MASTERNODE.CONF ENTRY

            UniValue statusObj(UniValue::VOBJ);

            if(!darkSendSigner.GetKeysFromSecret(mne.getPrivKey(), keyMasternode, pubKeyMasternode)) {
                failed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", strprintf("Invalid masternode key %s.", mne.getPrivKey())));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }

            // SEARCH FOR THIS MASTERNODE ON THE NETWORK, THE NODE MUST BE ACTIVE TO VOTE

            CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
            if(pmn == NULL)
            {
                failed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Masternode must be publically available on network to vote. Masternode not found."));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }

            // CREATE NEW GOVERNANCE OBJECT VOTE WITH OUTCOME/SIGNAL

            CGovernanceVote vote(pmn->vin, hash, eVoteSignal, eVoteOutcome);
            if(!vote.Sign(keyMasternode, pubKeyMasternode)){
                failed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Failure to sign."));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }

            // UPDATE LOCAL DATABASE WITH NEW OBJECT SETTINGS

            std::string strError = "";
            if(governance.AddOrUpdateVote(vote, NULL, strError)) {
                governance.AddSeenVote(vote.GetHash(), SEEN_OBJECT_IS_VALID);
                vote.Relay();
                success++;
                statusObj.push_back(Pair("result", "success"));
            } else {
                failed++;
                statusObj.push_back(Pair("result", strError.c_str()));
            }

            resultsObj.push_back(Pair(mne.getAlias(), statusObj));
        }

        // REPORT STATS TO THE USER

        UniValue returnObj(UniValue::VOBJ);
        returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed)));
        returnObj.push_back(Pair("detail", resultsObj));

        return returnObj;
    }

    // USERS CAN QUERY THE SYSTEM FOR A LIST OF VARIOUS GOVERNANCE ITEMS
    if(strCommand == "list" || strCommand == "diff")
    {
        if (params.size() > 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'gobject [list|diff] [valid]'");

        // GET MAIN PARAMETER FOR THIS MODE, VALID OR ALL?

        std::string strShow = "valid";
        if (params.size() == 2) strShow = params[1].get_str();
        if (strShow != "valid" && strShow != "all") return "Invalid mode, should be valid or all";

        // GET STARTING TIME TO QUERY SYSTEM WITH

        int nStartTime = 0; //list
        if(strCommand == "diff") nStartTime = governance.GetLastDiffTime();

        // SETUP BLOCK INDEX VARIABLE / RESULTS VARIABLE

        CBlockIndex* pindex;
        {
            LOCK(cs_main);
            pindex = chainActive.Tip();
        }

        UniValue objResult(UniValue::VOBJ);

        // GET MATCHING GOVERNANCE OBJECTS

        LOCK(governance.cs);

        std::vector<CGovernanceObject*> objs = governance.GetAllNewerThan(nStartTime);
        governance.UpdateLastDiffTime(GetTime());

        // CREATE RESULTS FOR USER

        BOOST_FOREACH(CGovernanceObject* pGovObj, objs)
        {
            // IF WE HAVE A SPECIFIC NODE REQUESTED TO VOTE, DO THAT
            if(strShow == "valid" && !pGovObj->fCachedValid) continue;

            UniValue bObj(UniValue::VOBJ);
            bObj.push_back(Pair("Name",  pGovObj->GetName()));
            bObj.push_back(Pair("DataHex",  pGovObj->GetDataAsHex()));
            bObj.push_back(Pair("DataString",  pGovObj->GetDataAsString()));
            bObj.push_back(Pair("Hash",  pGovObj->GetHash().ToString()));
            bObj.push_back(Pair("CollateralHash",  pGovObj->nCollateralHash.ToString()));

            // REPORT STATUS FOR FUNDING VOTES SPECIFICALLY
            bObj.push_back(Pair("AbsoluteYesCount",  (int64_t)pGovObj->GetYesCount(VOTE_SIGNAL_FUNDING)-(int64_t)pGovObj->GetNoCount(VOTE_SIGNAL_FUNDING)));
            bObj.push_back(Pair("YesCount",  (int64_t)pGovObj->GetYesCount(VOTE_SIGNAL_FUNDING)));
            bObj.push_back(Pair("NoCount",  (int64_t)pGovObj->GetNoCount(VOTE_SIGNAL_FUNDING)));
            bObj.push_back(Pair("AbstainCount",  (int64_t)pGovObj->GetAbstainCount(VOTE_SIGNAL_FUNDING)));

            // REPORT VALIDITY AND CACHING FLAGS FOR VARIOUS SETTINGS
            std::string strError = "";
            bObj.push_back(Pair("fBlockchainValidity",  pGovObj->IsValidLocally(pindex , strError, false)));
            bObj.push_back(Pair("IsValidReason",  strError.c_str()));
            bObj.push_back(Pair("fCachedValid",  pGovObj->fCachedValid));
            bObj.push_back(Pair("fCachedFunding",  pGovObj->fCachedFunding));
            bObj.push_back(Pair("fCachedDelete",  pGovObj->fCachedDelete));
            bObj.push_back(Pair("fCachedEndorsed",  pGovObj->fCachedEndorsed));

            objResult.push_back(Pair(pGovObj->GetName(), bObj));
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
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown govobj");

        // REPORT BASIC OBJECT STATS

        UniValue objResult(UniValue::VOBJ);
        objResult.push_back(Pair("Name",  pGovObj->GetName()));
        objResult.push_back(Pair("Hash",  pGovObj->GetHash().ToString()));
        objResult.push_back(Pair("CollateralHash",  pGovObj->nCollateralHash.ToString()));

        // SHOW (MUCH MORE) INFORMATION ABOUT VOTES FOR GOVERNANCE OBJECT (THAN LIST/DIFF ABOVE)
        // -- FUNDING VOTING RESULTS

        UniValue objFundingResult(UniValue::VOBJ);
        objFundingResult.push_back(Pair("AbsoluteYesCount",  (int64_t)pGovObj->GetYesCount(VOTE_SIGNAL_FUNDING)-(int64_t)pGovObj->GetNoCount(VOTE_SIGNAL_FUNDING)));
        objFundingResult.push_back(Pair("YesCount",  (int64_t)pGovObj->GetYesCount(VOTE_SIGNAL_FUNDING)));
        objFundingResult.push_back(Pair("NoCount",  (int64_t)pGovObj->GetNoCount(VOTE_SIGNAL_FUNDING)));
        objFundingResult.push_back(Pair("AbstainCount",  (int64_t)pGovObj->GetAbstainCount(VOTE_SIGNAL_FUNDING)));        
        objResult.push_back(Pair("FundingResult", objFundingResult));

        // -- VALIDITY VOTING RESULTS
        UniValue objValid(UniValue::VOBJ);
        objValid.push_back(Pair("AbsoluteYesCount",  (int64_t)pGovObj->GetYesCount(VOTE_SIGNAL_VALID)-(int64_t)pGovObj->GetNoCount(VOTE_SIGNAL_VALID)));
        objValid.push_back(Pair("YesCount",  (int64_t)pGovObj->GetYesCount(VOTE_SIGNAL_VALID)));
        objValid.push_back(Pair("NoCount",  (int64_t)pGovObj->GetNoCount(VOTE_SIGNAL_VALID)));
        objValid.push_back(Pair("AbstainCount",  (int64_t)pGovObj->GetAbstainCount(VOTE_SIGNAL_VALID)));        
        objResult.push_back(Pair("ValidResult", objValid));

        // -- DELETION CRITERION VOTING RESULTS
        UniValue objDelete(UniValue::VOBJ);
        objDelete.push_back(Pair("AbsoluteYesCount",  (int64_t)pGovObj->GetYesCount(VOTE_SIGNAL_DELETE)-(int64_t)pGovObj->GetNoCount(VOTE_SIGNAL_DELETE)));
        objDelete.push_back(Pair("YesCount",  (int64_t)pGovObj->GetYesCount(VOTE_SIGNAL_DELETE)));
        objDelete.push_back(Pair("NoCount",  (int64_t)pGovObj->GetNoCount(VOTE_SIGNAL_DELETE)));
        objDelete.push_back(Pair("AbstainCount",  (int64_t)pGovObj->GetAbstainCount(VOTE_SIGNAL_DELETE)));        
        objResult.push_back(Pair("DeleteResult", objDelete));

        // -- ENDORSED VIA MASTERNODE-ELECTED BOARD 
        UniValue objEndorsed(UniValue::VOBJ);
        objEndorsed.push_back(Pair("AbsoluteYesCount",  (int64_t)pGovObj->GetYesCount(VOTE_SIGNAL_ENDORSED)-(int64_t)pGovObj->GetNoCount(VOTE_SIGNAL_ENDORSED)));
        objEndorsed.push_back(Pair("YesCount",  (int64_t)pGovObj->GetYesCount(VOTE_SIGNAL_ENDORSED)));
        objEndorsed.push_back(Pair("NoCount",  (int64_t)pGovObj->GetNoCount(VOTE_SIGNAL_ENDORSED)));
        objEndorsed.push_back(Pair("AbstainCount",  (int64_t)pGovObj->GetAbstainCount(VOTE_SIGNAL_ENDORSED)));        
        objResult.push_back(Pair("EndorsedResult", objEndorsed));

        // -- 
        std::string strError = "";
        objResult.push_back(Pair("fLocalValidity",  pGovObj->IsValidLocally(chainActive.Tip(), strError, false)));
        objResult.push_back(Pair("fCachedValid",  pGovObj->fCachedValid));

        return objResult;
    }

    // GETVOTES FOR SPECIFIC GOVERNANCE OBJECT
    if(strCommand == "getvotes")
    {
        if (params.size() != 2)
            throw runtime_error(
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

        std::vector<CGovernanceVote*> vecVotes = governance.GetMatchingVotes(hash);
        BOOST_FOREACH(CGovernanceVote* pVote, vecVotes) {
            bResult.push_back(Pair(pVote->GetHash().ToString(),  pVote->ToString()));
        }

        return bResult;
    }

    return NullUniValue;
}

UniValue voteraw(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 6)
        throw runtime_error(
                "voteraw <masternode-tx-hash> <masternode-tx-index> <governance-hash> <vote-signal> [yes|no|abstain] <time> <vote-sig>\n"
                "Compile and relay a governance vote with provided external signature instead of signing vote internally\n"
                );

    uint256 hashMnTx = ParseHashV(params[0], "mn tx hash");
    int nMnTxIndex = params[1].get_int();
    CTxIn vin = CTxIn(hashMnTx, nMnTxIndex);

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
    vector<unsigned char> vchSig = DecodeBase64(strSig.c_str(), &fInvalid);

    if (fInvalid) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");
    }

    CMasternode* pmn = mnodeman.Find(vin);
    if(pmn == NULL)
    {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failure to find masternode in list : " + vin.ToString());
    }

    CGovernanceVote vote(vin, hashGovObj, eVoteSignal, eVoteOutcome);
    vote.SetTime(nTime);
    vote.SetSignature(vchSig);

    if(!vote.IsValid(true)){
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failure to verify vote.");
    }

    std::string strError = "";
    if(governance.AddOrUpdateVote(vote, NULL, strError)){
        governance.AddSeenVote(vote.GetHash(), SEEN_OBJECT_IS_VALID);
        vote.Relay();
        return "Voted successfully";
    } else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Error voting : " + strError);
    }
}

UniValue getgovernanceinfo(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0) {
        throw runtime_error(
            "getgovernanceinfo\n"
            "Returns an object containing governance parameters.\n"
            "\nResult:\n"
            "{\n"
            "  \"governanceminquorum\": xxxxx,  (numeric) the absolute minimum number of votes needed to trigger a governance action\n"
            "  \"proposalfee\": xxx.xx,         (numeric) the collateral transaction fee which must be paid to create a proposal in " + CURRENCY_UNIT + "\n"
            "  \"superblockcycle\": xxxxx,      (numeric) the number of blocks between superblocks\n"
            "  \"lastsuperblock\": xxxxx,       (numeric) the block number of the last superblock\n"
            "  \"nextsuperblock\": xxxxx,       (numeric) the block number of the next superblock\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getgovernanceinfo", "")
            + HelpExampleRpc("getgovernanceinfo", "")
            );
    }

    // Compute last/next superblock
    int nLastSuperblock, nNextSuperblock;

    // Get current block height
    int nBlockHeight = (int)chainActive.Height();

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
    obj.push_back(Pair("proposalfee", ValueFromAmount(GOVERNANCE_PROPOSAL_FEE_TX)));
    obj.push_back(Pair("superblockcycle", Params().GetConsensus().nSuperblockCycle));
    obj.push_back(Pair("lastsuperblock", nLastSuperblock));
    obj.push_back(Pair("nextsuperblock", nNextSuperblock));

    return obj;
}

UniValue getsuperblockbudget(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "getsuperblockbudget index\n"
            "\nReturns the absolute minimum number of votes needed to trigger a governance action.\n"
            "\nArguments:\n"
            "1. index         (numeric, required) The block index\n"
            "\nResult:\n"
            "n    (numeric) The current minimum governance quorum\n"
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
