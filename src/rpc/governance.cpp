// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//#define ENABLE_DASH_DEBUG

#include "activemasternode.h"
#include "consensus/validation.h"
#include "governance.h"
#include "governance-vote.h"
#include "governance-classes.h"
#include "governance-validators.h"
#include "init.h"
#include "validation.h"
#include "masternode.h"
#include "masternode-sync.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "messagesigner.h"
#include "rpc/server.h"
#include "util.h"
#include "utilmoneystr.h"
#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif // ENABLE_WALLET

bool EnsureWalletIsAvailable(bool avoidException);

void gobject_count_help()
{
    throw std::runtime_error(
                "gobject count (\"mode\")\n"
                "Count governance objects and votes\n"
                "\nArguments:\n"
                "1. \"mode\"   (string, optional, default: \"json\") Output format: json (\"json\") or string in free form (\"all\")\n"
                );
}

UniValue gobject_count(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 2)
        gobject_count_help();

    std::string strMode{"json"};

    if (request.params.size() == 2) {
        strMode = request.params[1].get_str();
    }

    if (strMode != "json" && strMode != "all")
        gobject_count_help();

    return strMode == "json" ? governance.ToJson() : governance.ToString();
}

void gobject_deserialize_help()
{
    throw std::runtime_error(
                "gobject deserialize \"hex_data\"\n"
                "Deserialize governance object from hex string to JSON\n"
                "\nArguments:\n"
                "1. \"hex_data\"   (string, required) data in hex string form\n"
                );
}

UniValue gobject_deserialize(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        gobject_deserialize_help();

    std::string strHex = request.params[1].get_str();

    std::vector<unsigned char> v = ParseHex(strHex);
    std::string s(v.begin(), v.end());

    UniValue u(UniValue::VOBJ);
    u.read(s);

    return u.write().c_str();
}

void gobject_check_help()
{
    throw std::runtime_error(
                "gobject check \"hex_data\"\n"
                "Validate governance object data (proposal only)\n"
                "\nArguments:\n"
                "1. \"hex_data\"   (string, required) data in hex string form\n"
                );
}

UniValue gobject_check(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        gobject_check_help();

    // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

    uint256 hashParent;

    int nRevision = 1;

    int64_t nTime = GetAdjustedTime();
    std::string strDataHex = request.params[1].get_str();

    CGovernanceObject govobj(hashParent, nRevision, nTime, uint256(), strDataHex);

    if (govobj.GetObjectType() == GOVERNANCE_OBJECT_PROPOSAL) {
        CProposalValidator validator(strDataHex);
        if (!validator.Validate())  {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid proposal data, error messages:" + validator.GetErrorMessages());
        }
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid object type, only proposals can be validated");
    }

    UniValue objResult(UniValue::VOBJ);

    objResult.push_back(Pair("Object status", "OK"));

    return objResult;
}

#ifdef ENABLE_WALLET
void gobject_prepare_help()
{
    throw std::runtime_error(
                "gobject prepare <parent-hash> <revision> <time> <data-hex>\n"
                "Prepare governance object by signing and creating tx\n"
                "\nArguments:\n"
                "1. parent-hash   (string, required) hash of the parent object, \"0\" is root\n"
                "2. revision      (numeric, required) object revision in the system\n"
                "3. time          (numeric, required) time this object was created\n"
                "4. data-hex      (string, required)  data in hex string form\n"
                );
}

UniValue gobject_prepare(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 5)
        gobject_prepare_help();

    if (!EnsureWalletIsAvailable(request.fHelp))
        return NullUniValue;

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
    int nRevision = atoi(strRevision);
    int64_t nTime = atoi64(strTime);
    std::string strDataHex = request.params[4].get_str();

    // CREATE A NEW COLLATERAL TRANSACTION FOR THIS SPECIFIC OBJECT

    CGovernanceObject govobj(hashParent, nRevision, nTime, uint256(), strDataHex);

    // This command is dangerous because it consumes 5 DASH irreversibly.
    // If params are lost, it's very hard to bruteforce them and yet
    // users ignore all instructions on dashcentral etc. and do not save them...
    // Let's log them here and hope users do not mess with debug.log
    LogPrintf("gobject_prepare -- params: %s %s %s %s, data: %s, hash: %s\n",
                request.params[1].get_str(), request.params[2].get_str(),
                request.params[3].get_str(), request.params[4].get_str(),
                govobj.GetDataAsPlainString(), govobj.GetHash().ToString());

    if (govobj.GetObjectType() == GOVERNANCE_OBJECT_PROPOSAL) {
        CProposalValidator validator(strDataHex);
        if (!validator.Validate()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid proposal data, error messages:" + validator.GetErrorMessages());
        }
    }

    if (govobj.GetObjectType() == GOVERNANCE_OBJECT_TRIGGER) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Trigger objects need not be prepared (however only masternodes can create them)");
    }

    if (govobj.GetObjectType() == GOVERNANCE_OBJECT_WATCHDOG) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Watchdogs are deprecated");
    }

    LOCK2(cs_main, pwalletMain->cs_wallet);

    std::string strError = "";
    if (!govobj.IsValidLocally(strError, false))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Governance object is not valid - " + govobj.GetHash().ToString() + " - " + strError);

    EnsureWalletIsUnlocked();

    CWalletTx wtx;
    if (!pwalletMain->GetBudgetSystemCollateralTX(wtx, govobj.GetHash(), govobj.GetMinCollateralFee(), false)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Error making collateral transaction for governance object. Please check your wallet balance and make sure your wallet is unlocked.");
    }

    // -- make our change address
    CReserveKey reservekey(pwalletMain);
    // -- send the tx to the network
    CValidationState state;
    if (!pwalletMain->CommitTransaction(wtx, reservekey, g_connman.get(), state, NetMsgType::TX)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "CommitTransaction failed! Reason given: " + state.GetRejectReason());
    }

    DBG( std::cout << "gobject: prepare "
         << " GetDataAsPlainString = " << govobj.GetDataAsPlainString()
         << ", hash = " << govobj.GetHash().GetHex()
         << ", txidFee = " << wtx.GetHash().GetHex()
         << std::endl; );

    return wtx.GetHash().ToString();
}
#endif // ENABLE_WALLET

