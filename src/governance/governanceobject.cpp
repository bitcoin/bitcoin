// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/governanceobject.h>
#include <core_io.h>
#include <governance/governancevalidators.h>
#include <governance/governancevote.h>
#include <governance/governance.h>
#include <masternode/masternodemeta.h>
#include <masternode/masternodesync.h>
#include <evo/deterministicmns.h>
#include <messagesigner.h>
#include <net.h>
#include <util/system.h>
#include <validation.h>
#include <node/transaction.h>
#include <string>
#include <univalue.h>
#include <timedata.h>
#include <net_processing.h>
#include <llmq/quorums_utils.h>
using node::GetTransaction;
CGovernanceObject::CGovernanceObject() :
    cs(),
    nObjectType(GOVERNANCE_OBJECT_UNKNOWN),
    nHashParent(),
    nRevision(0),
    nTime(0),
    nDeletionTime(0),
    nCollateralHash(),
    vchData(),
    masternodeOutpoint(),
    vchSig(),
    fCachedLocalValidity(false),
    strLocalValidityError(),
    fCachedFunding(false),
    fCachedValid(true),
    fCachedDelete(false),
    fCachedEndorsed(false),
    fDirtyCache(true),
    fExpired(false),
    fUnparsable(false),
    mapCurrentMNVotes(),
    fileVotes()
{
    // PARSE JSON DATA STORAGE (VCHDATA)
    LoadData();
}

CGovernanceObject::CGovernanceObject(const uint256& nHashParentIn, int nRevisionIn, int64_t nTimeIn, const uint256& nCollateralHashIn, const std::string& strDataHexIn) :
    cs(),
    nObjectType(GOVERNANCE_OBJECT_UNKNOWN),
    nHashParent(nHashParentIn),
    nRevision(nRevisionIn),
    nTime(nTimeIn),
    nDeletionTime(0),
    nCollateralHash(nCollateralHashIn),
    vchData(ParseHex(strDataHexIn)),
    masternodeOutpoint(),
    vchSig(),
    fCachedLocalValidity(false),
    strLocalValidityError(),
    fCachedFunding(false),
    fCachedValid(true),
    fCachedDelete(false),
    fCachedEndorsed(false),
    fDirtyCache(true),
    fExpired(false),
    fUnparsable(false),
    mapCurrentMNVotes(),
    fileVotes()
{
    // PARSE JSON DATA STORAGE (VCHDATA)
    LoadData();
}

CGovernanceObject::CGovernanceObject(const CGovernanceObject& other) :
    cs(),
    nObjectType(other.nObjectType),
    nHashParent(other.nHashParent),
    nRevision(other.nRevision),
    nTime(other.nTime),
    nDeletionTime(other.nDeletionTime),
    nCollateralHash(other.nCollateralHash),
    vchData(other.vchData),
    masternodeOutpoint(other.masternodeOutpoint),
    vchSig(other.vchSig),
    fCachedLocalValidity(other.fCachedLocalValidity),
    strLocalValidityError(other.strLocalValidityError),
    fCachedFunding(other.fCachedFunding),
    fCachedValid(other.fCachedValid),
    fCachedDelete(other.fCachedDelete),
    fCachedEndorsed(other.fCachedEndorsed),
    fDirtyCache(other.fDirtyCache),
    fExpired(other.fExpired),
    fUnparsable(other.fUnparsable),
    mapCurrentMNVotes(other.mapCurrentMNVotes),
    fileVotes(other.fileVotes)
{
}

