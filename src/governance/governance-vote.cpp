// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/governance-vote.h>

#include <bls/bls.h>
#include <key.h>
#include <masternode/masternode-sync.h>
#include <messagesigner.h>
#include <net.h>
#include <util/system.h>

#include <evo/deterministicmns.h>

std::string CGovernanceVoting::ConvertOutcomeToString(vote_outcome_enum_t nOutcome)
{
    static const std::map<vote_outcome_enum_t, std::string> mapOutcomeString = {
        { VOTE_OUTCOME_NONE, "none" },
        { VOTE_OUTCOME_YES, "yes" },
        { VOTE_OUTCOME_NO, "no" },
        { VOTE_OUTCOME_ABSTAIN, "abstain" } };

    const auto& it = mapOutcomeString.find(nOutcome);
    if (it == mapOutcomeString.end()) {
        LogPrintf("CGovernanceVoting::%s -- ERROR: Unknown outcome %d\n", __func__, nOutcome);
        return "error";
    }
    return it->second;
}

std::string CGovernanceVoting::ConvertSignalToString(vote_signal_enum_t nSignal)
{
    static const std::map<vote_signal_enum_t, std::string> mapSignalsString = {
        { VOTE_SIGNAL_FUNDING, "funding" },
        { VOTE_SIGNAL_VALID, "valid" },
        { VOTE_SIGNAL_DELETE, "delete" },
        { VOTE_SIGNAL_ENDORSED, "endorsed" } };

    const auto& it = mapSignalsString.find(nSignal);
    if (it == mapSignalsString.end()) {
        LogPrintf("CGovernanceVoting::%s -- ERROR: Unknown signal %d\n", __func__, nSignal);
        return "none";
    }
    return it->second;
}


vote_outcome_enum_t CGovernanceVoting::ConvertVoteOutcome(const std::string& strVoteOutcome)
{
    static const std::map<std::string, vote_outcome_enum_t> mapStringOutcome = {
        { "none", VOTE_OUTCOME_NONE },
        { "yes", VOTE_OUTCOME_YES },
        { "no", VOTE_OUTCOME_NO },
        { "abstain", VOTE_OUTCOME_ABSTAIN } };

    const auto& it = mapStringOutcome.find(strVoteOutcome);
    if (it == mapStringOutcome.end()) {
        LogPrintf("CGovernanceVoting::%s -- ERROR: Unknown outcome %s\n", __func__, strVoteOutcome);
        return VOTE_OUTCOME_NONE;
    }
    return it->second;

}

vote_signal_enum_t CGovernanceVoting::ConvertVoteSignal(const std::string& strVoteSignal)
{
    static const std::map<std::string, vote_signal_enum_t> mapStrVoteSignals = {
        {"funding", VOTE_SIGNAL_FUNDING},
        {"valid", VOTE_SIGNAL_VALID},
        {"delete", VOTE_SIGNAL_DELETE},
        {"endorsed", VOTE_SIGNAL_ENDORSED}};

    const auto& it = mapStrVoteSignals.find(strVoteSignal);
    if (it == mapStrVoteSignals.end()) {
        LogPrintf("CGovernanceVoting::%s -- ERROR: Unknown signal %s\n", __func__, strVoteSignal);
        return VOTE_SIGNAL_NONE;
    }
    return it->second;
}

CGovernanceVote::CGovernanceVote() :
    fValid(true),
    fSynced(false),
    nVoteSignal(int(VOTE_SIGNAL_NONE)),
    masternodeOutpoint(),
    nParentHash(),
    nVoteOutcome(int(VOTE_OUTCOME_NONE)),
    nTime(0),
    vchSig()
{
}

CGovernanceVote::CGovernanceVote(const COutPoint& outpointMasternodeIn, const uint256& nParentHashIn, vote_signal_enum_t eVoteSignalIn, vote_outcome_enum_t eVoteOutcomeIn) :
    fValid(true),
    fSynced(false),
    nVoteSignal(eVoteSignalIn),
    masternodeOutpoint(outpointMasternodeIn),
    nParentHash(nParentHashIn),
    nVoteOutcome(eVoteOutcomeIn),
    nTime(GetAdjustedTime()),
    vchSig()
{
    UpdateHash();
}

