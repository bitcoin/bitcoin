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
#include "governance.h"
#include "governance-classes.h"
#include "governance-types.h"
#include "rpcserver.h"
#include "utilmoneystr.h"
#include <boost/lexical_cast.hpp>

#include <fstream>
#include <univalue.h>
#include <iostream>
#include <sstream>

using namespace std;

UniValue governance(const UniValue& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "sign" && strCommand != "create" && strCommand != "vote-once" && 
            strCommand != "vote-many" && strCommand != "vote-alias" && strCommand != "list"))
        throw runtime_error(
                "governance \"command1\",\"command2\",\"command3\" ...\n"
                "Manage governance system\n"
                "\n See online documentation at http://govman.dash.org/\n"
                );


        /*

governance sign {WITH-PUBKEY : pubkey, HASH: SIGN-HASH} // Sign a key as a company, user, group, etc

    => {result = "success", "sig" : "SIGNATURE1", "signature-type": "user"}

governance create {parentHash: HASH, name:"proposal", sig1: SIGNATURE1, sig2: sig2.. }

    => {result = "success", "hash" : "HASH1", "proposal-type": "generic", ...}

// WAIT 30 MINUTES

governance create {FEEHASH: HASH, parentHash: HASH,  name:"proposal", sig1: sig1, sig2: sig2.. }

    => {result = "success", "hash" : "HASH2", "proposal-type": "generic", ...}

governance (vote-once|vote-many) GOV_HASH_1 yes
governance vote-alias MASTERNODE_NAME GOV_HASH_1 yes

// this is not meant to be readable, see http://www.github.com/dashpay/fetch for more info
governance list

        */