bool CGovernanceObject::ProcessVote(const CBlockIndex* pindex, CNode* pfrom,
    const CGovernanceVote& vote,
    CGovernanceException& exception)
{
    LOCK(cs);

    // do not process already known valid votes twice
    if (fileVotes.HasVote(vote.GetHash())) {
        // nothing to do here, not an error
        std::ostringstream ostr;
        ostr << "CGovernanceObject::ProcessVote -- Already known valid vote";
        LogPrint(BCLog::GOBJECT, "%s\n", ostr.str());
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_NONE);
        return false;
    }
    auto mnList = deterministicMNManager->GetListAtChainTip();
    auto dmn = mnList.GetMNByCollateral(vote.GetMasternodeOutpoint());

    if (!dmn) {
        std::ostringstream ostr;
        ostr << "CGovernanceObject::ProcessVote -- Masternode " << vote.GetMasternodeOutpoint().ToStringShort() << " not found";
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_PERMANENT_ERROR, 20);
        return false;
    }

    auto it = mapCurrentMNVotes.emplace(vote_m_t::value_type(vote.GetMasternodeOutpoint(), vote_rec_t())).first;
    vote_rec_t& voteRecordRef = it->second;
    vote_signal_enum_t eSignal = vote.GetSignal();
    if (eSignal == VOTE_SIGNAL_NONE) {
        std::ostringstream ostr;
        ostr << "CGovernanceObject::ProcessVote -- Vote signal: none";
        LogPrint(BCLog::GOBJECT, "%s\n", ostr.str());
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_WARNING);
        return false;
    }
    if (eSignal > MAX_SUPPORTED_VOTE_SIGNAL) {
        std::ostringstream ostr;
        ostr << "CGovernanceObject::ProcessVote -- Unsupported vote signal: " << CGovernanceVoting::ConvertSignalToString(vote.GetSignal());
        LogPrintf("%s\n", ostr.str());
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_PERMANENT_ERROR, 20);
        return false;
    }
    auto it2 = voteRecordRef.mapInstances.emplace(vote_instance_m_t::value_type(int(eSignal), vote_instance_t())).first;
    vote_instance_t& voteInstanceRef = it2->second;

    // Reject obsolete votes
    if (vote.GetTimestamp() < voteInstanceRef.nCreationTime) {
        std::ostringstream ostr;
        ostr << "CGovernanceObject::ProcessVote -- Obsolete vote";
        LogPrint(BCLog::GOBJECT, "%s\n", ostr.str());
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_NONE);
        return false;
    } else if (vote.GetTimestamp() == voteInstanceRef.nCreationTime) {
        // Someone is doing something fishy, there can be no two votes from the same masternode
        // with the same timestamp for the same object and signal and yet different hash/outcome.
        std::ostringstream ostr;
        ostr << "CGovernanceObject::ProcessVote -- Invalid vote, same timestamp for the different outcome";
        if (vote.GetOutcome() < voteInstanceRef.eOutcome) {
            // This is an arbitrary comparison, we have to agree on some way
            // to pick the "winning" vote.
            ostr << ", rejected";
            LogPrint(BCLog::GOBJECT, "%s\n", ostr.str());
            exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_NONE);
            return false;
        }
        ostr << ", accepted";
        LogPrint(BCLog::GOBJECT, "%s\n", ostr.str());
    }

    int64_t nNow = TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime());
    int64_t nVoteTimeUpdate = voteInstanceRef.nTime;
    if (governance->AreRateChecksEnabled()) {
        int64_t nTimeDelta = nNow - voteInstanceRef.nTime;
        if (nTimeDelta < GOVERNANCE_UPDATE_MIN) {
            std::ostringstream ostr;
            ostr << "CGovernanceObject::ProcessVote -- Masternode voting too often"
                 << ", MN outpoint = " << vote.GetMasternodeOutpoint().ToStringShort()
                 << ", governance object hash = " << GetHash().ToString()
                 << ", time delta = " << nTimeDelta;
            LogPrint(BCLog::GOBJECT, "%s\n", ostr.str());
            exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_TEMPORARY_ERROR);
            return false;
        }
        nVoteTimeUpdate = nNow;
    }

    bool onlyVotingKeyAllowed = nObjectType == GOVERNANCE_OBJECT_PROPOSAL && vote.GetSignal() == VOTE_SIGNAL_FUNDING;

    // Finally check that the vote is actually valid (done last because of cost of signature verification)
    if (!vote.IsValid(pindex, onlyVotingKeyAllowed)) {
        std::ostringstream ostr;
        ostr << "CGovernanceObject::ProcessVote -- Invalid vote"
             << ", MN outpoint = " << vote.GetMasternodeOutpoint().ToStringShort()
             << ", governance object hash = " << GetHash().ToString()
             << ", vote hash = " << vote.GetHash().ToString();
        LogPrintf("%s\n", ostr.str());
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_PERMANENT_ERROR, 20);
        governance->AddInvalidVote(vote);
        return false;
    }

    if (!mmetaman.AddGovernanceVote(dmn->proTxHash, vote.GetParentHash())) {
        std::ostringstream ostr;
        ostr << "CGovernanceObject::ProcessVote -- Unable to add governance vote"
             << ", MN outpoint = " << vote.GetMasternodeOutpoint().ToStringShort()
             << ", governance object hash = " << GetHash().ToString();
        LogPrint(BCLog::GOBJECT, "%s\n", ostr.str());
        exception = CGovernanceException(ostr.str(), GOVERNANCE_EXCEPTION_PERMANENT_ERROR);
        return false;
    }

    voteInstanceRef = vote_instance_t(vote.GetOutcome(), nVoteTimeUpdate, vote.GetTimestamp());
    fileVotes.AddVote(vote);
    fDirtyCache = true;
    return true;
}

