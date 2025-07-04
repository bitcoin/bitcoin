// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/object.h>

#include <bls/bls.h>
#include <chainparams.h>
#include <core_io.h>
#include <evo/deterministicmns.h>
#include <governance/governance.h>
#include <governance/validators.h>
#include <index/txindex.h>
#include <masternode/meta.h>
#include <masternode/node.h>
#include <masternode/sync.h>
#include <messagesigner.h>
#include <net_processing.h>
#include <timedata.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

#include <string>

CGovernanceObject::CGovernanceObject()
{
    // PARSE JSON DATA STORAGE (VCHDATA)
    LoadData();
}

CGovernanceObject::CGovernanceObject(const uint256& nHashParentIn, int nRevisionIn, int64_t nTimeIn,
                                     const uint256& nCollateralHashIn, const std::string& strDataHexIn) :
    cs(),
    m_obj{nHashParentIn, nRevisionIn, nTimeIn, nCollateralHashIn, strDataHexIn}
{
    // PARSE JSON DATA STORAGE (VCHDATA)
    LoadData();
}

CGovernanceObject::CGovernanceObject(const CGovernanceObject& other) :
    cs(),
    m_obj{other.m_obj},
    nDeletionTime(other.nDeletionTime),
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

bool CGovernanceObject::ProcessVote(CMasternodeMetaMan& mn_metaman, CGovernanceManager& govman, const CDeterministicMNList& tip_mn_list,
                                    const CGovernanceVote& vote, CGovernanceException& exception)
{
    assert(mn_metaman.IsValid());

    LOCK(cs);

    // do not process already known valid votes twice
    if (fileVotes.HasVote(vote.GetHash())) {
        // nothing to do here, not an error
        std::string msg{strprintf("CGovernanceObject::%s -- Already known valid vote", __func__)};
        LogPrint(BCLog::GOBJECT, "%s\n", msg);
        exception = CGovernanceException(msg, GOVERNANCE_EXCEPTION_NONE);
        return false;
    }

    auto dmn = tip_mn_list.GetMNByCollateral(vote.GetMasternodeOutpoint());
    if (!dmn) {
        std::string msg{strprintf("CGovernanceObject::%s -- Masternode %s not found", __func__,
            vote.GetMasternodeOutpoint().ToStringShort())};
        exception = CGovernanceException(msg, GOVERNANCE_EXCEPTION_PERMANENT_ERROR, 20);
        return false;
    }

    auto it = mapCurrentMNVotes.emplace(vote_m_t::value_type(vote.GetMasternodeOutpoint(), vote_rec_t())).first;
    vote_rec_t& voteRecordRef = it->second;
    vote_signal_enum_t eSignal = vote.GetSignal();
    if (eSignal == VOTE_SIGNAL_NONE) {
        std::string msg{strprintf("CGovernanceObject::%s -- Vote signal: none", __func__)};
        LogPrint(BCLog::GOBJECT, "%s\n", msg);
        exception = CGovernanceException(msg, GOVERNANCE_EXCEPTION_WARNING);
        return false;
    }
    if (eSignal < VOTE_SIGNAL_NONE || eSignal >= VOTE_SIGNAL_UNKNOWN) {
        std::string msg{strprintf("CGovernanceObject::%s -- Unsupported vote signal: %s", __func__,
            CGovernanceVoting::ConvertSignalToString(vote.GetSignal()))};
        LogPrintf("%s\n", msg);
        exception = CGovernanceException(msg, GOVERNANCE_EXCEPTION_PERMANENT_ERROR, 20);
        return false;
    }
    auto it2 = voteRecordRef.mapInstances.emplace(vote_instance_m_t::value_type(int(eSignal), vote_instance_t())).first;
    vote_instance_t& voteInstanceRef = it2->second;

    // Reject obsolete votes
    if (vote.GetTimestamp() < voteInstanceRef.nCreationTime) {
        std::string msg{strprintf("CGovernanceObject::%s -- Obsolete vote", __func__)};
        LogPrint(BCLog::GOBJECT, "%s\n", msg);
        exception = CGovernanceException(msg, GOVERNANCE_EXCEPTION_NONE);
        return false;
    } else if (vote.GetTimestamp() == voteInstanceRef.nCreationTime) {
        // Someone is doing something fishy, there can be no two votes from the same masternode
        // with the same timestamp for the same object and signal and yet different hash/outcome.
        std::string msg{strprintf("CGovernanceObject::%s -- Invalid vote, same timestamp for the different outcome", __func__)};
        if (vote.GetOutcome() < voteInstanceRef.eOutcome) {
            // This is an arbitrary comparison, we have to agree on some way
            // to pick the "winning" vote.
            msg += ", rejected";
            LogPrint(BCLog::GOBJECT, "%s\n", msg);
            exception = CGovernanceException(msg, GOVERNANCE_EXCEPTION_NONE);
            return false;
        }
        msg += ", accepted";
        LogPrint(BCLog::GOBJECT, "%s\n", msg);
    }

    int64_t nNow = GetAdjustedTime();
    int64_t nVoteTimeUpdate = voteInstanceRef.nTime;
    if (govman.AreRateChecksEnabled()) {
        int64_t nTimeDelta = nNow - voteInstanceRef.nTime;
        if (nTimeDelta < GOVERNANCE_UPDATE_MIN) {
            std::string msg{strprintf("CGovernanceObject::%s -- Masternode voting too often, MN outpoint = %s, "
                                      "governance object hash = %s, time delta = %d",
                __func__, vote.GetMasternodeOutpoint().ToStringShort(), GetHash().ToString(), nTimeDelta)};
            LogPrint(BCLog::GOBJECT, "%s\n", msg);
            exception = CGovernanceException(msg, GOVERNANCE_EXCEPTION_TEMPORARY_ERROR);
            return false;
        }
        nVoteTimeUpdate = nNow;
    }

    bool onlyVotingKeyAllowed = m_obj.type == GovernanceObject::PROPOSAL && vote.GetSignal() == VOTE_SIGNAL_FUNDING;

    // Finally check that the vote is actually valid (done last because of cost of signature verification)
    if (!vote.IsValid(tip_mn_list, onlyVotingKeyAllowed)) {
        std::string msg{strprintf("CGovernanceObject::%s -- Invalid vote, MN outpoint = %s, governance object hash = %s, "
                                  "vote hash = %s",
            __func__, vote.GetMasternodeOutpoint().ToStringShort(), GetHash().ToString(), vote.GetHash().ToString())};
        LogPrintf("%s\n", msg);
        exception = CGovernanceException(msg, GOVERNANCE_EXCEPTION_PERMANENT_ERROR, 20);
        govman.AddInvalidVote(vote);
        return false;
    }

    if (!mn_metaman.AddGovernanceVote(dmn->proTxHash, vote.GetParentHash())) {
        std::string msg{strprintf("CGovernanceObject::%s -- Unable to add governance vote, MN outpoint = %s, "
                                  "governance object hash = %s",
            __func__, vote.GetMasternodeOutpoint().ToStringShort(), GetHash().ToString())};
        LogPrint(BCLog::GOBJECT, "%s\n", msg);
        exception = CGovernanceException(msg, GOVERNANCE_EXCEPTION_PERMANENT_ERROR);
        return false;
    }

    voteInstanceRef = vote_instance_t(vote.GetOutcome(), nVoteTimeUpdate, vote.GetTimestamp());
    fileVotes.AddVote(vote);
    fDirtyCache = true;
    // SEND NOTIFICATION TO SCRIPT/ZMQ
    GetMainSignals().NotifyGovernanceVote(tip_mn_list, std::make_shared<const CGovernanceVote>(vote));
    return true;
}

