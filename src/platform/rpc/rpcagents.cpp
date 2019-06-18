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
#include "platform/governance.h"
#include "platform/specialtx.h"
#include "platform/rpc/specialtx-rpc-utils.h"

namespace Platform
{
    json_spirit::Object SendVotingTransaction(
        const CPubKey& pubKeyMasternode,
        const CKey& keyMasternode,
        const CTxIn& vin,
        Platform::VoteValue voteValue,
        const uint256& hash
    )
    {
        try
        {
            json_spirit::Object statusObj;

            CMutableTransaction tx;
            tx.nVersion = 3;
            tx.nType = TRANSACTION_GOVERNANCE_VOTE;

            auto vote = Platform::Vote{hash, voteValue, GetTime(), vin};
            auto voteTx = Platform::VoteTx(vote, 1);

            FundSpecialTx(tx, voteTx);
            voteTx.keyId = keyMasternode.GetPubKey().GetID();
            SignSpecialTxPayload(tx, voteTx, keyMasternode);
            SetTxPayload(tx, voteTx);

            std::string result = SignAndSendSpecialTx(tx);

            statusObj.push_back(json_spirit::Pair("result", result));
            return statusObj;
        }
        catch(const std::exception& ex)
        {
            json_spirit::Object statusObj;
            statusObj.push_back(json_spirit::Pair("result", "failed"));
            statusObj.push_back(json_spirit::Pair("errorMessage", ex.what()));
            return statusObj;
        }
    }

    json_spirit::Object CastSingleVote(const uint256& hash, Platform::VoteValue vote, const CNodeEntry& mne)
    {
        std::string errorMessage;
        std::vector<unsigned char> vchMasterNodeSignature;
        std::string strMasterNodeSignMessage;

        CKey keyCollateralAddress;
        CPubKey pubKeyMasternode;
        CKey keyMasternode;

        json_spirit::Object statusObj;

        if(!legacySigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)){
            statusObj.push_back(json_spirit::Pair("result", "failed"));
            statusObj.push_back(json_spirit::Pair("errorMessage", "Masternode signing error, could not set key correctly: " + errorMessage));
            return statusObj;
        }

        CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
        if(pmn == nullptr)
        {
            statusObj.push_back(json_spirit::Pair("result", "failed"));
            statusObj.push_back(json_spirit::Pair("errorMessage", "Can't find masternode by pubkey"));
            return statusObj;
        }

        assert(pmn->pubkey2 == pubKeyMasternode);

        return SendVotingTransaction(pubKeyMasternode, keyMasternode, pmn->vin, vote, hash);
    }

    Platform::VoteValue ParseVote(const std::string& voteText)
    {
        if(voteText != "yes" && voteText != "no")
            throw std::runtime_error("You can only vote 'yes' or 'no'");

        else if(voteText == "yes")
            return Platform::VoteValue::yes;
        else
            return Platform::VoteValue::no;
    }
}

json_spirit::Value agents(const json_spirit::Array& params, bool fHelp)
{
    auto command = Platform::GetCommand(params, "usage: agents vote|list");

    if (command == "vote")
        return detail::vote(params, fHelp);
    else if (command == "list")
        return detail::list(params, fHelp);
    else
        throw std::runtime_error("wrong format");
}

json_spirit::Value detail::vote(const json_spirit::Array& params, bool fHelp)
{
    if (params.size() != 3)
        throw std::runtime_error("Correct usage is 'mnbudget vote proposal-hash yes|no'");

    auto hash = ParseHashV(params[1], "parameter 1");
    auto nVote = Platform::ParseVote(params[2].get_str());

    auto success = 0;
    auto failed = 0;
    auto resultsObj = json_spirit::Object{};

    for(const auto& mne: masternodeConfig.getEntries())
    {
        auto statusObj = CastSingleVote(hash, nVote, mne);
        auto result = json_spirit::find_value(statusObj, "result");
        if (result.type() != json_spirit::Value_type::str_type || result.get_str() == "failed")
            ++failed;
        else
            ++success;

        resultsObj.push_back(json_spirit::Pair(mne.getAlias(), statusObj));

    }

    json_spirit::Object returnObj;
    returnObj.push_back(json_spirit::Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed)));
    returnObj.push_back(json_spirit::Pair("detail", resultsObj));

    return returnObj;
}

json_spirit::Value detail::list(const json_spirit::Array& params, bool )
{
    json_spirit::Array result;
    for (const auto& agent: Platform::GetAgentRegistry())
    {
        result.push_back(agent.id.ToString());
    }

    return result;
}