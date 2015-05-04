// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
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

#include <boost/lexical_cast.hpp>
#include <fstream>
using namespace json_spirit;
using namespace std;

Value mnbudget(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "vote-many" && strCommand != "vote" && strCommand != "getvotes" && strCommand != "getinfo"))
        throw runtime_error(
                "mnbudget \"command\"... ( \"passphrase\" )\n"
                "Vote or show current budgets\n"
                "\nAvailable commands:\n"
                "  vote-many    - Vote on a Dash initiative\n"
                "  vote         - Vote on a Dash initiative\n"
                "  getvotes     - Show current masternode budgets\n"
                "  getinfo      - Show current masternode budgets\n"
                );

    if(strCommand == "vote-many")
    {
        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        if (params.size() != 5)
            throw runtime_error("Correct usage of vote-many is 'mnbudget vote-many PROPOSAL-NAME PAYMENT_COUNT DASH_ADDRESS USD_AMOUNT YES|NO|ABSTAIN'");

        std::string strProposalName = params[1].get_str();
        if(strProposalName.size() > 20)
            return "Invalid proposal name, limit of 20 characters.";

        int nPaymentCount = params[2].get_int();
        CBitcoinAddress address(params[3].get_str());
        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Dash address");

        // Parse Dash address
        CScript scriptPubKey = GetScriptForDestination(address.Get());

        CAmount nAmount = AmountFromValue(params[4]);
        std::string strVote = params[5].get_str().c_str();

        if(strVote != "yes" && strVote != "no") return "You can only vote 'yes' or 'no'";
        int nVote = VOTE_ABSTAIN;
        if(strVote == "yes") nVote = VOTE_YES;
        if(strVote == "no") nVote = VOTE_NO;

        int success = 0;
        int failed = 0;

        Object resultObj;

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            std::string errorMessage;
            std::vector<unsigned char> vchMasterNodeSignature;
            std::string strMasterNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            if(!darkSendSigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)){
                printf(" Error upon calling SetKey for %s\n", mne.getAlias().c_str());
                failed++;
                continue;
            }

            CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
            if(pmn == NULL)
            {
                printf("Can't find masternode by pubkey for %s\n", mne.getAlias().c_str());
                failed++;
                continue;
            }    

            if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
                return(" Error upon calling SetKey");

            CBudgetVote vote(pmn->vin, strProposalName, nPaymentCount, scriptPubKey, nAmount, nVote);
            if(!vote.Sign(keyMasternode, pubKeyMasternode)){
                return "Failure to sign.";
            }

            success++;
            vote.Relay();
        }

        return("Voted successfully " + boost::lexical_cast<std::string>(success) + " time(s) and failed " + boost::lexical_cast<std::string>(failed) + " time(s).");
    }

    if(strCommand == "vote")
    {
        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        if (params.size() != 5)
            throw runtime_error("Correct usage of vote-many is 'mnbudget vote PROPOSAL-NAME PAYMENT_COUNT DASH_ADDRESS USD_AMOUNT YES|NO|ABSTAIN'");

        std::string strProposalName = params[1].get_str();

        if(strProposalName.size() > 20)
            return "Invalid proposal name, limit of 20 characters.";

        int nPaymentCount = params[2].get_int();
        CBitcoinAddress address(params[3].get_str());
        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Dash address");

        // Parse Dash address
        CScript scriptPubKey = GetScriptForDestination(address.Get());

        CAmount nAmount = AmountFromValue(params[4]);
        std::string strVote = params[5].get_str().c_str();

        if(strVote != "yes" && strVote != "no") return "You can only vote 'yes' or 'no'";
        int nVote = VOTE_ABSTAIN;
        if(strVote == "yes") nVote = VOTE_YES;
        if(strVote == "no") nVote = VOTE_NO;

        CPubKey pubKeyMasternode;
        CKey keyMasternode;
        std::string errorMessage;

        if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
            return(" Error upon calling SetKey");

        CBudgetVote vote(activeMasternode.vin, strProposalName, nPaymentCount, scriptPubKey, nAmount, nVote);
        if(!vote.Sign(keyMasternode, pubKeyMasternode)){
            return "Failure to sign.";
        }

        vote.Relay();

    }

    if(strCommand == "show")
    {
        std::string strProposalName = params[1].get_str();

        CBudgetProposal* prop = budget.Find(strProposalName);

        if(prop == NULL) return "Unknown proposal name";

        Object resultObj;
        int64_t nTotalAllotted = 0;

        std::vector<CBudgetProposal*> winningProps = budget.GetBudget();
        BOOST_FOREACH(CBudgetProposal* prop, winningProps)
        {
            nTotalAllotted += prop->GetAllotted();

            CTxDestination address1;
            ExtractDestination(prop->GetPayee(), address1);
            CBitcoinAddress address2(address1);

            Object bObj;
            bObj.push_back(Pair("Name",  prop->GetName().c_str()));
            bObj.push_back(Pair("BlockStart",  (int64_t)prop->GetBlockStart()));
            bObj.push_back(Pair("BlockEnd",    (int64_t)prop->GetBlockEnd()));
            bObj.push_back(Pair("PaymentAddress",  address2.ToString().c_str()));
            bObj.push_back(Pair("Ratio",  prop->GetRatio()));
            bObj.push_back(Pair("Yeas",  (int64_t)prop->GetYeas()));
            bObj.push_back(Pair("Nays",  (int64_t)prop->GetNays()));
            bObj.push_back(Pair("Abstains",  (int64_t)prop->GetAbstains()));
            bObj.push_back(Pair("Alloted",  (int64_t)prop->GetAllotted()));
            bObj.push_back(Pair("TotalBudgetAlloted",  nTotalAllotted));
            resultObj.push_back(Pair("masternode", bObj));
        }

        return resultObj;
    }

    if(strCommand == "getinfo")
    {
        if (params.size() != 2)
            throw runtime_error("Correct usage of getinfo is 'mnbudget getinfo profilename'");

        std::string strProposalName = params[1].get_str();

        CBudgetProposal* prop = budget.Find(strProposalName);

        if(prop == NULL) return "Unknown proposal name";

        CTxDestination address1;
        ExtractDestination(prop->GetPayee(), address1);
        CBitcoinAddress address2(address1);

        Object obj;
        obj.push_back(Pair("Name",  prop->GetName().c_str()));
        obj.push_back(Pair("BlockStart",  (int64_t)prop->GetBlockStart()));
        obj.push_back(Pair("BlockEnd",    (int64_t)prop->GetBlockEnd()));
        obj.push_back(Pair("PaymentAddress",   address2.ToString().c_str()));
        obj.push_back(Pair("Ratio",  prop->GetRatio()));
        obj.push_back(Pair("Yeas",  (int64_t)prop->GetYeas()));
        obj.push_back(Pair("Nays",  (int64_t)prop->GetNays()));
        obj.push_back(Pair("Abstains",  (int64_t)prop->GetAbstains()));
        obj.push_back(Pair("Alloted",  (int64_t)prop->GetAllotted()));

        return obj;
    }

    if(strCommand == "getvotes")
    {
        
        if (params.size() != 2)
            throw runtime_error("Correct usage of getvotes is 'mnbudget getinfo profilename'");

        std::string strProposalName = params[1].get_str();

        CBudgetProposal* prop = budget.Find(strProposalName);

        if(prop == NULL) return "Unknown proposal name";

        Object obj;
   
        int c = 0;

        std::map<uint256, CBudgetVote>::iterator it = prop->mapVotes.begin();
        while(it != prop->mapVotes.end())
            obj.push_back(Pair((*it).second.strProposalName.c_str(),  (*it).second.GetVoteString().c_str()));
        

        return obj;
    }

    return Value::null;
}