void gobject_submit_help()
{
    throw std::runtime_error(
                "gobject submit <parent-hash> <revision> <time> <data-hex> <fee-txid>\n"
                "Submit governance object to network\n"
                "\nArguments:\n"
                "1. parent-hash   (string, required) hash of the parent object, \"0\" is root\n"
                "2. revision      (numeric, required) object revision in the system\n"
                "3. time          (numeric, required) time this object was created\n"
                "4. data-hex      (string, required) data in hex string form\n"
                "5. fee-txid      (string, optional) fee-tx id, required for all objects except triggers"
                );
}

UniValue gobject_submit(const JSONRPCRequest& request)
{
    if (request.fHelp || ((request.params.size() < 5) || (request.params.size() > 6)))
        gobject_submit_help();

    if(!masternodeSync.IsBlockchainSynced()) {
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Must wait for client to sync with masternode network. Try again in a minute or so.");
    }

    bool fMnFound = mnodeman.Has(activeMasternodeInfo.outpoint);

    DBG( std::cout << "gobject: submit activeMasternodeInfo.keyIDOperator = " << activeMasternodeInfo.keyIDOperator.ToString()
         << ", outpoint = " << activeMasternodeInfo.outpoint.ToStringShort()
         << ", params.size() = " << request.params.size()
         << ", fMnFound = " << fMnFound << std::endl; );

    // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

    uint256 txidFee;

    if (request.params.size() == 6) {
        txidFee = ParseHashV(request.params[5], "fee-txid, parameter 6");
    }
    uint256 hashParent;
    if (request.params[1].get_str() == "0") { // attach to root node (root node doesn't really exist, but has a hash of zero)
        hashParent = uint256();
    } else {
        hashParent = ParseHashV(request.params[1], "parent object hash, parameter 2");
    }

    // GET THE PARAMETERS FROM USER

    std::string strRevision = request.params[2].get_str();
    std::string strTime = request.params[3].get_str();
    int nRevision = atoi(strRevision);
    int64_t nTime = atoi64(strTime);
    std::string strDataHex = request.params[4].get_str();

    CGovernanceObject govobj(hashParent, nRevision, nTime, txidFee, strDataHex);

    DBG( std::cout << "gobject: submit "
         << " GetDataAsPlainString = " << govobj.GetDataAsPlainString()
         << ", hash = " << govobj.GetHash().GetHex()
         << ", txidFee = " << txidFee.GetHex()
         << std::endl; );

    if (govobj.GetObjectType() == GOVERNANCE_OBJECT_PROPOSAL) {
        CProposalValidator validator(strDataHex);
        if (!validator.Validate()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid proposal data, error messages:" + validator.GetErrorMessages());
        }
    }

    if (govobj.GetObjectType() == GOVERNANCE_OBJECT_WATCHDOG) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Watchdogs are deprecated");
    }

    // Attempt to sign triggers if we are a MN
    if (govobj.GetObjectType() == GOVERNANCE_OBJECT_TRIGGER) {
        if (fMnFound) {
            govobj.SetMasternodeOutpoint(activeMasternodeInfo.outpoint);
            govobj.Sign(activeMasternodeInfo.keyOperator, activeMasternodeInfo.keyIDOperator);
        } else {
            LogPrintf("gobject(submit) -- Object submission rejected because node is not a masternode\n");
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Only valid masternodes can submit this type of object");
        }
    } else if (request.params.size() != 6) {
        LogPrintf("gobject(submit) -- Object submission rejected because fee tx not provided\n");
        throw JSONRPCError(RPC_INVALID_PARAMETER, "The fee-txid parameter must be included to submit this type of object");
    }

    std::string strHash = govobj.GetHash().ToString();

    std::string strError = "";
    bool fMissingMasternode;
    bool fMissingConfirmations;
    {
        LOCK(cs_main);
        if (!govobj.IsValidLocally(strError, fMissingMasternode, fMissingConfirmations, true) && !fMissingConfirmations) {
            LogPrintf("gobject(submit) -- Object submission rejected because object is not valid - hash = %s, strError = %s\n", strHash, strError);
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Governance object is not valid - " + strHash + " - " + strError);
        }
    }

    // RELAY THIS OBJECT
    // Reject if rate check fails but don't update buffer
    if (!governance.MasternodeRateCheck(govobj)) {
        LogPrintf("gobject(submit) -- Object submission rejected because of rate check failure - hash = %s\n", strHash);
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Object creation rate limit exceeded");
    }

    LogPrintf("gobject(submit) -- Adding locally created governance object - %s\n", strHash);

    if (fMissingConfirmations) {
        governance.AddPostponedObject(govobj);
        govobj.Relay(*g_connman);
    } else {
        governance.AddGovernanceObject(govobj, *g_connman);
    }

    return govobj.GetHash().ToString();
}

