// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "db.h"
#include "init.h"
#include "activemasternode.h"
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

UniValue mnbudget(const UniValue& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "vote-many" && strCommand != "vote-alias" && strCommand != "prepare" && strCommand != "submit" &&
         strCommand != "vote" && strCommand != "getvotes" && strCommand != "getproposal" && strCommand != "getproposalhash" &&
         strCommand != "list" && strCommand != "projection" && strCommand != "check" && strCommand != "nextblock" && strCommand != "nextsuperblocksize"))
        throw runtime_error(
                "mnbudget \"command\"...\n"
                "Manage proposals\n"
                "\nAvailable commands:\n"
                "  check              - Scan proposals and remove invalid from proposals list\n"
                "  prepare            - Prepare proposal by signing and creating tx\n"
                "  submit             - Submit proposal to network\n"
                "  getproposalhash    - Get proposal hash(es) by proposal name\n"
                "  getproposal        - Show proposal\n"
                "  getvotes           - Show detailed votes list for proposal\n"
                "  list               - List all proposals\n"
                "  nextblock          - Get info about next superblock for budget system\n"
                "  nextsuperblocksize - Get superblock size for a given blockheight\n"
                "  projection         - Show the projection of which proposals will be paid the next cycle\n"
                "  vote               - Vote on a proposal by single masternode (using dash.conf setup)\n"
                "  vote-many          - Vote on a proposal by all masternodes (using masternode.conf setup)\n"
                "  vote-alias         - Vote on a proposal by alias\n"
                );

    if(strCommand == "nextblock")
    {
        LOCK(cs_main);
        CBlockIndex* pindex = chainActive.Tip();
        if(!pindex) return "unknown";

        int nNext = pindex->nHeight - pindex->nHeight % Params().GetConsensus().nBudgetPaymentsCycleBlocks + Params().GetConsensus().nBudgetPaymentsCycleBlocks;
        return nNext;
    }

    if(strCommand == "prepare")
    {
        if (params.size() != 7)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'mnbudget prepare <proposal-name> <url> <payment-count> <block-start> <dash-address> <monthly-payment-dash>'");

        int nBlockMin = 0;
        LOCK(cs_main);
        CBlockIndex* pindex = chainActive.Tip();

        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        std::string strName = SanitizeString(params[1].get_str());
        std::string strURL = SanitizeString(params[2].get_str());
        int nPaymentCount = params[3].get_int();
        int nBlockStart = params[4].get_int();

        //set block min
        if(pindex != NULL) nBlockMin = pindex->nHeight;

        if(nBlockStart < nBlockMin)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid block start, must be more than current height.");

        CBitcoinAddress address(params[5].get_str());
        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Dash address");

        // Parse Dash address
        CScript scriptPubKey = GetScriptForDestination(address.Get());
        CAmount nAmount = AmountFromValue(params[6]);

        //*************************************************************************

        // create transaction 15 minutes into the future, to allow for confirmation time
        CBudgetProposal budgetProposalBroadcast(strName, strURL, nPaymentCount, scriptPubKey, nAmount, GetTime(), 253370764800, uint256());

        std::string strError = "";
        if(!budgetProposalBroadcast.IsValid(pindex, strError, false))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Proposal is not valid - " + budgetProposalBroadcast.GetHash().ToString() + " - " + strError);

        bool useIX = false; //true;
        // if (params.size() > 7) {
        //     if(params[7].get_str() != "false" && params[7].get_str() != "true")
        //         return "Invalid use_ix, must be true or false";
        //     useIX = params[7].get_str() == "true" ? true : false;
        // }

        CWalletTx wtx;
        if(!pwalletMain->GetBudgetSystemCollateralTX(wtx, budgetProposalBroadcast.GetHash(), useIX)){
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Error making collateral transaction for proposal. Please check your wallet balance and make sure your wallet is unlocked.");
        }

        // make our change address
        CReserveKey reservekey(pwalletMain);
        //send the tx to the network
        pwalletMain->CommitTransaction(wtx, reservekey, useIX ? NetMsgType::IX : NetMsgType::TX);

        return wtx.GetHash().ToString();
    }

    if(strCommand == "submit")
    {
        if (params.size() != 8)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'mnbudget submit <proposal-name> <url> <payment-count> <block-start> <dash-address> <monthly-payment-dash> <fee-tx>'");

        if(!masternodeSync.IsBlockchainSynced()){
            throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Must wait for client to sync with masternode network. Try again in a minute or so.");
        }

        int nBlockMin = 0;
        LOCK(cs_main);
        CBlockIndex* pindex = chainActive.Tip();

        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        std::string strName = SanitizeString(params[1].get_str());
        std::string strURL = SanitizeString(params[2].get_str());
        int nPaymentCount = params[3].get_int();
        int nBlockStart = params[4].get_int();

        //set block min
        if(pindex != NULL) nBlockMin = pindex->nHeight;

        if(nBlockStart < nBlockMin)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid payment count, must be more than current height.");

        CBitcoinAddress address(params[5].get_str());
        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Dash address");

        // Parse Dash address
        CScript scriptPubKey = GetScriptForDestination(address.Get());
        CAmount nAmount = AmountFromValue(params[6]);
        uint256 hash = ParseHashV(params[7], "Proposal hash");

        //create the proposal incase we're the first to make it
        CBudgetProposal budgetProposalBroadcast(strName, strURL, nPaymentCount, scriptPubKey, nAmount, GetTime(), 253370764800, hash);

        std::string strError = "";

        if(!budgetProposalBroadcast.IsValid(pindex, strError)){
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Proposal is not valid - " + budgetProposalBroadcast.GetHash().ToString() + " - " + strError);
        }

        int nConf = 0;
        if(!IsCollateralValid(hash, budgetProposalBroadcast.GetHash(), strError, budgetProposalBroadcast.nTime, nConf, GOVERNANCE_FEE_TX)){
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Proposal FeeTX is not valid - " + hash.ToString() + " - " + strError);
        }

        governance.mapSeenMasternodeBudgetProposals.insert(make_pair(budgetProposalBroadcast.GetHash(), budgetProposalBroadcast));
        budgetProposalBroadcast.Relay();
        governance.AddProposal(budgetProposalBroadcast);

        return budgetProposalBroadcast.GetHash().ToString();

    }

    if(strCommand == "vote-many")
    {
        if(params.size() != 3)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'mnbudget vote-many <proposal-hash> [yes|no]'");

        uint256 hash;
        std::string strVote;

        hash = ParseHashV(params[1], "Proposal hash");
        strVote = params[2].get_str();

        int nVote = VOTE_OUTCOME_NONE;
        if(strVote == "yes") nVote = VOTE_OUTCOME_YES;
        if(strVote == "no") nVote = VOTE_OUTCOME_NO;
        if(strVote == "abstain") nVote = VOTE_OUTCOME_ABSTAIN;
        if(nVote == VOTE_OUTCOME_NONE)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "You can only vote 'yes', 'no' or 'abstain'");

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

            if(!darkSendSigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)){
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

            CBudgetVote vote(pmn->vin, hash, nVote, VOTE_TYPE_NONE);
            if(!vote.Sign(keyMasternode, pubKeyMasternode)){
                failed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Failure to sign."));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }


            std::string strError = "";
            if(governance.UpdateProposal(vote, NULL, strError)) {
                governance.mapSeenMasternodeBudgetVotes.insert(make_pair(vote.GetHash(), vote));
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

    if(strCommand == "vote-alias")
    {
        if(params.size() != 4)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'mnbudget vote-alias <proposal-hash> [yes|no] <alias-name>'");

        uint256 hash;
        std::string strVote;

        hash = ParseHashV(params[1], "Proposal hash");
        strVote = params[2].get_str();
        std::string strAlias = params[3].get_str();

        int nVote = VOTE_OUTCOME_NONE;
        if(strVote == "yes") nVote = VOTE_OUTCOME_YES;
        if(strVote == "no") nVote = VOTE_OUTCOME_NO;
        if(strVote == "abstain") nVote = VOTE_OUTCOME_ABSTAIN;
        if(nVote == VOTE_OUTCOME_NONE)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "You can only vote 'yes', 'no' or 'abstain'");

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

            if(!darkSendSigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)){
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

            CBudgetVote vote(pmn->vin, hash, nVote, VOTE_TYPE_NONE);
            if(!vote.Sign(keyMasternode, pubKeyMasternode)){
                failed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Failure to sign."));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }


            std::string strError = "";
            if(governance.UpdateProposal(vote, NULL, strError)) {
                governance.mapSeenMasternodeBudgetVotes.insert(make_pair(vote.GetHash(), vote));
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

    if(strCommand == "vote")
    {
        if (params.size() != 3)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'mnbudget vote <proposal-hash> [yes|no]'");

        uint256 hash = ParseHashV(params[1], "Proposal hash");
        std::string strVote = params[2].get_str();

        int nVote = VOTE_OUTCOME_NONE;
        if(strVote == "yes") nVote = VOTE_OUTCOME_YES;
        if(strVote == "no") nVote = VOTE_OUTCOME_NO;
        if(strVote == "abstain") nVote = VOTE_OUTCOME_ABSTAIN;
        if(nVote == VOTE_OUTCOME_NONE)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "You can only vote 'yes', 'no' or 'abstain'");

        CPubKey pubKeyMasternode;
        CKey keyMasternode;
        std::string errorMessage;

        if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Error upon calling SetKey");

        CMasternode* pmn = mnodeman.Find(activeMasternode.vin);
        if(pmn == NULL)
        {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Failure to find masternode in list : " + activeMasternode.vin.ToString());
        }

        CBudgetVote vote(activeMasternode.vin, hash, nVote, VOTE_TYPE_NONE);
        if(!vote.Sign(keyMasternode, pubKeyMasternode)){
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Failure to sign.");
        }

        std::string strError = "";
        if(governance.UpdateProposal(vote, NULL, strError)){
            governance.mapSeenMasternodeBudgetVotes.insert(make_pair(vote.GetHash(), vote));
            vote.Relay();
            return "Voted successfully";
        } else {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Error voting : " + strError);
        }
    }

    if(strCommand == "list")
    {
        if (params.size() > 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'mnbudget list [valid]'");

        std::string strShow = "valid";
        if (params.size() == 2) strShow = params[1].get_str();

        UniValue resultObj(UniValue::VOBJ);

        CBlockIndex* pindex;
        {
            LOCK(cs_main);
            pindex = chainActive.Tip();
        }

        std::vector<CBudgetProposal*> winningProps = governance.GetAllProposals();
        BOOST_FOREACH(CBudgetProposal* pbudgetProposal, winningProps)
        {
            if(strShow == "valid" && !pbudgetProposal->fValid) continue;

            CTxDestination address1;
            ExtractDestination(pbudgetProposal->GetPayee(), address1);
            CBitcoinAddress address2(address1);

            UniValue bObj(UniValue::VOBJ);
            bObj.push_back(Pair("Name",  pbudgetProposal->GetName()));
            bObj.push_back(Pair("URL",  pbudgetProposal->GetURL()));
            bObj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString()));
            bObj.push_back(Pair("FeeTXHash",  pbudgetProposal->nFeeTXHash.ToString()));
            bObj.push_back(Pair("StartTime",  (int64_t)pbudgetProposal->GetStartTime()));
            bObj.push_back(Pair("EndTime",    (int64_t)pbudgetProposal->GetEndTime()));
            bObj.push_back(Pair("AbsoluteYesCount",  (int64_t)pbudgetProposal->GetYesCount()-(int64_t)pbudgetProposal->GetNoCount()));
            bObj.push_back(Pair("YesCount",  (int64_t)pbudgetProposal->GetYesCount()));
            bObj.push_back(Pair("NoCount",  (int64_t)pbudgetProposal->GetNoCount()));
            bObj.push_back(Pair("AbstainCount",  (int64_t)pbudgetProposal->GetAbstainCount()));
            bObj.push_back(Pair("PaymentAddress",   address2.ToString()));
            bObj.push_back(Pair("MonthlyPayment",  ValueFromAmount(pbudgetProposal->GetAmount())));
            bObj.push_back(Pair("IsEstablished",  pbudgetProposal->IsEstablished()));

            std::string strError = "";
            bObj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(pindex, strError)));
            bObj.push_back(Pair("IsValidReason",  strError.c_str()));
            bObj.push_back(Pair("fValid",  pbudgetProposal->fValid));

            resultObj.push_back(Pair(pbudgetProposal->GetName(), bObj));
        }

        return resultObj;
    }

    if(strCommand == "getproposalhash")
    {
        if (params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'mnbudget getproposalhash <proposal-name>'");

        std::string strName = SanitizeString(params[1].get_str());

        CBudgetProposal* pbudgetProposal = governance.FindProposal(strName);

        if(pbudgetProposal == NULL)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown proposal");

        UniValue resultObj(UniValue::VOBJ);

        std::vector<CBudgetProposal*> winningProps = governance.GetAllProposals();
        BOOST_FOREACH(CBudgetProposal* pbudgetProposal, winningProps)
        {
            if(pbudgetProposal->GetName() != strName) continue;
            if(!pbudgetProposal->fValid) continue;

            CTxDestination address1;
            ExtractDestination(pbudgetProposal->GetPayee(), address1);
            CBitcoinAddress address2(address1);

            resultObj.push_back(Pair(pbudgetProposal->GetHash().ToString(), pbudgetProposal->GetHash().ToString()));
        }

        if(resultObj.size() > 1) return resultObj;

        return pbudgetProposal->GetHash().ToString();
    }

    if(strCommand == "getproposal")
    {
        if (params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'mnbudget getproposal <proposal-hash>'");

        uint256 hash = ParseHashV(params[1], "Proposal hash");

        CBudgetProposal* pbudgetProposal = governance.FindProposal(hash);

        if(pbudgetProposal == NULL)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown proposal");

        CBlockIndex* pindex;
        {
            LOCK(cs_main);
            pindex = chainActive.Tip();
        }

        CTxDestination address1;
        ExtractDestination(pbudgetProposal->GetPayee(), address1);
        CBitcoinAddress address2(address1);

        LOCK(cs_main);
        UniValue obj(UniValue::VOBJ);
        obj.push_back(Pair("Name",  pbudgetProposal->GetName()));
        obj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString()));
        obj.push_back(Pair("FeeTXHash",  pbudgetProposal->nFeeTXHash.ToString()));
        obj.push_back(Pair("URL",  pbudgetProposal->GetURL()));
        obj.push_back(Pair("StartTime",  (int64_t)pbudgetProposal->GetStartTime()));
        obj.push_back(Pair("EndTime",    (int64_t)pbudgetProposal->GetEndTime()));
        obj.push_back(Pair("PaymentAddress",   address2.ToString()));
        obj.push_back(Pair("AbsoluteYesCount",  (int64_t)pbudgetProposal->GetYesCount()-(int64_t)pbudgetProposal->GetNoCount()));
        obj.push_back(Pair("YesCount",  (int64_t)pbudgetProposal->GetYesCount()));
        obj.push_back(Pair("NoCount",  (int64_t)pbudgetProposal->GetNoCount()));
        obj.push_back(Pair("AbstainCount",  (int64_t)pbudgetProposal->GetAbstainCount()));
        obj.push_back(Pair("MonthlyPayment",  ValueFromAmount(pbudgetProposal->GetAmount())));
        
        obj.push_back(Pair("IsEstablished",  pbudgetProposal->IsEstablished()));

        std::string strError = "";
        obj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(chainActive.Tip(), strError)));
        obj.push_back(Pair("fValid",  pbudgetProposal->fValid));

        return obj;
    }

    if(strCommand == "getvotes")
    {
        if (params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'mnbudget getvotes <proposal-hash>'");

        uint256 hash = ParseHashV(params[1], "Proposal hash");

        UniValue obj(UniValue::VOBJ);

        CBudgetProposal* pbudgetProposal = governance.FindProposal(hash);

        if(pbudgetProposal == NULL)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown proposal");

        // 12.1 - rewrite
        // std::map<uint256, CBudgetVote>::iterator it = pbudgetProposal->mapVotes.begin();
        // while(it != pbudgetProposal->mapVotes.end()){

        //     UniValue bObj(UniValue::VOBJ);
        //     bObj.push_back(Pair("nHash",  (*it).first.ToString().c_str()));
        //     bObj.push_back(Pair("Vote",  (*it).second.GetVoteString()));
        //     bObj.push_back(Pair("nTime",  (int64_t)(*it).second.nTime));
        //     bObj.push_back(Pair("fValid",  (*it).second.fValid));

        //     obj.push_back(Pair((*it).second.vin.prevout.ToStringShort(), bObj));

        //     it++;
        // }


        return obj;
    }

    return NullUniValue;
}