std::string CGovernanceVote::ToString() const
{
    std::ostringstream ostr;
    ostr << masternodeOutpoint.ToStringShort() << ":"
         << nTime << ":"
         << CGovernanceVoting::ConvertOutcomeToString(GetOutcome()) << ":"
         << CGovernanceVoting::ConvertSignalToString(GetSignal());
    return ostr.str();
}

void CGovernanceVote::Relay(CConnman& connman) const
{
    // Do not relay until fully synced
    if (!masternodeSync.IsSynced()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceVote::Relay -- won't relay until fully synced\n");
        return;
    }

    auto mnList = deterministicMNManager->GetListAtChainTip();
    auto dmn = mnList.GetMNByCollateral(masternodeOutpoint);
    if (!dmn) {
        return;
    }

    // When this vote is from non-valid (PoSe banned) MN, we should only announce it to v0.14.0.1 nodes as older nodes
    // will ban us otherwise.
    int minVersion = MIN_GOVERNANCE_PEER_PROTO_VERSION;
    if (!CDeterministicMNList::IsMNValid(dmn)) {
        minVersion = GOVERNANCE_POSE_BANNED_VOTES_VERSION;
    }

    CInv inv(MSG_GOVERNANCE_OBJECT_VOTE, GetHash());
    connman.RelayInv(inv, minVersion);
}

void CGovernanceVote::UpdateHash() const
{
    // Note: doesn't match serialization

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << masternodeOutpoint << uint8_t{} << 0xffffffff; // adding dummy values here to match old hashing format
    ss << nParentHash;
    ss << nVoteSignal;
    ss << nVoteOutcome;
    ss << nTime;
    *const_cast<uint256*>(&hash) = ss.GetHash();
}

uint256 CGovernanceVote::GetHash() const
{
    return hash;
}

uint256 CGovernanceVote::GetSignatureHash() const
{
    return SerializeHash(*this);
}

bool CGovernanceVote::Sign(const CKey& key, const CKeyID& keyID)
{
    std::string strError;

    // Harden Spork6 so that it is active on testnet and no other networks
    if (Params().NetworkIDString() == CBaseChainParams::TESTNET) {
        uint256 hash = GetSignatureHash();

        if (!CHashSigner::SignHash(hash, key, vchSig)) {
            LogPrintf("CGovernanceVote::Sign -- SignHash() failed\n");
            return false;
        }

        if (!CHashSigner::VerifyHash(hash, keyID, vchSig, strError)) {
            LogPrintf("CGovernanceVote::Sign -- VerifyHash() failed, error: %s\n", strError);
            return false;
        }
    } else {
        std::string strMessage = masternodeOutpoint.ToStringShort() + "|" + nParentHash.ToString() + "|" +
                                 std::to_string(nVoteSignal) + "|" + std::to_string(nVoteOutcome) + "|" + std::to_string(nTime);

        if (!CMessageSigner::SignMessage(strMessage, vchSig, key)) {
            LogPrintf("CGovernanceVote::Sign -- SignMessage() failed\n");
            return false;
        }

        if (!CMessageSigner::VerifyMessage(keyID, vchSig, strMessage, strError)) {
            LogPrintf("CGovernanceVote::Sign -- VerifyMessage() failed, error: %s\n", strError);
            return false;
        }
    }

    return true;
}

bool CGovernanceVote::CheckSignature(const CKeyID& keyID) const
{
    std::string strError;

    // Harden Spork6 so that it is active on testnet and no other networks
    if (Params().NetworkIDString() == CBaseChainParams::TESTNET) {
        uint256 hash = GetSignatureHash();

        if (!CHashSigner::VerifyHash(hash, keyID, vchSig, strError)) {
            LogPrint(BCLog::GOBJECT, "CGovernanceVote::IsValid -- VerifyHash() failed, error: %s\n", strError);
            return false;
        }
    } else {
        std::string strMessage = masternodeOutpoint.ToStringShort() + "|" + nParentHash.ToString() + "|" +
                                 std::to_string(nVoteSignal) + "|" +
                                 std::to_string(nVoteOutcome) + "|" +
                                 std::to_string(nTime);

        if (!CMessageSigner::VerifyMessage(keyID, vchSig, strMessage, strError)) {
            LogPrint(BCLog::GOBJECT, "CGovernanceVote::IsValid -- VerifyMessage() failed, error: %s\n", strError);
            return false;
        }
    }

    return true;
}