void gobject_vote_conf_help()
{
    throw std::runtime_error(
                "gobject vote-conf <governance-hash> <vote> <vote-outcome>\n"
                "Vote on a governance object by masternode configured in dash.conf\n"
                "\nArguments:\n"
                "1. governance-hash   (string, required) hash of the governance object\n"
                "2. vote              (string, required) vote, possible values: [funding|valid|delete|endorsed]\n"
                "3. vote-outcome      (string, required) vote outcome, possible values: [yes|no|abstain]\n"
                );
}

UniValue gobject_vote_conf(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 4)
        gobject_vote_conf_help();

    uint256 hash;

    hash = ParseHashV(request.params[1], "Object hash");
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

    int govObjType;
    {
        LOCK(governance.cs);
        CGovernanceObject *pGovObj = governance.FindGovernanceObject(hash);
        if (!pGovObj) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Governance object not found");
        }
        govObjType = pGovObj->GetObjectType();
    }

    int nSuccessful = 0;
    int nFailed = 0;

    UniValue resultsObj(UniValue::VOBJ);

    UniValue statusObj(UniValue::VOBJ);
    UniValue returnObj(UniValue::VOBJ);

    CMasternode mn;
    bool fMnFound = mnodeman.Get(activeMasternodeInfo.outpoint, mn);

    if (!fMnFound) {
        nFailed++;
        statusObj.push_back(Pair("result", "failed"));
        statusObj.push_back(Pair("errorMessage", "Can't find masternode by collateral output"));
        resultsObj.push_back(Pair("dash.conf", statusObj));
        returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed)));
        returnObj.push_back(Pair("detail", resultsObj));
        return returnObj;
    }

    if (deterministicMNManager->IsDeterministicMNsSporkActive()) {
        if (govObjType == GOVERNANCE_OBJECT_PROPOSAL && mn.keyIDVoting != activeMasternodeInfo.keyIDOperator) {
            nFailed++;
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "Can't vote on proposal when operator key does not match voting key"));
            resultsObj.push_back(Pair("dash.conf", statusObj));
            returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed)));
            returnObj.push_back(Pair("detail", resultsObj));
            return returnObj;
        }
    }

    CGovernanceVote vote(mn.outpoint, hash, eVoteSignal, eVoteOutcome);
    if (!vote.Sign(activeMasternodeInfo.keyOperator, activeMasternodeInfo.keyIDOperator)) {
        nFailed++;
        statusObj.push_back(Pair("result", "failed"));
        statusObj.push_back(Pair("errorMessage", "Failure to sign."));
        resultsObj.push_back(Pair("dash.conf", statusObj));
        returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed)));
        returnObj.push_back(Pair("detail", resultsObj));
        return returnObj;
    }

    CGovernanceException exception;
    if (governance.ProcessVoteAndRelay(vote, exception, *g_connman)) {
        nSuccessful++;
        statusObj.push_back(Pair("result", "success"));
    } else {
        nFailed++;
        statusObj.push_back(Pair("result", "failed"));
        statusObj.push_back(Pair("errorMessage", exception.GetMessage()));
    }

    resultsObj.push_back(Pair("dash.conf", statusObj));

    returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed)));
    returnObj.push_back(Pair("detail", resultsObj));

    return returnObj;
}

