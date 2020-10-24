// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/activemasternode.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <governance/governance.h>
#include <governance/governancevote.h>
#include <governance/governanceclasses.h>
#include <governance/governancevalidators.h>
#include <init.h>
#include <txmempool.h>
#include <validation.h>
#include <masternode/masternodesync.h>
#include <messagesigner.h>
#include <rpc/server.h>
#include <util/system.h>
#include <util/moneystr.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>
#include <rpc/util.h>
#include <net.h>
#include <rpc/blockchain.h>
#include <node/context.h>


void gobject_prepare_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"gobject prepare",
               "Prepare governance object by signing and creating tx\n"
                + HELP_REQUIRING_PASSPHRASE +
                "\nArguments:\n"
                "1. parent-hash   (string, required) hash of the parent object, \"0\" is root\n"
                "2. revision      (numeric, required) object revision in the system\n"
                "3. time          (numeric, required) time this object was created\n"
                "4. data-hex      (string, required)  data in hex string form\n"
                "5. use-IS        (boolean, optional, default=false) Deprecated and ignored\n"
                "6. outputHash    (string, optional) the single output to submit the proposal fee from\n"
                "7. outputIndex   (numeric, optional) The output index.\n",
    {
    },
    RPCResult{RPCResult::Type::NONE, "", ""},
    RPCExamples{""},
    }.Check(request);
}

UniValue gobject_prepare(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;
    CWallet* const pwallet = wallet.get();
    if (request.fHelp || (request.params.size() != 5 && request.params.size() != 6 && request.params.size() != 8)) 
        gobject_prepare_help(request);

    NodeContext& node = EnsureNodeContext(request.context);
    if(!node.connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    EnsureWalletIsUnlocked(pwallet);

    // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

    uint256 hashParent;

    // -- attach to root node (root node doesn't really exist, but has a hash of zero)
    if (request.params[1].get_str() == "0") {
        hashParent = uint256();
    } else {
        hashParent = ParseHashV(request.params[1], "fee-txid, parameter 1");
    }

    std::string strRevision = request.params[2].get_str();
    std::string strTime = request.params[3].get_str();
    int nRevision;
    if(!ParseInt32(strRevision, &nRevision)){
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid revision");
    }
    int64_t nTime = atoi64(strTime);
    std::string strDataHex = request.params[4].get_str();

    // CREATE A NEW COLLATERAL TRANSACTION FOR THIS SPECIFIC OBJECT

    CGovernanceObject govobj(hashParent, nRevision, nTime, uint256(), strDataHex);

    // This command is dangerous because it consumes 150 SYS irreversibly.
    // If params are lost, it's very hard to bruteforce them and yet
    // users ignore all instructions on syshub etc. and do not save them...
    // Let's log them here and hope users do not mess with debug.log
    LogPrintf("gobject_prepare -- params: %s %s %s %s, data: %s, hash: %s\n",
                request.params[1].get_str(), request.params[2].get_str(),
                request.params[3].get_str(), request.params[4].get_str(),
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
    if (!request.params[6].isNull() && !request.params[7].isNull()) {
        uint256 collateralHash = ParseHashV(request.params[6], "outputHash");
        int32_t collateralIndex = request.params[7].get_int();
        if (collateralHash.IsNull() || collateralIndex < 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid hash or index: %s-%d", collateralHash.ToString(), collateralIndex));
        }
        outpoint = COutPoint(collateralHash, (uint32_t)collateralIndex);
    }

    CTransactionRef tx;
    if (!pwallet->GetBudgetSystemCollateralTX(tx, govobj.GetHash(), govobj.GetMinCollateralFee(), outpoint)) {
        std::string err = "Error making collateral transaction for governance object. Please check your wallet balance and make sure your wallet is unlocked.";
        if (!request.params[6].isNull() && !request.params[7].isNull()) {
            err += "Please verify your specified output is valid and is enough for the combined proposal fee and transaction fee.";
        }
        throw JSONRPCError(RPC_INTERNAL_ERROR, err);
    }

    // -- send the tx to the network
    mapValue_t mapValue;
    pwallet->CommitTransaction(tx, std::move(mapValue), {} /* orderForm */);

    LogPrint(BCLog::GOBJECT, "gobject_prepare -- GetDataAsPlainString = %s, hash = %s, txid = %s\n",
                govobj.GetDataAsPlainString(), govobj.GetHash().ToString(), tx->GetHash().ToString());

    return tx->GetHash().ToString();
}


void gobject_vote_many_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"gobject vote-many",
            "Vote on a governance object by all masternodes for which the voting key is present in the local wallet\n"
            + HELP_REQUIRING_PASSPHRASE +
            "\nArguments:\n"
            "1. governance-hash   (string, required) hash of the governance object\n"
            "2. vote              (string, required) vote, possible values: [funding|valid|delete|endorsed]\n"
            "3. vote-outcome      (string, required) vote outcome, possible values: [yes|no|abstain]\n",
    {
    },
    RPCResult{RPCResult::Type::NONE, "", ""},
    RPCExamples{""},
    }.Check(request);  
}