/*

    {
        'proof-of-destruction' : 'create', //or HASH
        'valid' : true,
        'name' : "Evan Duffield",

        --
        
        'type' : "proposal",
        // see governances-classes for mapping
        // CProposal lvl, proposal-type, status, status-error
        'level' : "I",
        'proposal-type' : "generic",
        'status' : "ok",
        'status-error' : "none",

        'time' : "now",
        'fee-hash' : "hash1",
        'parent-hash' : "hash2",

        'url' : "dashwhale.org/blah"
        'when' : 'nextsuperblock+3 ? blockheight ?'
        
        months
        amount
        payuser-by-name

    }

    CGovernanceObj obj(json);

    if json['proof-of-destruction'] == "create":

        // CWalletTx wtx;
        // if(!pwalletMain->GetBudgetSystemCollateralTX(wtx, budgetProposalBroadcast.GetHash(), useIX)){
        //     return "Error making collateral transaction for proposal. Please check your wallet balance and make sure your wallet is unlocked.";
        // }

        // // make our change address
        // CReserveKey reservekey(pwalletMain);
        // //send the tx to the network
        // pwalletMain->CommitTransaction(wtx, reservekey, useIX ? NetMsgType::IX : NetMsgType::TX);

        // return wtx.GetHash().ToString();

    else:

        if obj.IsValid() ?
            obj.Relay();



*/
    // TODO: 12.1, organize these in order


    if(strCommand == "create")
    {
        UniValue govObjJson = params[1].get_obj();

        CGovernanceNode govObj;
        GovernanceObjectType govType = GovernanceStringToType(govObjJson["type"].get_str());
        std::string strError = "";

        if(!CreateNewGovernanceObject(govObjJson, govObj, govType, strError))
        {
            return strError;
        }

        
        if(govObjJson["create"].get_int() == 1)
        {
            
        } else {
    
            // if(govObj.IsCollateralValid())
            // {
            //     //Relay
            // }
    
            // if(govObj.IsValid())
            // {
            //     //Relay
            // }
        }

    }

    // TODO: 12.1
    // if(strCommand == "list")
    // {
    //     return full json; //use fetch!
    // }

    // if(strCommand == "sign")
    // {
    //     string withPubkey = param(1);
    //     string withSignHash = param(2);

    //     // option 1
    //     // return CGovernanceMananger::SignHashWithKey(withPubkey, withSignHash);

    //     obj = CGovernanceNode(json);
    //     if(!obj.IsValid)
    //     {
    //         std::string strError = "";
    //         if(!obj.SignWithKeyByName(withPubkey, strError))
    //         {
    //             return obj.ToJson();
    //         }
    //     }
    // }

    // if(strCommand == "prepare")
    // {

    //     /*

    //         governance prepare new {parentHash: HASH, name:"proposal", sig1: sig1, sig2: sig2.. }

    //     */

    //     string strCommand2 = param(1)
    //     UniValue obj(VOBJ) = param(2);


    //     // if (params.size() != 7)
    //     //     throw runtime_error("Correct usage is 'proposal prepare <proposal-name> <url> <payment-count> <block-start> <dash-address> <monthly-payment-dash>'");

    //     // int nBlockMin = 0;
    //     // LOCK(cs_main);
    //     // CBlockIndex* pindex = chainActive.Tip();

    //     // std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
    //     // mnEntries = masternodeConfig.getEntries();

    //     // std::string strName = SanitizeString(params[1].get_str());
    //     // std::string strURL = SanitizeString(params[2].get_str());
    //     // int nPaymentCount = params[3].get_int();
    //     // int nBlockStart = params[4].get_int();

    //     // //set block min
    //     // if(pindex != NULL) nBlockMin = pindex->nHeight;

    //     // if(nBlockStart < nBlockMin)
    //     //     return "Invalid block start, must be more than current height.";

    //     // CBitcoinAddress address(params[5].get_str());
    //     // if (!address.IsValid())
    //     //     throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Dash address");

    //     // // Parse Dash address
    //     // CScript scriptPubKey = GetScriptForDestination(address.Get());
    //     // CAmount nAmount = AmountFromValue(params[6]);

    //     // //*************************************************************************

    //     // // create transaction 15 minutes into the future, to allow for confirmation time
    //     // CGovernanceNodeBroadcast budgetProposalBroadcast;
    //     // budgetProposalBroadcast.CreateProposal(strName, strURL, nPaymentCount, scriptPubKey, nAmount, nBlockStart, uint256());

    //     // std::string strError = "";
    //     // if(!budgetProposalBroadcast.IsValid(pindex, strError, false))
    //     //     return "Proposal is not valid - " + budgetProposalBroadcast.GetHash().ToString() + " - " + strError;

    //     // bool useIX = false; //true;
    //     // // if (params.size() > 7) {
    //     // //     if(params[7].get_str() != "false" && params[7].get_str() != "true")
    //     // //         return "Invalid use_ix, must be true or false";
    //     // //     useIX = params[7].get_str() == "true" ? true : false;
    //     // // }

    //     // CWalletTx wtx;
    //     // if(!pwalletMain->GetBudgetSystemCollateralTX(wtx, budgetProposalBroadcast.GetHash(), useIX)){
    //     //     return "Error making collateral transaction for proposal. Please check your wallet balance and make sure your wallet is unlocked.";
    //     // }

    //     // // make our change address
    //     // CReserveKey reservekey(pwalletMain);
    //     // //send the tx to the network
    //     // pwalletMain->CommitTransaction(wtx, reservekey, useIX ? NetMsgType::IX : NetMsgType::TX);

    //     // return wtx.GetHash().ToString();
    // }

    // if(strCommand == "submit")
    // {
    //     if (params.size() != 8)
    //         throw runtime_error("Correct usage is 'proposal submit <proposal-name> <url> <payment-count> <block-start> <dash-address> <monthly-payment-dash> <fee-tx>'");

    //     if(!masternodeSync.IsBlockchainSynced()){
    //         return "Must wait for client to sync with masternode network. Try again in a minute or so.";
    //     }

    //     int nBlockMin = 0;
    //     LOCK(cs_main);
    //     CBlockIndex* pindex = chainActive.Tip();

    //     std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
    //     mnEntries = masternodeConfig.getEntries();

    //     std::string strName = SanitizeString(params[1].get_str());
    //     std::string strURL = SanitizeString(params[2].get_str());
    //     int nPaymentCount = params[3].get_int();
    //     int nBlockStart = params[4].get_int();

    //     //set block min
    //     if(pindex != NULL) nBlockMin = pindex->nHeight;

    //     if(nBlockStart < nBlockMin)
    //         return "Invalid payment count, must be more than current height.";

    //     CBitcoinAddress address(params[5].get_str());
    //     if (!address.IsValid())
    //         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Dash address");

    //     // Parse Dash address
    //     CScript scriptPubKey = GetScriptForDestination(address.Get());
    //     CAmount nAmount = AmountFromValue(params[6]);
    //     uint256 hash = ParseHashV(params[7], "Proposal hash");

    //     //create the proposal incase we're the first to make it
    //     CGovernanceNodeBroadcast budgetProposalBroadcast;
    //     budgetProposalBroadcast.CreateProposal(strName, strURL, nPaymentCount, scriptPubKey, nAmount, nBlockStart, hash);

    //     std::string strError = "";

    //     if(!budgetProposalBroadcast.IsValid(pindex, strError)){
    //         return "Proposal is not valid - " + budgetProposalBroadcast.GetHash().ToString() + " - " + strError;
    //     }

    //     int nConf = 0;
    //     if(!IsCollateralValid(hash, budgetProposalBroadcast.GetHash(), strError, budgetProposalBroadcast.nTime, nConf, GOVERNANCE_FEE_TX)){
    //         return "Proposal FeeTX is not valid - " + hash.ToString() + " - " + strError;
    //     }

    //     govman.mapSeenGovernanceObjects.insert(make_pair(budgetProposalBroadcast.GetHash(), budgetProposalBroadcast));
    //     budgetProposalBroadcast.Relay();
    //     govman.AddGovernanceObject(budgetProposalBroadcast);

    //     return budgetProposalBroadcast.GetHash().ToString();

    // }

    // if(strCommand == "list")
    // {
    //     if (params.size() > 2)
    //         throw runtime_error("Correct usage is 'proposal list [valid|all|extended]'");

    //     std::string strShow = "valid";
    //     if (params.size() == 2) strShow = params[1].get_str();

    //     CBlockIndex* pindex;
    //     {
    //         LOCK(cs_main);
    //         pindex = chainActive.Tip();
    //     }

    //     UniValue resultObj(UniValue::VOBJ);
    //     int64_t nTotalAllotted = 0;

    //     std::vector<CGovernanceNode*> winningProps = govman.FindMatchingGovernanceObjects(Proposal);
    //     BOOST_FOREACH(CGovernanceNode* pbudgetProposal, winningProps)
    //     {
    //         if(strShow == "valid" && !pbudgetProposal->fValid) continue;

    //         nTotalAllotted += pbudgetProposal->GetAllotted();

    //         CTxDestination address1;
    //         ExtractDestination(pbudgetProposal->GetPayee(), address1);
    //         CBitcoinAddress address2(address1);

    //         UniValue bObj(UniValue::VOBJ);
    //         bObj.push_back(Pair("Name",  pbudgetProposal->GetName()));

    //         if(strShow == "extended") bObj.push_back(Pair("URL",  pbudgetProposal->GetURL()));
    //         bObj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString()));

    //         if(strShow == "extended")
    //         {
    //             bObj.push_back(Pair("FeeHash",  pbudgetProposal->nFeeTXHash.ToString()));
    //             bObj.push_back(Pair("BlockStart",  (int64_t)pbudgetProposal->GetBlockStart()));
    //             bObj.push_back(Pair("BlockEnd",    (int64_t)pbudgetProposal->GetBlockEnd()));
    //             bObj.push_back(Pair("TotalPaymentCount",  (int64_t)pbudgetProposal->GetTotalPaymentCount()));
    //             bObj.push_back(Pair("RemainingPaymentCount",  (int64_t)pbudgetProposal->GetRemainingPaymentCount(pindex->nHeight)));
    //             bObj.push_back(Pair("PaymentAddress",   address2.ToString()));
    //             bObj.push_back(Pair("Ratio",  pbudgetProposal->GetRatio()));
    //         }
        
    //         bObj.push_back(Pair("AbsoluteYesCount",  (int64_t)pbudgetProposal->GetYesCount()-(int64_t)pbudgetProposal->GetNoCount()));
    //         bObj.push_back(Pair("YesCount",  (int64_t)pbudgetProposal->GetYesCount()));
    //         bObj.push_back(Pair("NoCount",  (int64_t)pbudgetProposal->GetNoCount()));
            
    //         if(strShow == "extended")
    //         {
    //             bObj.push_back(Pair("AbstainCount",  (int64_t)pbudgetProposal->GetAbstainCount()));
    //             bObj.push_back(Pair("TotalPayment",  ValueFromAmount(pbudgetProposal->GetAmount()*pbudgetProposal->GetTotalPaymentCount())));
    //         }

    //         bObj.push_back(Pair("MonthlyPayment",  ValueFromAmount(pbudgetProposal->GetAmount())));

    //         if(strShow == "extended")
    //         {
    //             bObj.push_back(Pair("IsEstablished",  pbudgetProposal->IsEstablished()));

    //             std::string strError = "";
    //             bObj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(pindex, strError)));
    //             bObj.push_back(Pair("IsValidReason",  strError.c_str()));
    //             bObj.push_back(Pair("fValid",  pbudgetProposal->fValid));
    //         }

    //         resultObj.push_back(Pair(pbudgetProposal->GetName(), bObj));
    //     }

    //     return resultObj;
    // }

    // if(strCommand == "get")
    // {
    //     if (params.size() != 2)
    //         throw runtime_error("Correct usage is 'proposal get <proposal-hash>'");

    //     uint256 hash = ParseHashV(params[1], "Proposal hash");

    //     CGovernanceNode* pbudgetProposal = govman.FindGovernanceObject(hash);

    //     if(pbudgetProposal == NULL) return "Unknown proposal";

    //     CBlockIndex* pindex;
    //     {
    //         LOCK(cs_main);
    //         pindex = chainActive.Tip();
    //     }

    //     CTxDestination address1;
    //     ExtractDestination(pbudgetProposal->GetPayee(), address1);
    //     CBitcoinAddress address2(address1);

    //     LOCK(cs_main);
    //     UniValue obj(UniValue::VOBJ);
    //     obj.push_back(Pair("Name",  pbudgetProposal->GetName()));
    //     obj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString()));
    //     obj.push_back(Pair("FeeHash",  pbudgetProposal->nFeeTXHash.ToString()));
    //     obj.push_back(Pair("URL",  pbudgetProposal->GetURL()));
    //     obj.push_back(Pair("BlockStart",  (int64_t)pbudgetProposal->GetBlockStart()));
    //     obj.push_back(Pair("BlockEnd",    (int64_t)pbudgetProposal->GetBlockEnd()));
    //     obj.push_back(Pair("TotalPaymentCount",  (int64_t)pbudgetProposal->GetTotalPaymentCount()));
    //     obj.push_back(Pair("RemainingPaymentCount",  (int64_t)pbudgetProposal->GetRemainingPaymentCount(pindex->nHeight)));
    //     obj.push_back(Pair("PaymentAddress",   address2.ToString()));
    //     obj.push_back(Pair("Ratio",  pbudgetProposal->GetRatio()));
    //     obj.push_back(Pair("AbsoluteYesCount",  (int64_t)pbudgetProposal->GetYesCount()-(int64_t)pbudgetProposal->GetNoCount()));
    //     obj.push_back(Pair("YesCount",  (int64_t)pbudgetProposal->GetYesCount()));
    //     obj.push_back(Pair("NoCount",  (int64_t)pbudgetProposal->GetNoCount()));
    //     obj.push_back(Pair("AbstainCount",  (int64_t)pbudgetProposal->GetAbstainCount()));
    //     obj.push_back(Pair("TotalPayment",  ValueFromAmount(pbudgetProposal->GetAmount()*pbudgetProposal->GetTotalPaymentCount())));
    //     obj.push_back(Pair("MonthlyPayment",  ValueFromAmount(pbudgetProposal->GetAmount())));
        
    //     obj.push_back(Pair("IsEstablished",  pbudgetProposal->IsEstablished()));

    //     std::string strError = "";
    //     obj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(chainActive.Tip(), strError)));
    //     obj.push_back(Pair("fValid",  pbudgetProposal->fValid));

    //     return obj;
    // }


    // if(strCommand == "gethash")
    // {
    //     if (params.size() != 2)
    //         throw runtime_error("Correct usage is 'proposal gethash <proposal-name>'");

    //     std::string strName = SanitizeString(params[1].get_str());

    //     CGovernanceNode* pbudgetProposal = govman.FindGovernanceObject(strName);

    //     if(pbudgetProposal == NULL) return "Unknown proposal";

    //     UniValue resultObj(UniValue::VOBJ);

    //     std::vector<CGovernanceNode*> winningProps = govman.FindMatchingGovernanceObjects(Proposal);
    //     BOOST_FOREACH(CGovernanceNode* pbudgetProposal, winningProps)
    //     {
    //         if(pbudgetProposal->GetName() != strName) continue;
    //         if(!pbudgetProposal->fValid) continue;

    //         CTxDestination address1;
    //         ExtractDestination(pbudgetProposal->GetPayee(), address1);
    //         CBitcoinAddress address2(address1);

    //         resultObj.push_back(Pair(pbudgetProposal->GetHash().ToString(), pbudgetProposal->GetHash().ToString()));
    //     }

    //     if(resultObj.size() > 1) return resultObj;

    //     return pbudgetProposal->GetHash().ToString();
    // }

    return NullUniValue;
}

