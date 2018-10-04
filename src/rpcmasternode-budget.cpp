// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "db.h"
#include "init.h"
#include "activemasternode.h"
#include "masternodeman.h"
#include "masternode-payments.h"
#include "masternode-budget.h"
#include "masternodeconfig.h"
#include "rpcserver.h"
#include "utilmoneystr.h"

#include <fstream>
using namespace json_spirit;
using namespace std;

Value mnbudget(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "vote-many" && strCommand != "prepare" && strCommand != "submit" && strCommand != "vote" && strCommand != "getvotes" && strCommand != "getinfo" && strCommand != "show" && strCommand != "projection" && strCommand != "check" && strCommand != "nextblock"))
        throw runtime_error(
                "mnbudget \"command\"... ( \"passphrase\" )\n"
                "Vote or show current budgets\n"
                "\nAvailable commands:\n"
                "  prepare            - Prepare proposal for network by signing and creating tx\n"
                "  submit             - Submit proposal for network\n"
                "  vote-many          - Vote on a Crown initiative\n"
                "  vote-alias         - Vote on a Crown initiative\n"
                "  vote               - Vote on a Crown initiative/budget\n"
                "  getvotes           - Show current masternode budgets\n"
                "  getinfo            - Show current masternode budgets\n"
                "  show               - Show all budgets\n"
                "  projection         - Show the projection of which proposals will be paid the next cycle\n"
                "  check              - Scan proposals and remove invalid\n"
                "  nextblock          - Get next superblock for budget system\n"
                );

    if(strCommand == "nextblock")
    {
        CBlockIndex* pindexPrev = chainActive.Tip();
        if(!pindexPrev) return "unknown";

        return GetNextSuperblock(pindexPrev->nHeight);
    }

    if(strCommand == "prepare")
    {
        int nBlockMin = 0;
        CBlockIndex* pindexPrev = chainActive.Tip();

        std::vector<CNodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        if (params.size() != 7)
            throw runtime_error("Correct usage is 'mnbudget prepare proposal-name url payment_count block_start crown_address monthly_payment_crown'");

        std::string strProposalName = params[1].get_str();
        if(strProposalName.size() > 20)
            return "Invalid proposal name, limit of 20 characters.";

        std::string strURL = params[2].get_str();
        if(strURL.size() > 64)
            return "Invalid url, limit of 64 characters.";

        int nPaymentCount = params[3].get_int();
        if(nPaymentCount < 1)
            return "Invalid payment count, must be more than zero.";

        //set block min
        if(pindexPrev != NULL) nBlockMin = pindexPrev->nHeight - GetBudgetPaymentCycleBlocks() * (nPaymentCount + 1);

        int nBlockStart = params[4].get_int();
        if(nBlockStart % GetBudgetPaymentCycleBlocks() != 0){
            const int nextBlock = GetNextSuperblock(pindexPrev->nHeight);;
            return strprintf("Invalid block start - must be a budget cycle block. Next valid block: %d", nextBlock);
        }

        int nBlockEnd = nBlockStart + GetBudgetPaymentCycleBlocks() * nPaymentCount;

        if(nBlockStart < nBlockMin)
            return "Invalid block start, must be more than current height.";

        if(nBlockEnd < pindexPrev->nHeight)
            return "Invalid ending block, starting block + (payment_cycle*payments) must be more than current height.";

        if (pindexPrev)
        {
            const int64_t submissionBlock = pindexPrev->nHeight + BUDGET_FEE_CONFIRMATIONS + 1;
            const int64_t nextBlock = GetNextSuperblock(pindexPrev->nHeight);
            if (submissionBlock >= nextBlock - GetVotingThreshold())
                return "Sorry, your proposal can not be added as we're too close to the proposal payment. Please submit your proposal for the next proposal payment.";
        }

        CBitcoinAddress address(params[5].get_str());
        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Crown address");

        // Parse Crown address
        CScript scriptPubKey = GetScriptForDestination(address.Get());
        CAmount nAmount = AmountFromValue(params[6]);

        //*************************************************************************

        // create transaction 15 minutes into the future, to allow for confirmation time
        CBudgetProposalBroadcast budgetProposalBroadcast(strProposalName, strURL, nPaymentCount, scriptPubKey, nAmount, nBlockStart, uint256());

        std::string strError = "";
        if(!budgetProposalBroadcast.IsValid(strError, false))
            return "Proposal is not valid - " + budgetProposalBroadcast.GetHash().ToString() + " - " + strError;

        CWalletTx wtx;
        if(!pwalletMain->GetBudgetSystemCollateralTX(wtx, budgetProposalBroadcast.GetHash())){
            return "Error making collateral transaction for proposal. Please check your wallet balance and make sure your wallet is unlocked.";
        }

        // make our change address
        CReserveKey reservekey(pwalletMain);
        //send the tx to the network
        pwalletMain->CommitTransaction(wtx, reservekey);

        return wtx.GetHash().ToString();
    }

    if(strCommand == "submit")
    {
        int nBlockMin = 0;
        CBlockIndex* pindexPrev = chainActive.Tip();

        std::vector<CNodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        if (params.size() != 8)
            throw runtime_error("Correct usage is 'mnbudget submit proposal-name url payment_count block_start crown_address monthly_payment_crown fee_tx'");

        // Check these inputs the same way we check the vote commands:
        // **********************************************************

        std::string strProposalName = params[1].get_str();
        if(strProposalName.size() > 20)
            return "Invalid proposal name, limit of 20 characters.";

        std::string strURL = params[2].get_str();
        if(strURL.size() > 64)
            return "Invalid url, limit of 64 characters.";

        int nPaymentCount = params[3].get_int();
        if(nPaymentCount < 1)
            return "Invalid payment count, must be more than zero.";

        //set block min
        if(pindexPrev != NULL) nBlockMin = pindexPrev->nHeight - GetBudgetPaymentCycleBlocks() * (nPaymentCount + 1);

        int nBlockStart = params[4].get_int();
        if(nBlockStart % GetBudgetPaymentCycleBlocks() != 0){
            const int nextBlock = GetNextSuperblock(pindexPrev->nHeight);
            return strprintf("Invalid block start - must be a budget cycle block. Next valid block: %d", nextBlock);
        }

        int nBlockEnd = nBlockStart + (GetBudgetPaymentCycleBlocks()*nPaymentCount);

        if(nBlockStart < nBlockMin)
            return "Invalid payment count, must be more than current height.";

        if(nBlockEnd < pindexPrev->nHeight)
            return "Invalid ending block, starting block + (payment_cycle*payments) must be more than current height.";

        CBitcoinAddress address(params[5].get_str());
        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Crown address");

        // Parse Crown address
        CScript scriptPubKey = GetScriptForDestination(address.Get());
        CAmount nAmount = AmountFromValue(params[6]);
        uint256 hash = ParseHashV(params[7], "parameter 1");

        //create the proposal incase we're the first to make it
        CBudgetProposalBroadcast budgetProposalBroadcast(strProposalName, strURL, nPaymentCount, scriptPubKey, nAmount, nBlockStart, hash);

        std::string strError = "";
        int nConf = 0;
        if(!IsBudgetCollateralValid(hash, budgetProposalBroadcast.GetHash(), strError, budgetProposalBroadcast.nTime, nConf)){
            return "Proposal FeeTX is not valid - " + hash.ToString() + " - " + strError;
        }

        if(!masternodeSync.IsBlockchainSynced()){
            return "Must wait for client to sync with masternode network. Try again in a minute or so.";            
        }

        budgetProposalBroadcast.Relay();
        budget.AddProposal(budgetProposalBroadcast);

        return budgetProposalBroadcast.GetHash().ToString();

    }

    if(strCommand == "vote-many")
    {
        std::vector<CNodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        if (params.size() != 3)
            throw runtime_error("Correct usage is 'mnbudget vote-many proposal-hash yes|no'");

        uint256 hash = ParseHashV(params[1], "parameter 1");
        std::string strVote = params[2].get_str();

        if(strVote != "yes" && strVote != "no") return "You can only vote 'yes' or 'no'";
        int nVote = VOTE_ABSTAIN;
        if(strVote == "yes") nVote = VOTE_YES;
        if(strVote == "no") nVote = VOTE_NO;

        int success = 0;
        int failed = 0;

        Object resultsObj;

        BOOST_FOREACH(CNodeEntry mne, masternodeConfig.getEntries()) {
            std::string errorMessage;
            std::vector<unsigned char> vchMasterNodeSignature;
            std::string strMasterNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            Object statusObj;

            if(!legacySigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)){
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

            CBudgetVote vote(pmn->vin, hash, nVote);
            if(!vote.Sign(keyMasternode, pubKeyMasternode)){
                failed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Failure to sign."));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }


            std::string strError = "";
            if(budget.SubmitProposalVote(vote, strError)) {
                vote.Relay();
                success++;
                statusObj.push_back(Pair("result", "success"));
            } else {
                failed++;
                statusObj.push_back(Pair("result", strError.c_str()));
            }

            resultsObj.push_back(Pair(mne.getAlias(), statusObj));
        }

        Object returnObj;
        returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed)));
        returnObj.push_back(Pair("detail", resultsObj));

        return returnObj;
    }

    if(strCommand == "vote")
    {
        std::vector<CNodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        if (params.size() != 3)
            throw runtime_error("Correct usage is 'mnbudget vote proposal-hash yes|no'");

        uint256 hash = ParseHashV(params[1], "parameter 1");
        std::string strVote = params[2].get_str();

        if(strVote != "yes" && strVote != "no") return "You can only vote 'yes' or 'no'";
        int nVote = VOTE_ABSTAIN;
        if(strVote == "yes") nVote = VOTE_YES;
        if(strVote == "no") nVote = VOTE_NO;

        CPubKey pubKeyMasternode;
        CKey keyMasternode;
        std::string errorMessage;

        if(!legacySigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
            return "Error upon calling SetKey";

        CMasternode* pmn = mnodeman.Find(activeMasternode.vin);
        if(pmn == NULL)
        {
            return "Failure to find masternode in list : " + activeMasternode.vin.ToString();
        }

        CBudgetVote vote(activeMasternode.vin, hash, nVote);
        if(!vote.Sign(keyMasternode, pubKeyMasternode)){
            return "Failure to sign.";
        }

        std::string strError = "";
        if(budget.SubmitProposalVote(vote, strError)){
            vote.Relay();
            return "Voted successfully";
        } else {
            return "Error voting : " + strError;
        }
    }

    if(strCommand == "projection")
    {
        Object resultObj;
        CAmount nTotalAllotted = 0;

        std::vector<CBudgetProposal*> winningProps = budget.GetBudget();
        BOOST_FOREACH(CBudgetProposal* pbudgetProposal, winningProps)
        {
            nTotalAllotted += pbudgetProposal->GetAllotted();

            CTxDestination address1;
            ExtractDestination(pbudgetProposal->GetPayee(), address1);
            CBitcoinAddress address2(address1);

            Object bObj;
            bObj.push_back(Pair("URL",  pbudgetProposal->GetURL()));
            bObj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString()));
            bObj.push_back(Pair("BlockStart",  (int64_t)pbudgetProposal->GetBlockStart()));
            bObj.push_back(Pair("BlockEnd",    (int64_t)pbudgetProposal->GetBlockEnd()));
            bObj.push_back(Pair("TotalPaymentCount",  (int64_t)pbudgetProposal->GetTotalPaymentCount()));
            bObj.push_back(Pair("RemainingPaymentCount",  (int64_t)pbudgetProposal->GetRemainingPaymentCount()));
            bObj.push_back(Pair("PaymentAddress",   address2.ToString()));
            bObj.push_back(Pair("Ratio",  pbudgetProposal->GetRatio()));
            bObj.push_back(Pair("Yeas",  (int64_t)pbudgetProposal->GetYeas()));
            bObj.push_back(Pair("Nays",  (int64_t)pbudgetProposal->GetNays()));
            bObj.push_back(Pair("Abstains",  (int64_t)pbudgetProposal->GetAbstains()));
            bObj.push_back(Pair("TotalPayment",  ValueFromAmount(pbudgetProposal->GetAmount()*pbudgetProposal->GetTotalPaymentCount())));
            bObj.push_back(Pair("MonthlyPayment",  ValueFromAmount(pbudgetProposal->GetAmount())));
            bObj.push_back(Pair("Alloted",  ValueFromAmount(pbudgetProposal->GetAllotted())));
            bObj.push_back(Pair("TotalBudgetAlloted",  ValueFromAmount(nTotalAllotted)));

            std::string strError = "";
            bObj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(strError)));
            bObj.push_back(Pair("IsValidReason",  strError.c_str()));
            bObj.push_back(Pair("fValid",  pbudgetProposal->fValid));

            resultObj.push_back(Pair(pbudgetProposal->GetName(), bObj));
        }

        return resultObj;
    }

    if(strCommand == "show")
    {
        std::string strShow = "valid";
        if (params.size() == 2)
            std::string strProposalName = params[1].get_str();

        Object resultObj;
        int64_t nTotalAllotted = 0;

        std::vector<CBudgetProposal*> winningProps = budget.GetAllProposals();
        BOOST_FOREACH(CBudgetProposal* pbudgetProposal, winningProps)
        {
            if(strShow == "valid" && !pbudgetProposal->fValid) continue;

            nTotalAllotted += pbudgetProposal->GetAllotted();

            CTxDestination address1;
            ExtractDestination(pbudgetProposal->GetPayee(), address1);
            CBitcoinAddress address2(address1);

            Object bObj;
            bObj.push_back(Pair("Name",  pbudgetProposal->GetName()));
            bObj.push_back(Pair("URL",  pbudgetProposal->GetURL()));
            bObj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString()));
            bObj.push_back(Pair("FeeHash",  pbudgetProposal->nFeeTXHash.ToString()));
            bObj.push_back(Pair("BlockStart",  (int64_t)pbudgetProposal->GetBlockStart()));
            bObj.push_back(Pair("BlockEnd",    (int64_t)pbudgetProposal->GetBlockEnd()));
            bObj.push_back(Pair("TotalPaymentCount",  (int64_t)pbudgetProposal->GetTotalPaymentCount()));
            bObj.push_back(Pair("RemainingPaymentCount",  (int64_t)pbudgetProposal->GetRemainingPaymentCount()));
            bObj.push_back(Pair("PaymentAddress",   address2.ToString()));
            bObj.push_back(Pair("Ratio",  pbudgetProposal->GetRatio()));
            bObj.push_back(Pair("Yeas",  (int64_t)pbudgetProposal->GetYeas()));
            bObj.push_back(Pair("Nays",  (int64_t)pbudgetProposal->GetNays()));
            bObj.push_back(Pair("Abstains",  (int64_t)pbudgetProposal->GetAbstains()));
            bObj.push_back(Pair("TotalPayment",  ValueFromAmount(pbudgetProposal->GetAmount()*pbudgetProposal->GetTotalPaymentCount())));
            bObj.push_back(Pair("MonthlyPayment",  ValueFromAmount(pbudgetProposal->GetAmount())));

            bObj.push_back(Pair("IsEstablished",  pbudgetProposal->IsEstablished()));

            std::string strError = "";
            bObj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(strError)));
            bObj.push_back(Pair("IsValidReason",  strError.c_str()));
            bObj.push_back(Pair("fValid",  pbudgetProposal->fValid));

            resultObj.push_back(Pair(pbudgetProposal->GetName(), bObj));
        }

        return resultObj;
    }

    if(strCommand == "getinfo")
    {
        if (params.size() != 2)
            throw runtime_error("Correct usage is 'mnbudget getinfo profilename'");

        std::string strProposalName = params[1].get_str();

        CBudgetProposal* pbudgetProposal = budget.FindProposal(strProposalName);

        if(pbudgetProposal == NULL) return "Unknown proposal name";

        CTxDestination address1;
        ExtractDestination(pbudgetProposal->GetPayee(), address1);
        CBitcoinAddress address2(address1);

        Object obj;
        obj.push_back(Pair("Name",  pbudgetProposal->GetName()));
        obj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString()));
        obj.push_back(Pair("FeeHash",  pbudgetProposal->nFeeTXHash.ToString()));
        obj.push_back(Pair("URL",  pbudgetProposal->GetURL()));
        obj.push_back(Pair("BlockStart",  (int64_t)pbudgetProposal->GetBlockStart()));
        obj.push_back(Pair("BlockEnd",    (int64_t)pbudgetProposal->GetBlockEnd()));
        obj.push_back(Pair("TotalPaymentCount",  (int64_t)pbudgetProposal->GetTotalPaymentCount()));
        obj.push_back(Pair("RemainingPaymentCount",  (int64_t)pbudgetProposal->GetRemainingPaymentCount()));
        obj.push_back(Pair("PaymentAddress",   address2.ToString()));
        obj.push_back(Pair("Ratio",  pbudgetProposal->GetRatio()));
        obj.push_back(Pair("Yeas",  (int64_t)pbudgetProposal->GetYeas()));
        obj.push_back(Pair("Nays",  (int64_t)pbudgetProposal->GetNays()));
        obj.push_back(Pair("Abstains",  (int64_t)pbudgetProposal->GetAbstains()));
        obj.push_back(Pair("TotalPayment",  ValueFromAmount(pbudgetProposal->GetAmount()*pbudgetProposal->GetTotalPaymentCount())));
        obj.push_back(Pair("MonthlyPayment",  ValueFromAmount(pbudgetProposal->GetAmount())));
        
        obj.push_back(Pair("IsEstablished",  pbudgetProposal->IsEstablished()));

        std::string strError = "";
        obj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(strError)));
        obj.push_back(Pair("fValid",  pbudgetProposal->fValid));

        return obj;
    }

    if(strCommand == "getvotes")
    {
        if (params.size() != 2)
            throw runtime_error("Correct usage is 'mnbudget getvotes profilename'");

        std::string strProposalName = params[1].get_str();

        Object obj;

        CBudgetProposal* pbudgetProposal = budget.FindProposal(strProposalName);

        if(pbudgetProposal == NULL) return "Unknown proposal name";

        std::map<uint256, CBudgetVote>::iterator it = pbudgetProposal->mapVotes.begin();
        while(it != pbudgetProposal->mapVotes.end()){

            Object bObj;
            bObj.push_back(Pair("nHash",  (*it).first.ToString().c_str()));
            bObj.push_back(Pair("Vote",  (*it).second.GetVoteString()));
            bObj.push_back(Pair("nTime",  (int64_t)(*it).second.nTime));
            bObj.push_back(Pair("fValid",  (*it).second.fValid));

            obj.push_back(Pair((*it).second.vin.prevout.ToStringShort(), bObj));

            it++;
        }


        return obj;
    }

    if(strCommand == "check")
    {
        budget.CheckAndRemove();

        return "Success";
    }

    return Value::null;
}