void CGovernanceObject::ClearMasternodeVotes()
{
    LOCK(cs);
    auto mnList = deterministicMNManager->GetListAtChainTip();

    auto it = mapCurrentMNVotes.begin();
    while (it != mapCurrentMNVotes.end()) {
        if (!mnList.HasMNByCollateral(it->first)) {
            fileVotes.RemoveVotesFromMasternode(it->first);
            mapCurrentMNVotes.erase(it++);
            fDirtyCache = true;
        } else {
            ++it;
        }
    }
}

std::set<uint256> CGovernanceObject::RemoveInvalidVotes(const CBlockIndex *pindex, const COutPoint& mnOutpoint)
{
    
    LOCK(cs);

    auto it = mapCurrentMNVotes.find(mnOutpoint);
    if (it == mapCurrentMNVotes.end()) {
        // don't even try as we don't have any votes from this MN
        return {};
    }

    auto removedVotes = fileVotes.RemoveInvalidVotes(pindex, mnOutpoint, nObjectType == GOVERNANCE_OBJECT_PROPOSAL);
    if (removedVotes.empty()) {
        return {};
    }

    auto nParentHash = GetHash();
    for (auto jt = it->second.mapInstances.begin(); jt != it->second.mapInstances.end(); ) {
        CGovernanceVote tmpVote(mnOutpoint, nParentHash, (vote_signal_enum_t)jt->first, jt->second.eOutcome);
        tmpVote.SetTime(jt->second.nCreationTime);
        if (removedVotes.count(tmpVote.GetHash())) {
            jt = it->second.mapInstances.erase(jt);
        } else {
            ++jt;
        }
    }
    if (it->second.mapInstances.empty()) {
        mapCurrentMNVotes.erase(it);
    }

    std::string removedStr;
    for (auto& h : removedVotes) {
        removedStr += strprintf("  %s\n", h.ToString());
    }
    LogPrintf("CGovernanceObject::%s -- Removed %d invalid votes for %s from MN %s:\n%s", __func__, removedVotes.size(), nParentHash.ToString(), mnOutpoint.ToString(), removedStr); /* Continued */
    fDirtyCache = true;

    return removedVotes;
}

uint256 CGovernanceObject::GetHash() const
{
    // Note: doesn't match serialization

    // CREATE HASH OF ALL IMPORTANT PIECES OF DATA

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << nHashParent;
    ss << nRevision;
    ss << nTime;
    ss << GetDataAsHexString();
    ss << masternodeOutpoint;
    ss << vchSig;
    // fee_tx is left out on purpose

    return ss.GetHash();
}

uint256 CGovernanceObject::GetSignatureHash() const
{
    return SerializeHash(*this);
}

void CGovernanceObject::SetMasternodeOutpoint(const COutPoint& outpoint)
{
    masternodeOutpoint = outpoint;
}

bool CGovernanceObject::Sign(const CBLSSecretKey& key)
{
    CBLSSignature sig = key.Sign(GetSignatureHash());
    if (!sig.IsValid()) {
        return false;
    }
    vchSig = sig.ToByteVector();
    return true;
}

bool CGovernanceObject::CheckSignature(const CBlockIndex* pindexIn, const CBLSPublicKey& pubKey) const
{
    CBLSSignature sig;
    const auto pindex = llmq::CLLMQUtils::V19ActivationIndex(pindexIn);
    bool is_bls_legacy_scheme = pindex == nullptr || nTime < pindex->nTime;
    sig.SetByteVector(vchSig, is_bls_legacy_scheme);
    if (!sig.VerifyInsecure(pubKey, GetSignatureHash())) {
        LogPrintf("CGovernanceObject::CheckSignature -- VerifyInsecure() failed\n");
        return false;
    }
    return true;
}

/**
   Return the actual object from the vchData JSON structure.

   Returns an empty object on error.
 */