UniValue contract(const UniValue& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "prepare" && strCommand != "submit" &&
         strCommand != "get" && strCommand != "gethash" && strCommand != "list"))
        throw runtime_error(
                "contract \"command\"...\n"
                "Manage contracts\n"
                "\nAvailable commands:\n"
                "  prepare            - Prepare contract by signing and creating tx\n"
                "  submit             - Submit contract to network\n"
                "  list               - List all contracts\n"
                "  get                - get contract\n"
                "  gethash            - Get contract hash(es) by contract name\n"
                );

    // if(strCommand == "prepare")
    // {
    //     if (params.size() != 7)
    //         throw runtime_error("Correct usage is 'contract prepare <contract-name> <url> <month-count> <block-start> <dash-address> <monthly-payment-dash>'");

    //     int nBlockMin = 0;
    //     LOCK(cs_main);
    //     CBlockIndex* pindex = chainActive.Tip();

    //     std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
    //     mnEntries = masternodeConfig.getEntries();

    //     std::string strName = SanitizeString(params[1].get_str());
    //     std::string strURL = SanitizeString(params[2].get_str());
    //     int nMonthCount = params[3].get_int();
    //     int nBlockStart = params[4].get_int();

    //     //set block min
    //     if(pindex != NULL) nBlockMin = pindex->nHeight;

    //     if(nBlockStart < nBlockMin)
    //         return "Invalid block start, must be more than current height.";

    //     CBitcoinAddress address(params[5].get_str());
    //     if (!address.IsValid())
    //         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Dash address");

    //     // Parse Dash address
    //     CScript scriptPubKey = GetScriptForDestination(address.Get());
    //     CAmount nAmount = AmountFromValue(params[6]);

    //     //*************************************************************************

    //     // create transaction 15 minutes into the future, to allow for confirmation time
    //     CGovernanceNodeBroadcast contract;
    //     contract.CreateContract(strName, strURL, nMonthCount, scriptPubKey, nAmount, nBlockStart, uint256());

    //     std::string strError = "";
    //     if(!contract.IsValid(pindex, strError, false))
    //         return "Contract is not valid - " + contract.GetHash().ToString() + " - " + strError;

    //     CWalletTx wtx;
    //     if(!pwalletMain->GetBudgetSystemCollateralTX(wtx, contract.GetHash(), false)){
    //         return "Error making collateral transaction for proposal. Please check your wallet balance and make sure your wallet is unlocked.";
    //     }

    //     // make our change address
    //     CReserveKey reservekey(pwalletMain);
    //     //send the tx to the network
    //     pwalletMain->CommitTransaction(wtx, reservekey, NetMsgType::TX);

    //     return wtx.GetHash().ToString();
    // }

    // if(strCommand == "submit")
    // {
    //     if (params.size() != 8)
    //         throw runtime_error("Correct usage is 'contract submit <contract-name> <url> <payment-count> <block-start> <dash-address> <monthly-payment-dash> <fee-tx>'");

    //     if(!masternodeSync.IsBlockchainSynced()){
    //         return "Must wait for client to sync with masternode network. Try again in a minute or so.";
    //     }

    //     int nBlockMin = 0;
    //     LOCK(cs_main);
    //     CBlockIndex* pindex = chainActive.Tip();

    //     std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
    //     mnEntries = masternodeConfig.getEntries();

    //     std::string strName = SanitizeString(params[1].get_str());
    //     std::string strURL = SanitizeString(params[2].get_str());
    //     int nPaymentCount = params[3].get_int();
    //     int nBlockStart = params[4].get_int();

    //     //set block min
    //     if(pindex != NULL) nBlockMin = pindex->nHeight;

    //     if(nBlockStart < nBlockMin)
    //         return "Invalid payment count, must be more than current height.";

    //     CBitcoinAddress address(params[5].get_str());
    //     if (!address.IsValid())
    //         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Dash address");

    //     // Parse Dash address
    //     CScript scriptPubKey = GetScriptForDestination(address.Get());
    //     CAmount nAmount = AmountFromValue(params[6]);
    //     uint256 hash = ParseHashV(params[7], "Contract hash");

    //     //create the contract incase we're the first to make it
    //     CGovernanceNodeBroadcast contractBroadcast;
    //     contractBroadcast.CreateContract(strName, strURL, nPaymentCount, scriptPubKey, nAmount, nBlockStart, hash);

    //     std::string strError = "";

    //     if(!contractBroadcast.IsValid(pindex, strError)){
    //         return "Contract is not valid - " + contractBroadcast.GetHash().ToString() + " - " + strError;
    //     }

    //     int nConf = 0;
    //     if(!IsCollateralValid(hash, contractBroadcast.GetHash(), strError, contractBroadcast.nTime, nConf, GOVERNANCE_FEE_TX)){
    //         return "Contract FeeTX is not valid - " + hash.ToString() + " - " + strError;
    //     }

    //     govman.mapSeenGovernanceObjects.insert(make_pair(contractBroadcast.GetHash(), contractBroadcast));
    //     contractBroadcast.Relay();
    //     govman.AddGovernanceObject(contractBroadcast);

    //     return contractBroadcast.GetHash().ToString();

    // }

    // if(strCommand == "list")
    // {
    //     if (params.size() > 2)
    //         throw runtime_error("Correct usage is 'proposal list [valid]'");

    //     std::string strShow = "valid";
    //     if (params.size() == 2) strShow = params[1].get_str();

    //     CBlockIndex* pindex;
    //     {
    //         LOCK(cs_main);
    //         pindex = chainActive.Tip();
    //     }

    //     UniValue resultObj(UniValue::VOBJ);
    //     int64_t nTotalAllotted = 0;

    //     std::vector<CGovernanceNode*> winningProps = govman.FindMatchingGovernanceObjects(Proposal);
    //     BOOST_FOREACH(CGovernanceNode* pbudgetProposal, winningProps)
    //     {
    //         if(strShow == "valid" && !pbudgetProposal->fValid) continue;

    //         nTotalAllotted += pbudgetProposal->GetAllotted();

    //         CTxDestination address1;
    //         ExtractDestination(pbudgetProposal->GetPayee(), address1);
    //         CBitcoinAddress address2(address1);

    //         UniValue bObj(UniValue::VOBJ);
    //         bObj.push_back(Pair("Name",  pbudgetProposal->GetName()));
    //         bObj.push_back(Pair("URL",  pbudgetProposal->GetURL()));
    //         bObj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString()));
    //         bObj.push_back(Pair("FeeHash",  pbudgetProposal->nFeeTXHash.ToString()));
    //         bObj.push_back(Pair("BlockStart",  (int64_t)pbudgetProposal->GetBlockStart()));
    //         bObj.push_back(Pair("BlockEnd",    (int64_t)pbudgetProposal->GetBlockEnd()));
    //         bObj.push_back(Pair("TotalPaymentCount",  (int64_t)pbudgetProposal->GetTotalPaymentCount()));
    //         bObj.push_back(Pair("RemainingPaymentCount",  (int64_t)pbudgetProposal->GetRemainingPaymentCount(pindex->nHeight)));
    //         bObj.push_back(Pair("PaymentAddress",   address2.ToString()));
    //         bObj.push_back(Pair("Ratio",  pbudgetProposal->GetRatio()));
    //         bObj.push_back(Pair("AbsoluteYesCount",  (int64_t)pbudgetProposal->GetYesCount()-(int64_t)pbudgetProposal->GetNoCount()));
    //         bObj.push_back(Pair("YesCount",  (int64_t)pbudgetProposal->GetYesCount()));
    //         bObj.push_back(Pair("NoCount",  (int64_t)pbudgetProposal->GetNoCount()));
    //         bObj.push_back(Pair("AbstainCount",  (int64_t)pbudgetProposal->GetAbstainCount()));
    //         bObj.push_back(Pair("TotalPayment",  ValueFromAmount(pbudgetProposal->GetAmount()*pbudgetProposal->GetTotalPaymentCount())));
    //         bObj.push_back(Pair("MonthlyPayment",  ValueFromAmount(pbudgetProposal->GetAmount())));

    //         bObj.push_back(Pair("IsEstablished",  pbudgetProposal->IsEstablished()));

    //         std::string strError = "";
    //         bObj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(pindex, strError)));
    //         bObj.push_back(Pair("IsValidReason",  strError.c_str()));
    //         bObj.push_back(Pair("fValid",  pbudgetProposal->fValid));

    //         resultObj.push_back(Pair(pbudgetProposal->GetName(), bObj));
    //     }

    //     return resultObj;
    // }

    // if(strCommand == "get")
    // {
    //     if (params.size() != 2)
    //         throw runtime_error("Correct usage is 'proposal get <proposal-hash>'");

    //     uint256 hash = ParseHashV(params[1], "Proposal hash");

    //     CGovernanceNode* pbudgetProposal = govman.FindGovernanceObject(hash);

    //     if(pbudgetProposal == NULL) return "Unknown proposal";

    //     CBlockIndex* pindex;
    //     {
    //         LOCK(cs_main);
    //         pindex = chainActive.Tip();
    //     }

    //     CTxDestination address1;
    //     ExtractDestination(pbudgetProposal->GetPayee(), address1);
    //     CBitcoinAddress address2(address1);

    //     LOCK(cs_main);
    //     UniValue obj(UniValue::VOBJ);
    //     obj.push_back(Pair("Name",  pbudgetProposal->GetName()));
    //     obj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString()));
    //     obj.push_back(Pair("FeeHash",  pbudgetProposal->nFeeTXHash.ToString()));
    //     obj.push_back(Pair("URL",  pbudgetProposal->GetURL()));
    //     obj.push_back(Pair("BlockStart",  (int64_t)pbudgetProposal->GetBlockStart()));
    //     obj.push_back(Pair("BlockEnd",    (int64_t)pbudgetProposal->GetBlockEnd()));
    //     obj.push_back(Pair("TotalPaymentCount",  (int64_t)pbudgetProposal->GetTotalPaymentCount()));
    //     obj.push_back(Pair("RemainingPaymentCount",  (int64_t)pbudgetProposal->GetRemainingPaymentCount(pindex->nHeight)));
    //     obj.push_back(Pair("PaymentAddress",   address2.ToString()));
    //     obj.push_back(Pair("Ratio",  pbudgetProposal->GetRatio()));
    //     obj.push_back(Pair("AbsoluteYesCount",  (int64_t)pbudgetProposal->GetYesCount()-(int64_t)pbudgetProposal->GetNoCount()));
    //     obj.push_back(Pair("YesCount",  (int64_t)pbudgetProposal->GetYesCount()));
    //     obj.push_back(Pair("NoCount",  (int64_t)pbudgetProposal->GetNoCount()));
    //     obj.push_back(Pair("AbstainCount",  (int64_t)pbudgetProposal->GetAbstainCount()));
    //     obj.push_back(Pair("TotalPayment",  ValueFromAmount(pbudgetProposal->GetAmount()*pbudgetProposal->GetTotalPaymentCount())));
    //     obj.push_back(Pair("MonthlyPayment",  ValueFromAmount(pbudgetProposal->GetAmount())));
        
    //     obj.push_back(Pair("IsEstablished",  pbudgetProposal->IsEstablished()));

    //     std::string strError = "";
    //     obj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(chainActive.Tip(), strError)));
    //     obj.push_back(Pair("fValid",  pbudgetProposal->fValid));

    //     return obj;
    // }


    // if(strCommand == "gethash")
    // {
    //     if (params.size() != 2)
    //         throw runtime_error("Correct usage is 'proposal gethash <proposal-name>'");

    //     std::string strName = SanitizeString(params[1].get_str());

    //     CGovernanceNode* pbudgetProposal = govman.FindGovernanceObject(strName);

    //     if(pbudgetProposal == NULL) return "Unknown proposal";

    //     UniValue resultObj(UniValue::VOBJ);

    //     std::vector<CGovernanceNode*> winningProps = govman.FindMatchingGovernanceObjects(Proposal);
    //     BOOST_FOREACH(CGovernanceNode* pbudgetProposal, winningProps)
    //     {
    //         if(pbudgetProposal->GetName() != strName) continue;
    //         if(!pbudgetProposal->fValid) continue;

    //         CTxDestination address1;
    //         ExtractDestination(pbudgetProposal->GetPayee(), address1);
    //         CBitcoinAddress address2(address1);

    //         resultObj.push_back(Pair(pbudgetProposal->GetHash().ToString(), pbudgetProposal->GetHash().ToString()));
    //     }

    //     if(resultObj.size() > 1) return resultObj;

    //     return pbudgetProposal->GetHash().ToString();
    // }

    return NullUniValue;
}