UniValue gobject_vote_many(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;
    CWallet* const pwallet = wallet.get();
    if (request.fHelp || request.params.size() != 4)
        gobject_vote_many_help(request);


    NodeContext& node = EnsureNodeContext(request.context);
    if(!node.connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");


    uint256 hash = ParseHashV(request.params[1], "Object hash");
    std::string strVoteSignal = request.params[2].get_str();
    std::string strVoteOutcome = request.params[3].get_str();

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

    EnsureWalletIsUnlocked(pwallet);

    std::map<uint256, CKey> votingKeys;
    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();
    CDeterministicMNList mnList;
    deterministicMNManager->GetListAtChainTip(mnList);
    mnList.ForEachMN(true, [&](const CDeterministicMNCPtr& dmn) {
        LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*pwallet);
        LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);

        EnsureWalletIsUnlocked(pwallet);

        CKey key;
        if (spk_man.GetKey(dmn->pdmnState->keyIDVoting, key)) {
            votingKeys.emplace(dmn->proTxHash, key);
        }
    });

    return VoteWithMasternodes(votingKeys, hash, eVoteSignal, eVoteOutcome, *node.connman);
}

void gobject_vote_alias_help(const JSONRPCRequest& request)
{
     RPCHelpMan{"gobject vote-alias",
            "Vote on a governance object by masternode's voting key (if present in local wallet)\n"
            + HELP_REQUIRING_PASSPHRASE +
            "\nArguments:\n"
            "1. governance-hash   (string, required) hash of the governance object\n"
            "2. vote              (string, required) vote, possible values: [funding|valid|delete|endorsed]\n"
            "3. vote-outcome      (string, required) vote outcome, possible values: [yes|no|abstain]\n"
            "4. protx-hash        (string, required) masternode's proTxHash",
    {
    },
    RPCResult{RPCResult::Type::NONE, "", ""},
    RPCExamples{""},
    }.Check(request);     
}

UniValue gobject_vote_alias(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;
    CWallet* const pwallet = wallet.get();
    if (request.fHelp || request.params.size() != 5)
        gobject_vote_alias_help(request);


    NodeContext& node = EnsureNodeContext(request.context);
    if(!node.connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");


    uint256 hash = ParseHashV(request.params[1], "Object hash");
    std::string strVoteSignal = request.params[2].get_str();
    std::string strVoteOutcome = request.params[3].get_str();

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

    EnsureWalletIsUnlocked(pwallet);

    uint256 proTxHash = ParseHashV(request.params[4], "protx-hash");
    CDeterministicMNList mnList;
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
    votingKeys.emplace(proTxHash, key);

    return VoteWithMasternodes(votingKeys, hash, eVoteSignal, eVoteOutcome, *node.connman);
}

Span<const CRPCCommand> GetGovernanceWalletRPCCommands()
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
//  --------------------- ------------------------  -----------------------  ----------
    { "governancewallet",               "gobject_vote_alias",     &gobject_vote_alias,      {} },
    { "governancewallet",               "gobject_vote_many",      &gobject_vote_many,    {"index"} },
    { "governancewallet",               "gobject_prepare",        &gobject_prepare,          {"mode"} },

};
// clang-format on
    return MakeSpan(commands);
}