UniValue VoteWithMasternodeList(const std::vector<CMasternodeConfig::CMasternodeEntry>& entries,
                                const uint256& hash, vote_signal_enum_t eVoteSignal,
                                vote_outcome_enum_t eVoteOutcome)
{
    int govObjType;
    {
        LOCK(governance.cs);
        CGovernanceObject *pGovObj = governance.FindGovernanceObject(hash);
        if (!pGovObj) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Governance object not found");
        }
        govObjType = pGovObj->GetObjectType();
    }

    int nSuccessful = 0;
    int nFailed = 0;

    UniValue resultsObj(UniValue::VOBJ);

    for (const auto& mne : entries) {
        CPubKey pubKeyOperator;
        CKey keyOperator;

        UniValue statusObj(UniValue::VOBJ);

        if (!CMessageSigner::GetKeysFromSecret(mne.getPrivKey(), keyOperator, pubKeyOperator)) {
            nFailed++;
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "Masternode signing error, could not set key correctly"));
            resultsObj.push_back(Pair(mne.getAlias(), statusObj));
            continue;
        }

        uint256 nTxHash;
        nTxHash.SetHex(mne.getTxHash());

        int nOutputIndex = 0;
        if (!ParseInt32(mne.getOutputIndex(), &nOutputIndex)) {
            continue;
        }

        COutPoint outpoint(nTxHash, nOutputIndex);

        CMasternode mn;
        bool fMnFound = mnodeman.Get(outpoint, mn);

        if (!fMnFound) {
            nFailed++;
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "Can't find masternode by collateral output"));
            resultsObj.push_back(Pair(mne.getAlias(), statusObj));
            continue;
        }

        if (deterministicMNManager->IsDeterministicMNsSporkActive()) {
            if (govObjType == GOVERNANCE_OBJECT_PROPOSAL && mn.keyIDVoting != pubKeyOperator.GetID()) {
                nFailed++;
                statusObj.push_back(Pair("result", "failed"));
                statusObj.push_back(Pair("errorMessage", "Can't vote on proposal when key does not match voting key"));
                resultsObj.push_back(Pair(mne.getAlias(), statusObj));
                continue;
            }
        }

        CGovernanceVote vote(mn.outpoint, hash, eVoteSignal, eVoteOutcome);
        if (!vote.Sign(keyOperator, pubKeyOperator.GetID())) {
            nFailed++;
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "Failure to sign."));
            resultsObj.push_back(Pair(mne.getAlias(), statusObj));
            continue;
        }

        CGovernanceException exception;
        if (governance.ProcessVoteAndRelay(vote, exception, *g_connman)) {
            nSuccessful++;
            statusObj.push_back(Pair("result", "success"));
        } else {
            nFailed++;
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", exception.GetMessage()));
        }

        resultsObj.push_back(Pair(mne.getAlias(), statusObj));
    }

    UniValue returnObj(UniValue::VOBJ);
    returnObj.push_back(Pair("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed)));
    returnObj.push_back(Pair("detail", resultsObj));

    return returnObj;
}

void gobject_vote_many_help()
{
    throw std::runtime_error(
                "gobject vote-many <governance-hash> <vote> <vote-outcome>\n"
                "Vote on a governance object by all masternodes (using masternode.conf setup)\n"
                "\nArguments:\n"
                "1. governance-hash   (string, required) hash of the governance object\n"
                "2. vote              (string, required) vote, possible values: [funding|valid|delete|endorsed]\n"
                "3. vote-outcome      (string, required) vote outcome, possible values: [yes|no|abstain]\n"
                );
}

UniValue gobject_vote_many(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 4)
        gobject_vote_many_help();

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

    std::vector<CMasternodeConfig::CMasternodeEntry> entries = masternodeConfig.getEntries();

#ifdef ENABLE_WALLET
    // This is a hack to maintain code-level backwards compatibility with masternode.conf and the deterministic masternodes.
    // Deterministic masternode keys are managed inside the wallet instead of masternode.conf
    // This allows voting on proposals when you have the MN voting key in your wallet
    // We can remove this when we remove support for masternode.conf and only support wallet based masternode
    // management
    if (deterministicMNManager->IsDeterministicMNsSporkActive()) {
        auto mnList = deterministicMNManager->GetListAtChainTip();
        for (const auto &dmn : mnList.valid_range()) {
            bool found = false;
            for (const auto &mne : entries) {
                uint256 nTxHash;
                nTxHash.SetHex(mne.getTxHash());

                int nOutputIndex = 0;
                if(!ParseInt32(mne.getOutputIndex(), &nOutputIndex)) {
                    continue;
                }

                if (nTxHash == dmn->proTxHash && (uint32_t)nOutputIndex == dmn->nCollateralIndex) {
                    found = true;
                    break;
                }
            }
            if (found) {
                continue;
            }

            CKey ownerKey;
            if (pwalletMain->GetKey(dmn->pdmnState->keyIDVoting, ownerKey)) {
                CBitcoinSecret secret(ownerKey);
                CMasternodeConfig::CMasternodeEntry mne(dmn->proTxHash.ToString(), dmn->pdmnState->addr.ToStringIPPort(false), secret.ToString(), dmn->proTxHash.ToString(), itostr(dmn->nCollateralIndex));
                entries.push_back(mne);
            }
        }
    }
#endif

    return VoteWithMasternodeList(entries, hash, eVoteSignal, eVoteOutcome);
}

void gobject_vote_alias_help()
{
    throw std::runtime_error(
                "gobject vote-alias <governance-hash> <vote> <vote-outcome> <alias-name>\n"
                "Vote on a governance object by masternode alias (using masternode.conf setup)\n"
                "\nArguments:\n"
                "1. governance-hash   (string, required) hash of the governance object\n"
                "2. vote              (string, required) vote, possible values: [funding|valid|delete|endorsed]\n"
                "3. vote-outcome      (string, required) vote outcome, possible values: [yes|no|abstain]\n"
                "4. alias-name        (string, required) masternode alias"
                );
}

UniValue gobject_vote_alias(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 5)
        gobject_vote_alias_help();

    uint256 hash = ParseHashV(request.params[1], "Object hash");
    std::string strVoteSignal = request.params[2].get_str();
    std::string strVoteOutcome = request.params[3].get_str();

    std::string strAlias = request.params[4].get_str();

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

    std::vector<CMasternodeConfig::CMasternodeEntry> entries;
    for (const auto& mne : masternodeConfig.getEntries()) {
        if (strAlias == mne.getAlias())
            entries.push_back(mne);
    }

    return VoteWithMasternodeList(entries, hash, eVoteSignal, eVoteOutcome);
}