UniValue CGovernanceObject::GetJSONObject()
{
    UniValue obj(UniValue::VOBJ);
    if (vchData.empty()) {
        return obj;
    }

    UniValue objResult(UniValue::VOBJ);
    GetData(objResult);

    if (objResult.isObject()) {
        obj = objResult;
    } else {
        std::vector<UniValue> arr1 = objResult.getValues();
        std::vector<UniValue> arr2 = arr1.at(0).getValues();
        obj = arr2.at(1);
    }

    return obj;
}

/**
*   LoadData
*   --------------------------------------------------------
*
*   Attempt to load data from vchData
*
*/

void CGovernanceObject::LoadData()
{
    if (vchData.empty()) {
        return;
    }

    try {
        // ATTEMPT TO LOAD JSON STRING FROM VCHDATA
        UniValue objResult(UniValue::VOBJ);
        GetData(objResult);
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::LoadData -- GetDataAsPlainString = %s\n", GetDataAsPlainString());
        UniValue obj = GetJSONObject();
        nObjectType = obj["type"].getInt<int>();
    } catch (std::exception& e) {
        fUnparsable = true;
        std::ostringstream ostr;
        ostr << "CGovernanceObject::LoadData Error parsing JSON"
             << ", e.what() = " << e.what();
        LogPrintf("%s\n", ostr.str());
        return;
    } catch (...) {
        fUnparsable = true;
        std::ostringstream ostr;
        ostr << "CGovernanceObject::LoadData Unknown Error parsing JSON";
        LogPrintf("%s\n", ostr.str());
        return;
    }
}

/**
*   GetData - Example usage:
*   --------------------------------------------------------
*
*   Decode governance object data into UniValue(VOBJ)
*
*/

void CGovernanceObject::GetData(UniValue& objResult)
{
    UniValue o(UniValue::VOBJ);
    std::string s = GetDataAsPlainString();
    o.read(s);
    objResult = o;
}

/**
*   GetData - As
*   --------------------------------------------------------
*
*/

std::string CGovernanceObject::GetDataAsHexString() const
{
    return HexStr(vchData);
}

std::string CGovernanceObject::GetDataAsPlainString() const
{
    return std::string(vchData.begin(), vchData.end());
}

UniValue CGovernanceObject::ToJson() const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("objectHash", GetHash().ToString());
    obj.pushKV("parentHash", nHashParent.ToString());
    obj.pushKV("collateralHash", GetCollateralHash().ToString());
    obj.pushKV("createdAt", GetCreationTime());
    obj.pushKV("revision", nRevision);
    UniValue data;
    if (!data.read(GetDataAsPlainString())) {
        data.clear();
        data.setObject();
        data.pushKV("plain", GetDataAsPlainString());
        data.pushKV("hex", GetDataAsHexString());
    } else {
        data.pushKV("hex", GetDataAsHexString());
    }
    obj.pushKV("data", data);
    return obj;
}

void CGovernanceObject::UpdateLocalValidity(ChainstateManager &chainman)
{
    // THIS DOES NOT CHECK COLLATERAL, THIS IS CHECKED UPON ORIGINAL ARRIVAL
    fCachedLocalValidity = IsValidLocally(chainman, strLocalValidityError, false);
}


bool CGovernanceObject::IsValidLocally(ChainstateManager &chainman, std::string& strError, bool fCheckCollateral) const
{
    bool fMissingConfirmations = false;

    return IsValidLocally(chainman, strError, fMissingConfirmations, fCheckCollateral);
}

bool CGovernanceObject::IsValidLocally(ChainstateManager &chainman, std::string& strError, bool& fMissingConfirmations, bool fCheckCollateral) const
{
    fMissingConfirmations = false;

    if (fUnparsable) {
        strError = "Object data unparseable";
        return false;
    }

    switch (nObjectType) {
    case GOVERNANCE_OBJECT_PROPOSAL: {
        CProposalValidator validator(GetDataAsHexString(), true);
        // Note: It's ok to have expired proposals
        // they are going to be cleared by CGovernanceManager::UpdateCachesAndClean()
        // TODO: should they be tagged as "expired" to skip vote downloading?
        if (!validator.Validate(false)) {
            strError = strprintf("Invalid proposal data, error messages: %s", validator.GetErrorMessages());
            return false;
        }
        if (fCheckCollateral && !IsCollateralValid(chainman, strError, fMissingConfirmations)) {
            strError = "Invalid proposal collateral";
            return false;
        }
        
        return true;
    }
    case GOVERNANCE_OBJECT_TRIGGER: {
        if (!fCheckCollateral) {
            // nothing else we can check here (yet?)
            return true;
        }
        auto mnList = deterministicMNManager->GetListAtChainTip();

        std::string strOutpoint = masternodeOutpoint.ToStringShort();
        auto dmn = mnList.GetMNByCollateral(masternodeOutpoint);
        if (!dmn) {
            strError = "Failed to find Masternode by UTXO, missing masternode=" + strOutpoint;
            return false;
        }
        const CBlockIndex *pindex = WITH_LOCK(chainman.GetMutex(), return chainman.ActiveTip());
        // Check that we have a valid MN signature
        if (!CheckSignature(pindex, dmn->pdmnState->pubKeyOperator.Get())) {
            strError = "Invalid masternode signature for: " + strOutpoint + ", pubkey = " + dmn->pdmnState->pubKeyOperator.Get().ToString();
            return false;
        }

        return true;
    }
    default: {
        strError = strprintf("Invalid object type %d", nObjectType);
        return false;
    }
    }
}

