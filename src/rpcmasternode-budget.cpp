// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "db.h"
#include "init.h"
#include "activemasternode.h"
#include "darksend.h"
#include "governance.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "rpcserver.h"
#include "utilmoneystr.h"
#include <boost/lexical_cast.hpp>

#include <fstream>
#include <univalue.h>
#include <iostream>
#include <sstream>

using namespace std;

int ConvertVoteOutcome(std::string strVoteOutcome)
{
    int nVote = -1;
    if(strVoteOutcome == "yes") nVote = VOTE_OUTCOME_YES;
    if(strVoteOutcome == "no") nVote = VOTE_OUTCOME_NO;
    if(strVoteOutcome == "abstain") nVote = VOTE_OUTCOME_ABSTAIN;
    if(strVoteOutcome == "none") nVote = VOTE_OUTCOME_NONE;
    return nVote;
}

int ConvertVoteSignal(std::string strVoteSignal)
{
    if(strVoteSignal == "none") return 0;
    if(strVoteSignal == "funding") return 1;
    if(strVoteSignal == "valid") return 2;
    if(strVoteSignal == "delete") return 3;
    if(strVoteSignal == "endorsed") return 4;

    // ID FIVE THROUGH CUSTOM_START ARE TO BE USED BY GOVERNANCE ENGINE / TRIGGER SYSTEM

    // convert custom sentinel outcomes to integer and store
    try {
        int  i = boost::lexical_cast<int>(strVoteSignal);
        if(i < VOTE_SIGNAL_CUSTOM_START || i > VOTE_SIGNAL_CUSTOM_END) return -1;
        return i;
    }
    catch(std::exception const & e)
    {
         cout<<"error : " << e.what() <<endl;
    }

    return -1;
}