UniValue ListObjects(const std::string& strCachedSignal, const std::string& strType, int nStartTime)
{
    UniValue objResult(UniValue::VOBJ);

    // GET MATCHING GOVERNANCE OBJECTS

    LOCK2(cs_main, governance.cs);

    std::vector<const CGovernanceObject*> objs = governance.GetAllNewerThan(nStartTime);
    governance.UpdateLastDiffTime(GetTime());

    // CREATE RESULTS FOR USER

    for (const auto& pGovObj : objs) {
        if (strCachedSignal == "valid" && !pGovObj->IsSetCachedValid()) continue;
        if (strCachedSignal == "funding" && !pGovObj->IsSetCachedFunding()) continue;
        if (strCachedSignal == "delete" && !pGovObj->IsSetCachedDelete()) continue;
        if (strCachedSignal == "endorsed" && !pGovObj->IsSetCachedEndorsed()) continue;

        if (strType == "proposals" && pGovObj->GetObjectType() != GOVERNANCE_OBJECT_PROPOSAL) continue;
        if (strType == "triggers" && pGovObj->GetObjectType() != GOVERNANCE_OBJECT_TRIGGER) continue;

        UniValue bObj(UniValue::VOBJ);
        bObj.push_back(Pair("DataHex",  pGovObj->GetDataAsHexString()));
        bObj.push_back(Pair("DataString",  pGovObj->GetDataAsPlainString()));
        bObj.push_back(Pair("Hash",  pGovObj->GetHash().ToString()));
        bObj.push_back(Pair("CollateralHash",  pGovObj->GetCollateralHash().ToString()));
        bObj.push_back(Pair("ObjectType", pGovObj->GetObjectType()));
        bObj.push_back(Pair("CreationTime", pGovObj->GetCreationTime()));
        const COutPoint& masternodeOutpoint = pGovObj->GetMasternodeOutpoint();
        if (masternodeOutpoint != COutPoint()) {
            bObj.push_back(Pair("SigningMasternode", masternodeOutpoint.ToStringShort()));
        }

        // REPORT STATUS FOR FUNDING VOTES SPECIFICALLY
        bObj.push_back(Pair("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING)));
        bObj.push_back(Pair("YesCount",  pGovObj->GetYesCount(VOTE_SIGNAL_FUNDING)));
        bObj.push_back(Pair("NoCount",  pGovObj->GetNoCount(VOTE_SIGNAL_FUNDING)));
        bObj.push_back(Pair("AbstainCount",  pGovObj->GetAbstainCount(VOTE_SIGNAL_FUNDING)));

        // REPORT VALIDITY AND CACHING FLAGS FOR VARIOUS SETTINGS
        std::string strError = "";
        bObj.push_back(Pair("fBlockchainValidity",  pGovObj->IsValidLocally(strError, false)));
        bObj.push_back(Pair("IsValidReason",  strError.c_str()));
        bObj.push_back(Pair("fCachedValid",  pGovObj->IsSetCachedValid()));
        bObj.push_back(Pair("fCachedFunding",  pGovObj->IsSetCachedFunding()));
        bObj.push_back(Pair("fCachedDelete",  pGovObj->IsSetCachedDelete()));
        bObj.push_back(Pair("fCachedEndorsed",  pGovObj->IsSetCachedEndorsed()));

        objResult.push_back(Pair(pGovObj->GetHash().ToString(), bObj));
    }

    return objResult;
}

void gobject_list_help()
{
    throw std::runtime_error(
                "gobject list ( <signal> <type> )\n"
                "List governance objects (can be filtered by signal and/or object type)\n"
                "\nArguments:\n"
                "1. signal   (string, optional, default=valid) cached signal, possible values: [valid|funding|delete|endorsed|all]\n"
                "2. type     (string, optional, default=all) object type, possible values: [proposals|triggers|all]\n"
                );
}

UniValue gobject_list(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 3)
        gobject_list_help();

    std::string strCachedSignal = "valid";
    if (request.params.size() >= 2) strCachedSignal = request.params[1].get_str();
    if (strCachedSignal != "valid" && strCachedSignal != "funding" && strCachedSignal != "delete" && strCachedSignal != "endorsed" && strCachedSignal != "all")
        return "Invalid signal, should be 'valid', 'funding', 'delete', 'endorsed' or 'all'";

    std::string strType = "all";
    if (request.params.size() == 3) strType = request.params[2].get_str();
    if (strType != "proposals" && strType != "triggers" && strType != "all")
        return "Invalid type, should be 'proposals', 'triggers' or 'all'";

    return ListObjects(strCachedSignal, strType, 0);
}

void gobject_diff_help()
{
    throw std::runtime_error(
                "gobject diff ( <signal> <type> )\n"
                "List differences since last diff or list\n"
                "\nArguments:\n"
                "1. signal   (string, optional, default=valid) cached signal, possible values: [valid|funding|delete|endorsed|all]\n"
                "2. type     (string, optional, default=all) object type, possible values: [proposals|triggers|all]\n"
                );
}

UniValue gobject_diff(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 3)
        gobject_diff_help();

    std::string strCachedSignal = "valid";
    if (request.params.size() >= 2) strCachedSignal = request.params[1].get_str();
    if (strCachedSignal != "valid" && strCachedSignal != "funding" && strCachedSignal != "delete" && strCachedSignal != "endorsed" && strCachedSignal != "all")
        return "Invalid signal, should be 'valid', 'funding', 'delete', 'endorsed' or 'all'";

    std::string strType = "all";
    if (request.params.size() == 3) strType = request.params[2].get_str();
    if (strType != "proposals" && strType != "triggers" && strType != "all")
        return "Invalid type, should be 'proposals', 'triggers' or 'all'";

    return ListObjects(strCachedSignal, strType, governance.GetLastDiffTime());
}

void gobject_get_help()
{
    throw std::runtime_error(
                "gobject get <governance-hash>\n"
                "Get governance object by hash\n"
                "\nArguments:\n"
                "1. governance-hash   (string, required) object id\n"
                );
}

