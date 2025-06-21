// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <core_io.h>
#include <evo/deterministicmns.h>
#include <governance/classes.h>
#include <governance/common.h>
#include <governance/governance.h>
#include <governance/validators.h>
#include <governance/vote.h>
#include <index/txindex.h>
#include <masternode/node.h>
#include <masternode/sync.h>
#include <messagesigner.h>
#include <net.h>
#include <node/context.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <timedata.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <validation.h>
#include <wallet/rpc/util.h>

#ifdef ENABLE_WALLET
#include <wallet/spend.h>
#include <wallet/wallet.h>
#endif // ENABLE_WALLET

using node::NodeContext;

static RPCHelpMan gobject_count()
{
    return RPCHelpMan{"gobject count",
        "Count governance objects and votes\n",
        {
            {"mode", RPCArg::Type::STR, RPCArg::DefaultHint{"json"}, "Output format: json (\"json\") or string in free form (\"all\")"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string strMode{"json"};

    if (!request.params[0].isNull()) {
        strMode = request.params[0].get_str();
    }

    if (strMode != "json" && strMode != "all")
        throw JSONRPCError(RPC_INVALID_PARAMETER, "mode can be 'json' or 'all'");

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    CHECK_NONFATAL(node.govman);
    return strMode == "json" ? node.govman->ToJson() : node.govman->ToString();
},
    };
}

// DEBUG : TEST DESERIALIZATION OF GOVERNANCE META DATA
static RPCHelpMan gobject_deserialize()
{
    return RPCHelpMan{"gobject deserialize",
        "Deserialize governance object from hex string to JSON\n",
        {
            {"hex_data", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "data in hex string form"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string strHex = request.params[0].get_str();

    std::vector<unsigned char> v = ParseHex(strHex);
    std::string s(v.begin(), v.end());

    UniValue u(UniValue::VOBJ);
    u.read(s);

    return u.write().c_str();
},
    };
}

// VALIDATE A GOVERNANCE OBJECT PRIOR TO SUBMISSION
static RPCHelpMan gobject_check()
{
    return RPCHelpMan{"gobject check",
        "Validate governance object data (proposal only)\n",
        {
            {"hex_data", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "data in hex string format"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

    uint256 hashParent;

    int nRevision = 1;

    int64_t nTime = GetAdjustedTime();
    std::string strDataHex = request.params[0].get_str();

    CGovernanceObject govobj(hashParent, nRevision, nTime, uint256(), strDataHex);

    if (govobj.GetObjectType() == GovernanceObject::PROPOSAL) {
        CProposalValidator validator(strDataHex);
        if (!validator.Validate())  {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid proposal data, error messages:" + validator.GetErrorMessages());
        }
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid object type, only proposals can be validated");
    }

    UniValue objResult(UniValue::VOBJ);

    objResult.pushKV("Object status", "OK");

    return objResult;
},
    };
}

#ifdef ENABLE_WALLET
// PREPARE THE GOVERNANCE OBJECT BY CREATING A COLLATERAL TRANSACTION
static RPCHelpMan gobject_prepare()
{
    return RPCHelpMan{"gobject prepare",
        "Prepare governance object by signing and creating tx\n"
        + HELP_REQUIRING_PASSPHRASE,
        {
            {"parent-hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "hash of the parent object, \"0\" is root"},
            {"revision", RPCArg::Type::NUM, RPCArg::Optional::NO, "object revision in the system"},
            {"time", RPCArg::Type::NUM, RPCArg::Optional::NO, "time this object was created"},
            {"data-hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "data in hex string form"},
            {"use-IS", RPCArg::Type::BOOL, RPCArg::Default{false}, "Deprecated and ignored"},
            {"outputHash", RPCArg::Type::STR_HEX, RPCArg::Default{""}, "the single output to submit the proposal fee from"},
            {"outputIndex", RPCArg::Type::NUM, RPCArg::Default{0}, "The output index."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    EnsureWalletIsUnlocked(*wallet);

    // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

    uint256 hashParent;

    // -- attach to root node (root node doesn't really exist, but has a hash of zero)
    if (request.params[0].get_str() == "0") {
        hashParent = uint256();
    } else {
        hashParent = ParseHashV(request.params[0], "parent-hash");
    }

    int nRevision = ParseInt32V(request.params[1], "revision");
    int64_t nTime = ParseInt64V(request.params[2], "time");
    std::string strDataHex = request.params[3].get_str();

    // CREATE A NEW COLLATERAL TRANSACTION FOR THIS SPECIFIC OBJECT

    CGovernanceObject govobj(hashParent, nRevision, nTime, uint256(), strDataHex);

    // This command is dangerous because it consumes 5 DASH irreversibly.
    // If params are lost, it's very hard to bruteforce them and yet
    // users ignore all instructions on dashcentral etc. and do not save them...
    // Let's log them here and hope users do not mess with debug.log
    LogPrintf("gobject_prepare -- params: %s %s %s %s, data: %s, hash: %s\n",
                request.params[0].getValStr(), request.params[1].getValStr(),
                request.params[2].getValStr(), request.params[3].getValStr(),
                govobj.GetDataAsPlainString(), govobj.GetHash().ToString());

    if (govobj.GetObjectType() == GovernanceObject::PROPOSAL) {
        CProposalValidator validator(strDataHex);
        if (!validator.Validate()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid proposal data, error messages:" + validator.GetErrorMessages());
        }
    }

    if (govobj.GetObjectType() == GovernanceObject::TRIGGER) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Trigger objects need not be prepared (however only masternodes can create them)");
    }

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    LOCK(wallet->cs_wallet);

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    {
        LOCK(cs_main);
        std::string strError;
        if (!govobj.IsValidLocally(CHECK_NONFATAL(node.dmnman)->GetListAtChainTip(), chainman, strError, false))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Governance object is not valid - " + govobj.GetHash().ToString() + " - " + strError);
    }

    // If specified, spend this outpoint as the proposal fee
    COutPoint outpoint;
    outpoint.SetNull();
    if (!request.params[5].isNull() && !request.params[6].isNull()) {
        uint256 collateralHash(ParseHashV(request.params[5], "outputHash"));
        int32_t collateralIndex = ParseInt32V(request.params[6], "outputIndex");
        if (collateralHash.IsNull() || collateralIndex < 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid hash or index: %s-%d", collateralHash.ToString(), collateralIndex));
        }
        outpoint = COutPoint(collateralHash, (uint32_t)collateralIndex);
    }

    CTransactionRef tx;

    if (!GenBudgetSystemCollateralTx(*wallet, tx, govobj.GetHash(), govobj.GetMinCollateralFee(), outpoint)) {
        std::string err = "Error making collateral transaction for governance object. Please check your wallet balance and make sure your wallet is unlocked.";
        if (!request.params[5].isNull() && !request.params[6].isNull()) {
            err += "Please verify your specified output is valid and is enough for the combined proposal fee and transaction fee.";
        }
        throw JSONRPCError(RPC_INTERNAL_ERROR, err);
    }

    if (!wallet->WriteGovernanceObject(Governance::Object{hashParent, nRevision, nTime, tx->GetHash(), strDataHex})) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "WriteGovernanceObject failed");
    }

    // -- send the tx to the network
    wallet->CommitTransaction(tx, {}, {});

    LogPrint(BCLog::GOBJECT, "gobject_prepare -- GetDataAsPlainString = %s, hash = %s, txid = %s\n",
                govobj.GetDataAsPlainString(), govobj.GetHash().ToString(), tx->GetHash().ToString());

    return tx->GetHash().ToString();
},
    };
}

static RPCHelpMan gobject_list_prepared()
{
    return RPCHelpMan{"gobject list-prepared",
        "Returns a list of governance objects prepared by this wallet with \"gobject prepare\" sorted by their creation time.\n"
        + HELP_REQUIRING_PASSPHRASE,
        {
            {"count", RPCArg::Type::NUM, RPCArg::Default{10}, "Maximum number of objects to return."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;
    EnsureWalletIsUnlocked(*wallet);

    int64_t nCount = request.params.empty() ? 10 : ParseInt64V(request.params[0], "count");
    if (nCount < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative count");
    }
    // Get a list of all prepared governance objects stored in the wallet
    LOCK(wallet->cs_wallet);
    std::vector<const Governance::Object*> vecObjects = wallet->GetGovernanceObjects();
    // Sort the vector by the object creation time/hex data
    std::sort(vecObjects.begin(), vecObjects.end(), [](const Governance::Object* a, const Governance::Object* b) {
        if (a->time != b->time) return a->time < b->time;
        return a->GetDataAsHexString() < b->GetDataAsHexString();
    });

    UniValue jsonArray(UniValue::VARR);
    for (auto it = vecObjects.begin() + std::max<int>(0, vecObjects.size() - nCount); it != vecObjects.end(); ++it) {
        jsonArray.push_back((*it)->ToJson());
    }

    return jsonArray;
},
    };
}
#endif // ENABLE_WALLET

// AFTER COLLATERAL TRANSACTION HAS MATURED USER CAN SUBMIT GOVERNANCE OBJECT TO PROPAGATE NETWORK
/*
    ------ Example Governance Item ------

    gobject submit 6e622bb41bad1fb18e7f23ae96770aeb33129e18bd9efe790522488e580a0a03 0 1 1464292854 "beer-reimbursement" 5b5b22636f6e7472616374222c207b2270726f6a6563745f6e616d65223a20225c22626565722d7265696d62757273656d656e745c22222c20227061796d656e745f61646472657373223a20225c225879324c4b4a4a64655178657948726e34744744514238626a6876464564615576375c22222c2022656e645f64617465223a202231343936333030343030222c20226465736372697074696f6e5f75726c223a20225c227777772e646173687768616c652e6f72672f702f626565722d7265696d62757273656d656e745c22222c2022636f6e74726163745f75726c223a20225c22626565722d7265696d62757273656d656e742e636f6d2f3030312e7064665c22222c20227061796d656e745f616d6f756e74223a20223233342e323334323232222c2022676f7665726e616e63655f6f626a6563745f6964223a2037342c202273746172745f64617465223a202231343833323534303030227d5d5d1
*/
static RPCHelpMan gobject_submit()
{
    return RPCHelpMan{"gobject submit",
        "Submit governance object to network\n",
        {
            {"parent-hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "hash of the parent object, \"0\" is root"},
            {"revision", RPCArg::Type::NUM, RPCArg::Optional::NO, "object revision in the system"},
            {"time", RPCArg::Type::NUM, RPCArg::Optional::NO, "time this object was created"},
            {"data-hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "data in hex string form"},
            {"fee-txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "txid of the corresponding proposal fee transaction"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    if(!node.mn_sync->IsBlockchainSynced()) {
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Must wait for client to sync with masternode network. Try again in a minute or so.");
    }

    auto mnList = CHECK_NONFATAL(node.dmnman)->GetListAtChainTip();

    if (node.mn_activeman) {
        const bool fMnFound = mnList.HasValidMNByCollateral(node.mn_activeman->GetOutPoint());

        LogPrint(BCLog::GOBJECT, /* Continued */
                 "gobject_submit -- pubKeyOperator = %s, outpoint = %s, params.size() = %lld, fMnFound = %d\n",
                 node.mn_activeman->GetPubKey().ToString(false), node.mn_activeman->GetOutPoint().ToStringShort(),
                 request.params.size(), fMnFound);
    } else {
        LogPrint(BCLog::GOBJECT, "gobject_submit -- pubKeyOperator = N/A, outpoint = N/A, params.size() = %lld, fMnFound = %d\n",
                 request.params.size(),
                 false);
    }

    // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

    uint256 txidFee;

    if (!request.params[4].isNull()) {
        txidFee = ParseHashV(request.params[4], "fee-txid");
    }
    uint256 hashParent;
    if (request.params[0].get_str() == "0") { // attach to root node (root node doesn't really exist, but has a hash of zero)
        hashParent = uint256();
    } else {
        hashParent = ParseHashV(request.params[0], "parent-hash");
    }

    // GET THE PARAMETERS FROM USER

    int nRevision = ParseInt32V(request.params[1], "revision");
    int64_t nTime = ParseInt64V(request.params[2], "time");
    std::string strDataHex = request.params[3].get_str();

    CGovernanceObject govobj(hashParent, nRevision, nTime, txidFee, strDataHex);

    LogPrint(BCLog::GOBJECT, "gobject_submit -- GetDataAsPlainString = %s, hash = %s, txid = %s\n",
                govobj.GetDataAsPlainString(), govobj.GetHash().ToString(), txidFee.ToString());

    if (govobj.GetObjectType() == GovernanceObject::TRIGGER) {
        LogPrintf("govobject(submit) -- Object submission rejected because submission of trigger is disabled\n");
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Submission of triggers is not available");
    }

    if (govobj.GetObjectType() == GovernanceObject::PROPOSAL) {
        CProposalValidator validator(strDataHex);
        if (!validator.Validate()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid proposal data, error messages:" + validator.GetErrorMessages());
        }
    }

    std::string strHash = govobj.GetHash().ToString();

    const CTxMemPool& mempool = EnsureMemPool(node);
    bool fMissingConfirmations;
    {
        if (g_txindex) {
            g_txindex->BlockUntilSyncedToCurrentChain();
        }

        LOCK2(cs_main, mempool.cs);

        std::string strError;
        if (!govobj.IsValidLocally(node.dmnman->GetListAtChainTip(), chainman, strError, fMissingConfirmations, true) && !fMissingConfirmations) {
            LogPrintf("gobject(submit) -- Object submission rejected because object is not valid - hash = %s, strError = %s\n", strHash, strError);
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Governance object is not valid - " + strHash + " - " + strError);
        }
    }

    // RELAY THIS OBJECT
    // Reject if rate check fails but don't update buffer
    if (!CHECK_NONFATAL(node.govman)->MasternodeRateCheck(govobj)) {
        LogPrintf("gobject(submit) -- Object submission rejected because of rate check failure - hash = %s\n", strHash);
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Object creation rate limit exceeded");
    }

    LogPrintf("gobject(submit) -- Adding locally created governance object - %s\n", strHash);

    PeerManager& peerman = EnsurePeerman(node);
    if (fMissingConfirmations) {
        node.govman->AddPostponedObject(govobj);
        govobj.Relay(peerman, *CHECK_NONFATAL(node.mn_sync));
    } else {
        node.govman->AddGovernanceObject(govobj, peerman);
    }

    return govobj.GetHash().ToString();
},
    };
}

#ifdef ENABLE_WALLET
static bool SignVote(const CWallet& wallet, const CKeyID& keyID, CGovernanceVote& vote)
{
    // Special implementation for testnet (Harden Spork6 that has not been deployed to other networks)
    if (Params().NetworkIDString() == CBaseChainParams::TESTNET) {
        std::vector<unsigned char> signature;
        if (!wallet.SignSpecialTxPayload(vote.GetSignatureHash(), keyID, signature)) {
            LogPrintf("SignVote -- SignHash() failed\n");
            return false;
        }
        vote.SetSignature(signature);
        return true;
    } // end of testnet implementation

    std::string strMessage{vote.GetSignatureString()};
    std::string signature;
    SigningResult err = wallet.SignMessage(strMessage, PKHash{keyID}, signature);
    if (err != SigningResult::OK) {
        LogPrintf("SignVote failed due to: %s\n", SigningResultString(err));
        return false;
    }
    const auto opt_decoded = DecodeBase64(signature);
    CHECK_NONFATAL(opt_decoded.has_value()); // DecodeBase64 should not fail

    vote.SetSignature(std::vector<unsigned char>(opt_decoded->data(), opt_decoded->data() + opt_decoded->size()));
    return true;
}

static UniValue VoteWithMasternodes(const JSONRPCRequest& request, const CWallet& wallet,
                             const std::map<uint256, CKeyID>& votingKeys,
                             const uint256& hash, vote_signal_enum_t eVoteSignal,
                             vote_outcome_enum_t eVoteOutcome)
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    CHECK_NONFATAL(node.govman);
    {
        LOCK(node.govman->cs);
        const CGovernanceObject *pGovObj = node.govman->FindConstGovernanceObject(hash);
        if (!pGovObj) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Governance object not found");
        }
    }

    int nSuccessful = 0;
    int nFailed = 0;

    auto mnList = CHECK_NONFATAL(node.dmnman)->GetListAtChainTip();

    UniValue resultsObj(UniValue::VOBJ);

    for (const auto& p : votingKeys) {
        const auto& proTxHash = p.first;
        const auto& keyID = p.second;

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

        if (!SignVote(wallet, keyID, vote) || !vote.CheckSignature(keyID)) {
            nFailed++;
            statusObj.pushKV("result", "failed");
            statusObj.pushKV("errorMessage", "Failure to sign.");
            resultsObj.pushKV(proTxHash.ToString(), statusObj);
            continue;
        }

        CGovernanceException exception;
        CConnman& connman = EnsureConnman(node);
        PeerManager& peerman = EnsurePeerman(node);
        if (node.govman->ProcessVoteAndRelay(vote, exception, connman, peerman)) {
            nSuccessful++;
            statusObj.pushKV("result", "success");
        } else {
            nFailed++;
            statusObj.pushKV("result", "failed");
            statusObj.pushKV("errorMessage", exception.GetMessage());
        }

        resultsObj.pushKV(proTxHash.ToString(), statusObj);
    }

    UniValue returnObj(UniValue::VOBJ);
    returnObj.pushKV("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed));
    returnObj.pushKV("detail", resultsObj);

    return returnObj;
}

static bool CheckWalletOwnsKey(const CWallet& wallet, const CKeyID& keyid)
{
    const CScript script{GetScriptForDestination(PKHash(keyid))};
    LOCK(wallet.cs_wallet);

    return wallet.IsMine(script) == isminetype::ISMINE_SPENDABLE;
}

static RPCHelpMan gobject_vote_many()
{
    return RPCHelpMan{"gobject vote-many",
        "Vote on a governance object by all masternodes for which the voting key is present in the local wallet\n"
        + HELP_REQUIRING_PASSPHRASE,
        {
            {"governance-hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "hash of the governance object"},
            {"vote", RPCArg::Type::STR, RPCArg::Optional::NO, "vote, possible values: [funding|valid|delete|endorsed]"},
            {"vote-outcome", RPCArg::Type::STR, RPCArg::Optional::NO, "vote outcome, possible values: [yes|no|abstain]"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    const NodeContext& node = EnsureAnyNodeContext(request.context);

    uint256 hash(ParseHashV(request.params[0], "Object hash"));
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

    EnsureWalletIsUnlocked(*wallet);

    std::map<uint256, CKeyID> votingKeys;

    auto mnList = CHECK_NONFATAL(node.dmnman)->GetListAtChainTip();
    mnList.ForEachMN(true, [&](auto& dmn) {
        const bool is_mine = CheckWalletOwnsKey(*wallet, dmn.pdmnState->keyIDVoting);
        if (is_mine) {
            votingKeys.emplace(dmn.proTxHash, dmn.pdmnState->keyIDVoting);
        }
    });

    return VoteWithMasternodes(request, *wallet, votingKeys, hash, eVoteSignal, eVoteOutcome);
},
    };
}

static RPCHelpMan gobject_vote_alias()
{
    return RPCHelpMan{"gobject vote-alias",
        "Vote on a governance object by masternode's voting key (if present in local wallet)\n"
        + HELP_REQUIRING_PASSPHRASE,
        {
            {"governance-hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "hash of the governance object"},
            {"vote", RPCArg::Type::STR, RPCArg::Optional::NO, "vote, possible values: [funding|valid|delete|endorsed]"},
            {"vote-outcome", RPCArg::Type::STR, RPCArg::Optional::NO, "vote outcome, possible values: [yes|no|abstain]"},
            {"protx-hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "masternode's proTxHash"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    const NodeContext& node = EnsureAnyNodeContext(request.context);

    uint256 hash(ParseHashV(request.params[0], "Object hash"));
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

    EnsureWalletIsUnlocked(*wallet);

    uint256 proTxHash(ParseHashV(request.params[3], "protx-hash"));
    auto dmn = CHECK_NONFATAL(node.dmnman)->GetListAtChainTip().GetValidMN(proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid or unknown proTxHash");
    }


    const bool is_mine = CheckWalletOwnsKey(*wallet, dmn->pdmnState->keyIDVoting);
    if (!is_mine) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Private key for voting address %s not known by wallet", EncodeDestination(PKHash(dmn->pdmnState->keyIDVoting))));
    }

    std::map<uint256, CKeyID> votingKeys;
    votingKeys.emplace(proTxHash, dmn->pdmnState->keyIDVoting);

    return VoteWithMasternodes(request, *wallet, votingKeys, hash, eVoteSignal, eVoteOutcome);
},
    };
}
#endif

static UniValue ListObjects(CGovernanceManager& govman, const CDeterministicMNList& tip_mn_list, const ChainstateManager& chainman,
                            const std::string& strCachedSignal, const std::string& strType, int nStartTime)
{
    UniValue objResult(UniValue::VOBJ);

    // GET MATCHING GOVERNANCE OBJECTS

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    LOCK2(cs_main, govman.cs);

    std::vector<CGovernanceObject> objs;
    govman.GetAllNewerThan(objs, nStartTime);

    govman.UpdateLastDiffTime(GetTime());
    // CREATE RESULTS FOR USER

    for (const auto& govObj : objs) {
        if (strCachedSignal == "valid" && !govObj.IsSetCachedValid()) continue;
        if (strCachedSignal == "funding" && !govObj.IsSetCachedFunding()) continue;
        if (strCachedSignal == "delete" && !govObj.IsSetCachedDelete()) continue;
        if (strCachedSignal == "endorsed" && !govObj.IsSetCachedEndorsed()) continue;

        if (strType == "proposals" && govObj.GetObjectType() != GovernanceObject::PROPOSAL) continue;
        if (strType == "triggers" && govObj.GetObjectType() != GovernanceObject::TRIGGER) continue;

        UniValue bObj(UniValue::VOBJ);
        bObj.pushKV("DataHex",  govObj.GetDataAsHexString());
        bObj.pushKV("DataString",  govObj.GetDataAsPlainString());
        bObj.pushKV("Hash",  govObj.GetHash().ToString());
        bObj.pushKV("CollateralHash",  govObj.GetCollateralHash().ToString());
        bObj.pushKV("ObjectType", ToUnderlying(govObj.GetObjectType()));
        bObj.pushKV("CreationTime", govObj.GetCreationTime());
        const COutPoint& masternodeOutpoint = govObj.GetMasternodeOutpoint();
        if (masternodeOutpoint != COutPoint()) {
            bObj.pushKV("SigningMasternode", masternodeOutpoint.ToStringShort());
        }

        // REPORT STATUS FOR FUNDING VOTES SPECIFICALLY
        bObj.pushKV("AbsoluteYesCount",  govObj.GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_FUNDING));
        bObj.pushKV("YesCount",  govObj.GetYesCount(tip_mn_list, VOTE_SIGNAL_FUNDING));
        bObj.pushKV("NoCount",  govObj.GetNoCount(tip_mn_list, VOTE_SIGNAL_FUNDING));
        bObj.pushKV("AbstainCount",  govObj.GetAbstainCount(tip_mn_list, VOTE_SIGNAL_FUNDING));

        // REPORT VALIDITY AND CACHING FLAGS FOR VARIOUS SETTINGS
        std::string strError;
        bObj.pushKV("fBlockchainValidity",  govObj.IsValidLocally(tip_mn_list, chainman, strError, false));
        bObj.pushKV("IsValidReason",  strError.c_str());
        bObj.pushKV("fCachedValid",  govObj.IsSetCachedValid());
        bObj.pushKV("fCachedFunding",  govObj.IsSetCachedFunding());
        bObj.pushKV("fCachedDelete",  govObj.IsSetCachedDelete());
        bObj.pushKV("fCachedEndorsed",  govObj.IsSetCachedEndorsed());

        objResult.pushKV(govObj.GetHash().ToString(), bObj);
    }

    return objResult;
}

// USERS CAN QUERY THE SYSTEM FOR A LIST OF VARIOUS GOVERNANCE ITEMS
static RPCHelpMan gobject_list_helper(const bool make_a_diff)
{
    const std::string command{make_a_diff ? "gobject diff" : "gobject list"};
    const std::string description{make_a_diff ? "List differences since last diff or list\n"
            : "List governance objects (can be filtered by signal and/or object type)\n"};

    return RPCHelpMan{command,
        description,
        {
            {"signal", RPCArg::Type::STR, RPCArg::Default{"valid"}, "cached signal, possible values: [valid|funding|delete|endorsed|all]"},
            {"type", RPCArg::Type::STR, RPCArg::Default{"all"}, "object type, possible values: [proposals|triggers|all]"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string strCachedSignal = "valid";
    if (!request.params[0].isNull()) {
        strCachedSignal = request.params[0].get_str();
    }
    if (strCachedSignal != "valid" && strCachedSignal != "funding" && strCachedSignal != "delete" && strCachedSignal != "endorsed" && strCachedSignal != "all")
        return "Invalid signal, should be 'valid', 'funding', 'delete', 'endorsed' or 'all'";

    std::string strType = "all";
    if (!request.params[1].isNull()) {
        strType = request.params[1].get_str();
    }
    if (strType != "proposals" && strType != "triggers" && strType != "all")
        return "Invalid type, should be 'proposals', 'triggers' or 'all'";

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    const int64_t last_time = make_a_diff ? node.govman->GetLastDiffTime() : 0;
    return ListObjects(*CHECK_NONFATAL(node.govman), CHECK_NONFATAL(node.dmnman)->GetListAtChainTip(), chainman,
                       strCachedSignal, strType, last_time);
},
    };
}

static RPCHelpMan gobject_list()
{
    return gobject_list_helper(false);
}

static RPCHelpMan gobject_diff()
{
    return gobject_list_helper(true);
}

// GET SPECIFIC GOVERNANCE ENTRY
static RPCHelpMan gobject_get()
{
    return RPCHelpMan{"gobject get",
        "Get governance object by hash\n",
        {
            {"governance-hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "object id"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // COLLECT VARIABLES FROM OUR USER
    uint256 hash(ParseHashV(request.params[0], "GovObj hash"));

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    // FIND THE GOVERNANCE OBJECT THE USER IS LOOKING FOR
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    CHECK_NONFATAL(node.govman);
    LOCK2(cs_main, node.govman->cs);
    const CGovernanceObject* pGovObj = node.govman->FindConstGovernanceObject(hash);

    if (pGovObj == nullptr) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown governance object");
    }

    // REPORT BASIC OBJECT STATS

    UniValue objResult(UniValue::VOBJ);
    objResult.pushKV("DataHex",  pGovObj->GetDataAsHexString());
    objResult.pushKV("DataString",  pGovObj->GetDataAsPlainString());
    objResult.pushKV("Hash",  pGovObj->GetHash().ToString());
    objResult.pushKV("CollateralHash",  pGovObj->GetCollateralHash().ToString());
    objResult.pushKV("ObjectType", ToUnderlying(pGovObj->GetObjectType()));
    objResult.pushKV("CreationTime", pGovObj->GetCreationTime());
    const COutPoint& masternodeOutpoint = pGovObj->GetMasternodeOutpoint();
    if (masternodeOutpoint != COutPoint()) {
        objResult.pushKV("SigningMasternode", masternodeOutpoint.ToStringShort());
    }

    // SHOW (MUCH MORE) INFORMATION ABOUT VOTES FOR GOVERNANCE OBJECT (THAN LIST/DIFF ABOVE)
    // -- FUNDING VOTING RESULTS

    auto tip_mn_list = CHECK_NONFATAL(node.dmnman)->GetListAtChainTip();

    UniValue objFundingResult(UniValue::VOBJ);
    objFundingResult.pushKV("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_FUNDING));
    objFundingResult.pushKV("YesCount",  pGovObj->GetYesCount(tip_mn_list, VOTE_SIGNAL_FUNDING));
    objFundingResult.pushKV("NoCount",  pGovObj->GetNoCount(tip_mn_list, VOTE_SIGNAL_FUNDING));
    objFundingResult.pushKV("AbstainCount",  pGovObj->GetAbstainCount(tip_mn_list, VOTE_SIGNAL_FUNDING));
    objResult.pushKV("FundingResult", objFundingResult);

    // -- VALIDITY VOTING RESULTS
    UniValue objValid(UniValue::VOBJ);
    objValid.pushKV("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_VALID));
    objValid.pushKV("YesCount",  pGovObj->GetYesCount(tip_mn_list, VOTE_SIGNAL_VALID));
    objValid.pushKV("NoCount",  pGovObj->GetNoCount(tip_mn_list, VOTE_SIGNAL_VALID));
    objValid.pushKV("AbstainCount",  pGovObj->GetAbstainCount(tip_mn_list, VOTE_SIGNAL_VALID));
    objResult.pushKV("ValidResult", objValid);

    // -- DELETION CRITERION VOTING RESULTS
    UniValue objDelete(UniValue::VOBJ);
    objDelete.pushKV("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_DELETE));
    objDelete.pushKV("YesCount",  pGovObj->GetYesCount(tip_mn_list, VOTE_SIGNAL_DELETE));
    objDelete.pushKV("NoCount",  pGovObj->GetNoCount(tip_mn_list, VOTE_SIGNAL_DELETE));
    objDelete.pushKV("AbstainCount",  pGovObj->GetAbstainCount(tip_mn_list, VOTE_SIGNAL_DELETE));
    objResult.pushKV("DeleteResult", objDelete);

    // -- ENDORSED VIA MASTERNODE-ELECTED BOARD
    UniValue objEndorsed(UniValue::VOBJ);
    objEndorsed.pushKV("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_ENDORSED));
    objEndorsed.pushKV("YesCount",  pGovObj->GetYesCount(tip_mn_list, VOTE_SIGNAL_ENDORSED));
    objEndorsed.pushKV("NoCount",  pGovObj->GetNoCount(tip_mn_list, VOTE_SIGNAL_ENDORSED));
    objEndorsed.pushKV("AbstainCount",  pGovObj->GetAbstainCount(tip_mn_list, VOTE_SIGNAL_ENDORSED));
    objResult.pushKV("EndorsedResult", objEndorsed);

    // --
    std::string strError;
    objResult.pushKV("fLocalValidity",  pGovObj->IsValidLocally(tip_mn_list, chainman, strError, false));
    objResult.pushKV("IsValidReason",  strError.c_str());
    objResult.pushKV("fCachedValid",  pGovObj->IsSetCachedValid());
    objResult.pushKV("fCachedFunding",  pGovObj->IsSetCachedFunding());
    objResult.pushKV("fCachedDelete",  pGovObj->IsSetCachedDelete());
    objResult.pushKV("fCachedEndorsed",  pGovObj->IsSetCachedEndorsed());
    return objResult;
},
    };
}

// GET VOTES FOR SPECIFIC GOVERNANCE OBJECT
static RPCHelpMan gobject_getcurrentvotes()
{
    return RPCHelpMan{"gobject getcurrentvotes",
        "Get only current (tallying) votes for a governance object hash (does not include old votes)\n",
        {
            {"governance-hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "object id"},
            {"txid", RPCArg::Type::STR_HEX, RPCArg::Default{""}, "masternode collateral txid"},
            {"vout", RPCArg::Type::STR, RPCArg::Default{""}, "masternode collateral output index, required if <txid> present"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // COLLECT PARAMETERS FROM USER

    uint256 hash(ParseHashV(request.params[0], "Governance hash"));

    COutPoint mnCollateralOutpoint;
    if (!request.params[1].isNull() && !request.params[2].isNull()) {
        uint256 txid(ParseHashV(request.params[1], "Masternode Collateral hash"));
        std::string strVout = request.params[2].get_str();
        mnCollateralOutpoint = COutPoint(txid, LocaleIndependentAtoi<uint32_t>(strVout));
    }

    // FIND OBJECT USER IS LOOKING FOR
    const NodeContext& node = EnsureAnyNodeContext(request.context);

    CHECK_NONFATAL(node.govman);
    LOCK(node.govman->cs);
    const CGovernanceObject* pGovObj = node.govman->FindConstGovernanceObject(hash);

    if (pGovObj == nullptr) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown governance-hash");
    }

    // REPORT RESULTS TO USER

    UniValue bResult(UniValue::VOBJ);

    // GET MATCHING VOTES BY HASH, THEN SHOW USERS VOTE INFORMATION
    std::vector<CGovernanceVote> vecVotes = node.govman->GetCurrentVotes(hash, mnCollateralOutpoint);
    for (const auto& vote : vecVotes) {
        bResult.pushKV(vote.GetHash().ToString(), vote.ToString(CHECK_NONFATAL(node.dmnman)->GetListAtChainTip()));
    }

    return bResult;
},
    };
}

static RPCHelpMan gobject()
{
    return RPCHelpMan{"gobject",
        "Set of commands to manage governance objects.\n"
        "\nAvailable commands:\n"
        "  check              - Validate governance object data (proposal only)\n"
#ifdef ENABLE_WALLET
        "  prepare            - Prepare governance object by signing and creating tx\n"
        "  list-prepared      - Returns a list of governance objects prepared by this wallet with \"gobject prepare\"\n"
#endif // ENABLE_WALLET
        "  submit             - Submit governance object to network\n"
        "  deserialize        - Deserialize governance object from hex string to JSON\n"
        "  count              - Count governance objects and votes (additional param: 'json' or 'all', default: 'json')\n"
        "  get                - Get governance object by hash\n"
        "  getcurrentvotes    - Get only current (tallying) votes for a governance object hash (does not include old votes)\n"
        "  list               - List governance objects (can be filtered by signal and/or object type)\n"
        "  diff               - List differences since last diff\n"
#ifdef ENABLE_WALLET
        "  vote-alias         - Vote on a governance object by masternode proTxHash\n"
        "  vote-many          - Vote on a governance object by all masternodes for which the voting key is in the wallet\n"
#endif // ENABLE_WALLET
        ,
        {
            {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "The command to execute"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Must be a valid command");
},
    };
}

static RPCHelpMan voteraw()
{
    return RPCHelpMan{"voteraw",
        "Compile and relay a governance vote with provided external signature instead of signing vote internally\n",
        {
            {"mn-collateral-tx-hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, ""},
            {"mn-collateral-tx-index", RPCArg::Type::NUM, RPCArg::Optional::NO, ""},
            {"governance-hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, ""},
            {"vote-signal", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
            {"vote-outcome", RPCArg::Type::STR, RPCArg::Optional::NO, "yes|no|abstain"},
            {"time", RPCArg::Type::NUM, RPCArg::Optional::NO, ""},
            {"vote-sig", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, ""},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    uint256 hashMnCollateralTx(ParseHashV(request.params[0], "mn collateral tx hash"));
    int nMnCollateralTxIndex = request.params[1].get_int();
    COutPoint outpoint = COutPoint(hashMnCollateralTx, nMnCollateralTxIndex);

    uint256 hashGovObj(ParseHashV(request.params[2], "Governance hash"));
    std::string strVoteSignal = request.params[3].get_str();
    std::string strVoteOutcome = request.params[4].get_str();

    vote_signal_enum_t eVoteSignal = CGovernanceVoting::ConvertVoteSignal(strVoteSignal);
    if (eVoteSignal == VOTE_SIGNAL_NONE)  {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "Invalid vote signal. Please using one of the following: "
                           "(funding|valid|delete|endorsed)");
    }

    vote_outcome_enum_t eVoteOutcome = CGovernanceVoting::ConvertVoteOutcome(strVoteOutcome);
    if (eVoteOutcome == VOTE_OUTCOME_NONE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote outcome. Please use one of the following: 'yes', 'no' or 'abstain'");
    }

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    CHECK_NONFATAL(node.govman);
    GovernanceObject govObjType = [&]() {
        LOCK(node.govman->cs);
        const CGovernanceObject *pGovObj = node.govman->FindConstGovernanceObject(hashGovObj);
        if (!pGovObj) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Governance object not found");
        }
        return pGovObj->GetObjectType();
    }();


    int64_t nTime = request.params[5].get_int64();
    std::string strSig = request.params[6].get_str();
    auto opt_vchSig = DecodeBase64(strSig);

    if (!opt_vchSig.has_value()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");
    }

    const auto tip_mn_list = CHECK_NONFATAL(node.dmnman)->GetListAtChainTip();
    auto dmn = tip_mn_list.GetValidMNByCollateral(outpoint);

    if (!dmn) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failure to find masternode in list : " + outpoint.ToStringShort());
    }

    CGovernanceVote vote(outpoint, hashGovObj, eVoteSignal, eVoteOutcome);
    vote.SetTime(nTime);
    vote.SetSignature(opt_vchSig.value());

    bool onlyVotingKeyAllowed = govObjType == GovernanceObject::PROPOSAL && vote.GetSignal() == VOTE_SIGNAL_FUNDING;

    if (!vote.IsValid(tip_mn_list, onlyVotingKeyAllowed)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failure to verify vote.");
    }

    CConnman& connman = EnsureConnman(node);
    PeerManager& peerman = EnsurePeerman(node);

    CGovernanceException exception;
    if (node.govman->ProcessVoteAndRelay(vote, exception, connman, peerman)) {
        return "Voted successfully";
    } else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Error voting : " + exception.GetMessage());
    }
},
    };
}

static RPCHelpMan getgovernanceinfo()
{
    return RPCHelpMan{"getgovernanceinfo",
        "Returns an object containing governance parameters.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "governanceminquorum", "the absolute minimum number of votes needed to trigger a governance action"},
                {RPCResult::Type::NUM, "proposalfee", "the collateral transaction fee which must be paid to create a proposal in " + CURRENCY_UNIT + ""},
                {RPCResult::Type::NUM, "superblockcycle", "the number of blocks between superblocks"},
                {RPCResult::Type::NUM, "superblockmaturitywindow", "the superblock trigger creation window"},
                {RPCResult::Type::NUM, "lastsuperblock", "the block number of the last superblock"},
                {RPCResult::Type::NUM, "nextsuperblock", "the block number of the next superblock"},
                {RPCResult::Type::NUM, "fundingthreshold", "the number of absolute yes votes required for a proposal to be passing"},
                {RPCResult::Type::NUM, "governancebudget", "the governance budget for the next superblock in " + CURRENCY_UNIT + ""},
            }},
        RPCExamples{
            HelpExampleCli("getgovernanceinfo", "")
    + HelpExampleRpc("getgovernanceinfo", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    int nLastSuperblock = 0, nNextSuperblock = 0;

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureAnyChainman(request.context);

    const auto* pindex = WITH_LOCK(cs_main, return chainman.ActiveChain().Tip());
    int nBlockHeight = pindex->nHeight;

    CSuperblock::GetNearestSuperblocksHeights(nBlockHeight, nLastSuperblock, nNextSuperblock);

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("governanceminquorum", Params().GetConsensus().nGovernanceMinQuorum);
    obj.pushKV("proposalfee", ValueFromAmount(GOVERNANCE_PROPOSAL_FEE_TX));
    obj.pushKV("superblockcycle", Params().GetConsensus().nSuperblockCycle);
    obj.pushKV("superblockmaturitywindow", Params().GetConsensus().nSuperblockMaturityWindow);
    obj.pushKV("lastsuperblock", nLastSuperblock);
    obj.pushKV("nextsuperblock", nNextSuperblock);
    obj.pushKV("fundingthreshold", int(CHECK_NONFATAL(node.dmnman)->GetListAtChainTip().GetValidWeightedMNsCount() / 10));
    obj.pushKV("governancebudget", ValueFromAmount(CSuperblock::GetPaymentsLimit(chainman.ActiveChain(), nNextSuperblock)));

    return obj;
},
    };
}

static RPCHelpMan getsuperblockbudget()
{
    return RPCHelpMan{"getsuperblockbudget",
        "\nReturns the absolute maximum sum of superblock payments allowed.\n",
        {
            {"index", RPCArg::Type::NUM, RPCArg::Optional::NO, "The block index"},
        },
        RPCResult{
            RPCResult::Type::NUM, "n", "The absolute maximum sum of superblock payments allowed, in " + CURRENCY_UNIT
        },
        RPCExamples{
            HelpExampleCli("getsuperblockbudget", "1000")
    + HelpExampleRpc("getsuperblockbudget", "1000")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    int nBlockHeight = request.params[0].get_int();
    if (nBlockHeight < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
    }

    const ChainstateManager& chainman = EnsureAnyChainman(request.context);
    return ValueFromAmount(CSuperblock::GetPaymentsLimit(chainman.ActiveChain(), nBlockHeight));
},
    };
}

#ifdef ENABLE_WALLET
Span<const CRPCCommand> GetWalletGovernanceRPCCommands()
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              actor (function)
  //  --------------------- -----------------------
    { "dash",               &gobject_prepare,           },
    { "dash",               &gobject_list_prepared,     },
    { "dash",               &gobject_vote_many,         },
    { "dash",               &gobject_vote_alias,        },
};
// clang-format on
    return commands;
}
#endif // ENABLE_WALLET

void RegisterGovernanceRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              actor (function)
  //  --------------------- -----------------------
    /* Dash features */
    { "dash",               &getgovernanceinfo,         },
    { "dash",               &getsuperblockbudget,       },
    { "dash",               &gobject,                   },
    { "dash",               &gobject_count,             },
    { "dash",               &gobject_deserialize,       },
    { "dash",               &gobject_check,             },
    { "dash",               &gobject_submit,            },
    { "dash",               &gobject_list,              },
    { "dash",               &gobject_diff,              },
    { "dash",               &gobject_get,               },
    { "dash",               &gobject_getcurrentvotes,   },
    { "dash",               &voteraw,                   },
};
// clang-format on
    for (const auto& command : commands) {
        t.appendCommand(command.name, &command);
    }
}