CAmount CGovernanceObject::GetMinCollateralFee() const
{
    // Only 1 type has a fee for the moment but switch statement allows for future object types
    switch (nObjectType) {
    case GOVERNANCE_OBJECT_PROPOSAL:
        return GOVERNANCE_PROPOSAL_FEE_TX;
    case GOVERNANCE_OBJECT_TRIGGER:
        return 0;
    default:
        return MAX_MONEY;
    }
}

bool CGovernanceObject::IsCollateralValid(ChainstateManager &chainman, std::string& strError, bool& fMissingConfirmations) const
{
    LOCK(cs_main);
    strError = "";
    fMissingConfirmations = false;
    CAmount nMinFee = GetMinCollateralFee();
    uint256 nExpectedHash = GetHash();

    CTransactionRef txCollateral;
    uint32_t nBlockHeight;
    uint256 nBlockHash;
    // RETRIEVE TRANSACTION IN QUESTION
    if(!pblockindexdb->ReadBlockHeight(nCollateralHash, nBlockHeight)){	    
        
        strError = strprintf("Can't find collateral blockhash %s in block index", nCollateralHash.ToString());	
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::IsCollateralValid -- %s\n", strError);	
        return false;   	
    }
    txCollateral = GetTransaction(chainman.ActiveChain()[nBlockHeight], nullptr, nCollateralHash, Params().GetConsensus(), nBlockHash);
    if(!txCollateral) {
        strError = strprintf("Can't find collateral tx %s", nCollateralHash.ToString());
        LogPrint(BCLog::GOBJECT,"CGovernanceObject::IsCollateralValid -- %s\n", strError);
        return false;
    }
    int nConfirmationsIn = chainman.ActiveHeight() - nBlockHeight + 1;

    if (nBlockHash == uint256()) {
        strError = strprintf("Collateral tx %s is not mined yet", txCollateral->ToString());
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::IsCollateralValid -- %s\n", strError);
    }

    // LOOK FOR SPECIALIZED GOVERNANCE SCRIPT (PROOF OF BURN)

    CScript findScript;
    findScript << OP_RETURN << ToByteVector(nExpectedHash);

    LogPrint(BCLog::GOBJECT, "CGovernanceObject::IsCollateralValid -- txCollateral->vout.size() = %s, findScript = %s, nMinFee = %lld\n",
                txCollateral->vout.size(), ScriptToAsmStr(findScript, false), nMinFee);

    bool foundOpReturn = false;
    for (const auto& output : txCollateral->vout) {
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::IsCollateralValid -- txout = %s, output.nValue = %lld, output.scriptPubKey = %s\n",
                    output.ToString(), output.nValue, ScriptToAsmStr(output.scriptPubKey, false));
        if (output.scriptPubKey.IsUnspendable() && output.scriptPubKey == findScript && output.nValue >= nMinFee) {
            foundOpReturn = true;
        }
    }

    if (!foundOpReturn) {
        strError = strprintf("Couldn't find opReturn %s in %s", nExpectedHash.ToString(), txCollateral->ToString());
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::IsCollateralValid -- %s\n", strError);
        return false;
    }

    // GET CONFIRMATIONS FOR TRANSACTION

    if ((nConfirmationsIn < GOVERNANCE_FEE_CONFIRMATIONS)) {
        strError = strprintf("Collateral requires at least %d confirmations to be relayed throughout the network (it has only %d)", GOVERNANCE_FEE_CONFIRMATIONS, nConfirmationsIn);
        if (nConfirmationsIn >= GOVERNANCE_MIN_RELAY_FEE_CONFIRMATIONS) {
            fMissingConfirmations = true;
            strError += ", pre-accepted -- waiting for required confirmations";
        } else {
            strError += ", rejected -- try again later";
        }
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::IsCollateralValid -- %s\n", strError);

        return false;
    }

    strError = "valid";
    return true;
}