UniValue gobject_get(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        gobject_get_help();

    // COLLECT VARIABLES FROM OUR USER
    uint256 hash = ParseHashV(request.params[1], "GovObj hash");

    LOCK2(cs_main, governance.cs);

    // FIND THE GOVERNANCE OBJECT THE USER IS LOOKING FOR
    CGovernanceObject* pGovObj = governance.FindGovernanceObject(hash);

    if (pGovObj == nullptr) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown governance object");
    }

    // REPORT BASIC OBJECT STATS

    UniValue objResult(UniValue::VOBJ);
    objResult.push_back(Pair("DataHex",  pGovObj->GetDataAsHexString()));
    objResult.push_back(Pair("DataString",  pGovObj->GetDataAsPlainString()));
    objResult.push_back(Pair("Hash",  pGovObj->GetHash().ToString()));
    objResult.push_back(Pair("CollateralHash",  pGovObj->GetCollateralHash().ToString()));
    objResult.push_back(Pair("ObjectType", pGovObj->GetObjectType()));
    objResult.push_back(Pair("CreationTime", pGovObj->GetCreationTime()));
    const COutPoint& masternodeOutpoint = pGovObj->GetMasternodeOutpoint();
    if (masternodeOutpoint != COutPoint()) {
        objResult.push_back(Pair("SigningMasternode", masternodeOutpoint.ToStringShort()));
    }

    // SHOW (MUCH MORE) INFORMATION ABOUT VOTES FOR GOVERNANCE OBJECT (THAN LIST/DIFF ABOVE)
    // -- FUNDING VOTING RESULTS

    UniValue objFundingResult(UniValue::VOBJ);
    objFundingResult.push_back(Pair("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING)));
    objFundingResult.push_back(Pair("YesCount",  pGovObj->GetYesCount(VOTE_SIGNAL_FUNDING)));
    objFundingResult.push_back(Pair("NoCount",  pGovObj->GetNoCount(VOTE_SIGNAL_FUNDING)));
    objFundingResult.push_back(Pair("AbstainCount",  pGovObj->GetAbstainCount(VOTE_SIGNAL_FUNDING)));
    objResult.push_back(Pair("FundingResult", objFundingResult));

    // -- VALIDITY VOTING RESULTS
    UniValue objValid(UniValue::VOBJ);
    objValid.push_back(Pair("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_VALID)));
    objValid.push_back(Pair("YesCount",  pGovObj->GetYesCount(VOTE_SIGNAL_VALID)));
    objValid.push_back(Pair("NoCount",  pGovObj->GetNoCount(VOTE_SIGNAL_VALID)));
    objValid.push_back(Pair("AbstainCount",  pGovObj->GetAbstainCount(VOTE_SIGNAL_VALID)));
    objResult.push_back(Pair("ValidResult", objValid));

    // -- DELETION CRITERION VOTING RESULTS
    UniValue objDelete(UniValue::VOBJ);
    objDelete.push_back(Pair("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_DELETE)));
    objDelete.push_back(Pair("YesCount",  pGovObj->GetYesCount(VOTE_SIGNAL_DELETE)));
    objDelete.push_back(Pair("NoCount",  pGovObj->GetNoCount(VOTE_SIGNAL_DELETE)));
    objDelete.push_back(Pair("AbstainCount",  pGovObj->GetAbstainCount(VOTE_SIGNAL_DELETE)));
    objResult.push_back(Pair("DeleteResult", objDelete));

    // -- ENDORSED VIA MASTERNODE-ELECTED BOARD
    UniValue objEndorsed(UniValue::VOBJ);
    objEndorsed.push_back(Pair("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_ENDORSED)));
    objEndorsed.push_back(Pair("YesCount",  pGovObj->GetYesCount(VOTE_SIGNAL_ENDORSED)));
    objEndorsed.push_back(Pair("NoCount",  pGovObj->GetNoCount(VOTE_SIGNAL_ENDORSED)));
    objEndorsed.push_back(Pair("AbstainCount",  pGovObj->GetAbstainCount(VOTE_SIGNAL_ENDORSED)));
    objResult.push_back(Pair("EndorsedResult", objEndorsed));

    // --
    std::string strError = "";
    objResult.push_back(Pair("fLocalValidity",  pGovObj->IsValidLocally(strError, false)));
    objResult.push_back(Pair("IsValidReason",  strError.c_str()));
    objResult.push_back(Pair("fCachedValid",  pGovObj->IsSetCachedValid()));
    objResult.push_back(Pair("fCachedFunding",  pGovObj->IsSetCachedFunding()));
    objResult.push_back(Pair("fCachedDelete",  pGovObj->IsSetCachedDelete()));
    objResult.push_back(Pair("fCachedEndorsed",  pGovObj->IsSetCachedEndorsed()));
    return objResult;
}

void gobject_getvotes_help()
{
    throw std::runtime_error(
                "gobject getvotes <governance-hash>\n"
                "Get all votes for a governance object hash (including old votes)\n"
                "\nArguments:\n"
                "1. governance-hash   (string, required) object id\n"
                );
}

UniValue gobject_getvotes(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        gobject_getvotes_help();

    // COLLECT PARAMETERS FROM USER

    uint256 hash = ParseHashV(request.params[1], "Governance hash");

    // FIND OBJECT USER IS LOOKING FOR

    LOCK(governance.cs);

    CGovernanceObject* pGovObj = governance.FindGovernanceObject(hash);

    if (pGovObj == nullptr) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown governance-hash");
    }

    // REPORT RESULTS TO USER

    UniValue bResult(UniValue::VOBJ);

    // GET MATCHING VOTES BY HASH, THEN SHOW USERS VOTE INFORMATION

    std::vector<CGovernanceVote> vecVotes = governance.GetMatchingVotes(hash);
    for (const auto& vote : vecVotes) {
        bResult.push_back(Pair(vote.GetHash().ToString(),  vote.ToString()));
    }

    return bResult;
}

void gobject_getcurrentvotes_help()
{
    throw std::runtime_error(
                "gobject getcurrentvotes <governance-hash> (<txid> <vout>)\n"
                "Get only current (tallying) votes for a governance object hash (does not include old votes)\n"
                "\nArguments:\n"
                "1. governance-hash   (string, required) object id\n"
                "2. txid              (string, optional) masternode collateral txid\n"
                "3. vout              (string, optional) masternode collateral output index, required if <txid> presents\n"
                );
}

UniValue gobject_getcurrentvotes(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 2 && request.params.size() != 4))
        gobject_getcurrentvotes_help();

    // COLLECT PARAMETERS FROM USER

    uint256 hash = ParseHashV(request.params[1], "Governance hash");

    COutPoint mnCollateralOutpoint;
    if (request.params.size() == 4) {
        uint256 txid = ParseHashV(request.params[2], "Masternode Collateral hash");
        std::string strVout = request.params[3].get_str();
        mnCollateralOutpoint = COutPoint(txid, (uint32_t)atoi(strVout));
    }

    // FIND OBJECT USER IS LOOKING FOR

    LOCK(governance.cs);

    CGovernanceObject* pGovObj = governance.FindGovernanceObject(hash);

    if (pGovObj == nullptr) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown governance-hash");
    }

    // REPORT RESULTS TO USER

    UniValue bResult(UniValue::VOBJ);

    // GET MATCHING VOTES BY HASH, THEN SHOW USERS VOTE INFORMATION

    std::vector<CGovernanceVote> vecVotes = governance.GetCurrentVotes(hash, mnCollateralOutpoint);
    for (const auto& vote : vecVotes) {
        bResult.push_back(Pair(vote.GetHash().ToString(),  vote.ToString()));
    }

    return bResult;
}

void gobject_help()
{
    throw std::runtime_error(
            "gobject \"command\"...\n"
            "Manage governance objects\n"
            "\nAvailable commands:\n"
            "  check              - Validate governance object data (proposal only)\n"
#ifdef ENABLE_WALLET
            "  prepare            - Prepare governance object by signing and creating tx\n"
#endif // ENABLE_WALLET
            "  submit             - Submit governance object to network\n"
            "  deserialize        - Deserialize governance object from hex string to JSON\n"
            "  count              - Count governance objects and votes (additional param: 'json' or 'all', default: 'json')\n"
            "  get                - Get governance object by hash\n"
            "  getvotes           - Get all votes for a governance object hash (including old votes)\n"
            "  getcurrentvotes    - Get only current (tallying) votes for a governance object hash (does not include old votes)\n"
            "  list               - List governance objects (can be filtered by signal and/or object type)\n"
            "  diff               - List differences since last diff\n"
            "  vote-alias         - Vote on a governance object by masternode alias (using masternode.conf setup)\n"
            "  vote-conf          - Vote on a governance object by masternode configured in dash.conf\n"
            "  vote-many          - Vote on a governance object by all masternodes (using masternode.conf setup)\n"
            );
}

UniValue gobject(const JSONRPCRequest& request)
{
    std::string strCommand;
    if (request.params.size() >= 1)
        strCommand = request.params[0].get_str();

    if ((request.fHelp && strCommand.empty())  ||
        (
#ifdef ENABLE_WALLET
         strCommand != "prepare" &&
#endif // ENABLE_WALLET
         strCommand != "vote-many" && strCommand != "vote-conf" && strCommand != "vote-alias" && strCommand != "submit" && strCommand != "count" &&
         strCommand != "deserialize" && strCommand != "get" && strCommand != "getvotes" && strCommand != "getcurrentvotes" && strCommand != "list" && strCommand != "diff" &&
         strCommand != "check" ))
            gobject_help();

    if (strCommand == "count") {
        return gobject_count(request);
    } else if (strCommand == "deserialize") {
        // DEBUG : TEST DESERIALIZATION OF GOVERNANCE META DATA
        return gobject_deserialize(request);
    } else if (strCommand == "check") {
        // VALIDATE A GOVERNANCE OBJECT PRIOR TO SUBMISSION
        return gobject_check(request);
#ifdef ENABLE_WALLET
    } else if (strCommand == "prepare") {
        // PREPARE THE GOVERNANCE OBJECT BY CREATING A COLLATERAL TRANSACTION
        return gobject_prepare(request);
#endif // ENABLE_WALLET
    } else if (strCommand == "submit") {
        // AFTER COLLATERAL TRANSACTION HAS MATURED USER CAN SUBMIT GOVERNANCE OBJECT TO PROPAGATE NETWORK
        /*
            ------ Example Governance Item ------

            gobject submit 6e622bb41bad1fb18e7f23ae96770aeb33129e18bd9efe790522488e580a0a03 0 1 1464292854 "beer-reimbursement" 5b5b22636f6e7472616374222c207b2270726f6a6563745f6e616d65223a20225c22626565722d7265696d62757273656d656e745c22222c20227061796d656e745f61646472657373223a20225c225879324c4b4a4a64655178657948726e34744744514238626a6876464564615576375c22222c2022656e645f64617465223a202231343936333030343030222c20226465736372697074696f6e5f75726c223a20225c227777772e646173687768616c652e6f72672f702f626565722d7265696d62757273656d656e745c22222c2022636f6e74726163745f75726c223a20225c22626565722d7265696d62757273656d656e742e636f6d2f3030312e7064665c22222c20227061796d656e745f616d6f756e74223a20223233342e323334323232222c2022676f7665726e616e63655f6f626a6563745f6964223a2037342c202273746172745f64617465223a202231343833323534303030227d5d5d1
        */
        return gobject_submit(request);
    } else if (strCommand == "vote-conf") {
        return gobject_vote_conf(request);
    } else if (strCommand == "vote-many") {
        return gobject_vote_many(request);
    } else if (strCommand == "vote-alias") {
        return gobject_vote_alias(request);
    } else if (strCommand == "list") {
        // USERS CAN QUERY THE SYSTEM FOR A LIST OF VARIOUS GOVERNANCE ITEMS
        return gobject_list(request);
    } else if (strCommand == "diff") {
        return gobject_diff(request);
    } else if (strCommand == "get") {
        // GET SPECIFIC GOVERNANCE ENTRY
        return gobject_get(request);
    } else if (strCommand == "getvotes") {
        // GETVOTES FOR SPECIFIC GOVERNANCE OBJECT
        return gobject_getvotes(request);
    } else if (strCommand == "getcurrentvotes") {
        // GETVOTES FOR SPECIFIC GOVERNANCE OBJECT
        return gobject_getcurrentvotes(request);
    }

    throw std::runtime_error("invalid command: " + strCommand);
}

UniValue voteraw(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 7)
        throw std::runtime_error(
                "voteraw <masternode-tx-hash> <masternode-tx-index> <governance-hash> <vote-signal> [yes|no|abstain] <time> <vote-sig>\n"
                "Compile and relay a governance vote with provided external signature instead of signing vote internally\n"
                );

    uint256 hashMnTx = ParseHashV(request.params[0], "mn tx hash");
    int nMnTxIndex = request.params[1].get_int();
    COutPoint outpoint = COutPoint(hashMnTx, nMnTxIndex);

    uint256 hashGovObj = ParseHashV(request.params[2], "Governance hash");
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

    int govObjType;
    {
        LOCK(governance.cs);
        CGovernanceObject *pGovObj = governance.FindGovernanceObject(hashGovObj);
        if (!pGovObj) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Governance object not found");
        }
        govObjType = pGovObj->GetObjectType();
    }

    int64_t nTime = request.params[5].get_int64();
    std::string strSig = request.params[6].get_str();
    bool fInvalid = false;
    std::vector<unsigned char> vchSig = DecodeBase64(strSig.c_str(), &fInvalid);

    if (fInvalid) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");
    }

    CMasternode mn;
    bool fMnFound = mnodeman.Get(outpoint, mn);

    if (!fMnFound) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failure to find masternode in list : " + outpoint.ToStringShort());
    }

    CGovernanceVote vote(outpoint, hashGovObj, eVoteSignal, eVoteOutcome);
    vote.SetTime(nTime);
    vote.SetSignature(vchSig);

    bool onlyOwnerAllowed = govObjType == GOVERNANCE_OBJECT_PROPOSAL;

    if (!vote.IsValid(onlyOwnerAllowed)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failure to verify vote.");
    }

    CGovernanceException exception;
    if (governance.ProcessVoteAndRelay(vote, exception, *g_connman)) {
        return "Voted successfully";
    } else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Error voting : " + exception.GetMessage());
    }
}

