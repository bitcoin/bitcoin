#include "rpcagents.h"
#include "rpcserver.h"
#include "rpcprotocol.h"
#include "masternodeconfig.h"
#include "key.h"
#include "legacysigner.h"
#include "univalue/univalue.h"
#include "primitives/transaction.h"
#include "platform/agent.h"
#include "platform/governance-vote.h"
#include "platform/specialtx.h"

namespace
{
    std::string GetCommand(const json_spirit::Array& params)
    {
        if (params.empty())
            throw std::runtime_error("wrong format");

        return params[0].get_str();
    }

    std::string SignAndSendSpecialTx(const CMutableTransaction& tx)
    {
        LOCK(cs_main);
        CValidationState state;
        if (!CheckSpecialTx(tx, NULL, state))
            throw std::runtime_error(FormatStateMessage(state));

        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << tx;

        json_spirit::Array signReqeust;
        signReqeust.push_back(HexStr(ds.begin(), ds.end()));
        json_spirit::Value signResult = signrawtransaction(signReqeust, false);

        json_spirit::Array sendRequest;
        sendRequest.push_back(json_spirit::find_value(signResult.get_obj(), "hex").get_str());
        return sendrawtransaction(sendRequest, false).get_str();
    }

}

json_spirit::Value agents(const json_spirit::Array& params, bool fHelp)
{
    std::string command = GetCommand(params);

    if (command == "vote")
        return detail::vote(params, fHelp);
    else if (command == "list")
        return detail::list(params, fHelp);
    else
        throw std::runtime_error("wrong format");
}

json_spirit::Value detail::vote(const json_spirit::Array& params, bool fHelp)
{
    std::vector<CNodeEntry> mnEntries;
    mnEntries = masternodeConfig.getEntries();

    if (params.size() != 3)
        throw std::runtime_error("Correct usage is 'mnbudget vote proposal-hash yes|no'");

    uint256 hash = ParseHashV(params[1], "parameter 1");
    std::string strVote = params[2].get_str();

    if(strVote != "yes" && strVote != "no") return "You can only vote 'yes' or 'no'";
    VoteTx::Value nVote = VoteTx::abstain;
    if(strVote == "yes") nVote = VoteTx::yes;
    if(strVote == "no") nVote = VoteTx::no;

    int success = 0;
    int failed = 0;

    json_spirit::Object resultsObj;

    BOOST_FOREACH(const CNodeEntry& mne, masternodeConfig.getEntries())
    {
        std::string errorMessage;
        std::vector<unsigned char> vchMasterNodeSignature;
        std::string strMasterNodeSignMessage;

        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;
        CPubKey pubKeyMasternode;
        CKey keyMasternode;

        json_spirit::Object statusObj;

        if(!legacySigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)){
            failed++;
            statusObj.push_back(json_spirit::Pair("result", "failed"));
            statusObj.push_back(json_spirit::Pair("errorMessage", "Masternode signing error, could not set key correctly: " + errorMessage));
            resultsObj.push_back(json_spirit::Pair(mne.getAlias(), statusObj));
            continue;
        }

        CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
        if(pmn == NULL)
        {
            ++failed;
            statusObj.push_back(json_spirit::Pair("result", "failed"));
            statusObj.push_back(json_spirit::Pair("errorMessage", "Can't find masternode by pubkey"));
            resultsObj.push_back(json_spirit::Pair(mne.getAlias(), statusObj));
            continue;
        }

        CMutableTransaction tx;
        tx.nVersion = 2;
        tx.nType = TRANSACTION_GOVERNANCE_VOTE;

        VoteTx voteTx;
        voteTx.voterId = pmn->vin;
        voteTx.electionCode = 1;
        voteTx.vote = nVote;
        voteTx.candidate = hash;

        if(!voteTx.Sign(keyMasternode, pubKeyMasternode))
        {
            ++failed;
            statusObj.push_back(json_spirit::Pair("result", "failed"));
            statusObj.push_back(json_spirit::Pair("errorMessage", "Failure to sign"));
            resultsObj.push_back(json_spirit::Pair(mne.getAlias(), statusObj));
            continue;
        }

        FundSpecialTx(tx, voteTx);
        SignSpecialTxPayload(tx, voteTx, keyMasternode);
        SetTxPayload(tx, voteTx);

        std::string result = SignAndSendSpecialTx(tx);

        if(true)
        {
            ++success;
            statusObj.push_back(json_spirit::Pair("result", result));
        }
        else
        {
            ++failed;
            statusObj.push_back(json_spirit::Pair("result", "failed"));
            statusObj.push_back(json_spirit::Pair("errorMessage", "?"));
            resultsObj.push_back(json_spirit::Pair(mne.getAlias(), statusObj));
            continue;
        }

        resultsObj.push_back(json_spirit::Pair(mne.getAlias(), statusObj));

    }

    json_spirit::Object returnObj;
    returnObj.push_back(json_spirit::Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed)));
    returnObj.push_back(json_spirit::Pair("detail", resultsObj));

    return returnObj;
}

json_spirit::Value detail::list(const json_spirit::Array& params, bool fHelp)
{
    const AgentRegistry& agents = GetAgentRegistry();

    json_spirit::Array result;
    for (auto i = agents.begin(); i != agents.end(); ++i)
    {
        result.push_back(i->id.ToString());
    }

    return result;
}