Value mnbudgetvoteraw(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 6)
        throw runtime_error(
                "mnbudgetvoteraw <masternode-tx-hash> <masternode-tx-index> <proposal-hash> <yes|no> <time> <vote-sig>\n"
                "Compile and relay a proposal vote with provided external signature instead of signing vote internally\n"
                );

    uint256 hashMnTx = ParseHashV(params[0], "mn tx hash");
    int nMnTxIndex = params[1].get_int();
    CTxIn vin = CTxIn(hashMnTx, nMnTxIndex);

    uint256 hashProposal = ParseHashV(params[2], "Proposal hash");
    std::string strVote = params[3].get_str();

    if(strVote != "yes" && strVote != "no") return "You can only vote 'yes' or 'no'";
    int nVote = VOTE_ABSTAIN;
    if(strVote == "yes") nVote = VOTE_YES;
    if(strVote == "no") nVote = VOTE_NO;

    int64_t nTime = params[4].get_int64();
    std::string strSig = params[5].get_str();
    bool fInvalid = false;
    vector<unsigned char> vchSig = DecodeBase64(strSig.c_str(), &fInvalid);

    if (fInvalid)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    CMasternode* pmn = mnodeman.Find(vin);
    if(pmn == NULL)
    {
        return "Failure to find masternode in list : " + vin.ToString();
    }

    CBudgetVote vote(vin, hashProposal, nVote);
    vote.nTime = nTime;
    vote.vchSig = vchSig;

    if(!vote.SignatureValid(true)){
        return "Failure to verify signature.";
    }

    std::string strError = "";
    if(budget.SubmitProposalVote(vote, strError)){
        vote.Relay();
        return "Voted successfully";
    } else {
        return "Error voting : " + strError;
    }
}

