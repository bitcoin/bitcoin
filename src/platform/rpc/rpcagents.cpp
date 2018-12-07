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
        if (!Platform::CheckSpecialTx(tx, nullptr, state))
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

    json_spirit::Object SendVotingTransaction(
        const CPubKey& pubKeyMasternode,
        const CKey& keyMasternode,
        const CTxIn& vin,
        Platform::VoteTx::Value vote,
        const uint256& hash
    )
    {
        try
        {
            json_spirit::Object statusObj;

            CMutableTransaction tx;
            tx.nVersion = 2;
            tx.nType = TRANSACTION_GOVERNANCE_VOTE;

            Platform::VoteTx voteTx;
            voteTx.voterId = vin;
            voteTx.electionCode = 1;
            voteTx.vote = vote;
            voteTx.candidate = hash;

            if(!voteTx.Sign(keyMasternode, pubKeyMasternode))
            {
                statusObj.push_back(json_spirit::Pair("result", "failed"));
                statusObj.push_back(json_spirit::Pair("errorMessage", "Failure to sign"));
                return statusObj;
            }

            FundSpecialTx(tx, voteTx);
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

    json_spirit::Object CastSingleVote(const uint256& hash, Platform::VoteTx::Value vote, const CNodeEntry& mne)
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

        return SendVotingTransaction(pubKeyMasternode, keyMasternode, pmn->vin, vote, hash);
    }

    Platform::VoteTx::Value ParseVote(const std::string& voteText)
    {
        if(voteText != "yes" && voteText != "no")
            throw std::runtime_error("You can only vote 'yes' or 'no'");

        else if(voteText == "yes")
            return Platform::VoteTx::yes;
        else
            return Platform::VoteTx::no;
    }
}

json_spirit::Value agents(const json_spirit::Array& params, bool fHelp)
{
    auto command = GetCommand(params);

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
    auto nVote = ParseVote(params[2].get_str());

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