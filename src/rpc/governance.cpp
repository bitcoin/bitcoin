// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/activemasternode.h>
#include <core_io.h>
#include <governance/governance.h>
#include <governance/governanceclasses.h>
#include <governance/governancevalidators.h>
#include <evo/deterministicmns.h>
#include <validation.h>
#include <masternode/masternodesync.h>
#include <rpc/server.h>
#include <rpc/blockchain.h>
#include <node/context.h>
#include <timedata.h>
static RPCHelpMan gobject_count()
{
    return RPCHelpMan{"gobject_count",
        "\nCount governance objects and votes\n",
        {
            {"mode", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Output format: json (\"json\") or string in free form (\"all\")."},                
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
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

    return strMode == "json" ? governance->ToJson() : governance->ToString();
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
        RPCResult{RPCResult::Type::ANY, "", ""},
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
        RPCResult{RPCResult::Type::ANY, "", ""},
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

static RPCHelpMan gobject_submit()
{
    return RPCHelpMan{"gobject_submit",
        "\nSubmit governance object to network\n",
        {      
            {"parentHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Hash of the parent object, \"0\" is root."},
            {"revision", RPCArg::Type::NUM, RPCArg::Optional::NO, "Object revision in the system."},   
            {"time", RPCArg::Type::NUM, RPCArg::Optional::NO, "Time this object was created."},
            {"dataHex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Data in hex string form."},
            {"feeTxid", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "Fee-tx id, required for all objects except triggers."},                                         
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("gobject_submit", "")
            + HelpExampleRpc("gobject_submit", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if(!masternodeSync.IsBlockchainSynced()) {
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Must wait for client to sync with masternode network. Try again in a minute or so.");
    }
    NodeContext& node = EnsureAnyNodeContext(request.context);
    if(!node.connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    CDeterministicMNList mnList;
    if(deterministicMNManager)
        deterministicMNManager->GetListAtChainTip(mnList);
    bool fMnFound = WITH_LOCK(activeMasternodeInfoCs, return mnList.HasValidMNByCollateral(activeMasternodeInfo.outpoint));

    LogPrint(BCLog::GOBJECT, "gobject_submit -- pubKeyOperator = %s, outpoint = %s, params.size() = %lld, fMnFound = %d\n",
            (WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.blsPubKeyOperator ? activeMasternodeInfo.blsPubKeyOperator->ToString() : "N/A")),
            WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.outpoint.ToStringShort()), request.params.size(), fMnFound);

    // ASSEMBLE NEW GOVERNANCE OBJECT FROM USER PARAMETERS

    uint256 txidFee;

    if (!request.params[4].isNull()) {
        txidFee = ParseHashV(request.params[4], "feeTxid");
    }
    uint256 hashParent;
    if (request.params[0].get_str() == "0") { // attach to root node (root node doesn't really exist, but has a hash of zero)
        hashParent = uint256();
    } else {
        hashParent = ParseHashV(request.params[0], "parentHash");
    }

    // GET THE PARAMETERS FROM USER

    int nRevision = request.params[1].get_int();
    int64_t nTime = request.params[2].get_int64();
    std::string strDataHex = request.params[3].get_str();

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
            LOCK(activeMasternodeInfoCs);
            govobj.SetMasternodeOutpoint(activeMasternodeInfo.outpoint);
            govobj.Sign(*activeMasternodeInfo.blsKeyOperator);
        } else {
            LogPrintf("gobject(submit) -- Object submission rejected because node is not a masternode\n");
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Only valid masternodes can submit this type of object");
        }
    } else if (request.params.size() != 5) {
        LogPrintf("gobject(submit) -- Object submission rejected because fee tx not provided\n");
        throw JSONRPCError(RPC_INVALID_PARAMETER, "The fee-txid parameter must be included to submit this type of object");
    }

    std::string strHash = govobj.GetHash().ToString();

    std::string strError = "";
    bool fMissingConfirmations;
    
    if (!govobj.IsValidLocally(*node.chainman, strError, fMissingConfirmations, true) && !fMissingConfirmations) {
        LogPrintf("gobject(submit) -- Object submission rejected because object is not valid - hash = %s, strError = %s\n", strHash, strError);
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Governance object is not valid - " + strHash + " - " + strError);
    }
    

    // RELAY THIS OBJECT
    // Reject if rate check fails but don't update buffer
    if (!governance->MasternodeRateCheck(govobj)) {
        LogPrintf("gobject(submit) -- Object submission rejected because of rate check failure - hash = %s\n", strHash);
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Object creation rate limit exceeded");
    }

    LogPrintf("gobject(submit) -- Adding locally created governance object - %s\n", strHash);

    if (fMissingConfirmations) {
        governance->AddPostponedObject(govobj);
        govobj.Relay(*node.connman);
    } else {
        governance->AddGovernanceObject(govobj, *node.connman);
    }

    return govobj.GetHash().ToString();
},
    };
} 

static RPCHelpMan gobject_vote_conf()
{
    return RPCHelpMan{"gobject_vote_conf",
        "\nVote on a governance object by masternode configured in syscoin.conf\n",
        {      
            {"governanceHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Hash of the governance object."},
            {"vote", RPCArg::Type::STR, RPCArg::Optional::NO, "Vote, possible values: [funding|valid|delete|endorsed]."},   
            {"voteOutome", RPCArg::Type::STR, RPCArg::Optional::NO, "Vote outcome, possible values: [yes|no|abstain]."},                                  
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("gobject_vote_conf", "")
            + HelpExampleRpc("gobject_vote_conf", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
    if(!node.connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    uint256 hash;

    hash = ParseHashV(request.params[0], "Object hash");
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

    int govObjType;
    {
        LOCK(governance->cs);
        CGovernanceObject *pGovObj = governance->FindGovernanceObject(hash);
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
    if(deterministicMNManager)
        deterministicMNManager->GetListAtChainTip(mnList);
    auto dmn = WITH_LOCK(activeMasternodeInfoCs, return mnList.GetValidMNByCollateral(activeMasternodeInfo.outpoint));

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

    {
        LOCK(activeMasternodeInfoCs);
        if (activeMasternodeInfo.blsKeyOperator) {
            signSuccess = vote.Sign(*activeMasternodeInfo.blsKeyOperator);
        }
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
    if (governance->ProcessVoteAndRelay(vote, exception, *node.connman)) {
        nSuccessful++;
        statusObj.pushKV("result", "success");
    } else {
        nFailed++;
        statusObj.pushKV("result", "failed");
        statusObj.pushKV("errorMessage", exception.GetMessageStr());
    }

    resultsObj.pushKV("syscoin.conf", statusObj);

    returnObj.pushKV("overall", strprintf("Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed));
    returnObj.pushKV("detail", resultsObj);

    return returnObj;
},
    };
} 


UniValue ListObjects(ChainstateManager& chainman, const std::string& strCachedSignal, const std::string& strType, int nStartTime)
{
    UniValue objResult(UniValue::VOBJ);

    // GET MATCHING GOVERNANCE OBJECTS

    LOCK(governance->cs);

    std::vector<const CGovernanceObject*> objs = governance->GetAllNewerThan(nStartTime);
    governance->UpdateLastDiffTime(GetTime());

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
        bObj.pushKV("fBlockchainValidity",  pGovObj->IsValidLocally(chainman, strError, false));
        bObj.pushKV("IsValidReason",  strError.c_str());
        bObj.pushKV("fCachedValid",  pGovObj->IsSetCachedValid());
        bObj.pushKV("fCachedFunding",  pGovObj->IsSetCachedFunding());
        bObj.pushKV("fCachedDelete",  pGovObj->IsSetCachedDelete());
        bObj.pushKV("fCachedEndorsed",  pGovObj->IsSetCachedEndorsed());

        objResult.pushKV(pGovObj->GetHash().ToString(), bObj);
    }

    return objResult;
}

static RPCHelpMan gobject_list()
{
    return RPCHelpMan{"gobject_list",
        "\nList governance objects (can be filtered by signal and/or object type)\n",
        {      
            {"signal", RPCArg::Type::STR, RPCArg::Default{"valid"}, "Cached signal, possible values: [valid|funding|delete|endorsed|all]."},
            {"type", RPCArg::Type::STR, RPCArg::Default{"all"}, "Object type, possible values: [proposals|triggers|all]."},                                    
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("gobject_list", "")
            + HelpExampleRpc("gobject_list", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
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

    return ListObjects(*node.chainman, strCachedSignal, strType, 0);
},
    };
} 

static RPCHelpMan gobject_diff()
{
    return RPCHelpMan{"gobject_diff",
        "\nList differences since last diff or list\n",
        {      
            {"signal", RPCArg::Type::STR, RPCArg::Default{"valid"}, "Cached signal, possible values: [valid|funding|delete|endorsed|all]."},
            {"type", RPCArg::Type::STR, RPCArg::Default{"all"}, "Object type, possible values: [proposals|triggers|all]."},                                    
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("gobject_diff", "")
            + HelpExampleRpc("gobject_diff", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
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

    return ListObjects(*node.chainman, strCachedSignal, strType, governance->GetLastDiffTime());
},
    };
} 

static RPCHelpMan gobject_get()
{
    return RPCHelpMan{"gobject_get",
        "\nGet governance object by hash\n",
        {      
            {"governanceHash", RPCArg::Type::STR_HEX,  RPCArg::Optional::NO, "Object id."},                                   
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("gobject_get", "")
            + HelpExampleRpc("gobject_get", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // COLLECT VARIABLES FROM OUR USER
    uint256 hash = ParseHashV(request.params[0], "GovObj hash");
    NodeContext& node = EnsureAnyNodeContext(request.context);
    LOCK(governance->cs);

    // FIND THE GOVERNANCE OBJECT THE USER IS LOOKING FOR
    CGovernanceObject* pGovObj = governance->FindGovernanceObject(hash);

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
    objResult.pushKV("fLocalValidity",  pGovObj->IsValidLocally(*node.chainman, strError, false));
    objResult.pushKV("IsValidReason",  strError.c_str());
    objResult.pushKV("fCachedValid",  pGovObj->IsSetCachedValid());
    objResult.pushKV("fCachedFunding",  pGovObj->IsSetCachedFunding());
    objResult.pushKV("fCachedDelete",  pGovObj->IsSetCachedDelete());
    objResult.pushKV("fCachedEndorsed",  pGovObj->IsSetCachedEndorsed());
    return objResult;
},
    };
} 

static RPCHelpMan gobject_getcurrentvotes()
{
    return RPCHelpMan{"gobject_getcurrentvotes",
        "\nGet only current (tallying) votes for a governance object hash (does not include old votes)\n",
        {      
            {"governanceHash", RPCArg::Type::STR_HEX,  RPCArg::Optional::NO, "Object id."},
            {"txid", RPCArg::Type::STR_HEX,  RPCArg::Optional::OMITTED, "Masternode collateral txid."}, 
            {"vout", RPCArg::Type::NUM,  RPCArg::Optional::OMITTED, "Masternode collateral output index, required if <txid> presents."},                                   
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("gobject_getcurrentvotes", "")
            + HelpExampleRpc("gobject_getcurrentvotes", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // COLLECT PARAMETERS FROM USER

    uint256 hash = ParseHashV(request.params[0], "Governance hash");

    COutPoint mnCollateralOutpoint;
    if (!request.params[1].isNull() && !request.params[2].isNull()) {
        uint256 txid = ParseHashV(request.params[1], "Masternode Collateral hash");
        int nVout = request.params[2].get_int();
        mnCollateralOutpoint = COutPoint(txid, (uint32_t)nVout);
    }

    // FIND OBJECT USER IS LOOKING FOR

    LOCK(governance->cs);

    CGovernanceObject* pGovObj = governance->FindGovernanceObject(hash);

    if (pGovObj == nullptr) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown governance-hash");
    }

    // REPORT RESULTS TO USER

    UniValue bResult(UniValue::VOBJ);

    // GET MATCHING VOTES BY HASH, THEN SHOW USERS VOTE INFORMATION

    std::vector<CGovernanceVote> vecVotes = governance->GetCurrentVotes(hash, mnCollateralOutpoint);
    for (const auto& vote : vecVotes) {
        bResult.pushKV(vote.GetHash().ToString(),  vote.ToString());
    }

    return bResult;
},
    };
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
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("voteraw", "")
            + HelpExampleRpc("voteraw", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    NodeContext& node = EnsureAnyNodeContext(request.context);
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
        LOCK(governance->cs);
        CGovernanceObject *pGovObj = governance->FindGovernanceObject(hashGovObj);
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
    if(deterministicMNManager)
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
    if (governance->ProcessVoteAndRelay(vote, exception, *node.connman)) {
        return "Voted successfully";
    } else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Error voting : " + exception.GetMessageStr());
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
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "governanceminquorum", "The absolute minimum number of votes needed to trigger a governance action"},
                {RPCResult::Type::NUM, "proposalfee", "The collateral transaction fee which must be paid to create a proposal in " + CURRENCY_UNIT},
                {RPCResult::Type::NUM, "superblockcycle", "The number of blocks between superblocks"},
                {RPCResult::Type::NUM, "lastsuperblock", "The block number of the last superblock"},
                {RPCResult::Type::NUM, "nextsuperblock", "The block number of the next superblock"},
            },
        },
        RPCExamples{
                HelpExampleCli("getgovernanceinfo", "")
            + HelpExampleRpc("getgovernanceinfo", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
    LOCK(cs_main);

    int nLastSuperblock = 0, nNextSuperblock = 0;
    int nBlockHeight = node.chainman->ActiveHeight();

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
{ //  category              name                      actor (function)    
//  --------------------- ------------------------  -----------------------
    { "governance",               &getgovernanceinfo,      },
    { "governance",               &getsuperblockbudget,    },
    { "governance",               &gobject_count,          },
    { "governance",               &gobject_deserialize,    },
    { "governance",               &gobject_check,          },
    { "governance",               &gobject_getcurrentvotes,},
    { "governance",               &gobject_get,            },
    { "governance",               &gobject_submit,         },
    { "governance",               &gobject_vote_conf,      },
    { "governance",               &gobject_list,           },
    { "governance",               &gobject_diff,           },
    { "governance",               &voteraw,                },

};
// clang-format on
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
