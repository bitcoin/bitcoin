// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/governanceclasses.h>
#include <governance/governancevalidators.h>
#include <validation.h>
#include <rpc/server.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>
#include <rpc/blockchain.h>
#include <node/context.h>
#include <evo/deterministicmns.h>

UniValue VoteWithMasternodes(const std::map<uint256, CKey>& keys,
                             const uint256& hash, vote_signal_enum_t eVoteSignal,
                             vote_outcome_enum_t eVoteOutcome, CConnman& connman)
{
    {
        LOCK(governance.cs);
        CGovernanceObject *pGovObj = governance.FindGovernanceObject(hash);
        if (!pGovObj) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Governance object not found");
        }
    }

    int nSuccessful = 0;
    int nFailed = 0;
    CDeterministicMNList mnList;
    if(deterministicMNManager)
        deterministicMNManager->GetListAtChainTip(mnList);

    UniValue resultsObj(UniValue::VOBJ);

    for (const auto& p : keys) {
        const auto& proTxHash = p.first;
        const auto& key = p.second;

        UniValue statusObj(UniValue::VOBJ);

        auto dmn = mnList.GetValidMN(proTxHash);
        if (!dmn) {
            nFailed++;
            statusObj.pushKV("result", "failed");
            statusObj.pushKV("errorMessage", "Can't find masternode by proTxHash");
            resultsObj.pushKV(proTxHash.ToString(), statusObj);
            continue;
        }

        CGovernanceVote vote(dmn->collateralOutpoint, hash, eVoteSignal, eVoteOutcome);
        if (!vote.Sign(key, key.GetPubKey().GetID())) {
            nFailed++;
            statusObj.pushKV("result", "failed");
            statusObj.pushKV("errorMessage", "Failure to sign.");
            resultsObj.pushKV(proTxHash.ToString(), statusObj);
            continue;
        }

        CGovernanceException exception;
        if (governance.ProcessVoteAndRelay(vote, exception, connman)) {
            nSuccessful++;
            statusObj.pushKV("result", "success");
        } else {
            nFailed++;
            statusObj.pushKV("result", "failed");
            statusObj.pushKV("errorMessage", exception.GetMessageStr());
        }

        resultsObj.pushKV(proTxHash.ToString(), statusObj);
    }

    UniValue returnObj(UniValue::VOBJ);
    returnObj.pushKV("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed));
    returnObj.pushKV("detail", resultsObj);

    return returnObj;
}