void CGovernanceObject::ClearMasternodeVotes(const CDeterministicMNList& tip_mn_list)
{
    LOCK(cs);

    auto it = mapCurrentMNVotes.begin();
    while (it != mapCurrentMNVotes.end()) {
        if (!tip_mn_list.HasMNByCollateral(it->first)) {
            fileVotes.RemoveVotesFromMasternode(it->first);
            mapCurrentMNVotes.erase(it++);
            fDirtyCache = true;
        } else {
            ++it;
        }
    }
}

std::set<uint256> CGovernanceObject::RemoveInvalidVotes(const CDeterministicMNList& tip_mn_list, const COutPoint& mnOutpoint)
{
    LOCK(cs);

    auto it = mapCurrentMNVotes.find(mnOutpoint);
    if (it == mapCurrentMNVotes.end()) {
        // don't even try as we don't have any votes from this MN
        return {};
    }

    auto removedVotes = fileVotes.RemoveInvalidVotes(tip_mn_list, mnOutpoint, m_obj.type == GovernanceObject::PROPOSAL);
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
    for (const auto& h : removedVotes) {
        removedStr += strprintf("  %s\n", h.ToString());
    }
    LogPrintf("CGovernanceObject::%s -- Removed %d invalid votes for %s from MN %s:\n%s", __func__, removedVotes.size(), nParentHash.ToString(), mnOutpoint.ToString(), removedStr); /* Continued */
    fDirtyCache = true;

    return removedVotes;
}