UniValue budget(const UniValue& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();
    string strSubCommand;
    if (params.size() >= 2)
        strSubCommand = params[1].get_str();

    if (fHelp  ||
        (strCommand != "check" && strCommand != "get" && strCommand != "all" && 
            strCommand != "valid" && strCommand != "extended" && strCommand != "projection"))
        throw runtime_error(
                "budget \"command\"...\n"
                "Manage proposals\n"
                "\nAvailable commands:\n"
                "  check              - Scan proposals and remove invalid from proposals list\n"
                "  get                - Get a proposals|contract by hash\n"
                "  list               - Get all proposals (all|valid|extended)\n"
                "  projection         - Show the projection of which proposals will be paid the next superblocks\n"
                );

    // help messages
    if(strSubCommand == "help")
    {
        if(strCommand == "list")
        {
            throw runtime_error(
                "budget \"list\"...\n"
                "List all budgets in various configurations\n"
                "\nAvailable commands:\n"
                "  all            - Show all proposed settings for the network\n"
                "  valid          - Show only valid settings for the network\n"
                "  extended       - Show extended information about all settings\n"
                "  active         - Show active values for each network setting\n"
                );
        }
    }

    // if(strCommand == "check")
    // {
    //     govman.CheckAndRemove();

    //     return "Success";
    // }

    // if(strCommand == "projection")
    // {
    //     CBlockIndex* pindex;
    //     {
    //         LOCK(cs_main);
    //         pindex = chainActive.Tip();
    //     }

    //     UniValue resultObj(UniValue::VOBJ);
    //     CAmount nTotalAllotted = 0;

    //     std::vector<CGovernanceNode*> winningProps = govman.GetBudget();
    //     BOOST_FOREACH(CGovernanceNode* pbudgetProposal, winningProps)
    //     {
    //         nTotalAllotted += pbudgetProposal->GetAllotted();

    //         CTxDestination address1;
    //         ExtractDestination(pbudgetProposal->GetPayee(), address1);
    //         CBitcoinAddress address2(address1);

    //         UniValue bObj(UniValue::VOBJ);
    //         bObj.push_back(Pair("URL",  pbudgetProposal->GetURL()));
    //         bObj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString()));
    //         bObj.push_back(Pair("BlockStart",  (int64_t)pbudgetProposal->GetBlockStart()));
    //         bObj.push_back(Pair("BlockEnd",    (int64_t)pbudgetProposal->GetBlockEnd()));
    //         bObj.push_back(Pair("TotalPaymentCount",  (int64_t)pbudgetProposal->GetTotalPaymentCount()));
    //         bObj.push_back(Pair("RemainingPaymentCount",  (int64_t)pbudgetProposal->GetRemainingPaymentCount(pindex->nHeight)));
    //         bObj.push_back(Pair("PaymentAddress",   address2.ToString()));
    //         bObj.push_back(Pair("Ratio",  pbudgetProposal->GetRatio()));
    //         bObj.push_back(Pair("AbsoluteYesCount",  (int64_t)pbudgetProposal->GetYesCount()-(int64_t)pbudgetProposal->GetNoCount()));
    //         bObj.push_back(Pair("YesCount",  (int64_t)pbudgetProposal->GetYesCount()));
    //         bObj.push_back(Pair("NoCount",  (int64_t)pbudgetProposal->GetNoCount()));
    //         bObj.push_back(Pair("AbstainCount",  (int64_t)pbudgetProposal->GetAbstainCount()));
    //         bObj.push_back(Pair("TotalPayment",  ValueFromAmount(pbudgetProposal->GetAmount()*pbudgetProposal->GetTotalPaymentCount())));
    //         bObj.push_back(Pair("MonthlyPayment",  ValueFromAmount(pbudgetProposal->GetAmount())));
    //         bObj.push_back(Pair("Alloted",  ValueFromAmount(pbudgetProposal->GetAllotted())));

    //         std::string strError = "";
    //         bObj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(pindex, strError)));
    //         bObj.push_back(Pair("IsValidReason",  strError.c_str()));
    //         bObj.push_back(Pair("fValid",  pbudgetProposal->fValid));

    //         resultObj.push_back(Pair(pbudgetProposal->GetName(), bObj));
    //     }
    //     resultObj.push_back(Pair("TotalBudgetAlloted",  ValueFromAmount(nTotalAllotted)));

    //     return resultObj;
    // }

    // if(strCommand == "get")
    // {
    //     uint256 nMatchHash = uint256();
    //     std::string strMatchName = "";

    //     if (IsHex(params[1].get_str()))
    //     {
    //         nMatchHash = ParseHashV(params[1], "GovObj hash");
    //     } else {
    //         strMatchName = params[1].get_str();
    //     }

    //     CBlockIndex* pindex;
    //     {
    //         LOCK(cs_main);
    //         pindex = chainActive.Tip();
    //     }

    //     UniValue resultObj(UniValue::VOBJ);
    //     int64_t nTotalAllotted = 0;

    //     std::vector<CGovernanceNode*> winningProps = govman.FindMatchingGovernanceObjects(AllTypes);
        
    //     BOOST_FOREACH(CGovernanceNode* pbudgetProposal, winningProps)
    //     {
    //         if(pbudgetProposal->GetName() != strMatchName && pbudgetProposal->GetHash() != nMatchHash) continue;

    //         CTxDestination address1;
    //         ExtractDestination(pbudgetProposal->GetPayee(), address1);
    //         CBitcoinAddress address2(address1);

    //         UniValue bObj(UniValue::VOBJ);
    //         bObj.push_back(Pair("Type",  pbudgetProposal->GetGovernanceTypeAsString()));
    //         bObj.push_back(Pair("Name",  pbudgetProposal->GetName()));
    //         bObj.push_back(Pair("URL",  pbudgetProposal->GetURL()));
    //         bObj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString()));
    //         bObj.push_back(Pair("FeeHash",  pbudgetProposal->nFeeTXHash.ToString()));
    //         bObj.push_back(Pair("BlockStart",  (int64_t)pbudgetProposal->GetBlockStart()));
    //         bObj.push_back(Pair("BlockEnd",    (int64_t)pbudgetProposal->GetBlockEnd()));
    //         bObj.push_back(Pair("TotalPaymentCount",  (int64_t)pbudgetProposal->GetTotalPaymentCount()));
    //         bObj.push_back(Pair("RemainingPaymentCount",  (int64_t)pbudgetProposal->GetRemainingPaymentCount(pindex->nHeight)));
    //         bObj.push_back(Pair("PaymentAddress",   address2.ToString()));
    //         bObj.push_back(Pair("Ratio",  pbudgetProposal->GetRatio()));
    //         bObj.push_back(Pair("AbsoluteYesCount",  (int64_t)pbudgetProposal->GetYesCount()-(int64_t)pbudgetProposal->GetNoCount()));
    //         bObj.push_back(Pair("YesCount",  (int64_t)pbudgetProposal->GetYesCount()));
    //         bObj.push_back(Pair("NoCount",  (int64_t)pbudgetProposal->GetNoCount()));
            
    //         bObj.push_back(Pair("AbstainCount",  (int64_t)pbudgetProposal->GetAbstainCount()));
    //         bObj.push_back(Pair("TotalPayment",  ValueFromAmount(pbudgetProposal->GetAmount()*pbudgetProposal->GetTotalPaymentCount())));

    //         bObj.push_back(Pair("MonthlyPayment",  ValueFromAmount(pbudgetProposal->GetAmount())));
    //         bObj.push_back(Pair("IsEstablished",  pbudgetProposal->IsEstablished()));

    //         std::string strError = "";
    //         bObj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(pindex, strError)));
    //         bObj.push_back(Pair("IsValidReason",  strError.c_str()));
    //         bObj.push_back(Pair("fValid",  pbudgetProposal->fValid));
            
    //         resultObj.push_back(Pair(pbudgetProposal->GetName(), bObj));
    //     }

    //     return resultObj;
    // }

    // if(strCommand == "all" || strCommand == "valid" || strCommand == "extended") 
    // {

    //     CBlockIndex* pindex;
    //     {
    //         LOCK(cs_main);
    //         pindex = chainActive.Tip();
    //     }

    //     UniValue resultObj(UniValue::VOBJ);
    //     int64_t nTotalAllotted = 0;

    //     std::vector<CGovernanceNode*> winningProps = govman.FindMatchingGovernanceObjects(AllTypes);
        
    //     BOOST_FOREACH(CGovernanceNode* pbudgetProposal, winningProps)
    //     {
    //         if(strCommand == "valid" && !pbudgetProposal->fValid) continue;

    //         nTotalAllotted += pbudgetProposal->GetAllotted();

    //         CTxDestination address1;
    //         ExtractDestination(pbudgetProposal->GetPayee(), address1);
    //         CBitcoinAddress address2(address1);

    //         UniValue bObj(UniValue::VOBJ);
    //         bObj.push_back(Pair("Name",  pbudgetProposal->GetName()));
            
    //         if(strCommand == "extended")
    //         {
    //             bObj.push_back(Pair("URL",  pbudgetProposal->GetURL()));
    //             bObj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString()));
    //             bObj.push_back(Pair("FeeHash",  pbudgetProposal->nFeeTXHash.ToString()));
    //             bObj.push_back(Pair("BlockStart",  (int64_t)pbudgetProposal->GetBlockStart()));
    //             bObj.push_back(Pair("BlockEnd",    (int64_t)pbudgetProposal->GetBlockEnd()));
    //             bObj.push_back(Pair("TotalPaymentCount",  (int64_t)pbudgetProposal->GetTotalPaymentCount()));
    //             bObj.push_back(Pair("RemainingPaymentCount",  (int64_t)pbudgetProposal->GetRemainingPaymentCount(pindex->nHeight)));
    //             bObj.push_back(Pair("PaymentAddress",   address2.ToString()));
    //             bObj.push_back(Pair("Ratio",  pbudgetProposal->GetRatio()));
    //         }

    //         bObj.push_back(Pair("AbsoluteYesCount",  (int64_t)pbudgetProposal->GetYesCount()-(int64_t)pbudgetProposal->GetNoCount()));
    //         bObj.push_back(Pair("YesCount",  (int64_t)pbudgetProposal->GetYesCount()));
    //         bObj.push_back(Pair("NoCount",  (int64_t)pbudgetProposal->GetNoCount()));
            
    //         if(strCommand == "extended")
    //         {
    //             bObj.push_back(Pair("AbstainCount",  (int64_t)pbudgetProposal->GetAbstainCount()));
    //             bObj.push_back(Pair("TotalPayment",  ValueFromAmount(pbudgetProposal->GetAmount()*pbudgetProposal->GetTotalPaymentCount())));
    //         }

    //         bObj.push_back(Pair("MonthlyPayment",  ValueFromAmount(pbudgetProposal->GetAmount())));
    //         bObj.push_back(Pair("IsEstablished",  pbudgetProposal->IsEstablished()));

    //         if(strCommand == "extended")
    //         {
    //             std::string strError = "";
    //             bObj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(pindex, strError)));
    //             bObj.push_back(Pair("IsValidReason",  strError.c_str()));
    //             bObj.push_back(Pair("fValid",  pbudgetProposal->fValid));
    //         }

    //         resultObj.push_back(Pair(pbudgetProposal->GetName(), bObj));
    //     }

    //     return resultObj;
    // }

    return NullUniValue;
}