UniValue mngovernance(const UniValue& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "vote-many" && strCommand != "vote-alias" && strCommand != "prepare" && strCommand != "submit" &&
         strCommand != "vote" && strCommand != "get" && strCommand != "list" && strCommand != "diff"))
        throw runtime_error(
                "mngovernance \"command\"...\n"
                "Manage proposals\n"
                "\nAvailable commands:\n"
                "  prepare            - Prepare proposal by signing and creating tx\n"
                "  submit             - Submit proposal to network\n"
                "  get                - Get proposal hash(es) by proposal name\n"
                "  list               - List all proposals\n"
                "  diff               - List differences since last diff\n"
                "  vote-alias         - Vote on a governance object by masternode alias\n"
                );


    /*
        12.1 todo - 


        Example Governance Item
            - This should be valid for anyone to submit
            - This should propagate and show up when syncing


        Command:
            mngovernance submit 6e622bb41bad1fb18e7f23ae96770aeb33129e18bd9efe790522488e580a0a03 0 1 1464292854 "beer-reimbursement" 5b5b22636f6e7472616374222c207b2270726f6a6563745f6e616d65223a20225c22626565722d7265696d62757273656d656e745c22222c20227061796d656e745f61646472657373223a20225c225879324c4b4a4a64655178657948726e34744744514238626a6876464564615576375c22222c2022656e645f64617465223a202231343936333030343030222c20226465736372697074696f6e5f75726c223a20225c227777772e646173687768616c652e6f72672f702f626565722d7265696d62757273656d656e745c22222c2022636f6e74726163745f75726c223a20225c22626565722d7265696d62757273656d656e742e636f6d2f3030312e7064665c22222c20227061796d656e745f616d6f756e74223a20223233342e323334323232222c2022676f7665726e616e63655f6f626a6563745f6964223a2037342c202273746172745f64617465223a202231343833323534303030227d5d5d1
    */

    if(strCommand == "prepare")
    {
        if (params.size() != 6)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'mngovernance prepare <parent-hash> <revision> <time> <name> <registers-hex>'");

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
        std::string strRegisters = params[5].get_str();
        
        // CREATE A NEW COLLATERAL TRANSACTION FOR THIS SPECIFIC OBJECT

        CGovernanceObject govobj(hashParent, nRevision, strName, nTime, uint256());

        std::string strError = "";
        if(!govobj.IsValid(pindex, strError, false))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Governance object is not valid - " + govobj.GetHash().ToString() + " - " + strError);

        CWalletTx wtx;
        if(!pwalletMain->GetBudgetSystemCollateralTX(wtx, govobj.GetHash(), false)){
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Error making collateral transaction for proposal. Please check your wallet balance and make sure your wallet is unlocked.");
        }

        // -- make our change address
        CReserveKey reservekey(pwalletMain);
        // -- send the tx to the network
        pwalletMain->CommitTransaction(wtx, reservekey, NetMsgType::TX);

        return wtx.GetHash().ToString();
    }

    if(strCommand == "submit")
    {
        if (params.size() != 7)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'mngovernance submit <fee-tx> <parent-hash> <revision> <time> <name> <registers-hex>'");

        if(!masternodeSync.IsBlockchainSynced()) {
            throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Must wait for client to sync with masternode network. Try again in a minute or so.");
        }

        // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

        LOCK(cs_main);
        CBlockIndex* pindex = chainActive.Tip();

        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        uint256 fee_tx = ParseHashV(params[1], "fee-tx hash, parameter 1");
        uint256 hashParent;
        if(params[2].get_str() == "0") { // attach to root node (root node doesn't really exist, but has a hash of zero)
            hashParent = uint256();
        } else {
            hashParent = ParseHashV(params[2], "parent object hash, parameter 2");
        }

        std::string strRevision = params[3].get_str();
        std::string strTime = params[4].get_str();
        int nRevision = boost::lexical_cast<int>(strRevision);
        int nTime = boost::lexical_cast<int>(strTime);
        std::string strName = SanitizeString(params[5].get_str());
        std::string strRegisters = params[6].get_str();

        // CREATE A NEW COLLATERAL TRANSACTION FOR THIS SPECIFIC OBJECT

        CGovernanceObject govobj(hashParent, nRevision, strName, nTime, fee_tx);

        std::string strError = "";
        if(!govobj.IsValid(pindex, strError)){
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Governance object is not valid - " + govobj.GetHash().ToString() + " - " + strError);
        }

        governance.mapSeenGovernanceObjects.insert(make_pair(govobj.GetHash(), SEEN_OBJECT_IS_VALID));
        govobj.Relay();
        governance.AddGovernanceObject(govobj);

        return govobj.GetHash().ToString();

    }

    if(strCommand == "vote-alias")
    {
        if(params.size() != 5)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'mngovernance vote-alias <governance-hash> [funding|valid|delete] [yes|no|abstain] <alias-name>'");

        uint256 hash;
        std::string strVote;

        hash = ParseHashV(params[1], "Object hash");
        std::string strVoteAction = params[2].get_str();
        std::string strVoteOutcome = params[3].get_str();
        std::string strAlias = params[4].get_str();
        
        int nVoteSignal = ConvertVoteSignal(strVoteAction);
        if(nVoteSignal == VOTE_SIGNAL_NONE || nVoteSignal == -1)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote signal. Please use one of the following: 'yes', 'no' or 'abstain'");

        int nVoteOutcome = ConvertVoteOutcome(strVoteOutcome);
        if(nVoteOutcome == VOTE_OUTCOME_NONE || nVoteOutcome == -1)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote outcome. Please using one of the following: (funding|valid|delete|clear_registers|endorsed|release_bounty1|release_bounty2|release_bounty3) OR `custom sentinel code` "); 

        int success = 0;
        int failed = 0;

        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        UniValue resultsObj(UniValue::VOBJ);

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {

            if( strAlias != mne.getAlias()) continue;

            std::string errorMessage;
            std::vector<unsigned char> vchMasterNodeSignature;
            std::string strMasterNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            UniValue statusObj(UniValue::VOBJ);

            if(!darkSendSigner.GetKeysFromSecret(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)){
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

            CGovernanceVote vote(pmn->vin, hash, nVoteOutcome, nVoteSignal);
            if(!vote.Sign(keyMasternode, pubKeyMasternode)){
                failed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Failure to sign."));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }

            std::string strError = "";
            if(governance.UpdateGovernanceObject(vote, NULL, strError)) {
                governance.mapSeenVotes.insert(make_pair(vote.GetHash(), SEEN_OBJECT_IS_VALID));
                vote.Relay();
                success++;
                statusObj.push_back(Pair("result", "success"));
            } else {
                failed++;
                statusObj.push_back(Pair("result", strError.c_str()));
            }

            resultsObj.push_back(Pair(mne.getAlias(), statusObj));
        }

        UniValue returnObj(UniValue::VOBJ);
        returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed)));
        returnObj.push_back(Pair("detail", resultsObj));

        return returnObj;
    }

    if(strCommand == "list" || strCommand == "diff")
    {
        if (params.size() > 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'mngovernance list [valid]'");

        std::string strShow = "valid";
        if (params.size() == 2) strShow = params[1].get_str();

        UniValue resultObj(UniValue::VOBJ);

        CBlockIndex* pindex;
        {
            LOCK(cs_main);
            pindex = chainActive.Tip();
        }

        int nStartTime = 0; //list
        if(strCommand == "diff") nStartTime = governance.GetLastDiffTime();

        std::vector<CGovernanceObject*> winningProps = governance.GetAllProposals(nStartTime);
        governance.UpdateLastDiffTime(GetTime());

        BOOST_FOREACH(CGovernanceObject* pbudgetProposal, winningProps)
        {
            if(strShow == "valid" && !pbudgetProposal->fCachedValid) continue;

            UniValue bObj(UniValue::VOBJ);
            bObj.push_back(Pair("Name",  pbudgetProposal->GetName()));
            bObj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString()));
            bObj.push_back(Pair("FeeTXHash",  pbudgetProposal->nFeeTXHash.ToString()));

            // vote data for funding
            bObj.push_back(Pair("AbsoluteYesCount",  (int64_t)pbudgetProposal->GetYesCount(VOTE_SIGNAL_FUNDING)-(int64_t)pbudgetProposal->GetNoCount(VOTE_SIGNAL_FUNDING)));
            bObj.push_back(Pair("YesCount",  (int64_t)pbudgetProposal->GetYesCount(VOTE_SIGNAL_FUNDING)));
            bObj.push_back(Pair("NoCount",  (int64_t)pbudgetProposal->GetNoCount(VOTE_SIGNAL_FUNDING)));
            bObj.push_back(Pair("AbstainCount",  (int64_t)pbudgetProposal->GetAbstainCount(VOTE_SIGNAL_FUNDING)));
            //bObj.push_back(Pair("IsEstablished",  pbudgetProposal->IsEstablished(VOTE_SIGNAL_FUNDING)));

            std::string strError = "";
            bObj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(pindex, strError)));
            bObj.push_back(Pair("IsValidReason",  strError.c_str()));
            bObj.push_back(Pair("fCachedValid",  pbudgetProposal->fCachedValid));
            bObj.push_back(Pair("fCachedFunding",  pbudgetProposal->fCachedFunding));
            bObj.push_back(Pair("fCachedDelete",  pbudgetProposal->fCachedDelete));
            bObj.push_back(Pair("fCachedEndorsed",  pbudgetProposal->fCachedEndorsed));

            resultObj.push_back(Pair(pbudgetProposal->GetName(), bObj));
        }

        return resultObj;
    }

    if(strCommand == "getproposal")
    {
        if (params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'mngovernance getproposal <proposal-hash>'");

        uint256 hash = ParseHashV(params[1], "Proposal hash");

        CGovernanceObject* pbudgetProposal = governance.FindGovernanceObject(hash);

        if(pbudgetProposal == NULL)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown proposal");

        CBlockIndex* pindex;
        {
            LOCK(cs_main);
            pindex = chainActive.Tip();
        }

        LOCK(cs_main);
        UniValue obj(UniValue::VOBJ);
        obj.push_back(Pair("Name",  pbudgetProposal->GetName()));
        obj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString()));
        obj.push_back(Pair("FeeTXHash",  pbudgetProposal->nFeeTXHash.ToString()));
        obj.push_back(Pair("AbsoluteYesCount",  (int64_t)pbudgetProposal->GetYesCount(VOTE_SIGNAL_FUNDING)-(int64_t)pbudgetProposal->GetNoCount(VOTE_SIGNAL_FUNDING)));
        obj.push_back(Pair("YesCount",  (int64_t)pbudgetProposal->GetYesCount(VOTE_SIGNAL_FUNDING)));
        obj.push_back(Pair("NoCount",  (int64_t)pbudgetProposal->GetNoCount(VOTE_SIGNAL_FUNDING)));
        obj.push_back(Pair("AbstainCount",  (int64_t)pbudgetProposal->GetAbstainCount(VOTE_SIGNAL_FUNDING)));

        std::string strError = "";
        obj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(chainActive.Tip(), strError)));
        obj.push_back(Pair("fValid",  pbudgetProposal->fCachedValid));

        return obj;
    }

    if(strCommand == "getvotes")
    {
        if (params.size() != 3)
            throw runtime_error(
                "Correct usage is 'mngovernance getvotes <governance-hash> <vote-outcome>'"
                );

        uint256 hash = ParseHashV(params[1], "Governance hash");
        std::string strVoteSignal = params[2].get_str();
        int nVoteSignal = ConvertVoteSignal(strVoteSignal);
        if(nVoteSignal == -1)
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote signal. Please using one of the following: (funding|valid|delete|clear_registers|endorsed|release_bounty1|release_bounty2|release_bounty3) OR `custom sentinel code` "); 
        }

        CGovernanceObject* pbudgetProposal = governance.FindGovernanceObject(hash);

        if(pbudgetProposal == NULL)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown governance-hash");

        UniValue bObj(UniValue::VOBJ);
        bObj.push_back(Pair("AbsoluteYesCount",  (int64_t)pbudgetProposal->GetYesCount(nVoteSignal)-(int64_t)pbudgetProposal->GetNoCount(nVoteSignal)));
        bObj.push_back(Pair("YesCount",  (int64_t)pbudgetProposal->GetYesCount(nVoteSignal)));
        bObj.push_back(Pair("NoCount",  (int64_t)pbudgetProposal->GetNoCount(nVoteSignal)));
        bObj.push_back(Pair("AbstainCount",  (int64_t)pbudgetProposal->GetAbstainCount(nVoteSignal)));

        return bObj;
    }

    return NullUniValue;
}