static RPCHelpMan gobject_list_prepared()
{
    return RPCHelpMan{"gobject_list_prepared",
        "\nReturns a list of governance objects prepared by this wallet with \"gobject_prepare\" sorted by their creation time.\n",
        {      
            {"count", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Maximum number of objects to return."},                                                               
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("gobject_list_prepared", "")
            + HelpExampleRpc("gobject_list_prepared", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{   
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;
    EnsureWalletIsUnlocked(*pwallet);

    int nCount = request.params.size() > 0 ? request.params[0].get_int() : 10;
    if (nCount < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative count");
    }
    // Get a list of all prepared governance objects stored in the wallet
    LOCK(pwallet->cs_wallet);
    std::vector<const CGovernanceObject*> vecObjects = pwallet->GetGovernanceObjects();
    // Sort the vector by the object creation time/hex data
    std::sort(vecObjects.begin(), vecObjects.end(), [](const CGovernanceObject* a, const CGovernanceObject* b) {
        bool fGreater = a->GetCreationTime() > b->GetCreationTime();
        bool fEqual = a->GetCreationTime() == b->GetCreationTime();
        bool fHexGreater = a->GetDataAsHexString() > b->GetDataAsHexString();
        return fGreater || (fEqual && fHexGreater);
    });

    UniValue jsonArray(UniValue::VARR);
    auto it = vecObjects.rbegin() + std::max<int>(0, ((int)vecObjects.size()) - nCount);
    while (it != vecObjects.rend()) {
        jsonArray.push_back((*it++)->ToJson());
    }

    return jsonArray;
},
    };
}

static RPCHelpMan gobject_prepare()
{
    return RPCHelpMan{"gobject_prepare",
        "\nPrepare governance object by signing and creating tx.\n",
        {      
            {"parentHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Hash of the parent object, \"0\" is root."},
            {"revision", RPCArg::Type::NUM, RPCArg::Optional::NO, "Object revision in the system."},   
            {"time", RPCArg::Type::NUM, RPCArg::Optional::NO, "Time this object was created."},
            {"dataHex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Data in hex string form."},
            {"outputHash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The single output to submit the proposal fee from."},    
            {"outputIndex", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The output index."},                                        
        },
        RPCResult{RPCResult::Type::STR_HEX, "hash", "txid"},
        RPCExamples{
                HelpExampleCli("gobject_prepare", "")
            + HelpExampleRpc("gobject_prepare", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;
    NodeContext& node = EnsureAnyNodeContext(request.context);
    if(!node.connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    EnsureWalletIsUnlocked(*pwallet);

    // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

    uint256 hashParent;

    // -- attach to root node (root node doesn't really exist, but has a hash of zero)
    if (request.params[0].get_str() == "0") {
        hashParent = uint256();
    } else {
        hashParent = ParseHashV(request.params[0], "feeTxid");
    }

    int nRevision = request.params[1].get_int();
    int64_t nTime = request.params[2].get_int64();
    std::string strDataHex = request.params[3].get_str();

    // CREATE A NEW COLLATERAL TRANSACTION FOR THIS SPECIFIC OBJECT

    CGovernanceObject govobj(hashParent, nRevision, nTime, uint256(), strDataHex);

    // This command is dangerous because it consumes 150 SYS irreversibly.
    // If params are lost, it's very hard to bruteforce them and yet
    // users ignore all instructions on syshub etc. and do not save them...
    // Let's log them here and hope users do not mess with debug.log
    LogPrintf("gobject_prepare -- params: %s %s %s %s, data: %s, hash: %s\n",
                request.params[0].getValStr(), request.params[1].getValStr(),
                request.params[2].getValStr(), request.params[3].getValStr(),
                govobj.GetDataAsPlainString(), govobj.GetHash().ToString());

    if (govobj.GetObjectType() == GOVERNANCE_OBJECT_PROPOSAL) {
        CProposalValidator validator(strDataHex, false);
        if (!validator.Validate()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid proposal data, error messages:" + validator.GetErrorMessages());
        }
    }

    if (govobj.GetObjectType() == GOVERNANCE_OBJECT_TRIGGER) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Trigger objects need not be prepared (however only masternodes can create them)");
    }

    LOCK(pwallet->cs_wallet);

    std::string strError = "";
    if (!govobj.IsValidLocally(strError, false))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Governance object is not valid - " + govobj.GetHash().ToString() + " - " + strError);

    // If specified, spend this outpoint as the proposal fee
    COutPoint outpoint;
    outpoint.SetNull();
    if (!request.params[4].isNull() && !request.params[5].isNull()) {
        uint256 collateralHash = ParseHashV(request.params[4], "outputHash");
        int32_t collateralIndex = request.params[5].get_int();
        if (collateralHash.IsNull() || collateralIndex < 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid hash or index: %s-%d", collateralHash.ToString(), collateralIndex));
        }
        outpoint = COutPoint(collateralHash, (uint32_t)collateralIndex);
    }

    CTransactionRef tx;
    if (!pwallet->GetBudgetSystemCollateralTX(tx, govobj.GetHash(), govobj.GetMinCollateralFee(), outpoint)) {
        std::string err = "Error making collateral transaction for governance object. Please check your wallet balance and make sure your wallet is unlocked.";
        if (!request.params[4].isNull() && !request.params[5].isNull()) {
            err += "Please verify your specified output is valid and is enough for the combined proposal fee and transaction fee.";
        }
        throw JSONRPCError(RPC_INTERNAL_ERROR, err);
    }

    if (!pwallet->WriteGovernanceObject({hashParent, nRevision, nTime, tx->GetHash(), strDataHex})) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "WriteGovernanceObject failed");
    }

    // -- send the tx to the network
    mapValue_t mapValue;
    pwallet->CommitTransaction(tx, std::move(mapValue), {} /* orderForm */);

    LogPrint(BCLog::GOBJECT, "gobject_prepare -- GetDataAsPlainString = %s, hash = %s, txid = %s\n",
                govobj.GetDataAsPlainString(), govobj.GetHash().ToString(), tx->GetHash().ToString());

    return tx->GetHash().ToString();
},
    };
} 


static RPCHelpMan gobject_vote_many()
{
    return RPCHelpMan{"gobject_vote_many",
        "\nVote on a governance object by all masternodes for which the voting key is present in the local wallet.\n",
        {      
            {"governanceHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Hash of the governance object."},
            {"vote", RPCArg::Type::STR, RPCArg::Optional::NO, "Vote, possible values: [funding|valid|delete|endorsed]."},   
            {"voteOutome", RPCArg::Type::STR, RPCArg::Optional::NO, "Vote outcome, possible values: [yes|no|abstain]."},                                  
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("gobject_vote_many", "")
            + HelpExampleRpc("gobject_vote_many", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    NodeContext& node = EnsureAnyNodeContext(request.context);
    if(!node.connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");


    uint256 hash = ParseHashV(request.params[0], "Object hash");
    std::string strVoteSignal = request.params[1].get_str();
    std::string strVoteOutcome = request.params[2].get_str();

    vote_signal_enum_t eVoteSignal = CGovernanceVoting::ConvertVoteSignal(strVoteSignal);
    if (eVoteSignal == VOTE_SIGNAL_NONE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "Invalid vote signal. Please using one of the following: "
                           "(funding|valid|delete|endorsed)");
    }

    vote_outcome_enum_t eVoteOutcome = CGovernanceVoting::ConvertVoteOutcome(strVoteOutcome);
    if (eVoteOutcome == VOTE_OUTCOME_NONE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote outcome. Please use one of the following: 'yes', 'no' or 'abstain'");
    }

    EnsureWalletIsUnlocked(*pwallet);

    std::map<uint256, CKey> votingKeys;
    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();
    CDeterministicMNList mnList;
    if(deterministicMNManager)
        deterministicMNManager->GetListAtChainTip(mnList);
    mnList.ForEachMN(true, [&](const CDeterministicMNCPtr& dmn) {
        LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*pwallet);
        LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);

        EnsureWalletIsUnlocked(*pwallet);

        CKey key;
        if (spk_man.GetKey(dmn->pdmnState->keyIDVoting, key)) {
            votingKeys.try_emplace(dmn->proTxHash, key);
        }
    });

    return VoteWithMasternodes(votingKeys, hash, eVoteSignal, eVoteOutcome, *node.connman);
},
    };
} 

static RPCHelpMan gobject_vote_alias()
{
    return RPCHelpMan{"gobject_vote_alias",
        "\nVote on a governance object by masternode's voting key (if present in local wallet).\n",
        {      
            {"governanceHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Hash of the governance object."},
            {"vote", RPCArg::Type::STR, RPCArg::Optional::NO, "Vote, possible values: [funding|valid|delete|endorsed]."},   
            {"voteOutome", RPCArg::Type::STR, RPCArg::Optional::NO, "Vote outcome, possible values: [yes|no|abstain]."},   
            {"protxHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Masternode's proTxHash."},                                                                 
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("gobject_vote_alias", "")
            + HelpExampleRpc("gobject_vote_alias", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{   
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    NodeContext& node = EnsureAnyNodeContext(request.context);
    if(!node.connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");


    uint256 hash = ParseHashV(request.params[0], "Object hash");
    std::string strVoteSignal = request.params[1].get_str();
    std::string strVoteOutcome = request.params[2].get_str();

    vote_signal_enum_t eVoteSignal = CGovernanceVoting::ConvertVoteSignal(strVoteSignal);
    if (eVoteSignal == VOTE_SIGNAL_NONE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "Invalid vote signal. Please using one of the following: "
                           "(funding|valid|delete|endorsed)");
    }

    vote_outcome_enum_t eVoteOutcome = CGovernanceVoting::ConvertVoteOutcome(strVoteOutcome);
    if (eVoteOutcome == VOTE_OUTCOME_NONE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote outcome. Please use one of the following: 'yes', 'no' or 'abstain'");
    }

    EnsureWalletIsUnlocked(*pwallet);

    uint256 proTxHash = ParseHashV(request.params[3], "protxHash");
    CDeterministicMNList mnList;
    if(deterministicMNManager)
        deterministicMNManager->GetListAtChainTip(mnList);
    auto dmn = mnList.GetValidMN(proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid or unknown proTxHash");
    }
    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();
    CKey key;
    {
        LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*pwallet);
        LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);

        
        if (!spk_man.GetKey(dmn->pdmnState->keyIDVoting, key)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Private key for voting address %s not known by wallet", EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDVoting))));
        }
    }

    std::map<uint256, CKey> votingKeys;
    votingKeys.try_emplace(proTxHash, key);

    return VoteWithMasternodes(votingKeys, hash, eVoteSignal, eVoteOutcome, *node.connman);
},
    };
} 

Span<const CRPCCommand> GetGovernanceWalletRPCCommands()
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)       
//  --------------------- ------------------------  ----------------------- 
    { "governancewallet",            &gobject_vote_alias,      },
    { "governancewallet",            &gobject_vote_many,       },
    { "governancewallet",            &gobject_prepare,         },
    { "governancewallet",            &gobject_list_prepared,   },
};
// clang-format on
    return MakeSpan(commands);
}