UniValue superblock(const UniValue& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "vote-many" && strCommand != "vote" && strCommand != "show" && strCommand != "getvotes" && strCommand != "prepare" && strCommand != "submit"))
        throw runtime_error(
                "superblock \"command\"...\n"
                "Get information about the next superblocks\n"
                "\nAvailable commands:\n"
                "  info   - Get info about the next superblock\n"
                "  getvotes    - Get vote information for each finalized budget\n"
                "  prepare     - Manually prepare a finalized budget\n"
                "  submit      - Manually submit a finalized budget\n"
                );


    // if(strCommand == "nextblock")
    // {
    //     LOCK(cs_main);
    //     CBlockIndex* pindex = chainActive.Tip();
    //     if(!pindex) return "unknown";

    //     int nNext = pindex->nHeight - pindex->nHeight % Params().GetConsensus().nBudgetPaymentsCycleBlocks + Params().GetConsensus().nBudgetPaymentsCycleBlocks;
    //     return nNext;
    // }

    // if(strCommand == "nextsuperblocksize")
    // {
    //     LOCK(cs_main);
    //     CBlockIndex* pindex = chainActive.Tip();
    //     if(!pindex) return "unknown";

    //     int nHeight = pindex->nHeight - pindex->nHeight % Params().GetConsensus().nBudgetPaymentsCycleBlocks + Params().GetConsensus().nBudgetPaymentsCycleBlocks;

    //     CAmount nTotal = govman.GetTotalBudget(nHeight);
    //     return nTotal;
    // }

    // if(strCommand == "get")
    // {
    //     UniValue resultObj(UniValue::VOBJ);

    //     std::vector<CFinalizedBudget*> winningFbs = govman.GetFinalizedBudgets();
    //     LOCK(cs_main);
    //     BOOST_FOREACH(CFinalizedBudget* finalizedBudget, winningFbs)
    //     {
    //         UniValue bObj(UniValue::VOBJ);
    //         bObj.push_back(Pair("FeeTX",  finalizedBudget->nFeeTXHash.ToString()));
    //         bObj.push_back(Pair("Hash",  finalizedBudget->GetHash().ToString()));
    //         bObj.push_back(Pair("BlockStart",  (int64_t)finalizedBudget->GetBlockStart()));
    //         bObj.push_back(Pair("BlockEnd",    (int64_t)finalizedBudget->GetBlockEnd()));
    //         bObj.push_back(Pair("Proposals",  finalizedBudget->GetProposals()));
    //         bObj.push_back(Pair("VoteCount",  (int64_t)finalizedBudget->GetVoteCount()));
    //         bObj.push_back(Pair("Status",  finalizedBudget->GetStatus()));

    //         std::string strError = "";
    //         bObj.push_back(Pair("IsValid",  finalizedBudget->IsValid(chainActive.Tip(), strError)));
    //         bObj.push_back(Pair("IsValidReason",  strError.c_str()));

    //         resultObj.push_back(Pair(finalizedBudget->GetName(), bObj));
    //     }

    //     return resultObj;

    // }

    // /* TODO 
    //     Switch the preparation to a public key which the core team has
    //     The budget should be able to be created by any high up core team member then voted on by the network separately. 
    // */

    // if(strCommand == "prepare")
    // {
    //     if (params.size() != 2)
    //         throw runtime_error("Correct usage is 'mnfinalbudget prepare comma-separated-hashes'");

    //     std::string strHashes = params[1].get_str();
    //     std::istringstream ss(strHashes);
    //     std::string token;

    //     std::vector<CTxBudgetPayment> vecTxBudgetPayments;

    //     while(std::getline(ss, token, ',')) {
    //         uint256 nHash = uint256S(token);
    //         CGovernanceNode* prop = govman.FindGovernanceObject(nHash);

    //         CTxBudgetPayment txBudgetPayment;
    //         txBudgetPayment.nProposalHash = prop->GetHash();
    //         txBudgetPayment.payee = prop->GetPayee();
    //         txBudgetPayment.nAmount = prop->GetAllotted();
    //         vecTxBudgetPayments.push_back(txBudgetPayment);
    //     }

    //     if(vecTxBudgetPayments.size() < 1) {
    //         return "Invalid finalized proposal";
    //     }

    //     LOCK(cs_main);
    //     CBlockIndex* pindex = chainActive.Tip();
    //     if(!pindex) return "invalid chaintip";

    //     int nBlockStart = pindex->nHeight - pindex->nHeight % Params().GetConsensus().nBudgetPaymentsCycleBlocks + Params().GetConsensus().nBudgetPaymentsCycleBlocks;

    //     CFinalizedBudget tempBudget("main", nBlockStart, vecTxBudgetPayments, uint256());
    //     // if(mapSeenFinalizedBudgets.count(tempBudget.GetHash())) {
    //     //     return "already exists"; //already exists
    //     // }

    //     //create fee tx
    //     CTransaction tx;
        
    //     CWalletTx wtx;
    //     if(!pwalletMain->GetBudgetSystemCollateralTX(wtx, tempBudget.GetHash(), false)){
    //         printf("Can't make collateral transaction\n");
    //         return "can't make colateral tx";
    //     }
        
    //     // make our change address
    //     CReserveKey reservekey(pwalletMain);
    //     //send the tx to the network
    //     pwalletMain->CommitTransaction(wtx, reservekey, NetMsgType::IX);

    //     return wtx.GetHash().ToString();
    // }

    // if(strCommand == "submit")
    // {
    //     if (params.size() != 3)
    //         throw runtime_error("Correct usage is 'mnfinalbudget submit comma-separated-hashes collateralhash'");

    //     std::string strHashes = params[1].get_str();
    //     std::istringstream ss(strHashes);
    //     std::string token;

    //     std::vector<CTxBudgetPayment> vecTxBudgetPayments;

    //     uint256 nColHash = uint256S(params[2].get_str());

    //     while(std::getline(ss, token, ',')) {
    //         uint256 nHash = uint256S(token);
    //         CGovernanceNode* prop = govman.FindGovernanceObject(nHash);

    //         CTxBudgetPayment txBudgetPayment;
    //         txBudgetPayment.nProposalHash = prop->GetHash();
    //         txBudgetPayment.payee = prop->GetPayee();
    //         txBudgetPayment.nAmount = prop->GetAllotted();
    //         vecTxBudgetPayments.push_back(txBudgetPayment);

    //         printf("%lld\n", txBudgetPayment.nAmount);
    //     }

    //     if(vecTxBudgetPayments.size() < 1) {
    //         return "Invalid finalized proposal";
    //     }

    //     LOCK(cs_main);
    //     CBlockIndex* pindex = chainActive.Tip();
    //     if(!pindex) return "invalid chaintip";

    //     int nBlockStart = pindex->nHeight - pindex->nHeight % Params().GetConsensus().nBudgetPaymentsCycleBlocks + Params().GetConsensus().nBudgetPaymentsCycleBlocks;
      
    //     // CTxIn in(COutPoint(nColHash, 0));
    //     // int conf = GetInputAgeIX(nColHash, in);
        
    //     //     Wait will we have 1 extra confirmation, otherwise some clients might reject this feeTX
    //     //     -- This function is tied to NewBlock, so we will propagate this budget while the block is also propagating
        
    //     // if(conf < BUDGET_FEE_CONFIRMATIONS+1){
    //     //     printf ("Collateral requires at least %d confirmations - %s - %d confirmations\n", BUDGET_FEE_CONFIRMATIONS, nColHash.ToString().c_str(), conf);
    //     //     return "invalid collateral";
    //     // }

    //     //create the proposal incase we're the first to make it
    //     CFinalizedBudget finalizedBudgetBroadcast("main", nBlockStart, vecTxBudgetPayments, nColHash);

    //     std::string strError = "";
    //     if(!finalizedBudgetBroadcast.IsValid(pindex, strError)){
    //         printf("CGovernanceManager::SubmitFinalBudget - Invalid finalized budget - %s \n", strError.c_str());
    //         return "invalid finalized budget";
    //     }

    //     finalizedBudgetBroadcast.Relay();
    //     govman.AddFinalizedBudget(finalizedBudgetBroadcast);

    //     return finalizedBudgetBroadcast.GetHash().ToString();
    // }

    return NullUniValue;
}