UniValue voteraw(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 6)
        throw runtime_error(
                "voteraw <masternode-tx-hash> <masternode-tx-index> <governance-hash> <vote-outcome> [yes|no|abstain] <time> <vote-sig>\n"
                "Compile and relay a governance vote with provided external signature instead of signing vote internally\n"
                );

    uint256 hashMnTx = ParseHashV(params[0], "mn tx hash");
    int nMnTxIndex = params[1].get_int();
    CTxIn vin = CTxIn(hashMnTx, nMnTxIndex);

    uint256 hashProposal = ParseHashV(params[2], "Governance hash");
    std::string strVoteOutcome = params[3].get_str();
    std::string strVoteSignal = params[4].get_str();

    int nVoteSignal = ConvertVoteSignal(strVoteSignal);
    if(nVoteSignal == VOTE_OUTCOME_NONE || nVoteSignal == -1)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote signal. Please use one of the following: 'yes', 'no' or 'abstain'");

    int nVoteOutcome = ConvertVoteOutcome(strVoteOutcome);
    if(nVoteOutcome == VOTE_OUTCOME_NONE || nVoteOutcome == -1)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote action. Please using one of the following: (funding|valid|delete|clear_registers|endorsed|release_bounty1|release_bounty2|release_bounty3) OR `custom sentinel code` "); 


    int64_t nTime = params[4].get_int64();
    std::string strSig = params[5].get_str();
    bool fInvalid = false;
    vector<unsigned char> vchSig = DecodeBase64(strSig.c_str(), &fInvalid);

    if (fInvalid)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    CMasternode* pmn = mnodeman.Find(vin);
    if(pmn == NULL)
    {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failure to find masternode in list : " + vin.ToString());
    }

    CGovernanceVote vote(vin, hashProposal, nVoteOutcome, VOTE_SIGNAL_NONE);
    vote.nTime = nTime;
    vote.vchSig = vchSig;

    if(!vote.IsValid(true)){
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failure to verify vote.");
    }

    std::string strError = "";
    if(governance.UpdateGovernanceObject(vote, NULL, strError)){
        governance.mapSeenVotes.insert(make_pair(vote.GetHash(), SEEN_OBJECT_IS_VALID));
        vote.Relay();
        return "Voted successfully";
    } else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Error voting : " + strError);
    }
}
