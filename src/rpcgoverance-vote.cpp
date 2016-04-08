// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "db.h"
#include "init.h"
#include "activemasternode.h"
#include "masternode-governance.h"
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

UniValue vote(const UniValue& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "many" && strCommand != "alias" && strCommand != "get" && strCommand != "one"))
        throw runtime_error(
                "vote \"command\"...\n"
                "Vote on proposals, contracts, switches or settings\n"
                "\nAvailable commands:\n"
                "              - Prepare governance object by signing and creating tx\n"
                "  alias         - Vote on a governance object by alias\n"
                "  get           - Show detailed votes list for governance object\n"
                "  many          - Vote on a governance object by all masternodes (using masternode.conf setup)\n"
                "  once          - Vote on a governance object by single masternode (using dash.conf setup)\n"
                "  raw           - Submit raw governance object vote (used in trustless governance implementations)\n"
                );



    // if(strCommand == "get")
    // {
    //     if (params.size() != 2)
    //         throw runtime_error("Correct usage is 'vote get <governance-hash>'");

    //     uint256 hash = ParseHashV(params[1], "Governance hash");

    //     UniValue obj(UniValue::VOBJ);

    //     govman.GetGovernanceTypeByHash(hash);

    //     if(govman.GetGovernanceTypeByHash(hash) == Error)
    //     {
    //         return "Unknown governance object hash";
    //     }
    //     else if(govman.GetGovernanceTypeByHash(hash) == FinalizedBudget)
    //     {       
    //         CFinalizedBudget* pfinalBudget = govman.FindFinalizedBudget(hash);

    //         if(pfinalBudget == NULL) return "Unknown budget hash";

    //         std::map<uint256, CGovernanceVote>::iterator it = pfinalBudget->mapVotes.begin();
    //         while(it != pfinalBudget->mapVotes.end()){

    //             UniValue bObj(UniValue::VOBJ);
    //             bObj.push_back(Pair("nHash",  (*it).first.ToString().c_str()));
    //             bObj.push_back(Pair("nTime",  (int64_t)(*it).second.nTime));
    //             bObj.push_back(Pair("fValid",  (*it).second.fValid));

    //             obj.push_back(Pair((*it).second.vin.prevout.ToStringShort(), bObj));

    //             it++;
    //         }
    //     } 
    //     else // proposals, contracts, switches and settings
    //     { 
    //         CGovernanceObject* pGovernanceObj = govman.FindGovernanceObject(hash);

    //         if(pGovernanceObj == NULL) return "Unknown governance object hash";

    //         std::map<uint256, CGovernanceVote>::iterator it = pGovernanceObj->mapVotes.begin();
    //         while(it != pGovernanceObj->mapVotes.end()){

    //             UniValue bObj(UniValue::VOBJ);
    //             bObj.push_back(Pair("nHash",  (*it).first.ToString().c_str()));
    //             bObj.push_back(Pair("Vote",  (*it).second.GetVoteString()));
    //             bObj.push_back(Pair("nTime",  (int64_t)(*it).second.nTime));
    //             bObj.push_back(Pair("fValid",  (*it).second.fValid));

    //             obj.push_back(Pair((*it).second.vin.prevout.ToStringShort(), bObj));

    //             it++;
    //         }
    //     }

    //     return obj;
    // }

    // if(strCommand == "once")
    // {
    //     if (params.size() != 3)
    //         throw runtime_error("Correct usage is 'vote once <proposal-hash> <yes|no>'");

    //     uint256 hash = ParseHashV(params[1], "Proposal hash");
    //     std::string strVote = params[2].get_str();

    //     if(strVote != "yes" && strVote != "no") return "You can only vote 'yes' or 'no'";
    //     int nVote = VOTE_ABSTAIN;
    //     if(strVote == "yes") nVote = VOTE_YES;
    //     if(strVote == "no") nVote = VOTE_NO;

    //     CPubKey pubKeyMasternode;
    //     CKey keyMasternode;
    //     std::string errorMessage;

    //     if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
    //         return "Error upon calling SetKey";

    //     CMasternode* pmn = mnodeman.Find(activeMasternode.vin);
    //     if(pmn == NULL)
    //     {
    //         return "Failure to find masternode in list : " + activeMasternode.vin.ToString();
    //     }

    //     CGovernanceObject* goParent = govman.FindGovernanceObject(hash);
    //     if(!goParent)
    //     {
    //         return "Couldn't find governance obj";
    //     }

    //     CGovernanceVote vote(goParent, activeMasternode.vin, hash, nVote);
    //     if(!vote.Sign(keyMasternode, pubKeyMasternode)){
    //         return "Failure to sign.";
    //     }

    //     std::string strError = "";
    //     if(govman.UpdateGovernanceObjectVotes(vote, NULL, strError)){
    //         govman.mapSeenGovernanceVotes.insert(make_pair(vote.GetHash(), vote));
    //         vote.Relay();
    //         return "Voted successfully";
    //     } else {
    //         return "Error voting : " + strError;
    //     }
    // }


    // if(strCommand == "many")
    // {
    //     if(params.size() != 3)
    //         throw runtime_error("Correct usage is 'proposal vote-many <proposal-hash> <yes|no>'");

    //     uint256 hash;
    //     std::string strVote;

    //     hash = ParseHashV(params[1], "Proposal hash");
    //     strVote = params[2].get_str();

    //     if(strVote != "yes" && strVote != "no") return "You can only vote 'yes' or 'no'";
    //     int nVote = VOTE_ABSTAIN;
    //     if(strVote == "yes") nVote = VOTE_YES;
    //     if(strVote == "no") nVote = VOTE_NO;

    //     int success = 0;
    //     int failed = 0;

    //     std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
    //     mnEntries = masternodeConfig.getEntries();

    //     UniValue resultsObj(UniValue::VOBJ);

    //     BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
    //         std::string errorMessage;
    //         std::vector<unsigned char> vchMasterNodeSignature;
    //         std::string strMasterNodeSignMessage;

    //         CPubKey pubKeyCollateralAddress;
    //         CKey keyCollateralAddress;
    //         CPubKey pubKeyMasternode;
    //         CKey keyMasternode;

    //         UniValue statusObj(UniValue::VOBJ);

    //         if(!darkSendSigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)){
    //             failed++;
    //             statusObj.push_back(Pair("result", "failed"));
    //             statusObj.push_back(Pair("errorMessage", "Masternode signing error, could not set key correctly: " + errorMessage));
    //             resultsObj.push_back(Pair(mne.getAlias(), statusObj));
    //             continue;
    //         }

    //         CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
    //         if(pmn == NULL)
    //         {
    //             failed++;
    //             statusObj.push_back(Pair("result", "failed"));
    //             statusObj.push_back(Pair("errorMessage", "Can't find masternode by pubkey"));
    //             resultsObj.push_back(Pair(mne.getAlias(), statusObj));
    //             continue;
    //         }

    //         CGovernanceObject* goParent = govman.FindGovernanceObject(hash);
    //         if(!goParent)
    //         {
    //             failed++;
    //             statusObj.push_back(Pair("result", "failed"));
    //             statusObj.push_back(Pair("errorMessage", "Can't find governance object."));
    //             resultsObj.push_back(Pair(mne.getAlias(), statusObj));
    //         }

    //         CGovernanceVote vote(goParent, pmn->vin, hash, nVote);
    //         if(!vote.Sign(keyMasternode, pubKeyMasternode)){
    //             failed++;
    //             statusObj.push_back(Pair("result", "failed"));
    //             statusObj.push_back(Pair("errorMessage", "Failure to sign."));
    //             resultsObj.push_back(Pair(mne.getAlias(), statusObj));
    //             continue;
    //         }


    //         std::string strError = "";
    //         if(govman.UpdateGovernanceObjectVotes(vote, NULL, strError)) {
    //             govman.mapSeenGovernanceVotes.insert(make_pair(vote.GetHash(), vote));
    //             vote.Relay();
    //             success++;
    //             statusObj.push_back(Pair("result", "success"));
    //         } else {
    //             failed++;
    //             statusObj.push_back(Pair("result", strError.c_str()));
    //         }

    //         resultsObj.push_back(Pair(mne.getAlias(), statusObj));
    //     }

    //     UniValue returnObj(UniValue::VOBJ);
    //     returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed)));
    //     returnObj.push_back(Pair("detail", resultsObj));

    //     return returnObj;
    // }

    // if(strCommand == "alias")
    // {
    //     if(params.size() != 4)
    //         throw runtime_error("Correct usage is 'proposal vote-alias <proposal-hash> <yes|no> <alias-name>'");

    //     uint256 hash;
    //     std::string strVote;

    //     hash = ParseHashV(params[1], "Proposal hash");
    //     strVote = params[2].get_str();
    //     std::string strAlias = params[3].get_str();

    //     if(strVote != "yes" && strVote != "no") return "You can only vote 'yes' or 'no'";
    //     int nVote = VOTE_ABSTAIN;
    //     if(strVote == "yes") nVote = VOTE_YES;
    //     if(strVote == "no") nVote = VOTE_NO;

    //     int success = 0;
    //     int failed = 0;

    //     std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
    //     mnEntries = masternodeConfig.getEntries();

    //     UniValue resultsObj(UniValue::VOBJ);

    //     BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {

    //         if( strAlias != mne.getAlias()) continue;

    //         std::string errorMessage;
    //         std::vector<unsigned char> vchMasterNodeSignature;
    //         std::string strMasterNodeSignMessage;

    //         CPubKey pubKeyCollateralAddress;
    //         CKey keyCollateralAddress;
    //         CPubKey pubKeyMasternode;
    //         CKey keyMasternode;

    //         UniValue statusObj(UniValue::VOBJ);

    //         if(!darkSendSigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)){
    //             failed++;
    //             statusObj.push_back(Pair("result", "failed"));
    //             statusObj.push_back(Pair("errorMessage", "Masternode signing error, could not set key correctly: " + errorMessage));
    //             resultsObj.push_back(Pair(mne.getAlias(), statusObj));
    //             continue;
    //         }

    //         CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
    //         if(pmn == NULL)
    //         {
    //             failed++;
    //             statusObj.push_back(Pair("result", "failed"));
    //             statusObj.push_back(Pair("errorMessage", "Can't find masternode by pubkey"));
    //             resultsObj.push_back(Pair(mne.getAlias(), statusObj));
    //             continue;
    //         }
    
    //         CGovernanceObject* goParent = govman.FindGovernanceObject(hash);
    //         if(!goParent)
    //         {
    //             return "Couldn't find governance obj";
    //         }

    //         CGovernanceVote vote(goParent, pmn->vin, hash, nVote);
    //         if(!vote.Sign(keyMasternode, pubKeyMasternode)){
    //             failed++;
    //             statusObj.push_back(Pair("result", "failed"));
    //             statusObj.push_back(Pair("errorMessage", "Failure to sign."));
    //             resultsObj.push_back(Pair(mne.getAlias(), statusObj));
    //             continue;
    //         }


    //         std::string strError = "";
    //         if(govman.UpdateGovernanceObjectVotes(vote, NULL, strError)) {
    //             govman.mapSeenGovernanceVotes.insert(make_pair(vote.GetHash(), vote));
    //             vote.Relay();
    //             success++;
    //             statusObj.push_back(Pair("result", "success"));
    //         } else {
    //             failed++;
    //             statusObj.push_back(Pair("result", strError.c_str()));
    //         }

    //         resultsObj.push_back(Pair(mne.getAlias(), statusObj));
    //     }

    //     UniValue returnObj(UniValue::VOBJ);
    //     returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed)));
    //     returnObj.push_back(Pair("detail", resultsObj));

    //     return returnObj;
    // }

    // if(strCommand == "raw")
    // {
    //     if (fHelp || params.size() != 6)
    //         throw runtime_error(
    //                 "vote raw <masternode-tx-hash> <masternode-tx-index> <proposal-hash> <yes|no> <time> <vote-sig>\n"
    //                 "Compile and relay a governance object vote with provided external signature instead of signing vote internally\n"
    //                 );

    //     uint256 hashMnTx = ParseHashV(params[1], "mn tx hash");
    //     int nMnTxIndex = boost::lexical_cast<int>(params[2].get_str());

    //     CTxIn vin = CTxIn(hashMnTx, nMnTxIndex);

    //     uint256 hashProposal = ParseHashV(params[3], "Governance hash");
    //     std::string strVote = params[4].get_str();

    //     if(strVote != "yes" && strVote != "no") return "You can only vote 'yes' or 'no'";
    //     int nVote = VOTE_ABSTAIN;
    //     if(strVote == "yes") nVote = VOTE_YES;
    //     if(strVote == "no") nVote = VOTE_NO;

    //     int64_t nTime = boost::lexical_cast<int64_t>(params[5].get_str());
    //     std::string strSig = params[6].get_str();
    //     bool fInvalid = false;
    //     vector<unsigned char> vchSig = DecodeBase64(strSig.c_str(), &fInvalid);

    //     if (fInvalid)
    //         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    //     CMasternode* pmn = mnodeman.Find(vin);
    //     if(pmn == NULL)
    //     {
    //         return "Failure to find masternode in list : " + vin.ToString();
    //     }

    //     CGovernanceObject* goParent = govman.FindGovernanceObject(hashProposal);
    //     if(!goParent)
    //     {
    //         return "Couldn't find governance obj";
    //     }

    //     CGovernanceVote vote(goParent, vin, hashProposal, nVote);
    //     vote.nTime = nTime;
    //     vote.vchSig = vchSig;

    //     std::string strReason;
    //     if(!vote.IsValid(true, strReason)){
    //         return "Failure to verify vote - " + strReason;
    //     }

    //     std::string strError = "";
    //     if(govman.UpdateGovernanceObjectVotes(vote, NULL, strError)){
    //         govman.mapSeenGovernanceVotes.insert(make_pair(vote.GetHash(), vote));
    //         vote.Relay();
    //         return "Voted successfully";
    //     } else {
    //         return "Error voting : " + strError;
    //     }
    // }


    return NullUniValue;
}