Value mnfinalbudget(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "suggest" && strCommand != "vote-many" && strCommand != "vote" && strCommand != "show" && strCommand != "getvotes"))
        throw runtime_error(
                "mnfinalbudget \"command\"... ( \"passphrase\" )\n"
                "Vote or show current budgets\n"
                "\nAvailable commands:\n"
                "  vote-many   - Vote on a finalized budget\n"
                "  vote        - Vote on a finalized budget\n"
                "  show        - Show existing finalized budgets\n"
                "  getvotes     - Get vote information for each finalized budget\n"
                );

    if(strCommand == "vote-many")
    {
        std::vector<CNodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        if (params.size() != 2)
            throw runtime_error("Correct usage is 'mnfinalbudget vote-many BUDGET_HASH'");

        std::string strHash = params[1].get_str();
        uint256 hash(uint256S(strHash));

        int success = 0;
        int failed = 0;

        Object resultsObj;

        BOOST_FOREACH(CNodeEntry mne, masternodeConfig.getEntries()) {
            std::string errorMessage;
            std::vector<unsigned char> vchMasterNodeSignature;
            std::string strMasterNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            Object statusObj;

            if(!legacySigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)){
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


            BudgetDraftVote vote(pmn->vin, hash);
            if(!vote.Sign(keyMasternode, pubKeyMasternode)){
                failed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Failure to sign."));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }

            std::string strError = "";
            if(budget.UpdateBudgetDraft(vote, NULL, strError)){
                vote.Relay();
                success++;
                statusObj.push_back(Pair("result", "success"));
            } else {
                failed++;
                statusObj.push_back(Pair("result", strError.c_str()));
            }

            resultsObj.push_back(Pair(mne.getAlias(), statusObj));
        }

        Object returnObj;
        returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed)));
        returnObj.push_back(Pair("detail", resultsObj));

        return returnObj;
    }

    if(strCommand == "vote")
    {
        std::vector<CNodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        if (params.size() != 2)
            throw runtime_error("Correct usage is 'mnfinalbudget vote BUDGET_HASH'");

        std::string strHash = params[1].get_str();
        uint256 hash(uint256S(strHash));

        CPubKey pubKeyMasternode;
        CKey keyMasternode;
        std::string errorMessage;

        if(!legacySigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
            return "Error upon calling SetKey";

        CMasternode* pmn = mnodeman.Find(activeMasternode.vin);
        if(pmn == NULL)
        {
            return "Failure to find masternode in list : " + activeMasternode.vin.ToString();
        }

        BudgetDraftVote vote(activeMasternode.vin, hash);
        if(!vote.Sign(keyMasternode, pubKeyMasternode)){
            return "Failure to sign.";
        }

        std::string strError = "";
        if(budget.UpdateBudgetDraft(vote, NULL, strError)){
            vote.Relay();
            return "success";
        } else {
            return "Error voting : " + strError;
        }

    }

    if(strCommand == "show")
    {
        Object resultObj;

        std::vector<BudgetDraft*> drafts = budget.GetBudgetDrafts();
        BOOST_FOREACH(BudgetDraft* budgetDraft, drafts)
        {
            Object bObj;
            bObj.push_back(Pair("IsSubmittedManually",  budgetDraft->IsSubmittedManually()));
            bObj.push_back(Pair("Hash",  budgetDraft->GetHash().ToString()));
            bObj.push_back(Pair("BlockStart",  (int64_t)budgetDraft->GetBlockStart()));
            bObj.push_back(Pair("BlockEnd",    (int64_t)budgetDraft->GetBlockStart())); // Budgets are paid in a single block !
            bObj.push_back(Pair("Proposals",  budgetDraft->GetProposals()));
            bObj.push_back(Pair("VoteCount",  (int64_t)budgetDraft->GetVoteCount()));
            bObj.push_back(Pair("Status",  budgetDraft->GetStatus()));
            std::string strError = "";
            bObj.push_back(Pair("IsValid",  budgetDraft->IsValid(strError)));
            bObj.push_back(Pair("IsValidReason",  strError.c_str()));
            if (budgetDraft->IsSubmittedManually())
                bObj.push_back(Pair("FeeTX",  budgetDraft->GetFeeTxHash().ToString()));

            resultObj.push_back(Pair(budgetDraft->GetHash().ToString(), bObj));
        }

        return resultObj;

    }

    if(strCommand == "getvotes")
    {
        if (params.size() != 2)
            throw runtime_error("Correct usage is 'mnbudget getvotes budget-hash'");

        std::string strHash = params[1].get_str();
        uint256 hash(uint256S(strHash));

        Object obj;

        BudgetDraft* pbudgetDraft = budget.FindBudgetDraft(hash);

        if(pbudgetDraft == NULL) return "Unknown budget hash";

        const std::map<uint256, BudgetDraftVote>& fbVotes = pbudgetDraft->GetVotes();
        for (std::map<uint256, BudgetDraftVote>::const_iterator i = fbVotes.begin(); i != fbVotes.end(); ++i)
        {
            Object bObj;
            bObj.push_back(Pair("nHash", i->first.ToString().c_str()));
            bObj.push_back(Pair("nTime", static_cast<int64_t>(i->second.nTime)));
            bObj.push_back(Pair("fValid", i->second.fValid));

            obj.push_back(Pair(i->second.vin.prevout.ToStringShort(), bObj));
        }

        const std::map<uint256, BudgetDraftVote>& oldVotes = pbudgetDraft->GetObsoleteVotes();
        for (std::map<uint256, BudgetDraftVote>::const_iterator i = oldVotes.begin(); i != oldVotes.end(); ++i)
        {
            Object bObj;
            bObj.push_back(Pair("nHash", i->first.ToString().c_str()));
            bObj.push_back(Pair("nTime", static_cast<int64_t>(i->second.nTime)));
            bObj.push_back(Pair("fValid", i->second.fValid));
            bObj.push_back(Pair("obsolete", true));

            obj.push_back(Pair(i->second.vin.prevout.ToStringShort(), bObj));
        }

        return obj;
    }


    return Value::null;
}