UniValue getgovernanceinfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
            "getgovernanceinfo\n"
            "Returns an object containing governance parameters.\n"
            "\nResult:\n"
            "{\n"
            "  \"governanceminquorum\": xxxxx,           (numeric) the absolute minimum number of votes needed to trigger a governance action\n"
            "  \"masternodewatchdogmaxseconds\": xxxxx,  (numeric) sentinel watchdog expiration time in seconds (DEPRECATED)\n"
            "  \"sentinelpingmaxseconds\": xxxxx,        (numeric) sentinel ping expiration time in seconds\n"
            "  \"proposalfee\": xxx.xx,                  (numeric) the collateral transaction fee which must be paid to create a proposal in " + CURRENCY_UNIT + "\n"
            "  \"superblockcycle\": xxxxx,               (numeric) the number of blocks between superblocks\n"
            "  \"lastsuperblock\": xxxxx,                (numeric) the block number of the last superblock\n"
            "  \"nextsuperblock\": xxxxx,                (numeric) the block number of the next superblock\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getgovernanceinfo", "")
            + HelpExampleRpc("getgovernanceinfo", "")
            );
    }

    LOCK(cs_main);

    int nLastSuperblock = 0, nNextSuperblock = 0;
    int nBlockHeight = chainActive.Height();

    CSuperblock::GetNearestSuperblocksHeights(nBlockHeight, nLastSuperblock, nNextSuperblock);

    UniValue obj(UniValue::VOBJ);
    obj.push_back(Pair("governanceminquorum", Params().GetConsensus().nGovernanceMinQuorum));
    obj.push_back(Pair("masternodewatchdogmaxseconds", MASTERNODE_SENTINEL_PING_MAX_SECONDS));
    obj.push_back(Pair("sentinelpingmaxseconds", MASTERNODE_SENTINEL_PING_MAX_SECONDS));
    obj.push_back(Pair("proposalfee", ValueFromAmount(GOVERNANCE_PROPOSAL_FEE_TX)));
    obj.push_back(Pair("superblockcycle", Params().GetConsensus().nSuperblockCycle));
    obj.push_back(Pair("lastsuperblock", nLastSuperblock));
    obj.push_back(Pair("nextsuperblock", nNextSuperblock));

    return obj;
}