uint256 CGovernanceObject::GetHash() const
{
    return m_obj.GetHash();
}

uint256 CGovernanceObject::GetDataHash() const
{
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << GetDataAsHexString();

    return ss.GetHash();
}

uint256 CGovernanceObject::GetSignatureHash() const
{
    return SerializeHash(*this);
}

void CGovernanceObject::SetMasternodeOutpoint(const COutPoint& outpoint)
{
    m_obj.masternodeOutpoint = outpoint;
}

bool CGovernanceObject::Sign(const CActiveMasternodeManager& mn_activeman)
{
    CBLSSignature sig = mn_activeman.Sign(GetSignatureHash(), false);
    if (!sig.IsValid()) {
        return false;
    }
    m_obj.vchSig = sig.ToByteVector(false);
    return true;
}

bool CGovernanceObject::CheckSignature(const CBLSPublicKey& pubKey) const
{
    CBLSSignature sig;
    sig.SetBytes(m_obj.vchSig, false);
    if (!sig.VerifyInsecure(pubKey, GetSignatureHash(), false)) {
        LogPrintf("CGovernanceObject::CheckSignature -- VerifyInsecure() failed\n");
        return false;
    }
    return true;
}

/**
   Return the actual object from the vchData JSON structure.

   Returns an empty object on error.
 */