bool CGovernanceVote::Sign(const CBLSSecretKey& key)
{
    uint256 hash = GetSignatureHash();
    CBLSSignature sig = key.Sign(hash);
    if (!sig.IsValid()) {
        return false;
    }
    vchSig = sig.ToByteVector();
    return true;
}

bool CGovernanceVote::CheckSignature(const CBLSPublicKey& pubKey) const
{
    if (!CBLSSignature(vchSig).VerifyInsecure(pubKey, GetSignatureHash())) {
        LogPrintf("CGovernanceVote::CheckSignature -- VerifyInsecure() failed\n");
        return false;
    }
    return true;
}

bool CGovernanceVote::IsValid(bool useVotingKey) const
{
    if (nTime > GetAdjustedTime() + (60 * 60)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceVote::IsValid -- vote is too far ahead of current time - %s - nTime %lli - Max Time %lli\n", GetHash().ToString(), nTime, GetAdjustedTime() + (60 * 60));
        return false;
    }

    // support up to MAX_SUPPORTED_VOTE_SIGNAL, can be extended
    if (nVoteSignal > MAX_SUPPORTED_VOTE_SIGNAL) {
        LogPrint(BCLog::GOBJECT, "CGovernanceVote::IsValid -- Client attempted to vote on invalid signal(%d) - %s\n", nVoteSignal, GetHash().ToString());
        return false;
    }

    // 0=none, 1=yes, 2=no, 3=abstain. Beyond that reject votes
    if (nVoteOutcome > 3) {
        LogPrint(BCLog::GOBJECT, "CGovernanceVote::IsValid -- Client attempted to vote on invalid outcome(%d) - %s\n", nVoteSignal, GetHash().ToString());
        return false;
    }

    auto dmn = deterministicMNManager->GetListAtChainTip().GetMNByCollateral(masternodeOutpoint);
    if (!dmn) {
        LogPrint(BCLog::GOBJECT, "CGovernanceVote::IsValid -- Unknown Masternode - %s\n", masternodeOutpoint.ToStringShort());
        return false;
    }

    if (useVotingKey) {
        return CheckSignature(dmn->pdmnState->keyIDVoting);
    } else {
        return CheckSignature(dmn->pdmnState->pubKeyOperator.Get());
    }
}

bool operator==(const CGovernanceVote& vote1, const CGovernanceVote& vote2)
{
    bool fResult = ((vote1.masternodeOutpoint == vote2.masternodeOutpoint) &&
                    (vote1.nParentHash == vote2.nParentHash) &&
                    (vote1.nVoteOutcome == vote2.nVoteOutcome) &&
                    (vote1.nVoteSignal == vote2.nVoteSignal) &&
                    (vote1.nTime == vote2.nTime));
    return fResult;
}

bool operator<(const CGovernanceVote& vote1, const CGovernanceVote& vote2)
{
    bool fResult = (vote1.masternodeOutpoint < vote2.masternodeOutpoint);
    if (!fResult) {
        return false;
    }
    fResult = (vote1.masternodeOutpoint == vote2.masternodeOutpoint);

    fResult = fResult && (vote1.nParentHash < vote2.nParentHash);
    if (!fResult) {
        return false;
    }
    fResult = (vote1.nParentHash == vote2.nParentHash);

    fResult = fResult && (vote1.nVoteOutcome < vote2.nVoteOutcome);
    if (!fResult) {
        return false;
    }
    fResult = (vote1.nVoteOutcome == vote2.nVoteOutcome);

    fResult = fResult && (vote1.nVoteSignal == vote2.nVoteSignal);
    if (!fResult) {
        return false;
    }
    fResult = (vote1.nVoteSignal == vote2.nVoteSignal);

    fResult = fResult && (vote1.nTime < vote2.nTime);

    return fResult;
}