UniValue mnbudgetvoteraw(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 6)
        throw runtime_error(
                "mnbudgetvoteraw <masternode-tx-hash> <masternode-tx-index> <proposal-hash> [yes|no] <time> <vote-sig>\n"
                "Compile and relay a proposal vote with provided external signature instead of signing vote internally\n"
                );

    uint256 hashMnTx = ParseHashV(params[0], "mn tx hash");
    int nMnTxIndex = params[1].get_int();
    CTxIn vin = CTxIn(hashMnTx, nMnTxIndex);

    uint256 hashProposal = ParseHashV(params[2], "Proposal hash");
    std::string strVote = params[3].get_str();

    int nVote = VOTE_OUTCOME_NONE;
    if(strVote == "yes") nVote = VOTE_OUTCOME_YES;
    if(strVote == "no") nVote = VOTE_OUTCOME_NO;
    if(strVote == "abstain") nVote = VOTE_OUTCOME_ABSTAIN;
    if(nVote == VOTE_OUTCOME_NONE)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "You can only vote 'yes', 'no' or 'abstain'");


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

    CBudgetVote vote(vin, hashProposal, nVote, VOTE_TYPE_NONE);
    vote.nTime = nTime;
    vote.vchSig = vchSig;

    if(!vote.IsValid(true)){
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failure to verify vote.");
    }

    std::string strError = "";
    if(governance.UpdateProposal(vote, NULL, strError)){
        governance.mapSeenMasternodeBudgetVotes.insert(make_pair(vote.GetHash(), vote));
        vote.Relay();
        return "Voted successfully";
    } else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Error voting : " + strError);
    }
}