UniValue CGovernanceObject::GetJSONObject() const
{
    UniValue obj(UniValue::VOBJ);
    if (m_obj.vchData.empty()) {
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
    if (m_obj.vchData.empty()) {
        return;
    }

    try {
        // ATTEMPT TO LOAD JSON STRING FROM VCHDATA
        UniValue objResult(UniValue::VOBJ);
        GetData(objResult);
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::LoadData -- GetDataAsPlainString = %s\n", GetDataAsPlainString());
        UniValue obj = GetJSONObject();
        m_obj.type = GovernanceObject(obj["type"].get_int());
    } catch (std::exception& e) {
        fUnparsable = true;
        LogPrintf("%s\n", strprintf("CGovernanceObject::LoadData -- Error parsing JSON, e.what() = %s", e.what()));
        return;
    } catch (...) {
        fUnparsable = true;
        LogPrintf("%s\n", strprintf("CGovernanceObject::LoadData -- Unknown Error parsing JSON"));
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

void CGovernanceObject::GetData(UniValue& objResult) const
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
    return m_obj.GetDataAsHexString();
}

std::string CGovernanceObject::GetDataAsPlainString() const
{
    return m_obj.GetDataAsPlainString();
}

UniValue CGovernanceObject::ToJson() const
{
    return m_obj.ToJson();
}

void CGovernanceObject::UpdateLocalValidity(const CDeterministicMNList& tip_mn_list, const ChainstateManager& chainman)
{
    AssertLockHeld(::cs_main);
    // THIS DOES NOT CHECK COLLATERAL, THIS IS CHECKED UPON ORIGINAL ARRIVAL
    fCachedLocalValidity = IsValidLocally(tip_mn_list, chainman, strLocalValidityError, false);
}


bool CGovernanceObject::IsValidLocally(const CDeterministicMNList& tip_mn_list, const ChainstateManager& chainman, std::string& strError, bool fCheckCollateral) const
{
    bool fMissingConfirmations = false;

    return IsValidLocally(tip_mn_list, chainman, strError, fMissingConfirmations, fCheckCollateral);
}

bool CGovernanceObject::IsValidLocally(const CDeterministicMNList& tip_mn_list, const ChainstateManager& chainman, std::string& strError, bool& fMissingConfirmations, bool fCheckCollateral) const
{
    AssertLockHeld(::cs_main);

    fMissingConfirmations = false;

    if (fUnparsable) {
        strError = "Object data unparsable";
        return false;
    }

    switch (m_obj.type) {
    case GovernanceObject::PROPOSAL: {
        CProposalValidator validator(GetDataAsHexString());
        // Note: It's ok to have expired proposals
        // they are going to be cleared by CGovernanceManager::CheckAndRemove()
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
    case GovernanceObject::TRIGGER: {
        if (!fCheckCollateral) {
            // nothing else we can check here (yet?)
            return true;
        }

        std::string strOutpoint = m_obj.masternodeOutpoint.ToStringShort();
        auto dmn = tip_mn_list.GetMNByCollateral(m_obj.masternodeOutpoint);
        if (!dmn) {
            strError = "Failed to find Masternode by UTXO, missing masternode=" + strOutpoint;
            return false;
        }

        // Check that we have a valid MN signature
        if (!CheckSignature(dmn->pdmnState->pubKeyOperator.Get())) {
            strError = "Invalid masternode signature for: " + strOutpoint + ", pubkey = " + dmn->pdmnState->pubKeyOperator.ToString();
            return false;
        }

        return true;
    }
    default: {
        strError = strprintf("Invalid object type %d", ToUnderlying(m_obj.type));
        return false;
    }
    }
}

CAmount CGovernanceObject::GetMinCollateralFee() const
{
    // Only 1 type has a fee for the moment but switch statement allows for future object types
    switch (m_obj.type) {
        case GovernanceObject::PROPOSAL: {
            return GOVERNANCE_PROPOSAL_FEE_TX;
        }
        case GovernanceObject::TRIGGER: {
            return 0;
        }
        default: {
            return MAX_MONEY;
        }
    }
}

bool CGovernanceObject::IsCollateralValid(const ChainstateManager& chainman, std::string& strError, bool& fMissingConfirmations) const
{
    AssertLockHeld(::cs_main);

    strError = "";
    fMissingConfirmations = false;
    uint256 nExpectedHash = GetHash();

    CTransactionRef txCollateral;
    uint256 nBlockHash;
    if (g_txindex) {
        g_txindex->FindTx(m_obj.collateralHash, nBlockHash, txCollateral);
    }

    if (!txCollateral) {
        strError = strprintf("Can't find collateral tx %s", m_obj.collateralHash.ToString());
        LogPrintf("CGovernanceObject::IsCollateralValid -- %s\n", strError);
        return false;
    }

    if (nBlockHash == uint256()) {
        strError = strprintf("Collateral tx %s is not mined yet", txCollateral->ToString());
        LogPrintf("CGovernanceObject::IsCollateralValid -- %s\n", strError);
        return false;
    }

    if (txCollateral->vout.empty()) {
        strError = "tx vout is empty";
        LogPrintf("CGovernanceObject::IsCollateralValid -- %s\n", strError);
        return false;
    }

    // LOOK FOR SPECIALIZED GOVERNANCE SCRIPT (PROOF OF BURN)

    CScript findScript;
    findScript << OP_RETURN << ToByteVector(nExpectedHash);

    CAmount nMinFee = GetMinCollateralFee();

    LogPrint(BCLog::GOBJECT, "CGovernanceObject::IsCollateralValid -- txCollateral->vout.size() = %s, findScript = %s, nMinFee = %lld\n",
                txCollateral->vout.size(), ScriptToAsmStr(findScript, false), nMinFee);

    bool foundOpReturn = false;
    for (const auto& output : txCollateral->vout) {
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::IsCollateralValid -- txout = %s, output.nValue = %lld, output.scriptPubKey = %s\n",
                    output.ToString(), output.nValue, ScriptToAsmStr(output.scriptPubKey, false));
        if (!output.scriptPubKey.IsPayToPublicKeyHash() && !output.scriptPubKey.IsUnspendable()) {
            strError = strprintf("Invalid Script %s", txCollateral->ToString());
            LogPrintf("CGovernanceObject::IsCollateralValid -- %s\n", strError);
            return false;
        }
        if (output.scriptPubKey == findScript && output.nValue >= nMinFee) {
            foundOpReturn = true;
        }
    }

    if (!foundOpReturn) {
        strError = strprintf("Couldn't find opReturn %s in %s", nExpectedHash.ToString(), txCollateral->ToString());
        LogPrintf("CGovernanceObject::IsCollateralValid -- %s\n", strError);
        return false;
    }

    // GET CONFIRMATIONS FOR TRANSACTION

    AssertLockHeld(::cs_main);
    int nConfirmationsIn = 0;
    if (nBlockHash != uint256()) {
        const CBlockIndex* pindex = chainman.m_blockman.LookupBlockIndex(nBlockHash);
        if (pindex && chainman.ActiveChain().Contains(pindex)) {
            nConfirmationsIn += chainman.ActiveChain().Height() - pindex->nHeight + 1;
        }
    }

    if (nConfirmationsIn < GOVERNANCE_FEE_CONFIRMATIONS) {
        strError = strprintf("Collateral requires at least %d confirmations to be relayed throughout the network (it has only %d)", GOVERNANCE_FEE_CONFIRMATIONS, nConfirmationsIn);
        if (nConfirmationsIn >= GOVERNANCE_MIN_RELAY_FEE_CONFIRMATIONS) {
            fMissingConfirmations = true;
            strError += ", pre-accepted -- waiting for required confirmations";
        } else {
            strError += ", rejected -- try again later";
        }
        LogPrintf("CGovernanceObject::IsCollateralValid -- %s\n", strError);

        return false;
    }

    strError = "valid";
    return true;
}

int CGovernanceObject::CountMatchingVotes(const CDeterministicMNList& tip_mn_list, vote_signal_enum_t eVoteSignalIn, vote_outcome_enum_t eVoteOutcomeIn) const
{
    LOCK(cs);

    int nCount = 0;
    for (const auto& votepair : mapCurrentMNVotes) {
        const vote_rec_t& recVote = votepair.second;
        auto it2 = recVote.mapInstances.find(eVoteSignalIn);
        if (it2 != recVote.mapInstances.end() && it2->second.eOutcome == eVoteOutcomeIn) {
            // 4x times weight vote for EvoNode owners.
            // No need to check if v19 is active since no EvoNode are allowed to register before v19s
            auto dmn = tip_mn_list.GetMNByCollateral(votepair.first);
            if (dmn != nullptr) nCount += GetMnType(dmn->nType).voting_weight;
        }
    }
    return nCount;
}

/**
*   Get specific vote counts for each outcome (funding, validity, etc)
*/

int CGovernanceObject::GetAbsoluteYesCount(const CDeterministicMNList& tip_mn_list, vote_signal_enum_t eVoteSignalIn) const
{
    return GetYesCount(tip_mn_list, eVoteSignalIn) - GetNoCount(tip_mn_list, eVoteSignalIn);
}

int CGovernanceObject::GetAbsoluteNoCount(const CDeterministicMNList& tip_mn_list, vote_signal_enum_t eVoteSignalIn) const
{
    return GetNoCount(tip_mn_list, eVoteSignalIn) - GetYesCount(tip_mn_list, eVoteSignalIn);
}

int CGovernanceObject::GetYesCount(const CDeterministicMNList& tip_mn_list, vote_signal_enum_t eVoteSignalIn) const
{
    return CountMatchingVotes(tip_mn_list, eVoteSignalIn, VOTE_OUTCOME_YES);
}

int CGovernanceObject::GetNoCount(const CDeterministicMNList& tip_mn_list, vote_signal_enum_t eVoteSignalIn) const
{
    return CountMatchingVotes(tip_mn_list, eVoteSignalIn, VOTE_OUTCOME_NO);
}

int CGovernanceObject::GetAbstainCount(const CDeterministicMNList& tip_mn_list, vote_signal_enum_t eVoteSignalIn) const
{
    return CountMatchingVotes(tip_mn_list, eVoteSignalIn, VOTE_OUTCOME_ABSTAIN);
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

void CGovernanceObject::Relay(PeerManager& peerman, const CMasternodeSync& mn_sync) const
{
    // Do not relay until fully synced
    if (!mn_sync.IsSynced()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceObject::Relay -- won't relay until fully synced\n");
        return;
    }

    int minProtoVersion = MIN_PEER_PROTO_VERSION;
    if (m_obj.type == GovernanceObject::PROPOSAL) {
        // We know this proposal is valid locally, otherwise we would not get to the point we should relay it.
        // But we don't want to relay it to pre-GOVSCRIPT_PROTO_VERSION peers if payment_address is p2sh
        // because they won't accept it anyway and will simply ban us eventually.
        CProposalValidator validator(GetDataAsHexString(), false /* no script */);
        if (!validator.Validate(false /* ignore expiration */)) {
            // The only way we could get here is when proposal is valid but payment_address is actually p2sh.
            LogPrint(BCLog::GOBJECT, "CGovernanceObject::Relay -- won't relay %s to older peers\n", GetHash().ToString());
            minProtoVersion = GOVSCRIPT_PROTO_VERSION;
        }
    }

    CInv inv(MSG_GOVERNANCE_OBJECT, GetHash());
    peerman.RelayInv(inv, minProtoVersion);
}

void CGovernanceObject::UpdateSentinelVariables(const CDeterministicMNList& tip_mn_list)
{
    // CALCULATE MINIMUM SUPPORT LEVELS REQUIRED

    int nWeightedMnCount = (int)tip_mn_list.GetValidWeightedMNsCount();
    if (nWeightedMnCount == 0) return;

    // CALCULATE THE MINIMUM VOTE COUNT REQUIRED FOR FULL SIGNAL

    int nAbsVoteReq = std::max(Params().GetConsensus().nGovernanceMinQuorum, nWeightedMnCount / 10);
    int nAbsDeleteReq = std::max(Params().GetConsensus().nGovernanceMinQuorum, (2 * nWeightedMnCount) / 3);

    // SET SENTINEL FLAGS TO FALSE

    fCachedFunding = false;
    fCachedValid = true; //default to valid
    fCachedEndorsed = false;
    fDirtyCache = false;

    // SET SENTINEL FLAGS TO TRUE IF MINIMUM SUPPORT LEVELS ARE REACHED
    // ARE ANY OF THESE FLAGS CURRENTLY ACTIVATED?

    if (GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_FUNDING) >= nAbsVoteReq) fCachedFunding = true;
    if ((GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_DELETE) >= nAbsDeleteReq) && !fCachedDelete) {
        fCachedDelete = true;
        if (nDeletionTime == 0) {
            nDeletionTime = GetTime<std::chrono::seconds>().count();
        }
    }
    if (GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_ENDORSED) >= nAbsVoteReq) fCachedEndorsed = true;

    if (GetAbsoluteNoCount(tip_mn_list, VOTE_SIGNAL_VALID) >= nAbsVoteReq) fCachedValid = false;
}