int CGovernanceObject::CountMatchingVotes(vote_signal_enum_t eVoteSignalIn, vote_outcome_enum_t eVoteOutcomeIn) const
{
    LOCK(cs);

    int nCount = 0;
    for (const auto& votepair : mapCurrentMNVotes) {
        const vote_rec_t& recVote = votepair.second;
        auto it2 = recVote.mapInstances.find(eVoteSignalIn);
        if (it2 != recVote.mapInstances.end() && it2->second.eOutcome == eVoteOutcomeIn) {
            ++nCount;
        }
    }
    return nCount;
}

/**
*   Get specific vote counts for each outcome (funding, validity, etc)
*/

int CGovernanceObject::GetAbsoluteYesCount(vote_signal_enum_t eVoteSignalIn) const
{
    return GetYesCount(eVoteSignalIn) - GetNoCount(eVoteSignalIn);
}

int CGovernanceObject::GetAbsoluteNoCount(vote_signal_enum_t eVoteSignalIn) const
{
    return GetNoCount(eVoteSignalIn) - GetYesCount(eVoteSignalIn);
}

int CGovernanceObject::GetYesCount(vote_signal_enum_t eVoteSignalIn) const
{
    return CountMatchingVotes(eVoteSignalIn, VOTE_OUTCOME_YES);
}

int CGovernanceObject::GetNoCount(vote_signal_enum_t eVoteSignalIn) const
{
    return CountMatchingVotes(eVoteSignalIn, VOTE_OUTCOME_NO);
}

int CGovernanceObject::GetAbstainCount(vote_signal_enum_t eVoteSignalIn) const
{
    return CountMatchingVotes(eVoteSignalIn, VOTE_OUTCOME_ABSTAIN);
}

bool CGovernanceObject::GetCurrentMNVotes(const COutPoint& mnCollateralOutpoint, vote_rec_t& voteRecord) const
{
    LOCK(cs);

    auto it = mapCurrentMNVotes.find(mnCollateralOutpoint);
    if (it == mapCurrentMNVotes.end()) {
        return false;
    }
    voteRecord = it->second;
    return true;
}

void CGovernanceObject::Relay(PeerManager& peerman)
{
    // Do not relay until fully synced
    if (!masternodeSync.IsSynced()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::Relay -- won't relay until fully synced\n");
        return;
    }

    CInv inv(MSG_GOVERNANCE_OBJECT, GetHash());
    peerman.RelayTransactionOther(inv);
}

void CGovernanceObject::UpdateSentinelVariables()
{
    // CALCULATE MINIMUM SUPPORT LEVELS REQUIRED
    auto mnList = deterministicMNManager->GetListAtChainTip();
    int nMnCount = (int)mnList.GetValidMNsCount();
    if (nMnCount == 0) return;

    // CALCULATE THE MINIMUM VOTE COUNT REQUIRED FOR FULL SIGNAL

    int nAbsVoteReq = std::max(Params().GetConsensus().nGovernanceMinQuorum, nMnCount / 10);
    int nAbsDeleteReq = std::max(Params().GetConsensus().nGovernanceMinQuorum, (2 * nMnCount) / 3);

    // SET SENTINEL FLAGS TO FALSE

    fCachedFunding = false;
    fCachedValid = true; //default to valid
    fCachedEndorsed = false;
    fDirtyCache = false;

    // SET SENTINEL FLAGS TO TRUE IF MINIMUM SUPPORT LEVELS ARE REACHED
    // ARE ANY OF THESE FLAGS CURRENTLY ACTIVATED?

    if (GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING) >= nAbsVoteReq) fCachedFunding = true;
    if ((GetAbsoluteYesCount(VOTE_SIGNAL_DELETE) >= nAbsDeleteReq) && !fCachedDelete) {
        fCachedDelete = true;
        if (nDeletionTime == 0) {
            nDeletionTime = TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime());
        }
    }
    if (GetAbsoluteYesCount(VOTE_SIGNAL_ENDORSED) >= nAbsVoteReq) fCachedEndorsed = true;

    if (GetAbsoluteNoCount(VOTE_SIGNAL_VALID) >= nAbsVoteReq) fCachedValid = false;
}
