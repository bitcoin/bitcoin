// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/activemasternode.h>
#include <core_io.h>
#include <governance/governance.h>
#include <governance/governanceclasses.h>
#include <governance/governancevalidators.h>
#include <validation.h>
#include <masternode/masternodesync.h>
#include <rpc/server.h>
#include <rpc/blockchain.h>
#include <node/context.h>

static RPCHelpMan gobject_count()
{
    return RPCHelpMan{"gobject_count",
        "\nCount governance objects and votes\n",
        {
            {"mode", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Output format: json (\"json\") or string in free form (\"all\")."},                
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
                HelpExampleCli("gobject_count", "json")
            + HelpExampleRpc("gobject_count", "json")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    std::string strMode{"json"};

    if (!request.params[0].isNull()) {
        strMode = request.params[0].get_str();
    }

    return strMode == "json" ? governance.ToJson() : governance.ToString();
},
    };
} 


static RPCHelpMan gobject_deserialize()
{
    return RPCHelpMan{"gobject_deserialize",
        "\nDeserialize governance object from hex string to JSON\n",
        {
            {"hex_data", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Data in hex string form."},                
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
                HelpExampleCli("gobject_deserialize", "")
            + HelpExampleRpc("gobject_deserialize", "")
        },
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


static RPCHelpMan gobject_check()
{
    return RPCHelpMan{"gobject_check",
        "\nValidate governance object data (proposal only)\n",
        {
            {"hex_data", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Data in hex string form."},                
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
                HelpExampleCli("gobject_check", "")
            + HelpExampleRpc("gobject_check", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

    uint256 hashParent;

    int nRevision = 1;

    int64_t nTime = GetAdjustedTime();
    std::string strDataHex = request.params[0].get_str();

    CGovernanceObject govobj(hashParent, nRevision, nTime, uint256(), strDataHex);

    if (govobj.GetObjectType() == GOVERNANCE_OBJECT_PROPOSAL) {
        CProposalValidator validator(strDataHex, false);
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

void gobject_submit_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"gobject submit",
            "Submit governance object to network\n"
            "\nArguments:\n"
            "1. parent-hash   (string, required) hash of the parent object, \"0\" is root\n"
            "2. revision      (numeric, required) object revision in the system\n"
            "3. time          (numeric, required) time this object was created\n"
            "4. data-hex      (string, required) data in hex string form\n"
            "5. fee-txid      (string, optional) fee-tx id, required for all objects except triggers",
    {
    },
    RPCResult{RPCResult::Type::NONE, "", ""},
    RPCExamples{""},
    }.Check(request);  
}

UniValue gobject_submit(const JSONRPCRequest& request)
{
    if (request.fHelp || ((request.params.size() < 5) || (request.params.size() > 6)))
        gobject_submit_help(request);

    if(!masternodeSync.IsBlockchainSynced()) {
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Must wait for client to sync with masternode network. Try again in a minute or so.");
    }
    NodeContext& node = EnsureNodeContext(request.context);
    if(!node.connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    CDeterministicMNList mnList;
    deterministicMNManager->GetListAtChainTip(mnList);
    bool fMnFound = mnList.HasValidMNByCollateral(activeMasternodeInfo.outpoint);

    LogPrint(BCLog::GOBJECT, "gobject_submit -- pubKeyOperator = %s, outpoint = %s, params.size() = %lld, fMnFound = %d\n",
            (activeMasternodeInfo.blsPubKeyOperator ? activeMasternodeInfo.blsPubKeyOperator->ToString() : "N/A"),
            activeMasternodeInfo.outpoint.ToStringShort(), request.params.size(), fMnFound);

    // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

    uint256 txidFee;

    if (!request.params[5].isNull()) {
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
    int nRevision;
    if(!ParseInt32(strRevision, &nRevision)){
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid revision");
    }
    int64_t nTime = atoi64(strTime);
    std::string strDataHex = request.params[4].get_str();

    CGovernanceObject govobj(hashParent, nRevision, nTime, txidFee, strDataHex);

    LogPrint(BCLog::GOBJECT, "gobject_submit -- GetDataAsPlainString = %s, hash = %s, txid = %s\n",
                govobj.GetDataAsPlainString(), govobj.GetHash().ToString(), txidFee.ToString());

    if (govobj.GetObjectType() == GOVERNANCE_OBJECT_PROPOSAL) {
        CProposalValidator validator(strDataHex, false);
        if (!validator.Validate()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid proposal data, error messages:" + validator.GetErrorMessages());
        }
    }

    // Attempt to sign triggers if we are a MN
    if (govobj.GetObjectType() == GOVERNANCE_OBJECT_TRIGGER) {
        if (fMnFound) {
            govobj.SetMasternodeOutpoint(activeMasternodeInfo.outpoint);
            govobj.Sign(*activeMasternodeInfo.blsKeyOperator);
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
    bool fMissingConfirmations;
    
    if (!govobj.IsValidLocally(strError, fMissingConfirmations, true) && !fMissingConfirmations) {
        LogPrintf("gobject(submit) -- Object submission rejected because object is not valid - hash = %s, strError = %s\n", strHash, strError);
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Governance object is not valid - " + strHash + " - " + strError);
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
        govobj.Relay(*node.connman);
    } else {
        governance.AddGovernanceObject(govobj, *node.connman);
    }

    return govobj.GetHash().ToString();
}

void gobject_vote_conf_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"gobject vote-conf",
                "Vote on a governance object by masternode configured in syscoin.conf\n"
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

UniValue gobject_vote_conf(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 4)
        gobject_vote_conf_help(request);

    NodeContext& node = EnsureNodeContext(request.context);
    if(!node.connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

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
    CDeterministicMNList mnList;
    deterministicMNManager->GetListAtChainTip(mnList);
    auto dmn = mnList.GetValidMNByCollateral(activeMasternodeInfo.outpoint);

    if (!dmn) {
        nFailed++;
        statusObj.pushKV("result", "failed");
        statusObj.pushKV("errorMessage", "Can't find masternode by collateral output");
        resultsObj.pushKV("syscoin.conf", statusObj);
        returnObj.pushKV("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed));
        returnObj.pushKV("detail", resultsObj);
        return returnObj;
    }

    CGovernanceVote vote(dmn->collateralOutpoint, hash, eVoteSignal, eVoteOutcome);

    bool signSuccess = false;
    if (govObjType == GOVERNANCE_OBJECT_PROPOSAL && eVoteSignal == VOTE_SIGNAL_FUNDING) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Can't use vote-conf for proposals");
    }
    if (activeMasternodeInfo.blsKeyOperator) {
        signSuccess = vote.Sign(*activeMasternodeInfo.blsKeyOperator);
    }

    if (!signSuccess) {
        nFailed++;
        statusObj.pushKV("result", "failed");
        statusObj.pushKV("errorMessage", "Failure to sign.");
        resultsObj.pushKV("syscoin.conf", statusObj);
        returnObj.pushKV("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed));
        returnObj.pushKV("detail", resultsObj);
        return returnObj;
    }

    CGovernanceException exception;
    if (governance.ProcessVoteAndRelay(vote, exception, *node.connman)) {
        nSuccessful++;
        statusObj.pushKV("result", "success");
    } else {
        nFailed++;
        statusObj.pushKV("result", "failed");
        statusObj.pushKV("errorMessage", exception.GetMessage());
    }

    resultsObj.pushKV("syscoin.conf", statusObj);

    returnObj.pushKV("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed));
    returnObj.pushKV("detail", resultsObj);

    return returnObj;
}


UniValue ListObjects(const std::string& strCachedSignal, const std::string& strType, int nStartTime)
{
    UniValue objResult(UniValue::VOBJ);

    // GET MATCHING GOVERNANCE OBJECTS

    LOCK(governance.cs);

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
        bObj.pushKV("DataHex",  pGovObj->GetDataAsHexString());
        bObj.pushKV("DataString",  pGovObj->GetDataAsPlainString());
        bObj.pushKV("Hash",  pGovObj->GetHash().ToString());
        bObj.pushKV("CollateralHash",  pGovObj->GetCollateralHash().ToString());
        bObj.pushKV("ObjectType", pGovObj->GetObjectType());
        bObj.pushKV("CreationTime", pGovObj->GetCreationTime());
        const COutPoint& masternodeOutpoint = pGovObj->GetMasternodeOutpoint();
        if (masternodeOutpoint != COutPoint()) {
            bObj.pushKV("SigningMasternode", masternodeOutpoint.ToStringShort());
        }

        // REPORT STATUS FOR FUNDING VOTES SPECIFICALLY
        bObj.pushKV("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING));
        bObj.pushKV("YesCount",  pGovObj->GetYesCount(VOTE_SIGNAL_FUNDING));
        bObj.pushKV("NoCount",  pGovObj->GetNoCount(VOTE_SIGNAL_FUNDING));
        bObj.pushKV("AbstainCount",  pGovObj->GetAbstainCount(VOTE_SIGNAL_FUNDING));

        // REPORT VALIDITY AND CACHING FLAGS FOR VARIOUS SETTINGS
        std::string strError = "";
        bObj.pushKV("fBlockchainValidity",  pGovObj->IsValidLocally(strError, false));
        bObj.pushKV("IsValidReason",  strError.c_str());
        bObj.pushKV("fCachedValid",  pGovObj->IsSetCachedValid());
        bObj.pushKV("fCachedFunding",  pGovObj->IsSetCachedFunding());
        bObj.pushKV("fCachedDelete",  pGovObj->IsSetCachedDelete());
        bObj.pushKV("fCachedEndorsed",  pGovObj->IsSetCachedEndorsed());

        objResult.pushKV(pGovObj->GetHash().ToString(), bObj);
    }

    return objResult;
}

void gobject_list_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"gobject list",
            "List governance objects (can be filtered by signal and/or object type)\n"
            "\nArguments:\n"
            "1. signal   (string, optional, default=valid) cached signal, possible values: [valid|funding|delete|endorsed|all]\n"
            "2. type     (string, optional, default=all) object type, possible values: [proposals|triggers|all]\n",
    {
    },
    RPCResult{RPCResult::Type::NONE, "", ""},
    RPCExamples{""},
    }.Check(request);  
}

UniValue gobject_list(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 3)
        gobject_list_help(request);

    std::string strCachedSignal = "valid";
    if (!request.params[1].isNull()) {
        strCachedSignal = request.params[1].get_str();
    }
    if (strCachedSignal != "valid" && strCachedSignal != "funding" && strCachedSignal != "delete" && strCachedSignal != "endorsed" && strCachedSignal != "all")
        return "Invalid signal, should be 'valid', 'funding', 'delete', 'endorsed' or 'all'";

    std::string strType = "all";
    if (!request.params[2].isNull()) {
        strType = request.params[2].get_str();
    }
    if (strType != "proposals" && strType != "triggers" && strType != "all")
        return "Invalid type, should be 'proposals', 'triggers' or 'all'";

    return ListObjects(strCachedSignal, strType, 0);
}

void gobject_diff_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"gobject diff",
            "List differences since last diff or list\n"
            "\nArguments:\n"
            "1. signal   (string, optional, default=valid) cached signal, possible values: [valid|funding|delete|endorsed|all]\n"
            "2. type     (string, optional, default=all) object type, possible values: [proposals|triggers|all]\n",
    {
    },
    RPCResult{RPCResult::Type::NONE, "", ""},
    RPCExamples{""},
    }.Check(request);  
}

UniValue gobject_diff(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 3)
        gobject_diff_help(request);

    std::string strCachedSignal = "valid";
    if (!request.params[1].isNull()) {
        strCachedSignal = request.params[1].get_str();
    }
    if (strCachedSignal != "valid" && strCachedSignal != "funding" && strCachedSignal != "delete" && strCachedSignal != "endorsed" && strCachedSignal != "all")
        return "Invalid signal, should be 'valid', 'funding', 'delete', 'endorsed' or 'all'";

    std::string strType = "all";
    if (!request.params[2].isNull()) {
        strType = request.params[2].get_str();
    }
    if (strType != "proposals" && strType != "triggers" && strType != "all")
        return "Invalid type, should be 'proposals', 'triggers' or 'all'";

    return ListObjects(strCachedSignal, strType, governance.GetLastDiffTime());
}

void gobject_get_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"gobject get",
            "Get governance object by hash\n"
            "\nArguments:\n"
            "1. governance-hash   (string, required) object id\n",
    {
    },
    RPCResult{RPCResult::Type::NONE, "", ""},
    RPCExamples{""},
    }.Check(request);  
}

UniValue gobject_get(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        gobject_get_help(request);

    // COLLECT VARIABLES FROM OUR USER
    uint256 hash = ParseHashV(request.params[1], "GovObj hash");

    LOCK(governance.cs);

    // FIND THE GOVERNANCE OBJECT THE USER IS LOOKING FOR
    CGovernanceObject* pGovObj = governance.FindGovernanceObject(hash);

    if (pGovObj == nullptr) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown governance object");
    }

    // REPORT BASIC OBJECT STATS

    UniValue objResult(UniValue::VOBJ);
    objResult.pushKV("DataHex",  pGovObj->GetDataAsHexString());
    objResult.pushKV("DataString",  pGovObj->GetDataAsPlainString());
    objResult.pushKV("Hash",  pGovObj->GetHash().ToString());
    objResult.pushKV("CollateralHash",  pGovObj->GetCollateralHash().ToString());
    objResult.pushKV("ObjectType", pGovObj->GetObjectType());
    objResult.pushKV("CreationTime", pGovObj->GetCreationTime());
    const COutPoint& masternodeOutpoint = pGovObj->GetMasternodeOutpoint();
    if (masternodeOutpoint != COutPoint()) {
        objResult.pushKV("SigningMasternode", masternodeOutpoint.ToStringShort());
    }

    // SHOW (MUCH MORE) INFORMATION ABOUT VOTES FOR GOVERNANCE OBJECT (THAN LIST/DIFF ABOVE)
    // -- FUNDING VOTING RESULTS

    UniValue objFundingResult(UniValue::VOBJ);
    objFundingResult.pushKV("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING));
    objFundingResult.pushKV("YesCount",  pGovObj->GetYesCount(VOTE_SIGNAL_FUNDING));
    objFundingResult.pushKV("NoCount",  pGovObj->GetNoCount(VOTE_SIGNAL_FUNDING));
    objFundingResult.pushKV("AbstainCount",  pGovObj->GetAbstainCount(VOTE_SIGNAL_FUNDING));
    objResult.pushKV("FundingResult", objFundingResult);

    // -- VALIDITY VOTING RESULTS
    UniValue objValid(UniValue::VOBJ);
    objValid.pushKV("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_VALID));
    objValid.pushKV("YesCount",  pGovObj->GetYesCount(VOTE_SIGNAL_VALID));
    objValid.pushKV("NoCount",  pGovObj->GetNoCount(VOTE_SIGNAL_VALID));
    objValid.pushKV("AbstainCount",  pGovObj->GetAbstainCount(VOTE_SIGNAL_VALID));
    objResult.pushKV("ValidResult", objValid);

    // -- DELETION CRITERION VOTING RESULTS
    UniValue objDelete(UniValue::VOBJ);
    objDelete.pushKV("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_DELETE));
    objDelete.pushKV("YesCount",  pGovObj->GetYesCount(VOTE_SIGNAL_DELETE));
    objDelete.pushKV("NoCount",  pGovObj->GetNoCount(VOTE_SIGNAL_DELETE));
    objDelete.pushKV("AbstainCount",  pGovObj->GetAbstainCount(VOTE_SIGNAL_DELETE));
    objResult.pushKV("DeleteResult", objDelete);

    // -- ENDORSED VIA MASTERNODE-ELECTED BOARD
    UniValue objEndorsed(UniValue::VOBJ);
    objEndorsed.pushKV("AbsoluteYesCount",  pGovObj->GetAbsoluteYesCount(VOTE_SIGNAL_ENDORSED));
    objEndorsed.pushKV("YesCount",  pGovObj->GetYesCount(VOTE_SIGNAL_ENDORSED));
    objEndorsed.pushKV("NoCount",  pGovObj->GetNoCount(VOTE_SIGNAL_ENDORSED));
    objEndorsed.pushKV("AbstainCount",  pGovObj->GetAbstainCount(VOTE_SIGNAL_ENDORSED));
    objResult.pushKV("EndorsedResult", objEndorsed);

    // --
    std::string strError = "";
    objResult.pushKV("fLocalValidity",  pGovObj->IsValidLocally(strError, false));
    objResult.pushKV("IsValidReason",  strError.c_str());
    objResult.pushKV("fCachedValid",  pGovObj->IsSetCachedValid());
    objResult.pushKV("fCachedFunding",  pGovObj->IsSetCachedFunding());
    objResult.pushKV("fCachedDelete",  pGovObj->IsSetCachedDelete());
    objResult.pushKV("fCachedEndorsed",  pGovObj->IsSetCachedEndorsed());
    return objResult;
}

void gobject_getcurrentvotes_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"gobject getcurrentvotes",
            "Get only current (tallying) votes for a governance object hash (does not include old votes)\n"
            "\nArguments:\n"
            "1. governance-hash   (string, required) object id\n"
            "2. txid              (string, optional) masternode collateral txid\n"
            "3. vout              (string, optional) masternode collateral output index, required if <txid> presents\n",
    {
    },
    RPCResult{RPCResult::Type::NONE, "", ""},
    RPCExamples{""},
    }.Check(request);  
}

UniValue gobject_getcurrentvotes(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 2 && request.params.size() != 4))
        gobject_getcurrentvotes_help(request);

    // COLLECT PARAMETERS FROM USER

    uint256 hash = ParseHashV(request.params[1], "Governance hash");

    COutPoint mnCollateralOutpoint;
    if (!request.params[2].isNull() && !request.params[3].isNull()) {
        uint256 txid = ParseHashV(request.params[2], "Masternode Collateral hash");
        std::string strVout = request.params[3].get_str();
        int nVout;
        if(!ParseInt32(strVout, &nVout)){
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vout");
        }
        mnCollateralOutpoint = COutPoint(txid, (uint32_t)nVout);
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
        bResult.pushKV(vote.GetHash().ToString(),  vote.ToString());
    }

    return bResult;
}

static RPCHelpMan voteraw()
{
    return RPCHelpMan{"voteraw",
        "\nCompile and relay a governance vote with provided external signature instead of signing vote internally\n",
        {
            {"collateralTxHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Masternode collateral txid."}, 
            {"collateralTxIndex", RPCArg::Type::NUM, RPCArg::Optional::NO, "Masternode collateral output index."}, 
            {"governanceHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Governance proposal hash."},    
            {"voteSignal", RPCArg::Type::STR, RPCArg::Optional::NO, "One of following (funding|valid|delete|endorsed)."}, 
            {"voteOutcome", RPCArg::Type::STR, RPCArg::Optional::NO, "One of following (yes|no|abstain)."},
            {"time", RPCArg::Type::NUM, RPCArg::Optional::NO, "Time of vote."},
            {"voteSig", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Vote signature. Must be encoded in base64."},                 
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
                HelpExampleCli("voteraw", "")
            + HelpExampleRpc("voteraw", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    NodeContext& node = EnsureNodeContext(request.context);
    if(!node.connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    uint256 hashMnCollateralTx = ParseHashV(request.params[0], "mn collateral tx hash");
    int nMnCollateralTxIndex = request.params[1].get_int();
    COutPoint outpoint = COutPoint(hashMnCollateralTx, nMnCollateralTxIndex);

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
    CDeterministicMNList mnList;
    deterministicMNManager->GetListAtChainTip(mnList);
    auto dmn = mnList.GetValidMNByCollateral(outpoint);

    if (!dmn) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failure to find masternode in list : " + outpoint.ToStringShort());
    }

    CGovernanceVote vote(outpoint, hashGovObj, eVoteSignal, eVoteOutcome);
    vote.SetTime(nTime);
    vote.SetSignature(vchSig);

    bool onlyVotingKeyAllowed = govObjType == GOVERNANCE_OBJECT_PROPOSAL && vote.GetSignal() == VOTE_SIGNAL_FUNDING;

    if (!vote.IsValid(onlyVotingKeyAllowed)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failure to verify vote.");
    }

    CGovernanceException exception;
    if (governance.ProcessVoteAndRelay(vote, exception, *node.connman)) {
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
        "\nReturns an object containing governance parameters.\n",
        {                
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "", {
                {
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::NUM, "governanceminquorum", "The absolute minimum number of votes needed to trigger a governance action"},
                        {RPCResult::Type::NUM, "proposalfee", "The collateral transaction fee which must be paid to create a proposal in " + CURRENCY_UNIT},
                        {RPCResult::Type::NUM, "superblockcycle", "The number of blocks between superblocks"},
                        {RPCResult::Type::NUM, "lastsuperblock", "The block number of the last superblock"},
                        {RPCResult::Type::NUM, "nextsuperblock", "The block number of the next superblock"},
                    }
                },
            },
        },
        RPCExamples{
                HelpExampleCli("getgovernanceinfo", "")
            + HelpExampleRpc("getgovernanceinfo", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    LOCK(cs_main);

    int nLastSuperblock = 0, nNextSuperblock = 0;
    int nBlockHeight = ::ChainActive().Height();

    CSuperblock::GetNearestSuperblocksHeights(nBlockHeight, nLastSuperblock, nNextSuperblock);

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("governanceminquorum", Params().GetConsensus().nGovernanceMinQuorum);
    obj.pushKV("proposalfee", ValueFromAmount(GOVERNANCE_PROPOSAL_FEE_TX));
    obj.pushKV("superblockcycle", Params().GetConsensus().nSuperblockCycle);
    obj.pushKV("lastsuperblock", nLastSuperblock);
    obj.pushKV("nextsuperblock", nNextSuperblock);

    return obj;
},
    };
} 


static RPCHelpMan getsuperblockbudget()
{
    return RPCHelpMan{"getsuperblockbudget",
        "\nReturns the absolute maximum sum of superblock payments allowed.\n",
        {      
            {"index", RPCArg::Type::NUM, RPCArg::Optional::NO, "The block index."},           
        },
        RPCResult{RPCResult::Type::NUM, "n", "The absolute maximum sum of superblock payments allowed, in " + CURRENCY_UNIT},
        RPCExamples{
                HelpExampleCli("getsuperblockbudget", "")
            + HelpExampleRpc("getsuperblockbudget", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    int nBlockHeight = request.params[0].get_int();
    if (nBlockHeight < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
    }

    return ValueFromAmount(CSuperblock::GetPaymentsLimit(nBlockHeight));
},
    };
} 


void RegisterGovernanceRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
//  --------------------- ------------------------  -----------------------  ----------
    { "governance",               "getgovernanceinfo",      &getgovernanceinfo,      {} },
    { "governance",               "getsuperblockbudget",    &getsuperblockbudget,    {"index"} },
    { "governance",               "gobject_count",          &gobject_count,          {"mode"} },
    { "governance",               "gobject_deserialize",    &gobject_deserialize,    {"hex_data"} },
    { "governance",               "gobject_check",          &gobject_check,          {"hex_data"} },
    { "governance",               "voteraw",                &voteraw,                {"collateralTxHash","collateralTxIndex","governanceHash","voteSignal","voteOutcome","time","voteSig"} },

};
// clang-format on
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