UniValue getsuperblockbudget(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "getsuperblockbudget index\n"
            "\nReturns the absolute maximum sum of superblock payments allowed.\n"
            "\nArguments:\n"
            "1. index         (numeric, required) The block index\n"
            "\nResult:\n"
            "n                (numeric) The absolute maximum sum of superblock payments allowed, in " + CURRENCY_UNIT + "\n"
            "\nExamples:\n"
            + HelpExampleCli("getsuperblockbudget", "1000")
            + HelpExampleRpc("getsuperblockbudget", "1000")
        );
    }

    int nBlockHeight = request.params[0].get_int();
    if (nBlockHeight < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
    }

    CAmount nBudget = CSuperblock::GetPaymentsLimit(nBlockHeight);
    std::string strBudget = FormatMoney(nBudget);

    return strBudget;
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafe argNames
  //  --------------------- ------------------------  -----------------------  ------ ----------
    /* Dash features */
    { "dash",               "getgovernanceinfo",      &getgovernanceinfo,      true,  {} },
    { "dash",               "getsuperblockbudget",    &getsuperblockbudget,    true,  {"index"} },
    { "dash",               "gobject",                &gobject,                true,  {} },
    { "dash",               "voteraw",                &voteraw,                true,  {} },

};

void RegisterGovernanceRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
