// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governancevote.h>
#include <governanceobject.h>
#include <masternodesync.h>
#include <masternodeman.h>
#include <messagesigner.h>


std::string CGovernanceVoting::ConvertOutcomeToString(vote_outcome_enum_t nOutcome)
{
    switch(nOutcome)
    {
        case VOTE_OUTCOME_NONE:
            return "NONE"; break;
        case VOTE_OUTCOME_YES:
            return "YES"; break;
        case VOTE_OUTCOME_NO:
            return "NO"; break;
        case VOTE_OUTCOME_ABSTAIN:
            return "ABSTAIN"; break;
    }
    return "error";
}

std::string CGovernanceVoting::ConvertSignalToString(vote_signal_enum_t nSignal)
{
    std::string strReturn = "NONE";
    switch(nSignal)
    {
        case VOTE_SIGNAL_NONE:
            strReturn = "NONE";
            break;
        case VOTE_SIGNAL_FUNDING:
            strReturn = "FUNDING";
            break;
        case VOTE_SIGNAL_VALID:
            strReturn = "VALID";
            break;
        case VOTE_SIGNAL_DELETE:
            strReturn = "DELETE";
            break;
        case VOTE_SIGNAL_ENDORSED:
            strReturn = "ENDORSED";
            break;
    }

    return strReturn;
}


vote_outcome_enum_t CGovernanceVoting::ConvertVoteOutcome(const std::string& strVoteOutcome)
{
    vote_outcome_enum_t eVote = VOTE_OUTCOME_NONE;
    if(strVoteOutcome == "yes") {
        eVote = VOTE_OUTCOME_YES;
    }
    else if(strVoteOutcome == "no") {
        eVote = VOTE_OUTCOME_NO;
    }
    else if(strVoteOutcome == "abstain") {
        eVote = VOTE_OUTCOME_ABSTAIN;
    }
    return eVote;
}

vote_signal_enum_t CGovernanceVoting::ConvertVoteSignal(const std::string& strVoteSignal)
{
    static const std::map <std::string, vote_signal_enum_t> mapStrVoteSignals = {
        {"funding",  VOTE_SIGNAL_FUNDING},
        {"valid",    VOTE_SIGNAL_VALID},
        {"delete",   VOTE_SIGNAL_DELETE},
        {"endorsed", VOTE_SIGNAL_ENDORSED}
    };

    const auto& it = mapStrVoteSignals.find(strVoteSignal);
    if (it == mapStrVoteSignals.end()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceVoting::%s -- ERROR: Unknown signal %s\n", __func__, strVoteSignal);
        return VOTE_SIGNAL_NONE;
    }
    return it->second;
}

CGovernanceVote::CGovernanceVote()
    : fValid(true),
      fSynced(false),
      nVoteSignal(int(VOTE_SIGNAL_NONE)),
      masternodeOutpoint(),
      nParentHash(),
      nVoteOutcome(int(VOTE_OUTCOME_NONE)),
      nTime(0),
      vchSig()
{}

CGovernanceVote::CGovernanceVote(const COutPoint& outpointMasternodeIn, const uint256& nParentHashIn, vote_signal_enum_t eVoteSignalIn, vote_outcome_enum_t eVoteOutcomeIn)
    : fValid(true),
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
    if(!masternodeSync.IsSynced()) {
        LogPrint(BCLog::GOBJECT, "CGovernanceVote::Relay -- won't relay until fully synced\n");
        return;
    }

    CInv inv(MSG_GOVERNANCE_OBJECT_VOTE, GetHash());
    connman.RelayInv(inv, MIN_GOVERNANCE_PEER_PROTO_VERSION);
}

void CGovernanceVote::UpdateHash() const
{
    // Note: doesn't match serialization

    CHashWriter ss(SER_GETHASH, MIN_PEER_PROTO_VERSION);
    ss << masternodeOutpoint;
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

bool CGovernanceVote::Sign(const CKey& keyMasternode, const CPubKey& pubKeyMasternode)
{
    std::string strError;

    if (sporkManager.IsSporkActive(SPORK_6_NEW_SIGS)) {
        uint256 hash = GetSignatureHash();

        if(!CHashSigner::SignHash(hash, keyMasternode, vchSig)) {
            LogPrint(BCLog::GOBJECT, "CGovernanceVote::Sign -- SignHash() failed\n");
            return false;
        }

        if (!CHashSigner::VerifyHash(hash, pubKeyMasternode, vchSig, strError)) {
            LogPrint(BCLog::GOBJECT, "CGovernanceVote::Sign -- VerifyHash() failed, error: %s\n", strError);
            return false;
        }
    } 

    return true;
}

bool CGovernanceVote::CheckSignature(const CPubKey& pubKeyMasternode) const
{
    std::string strError;

    if (sporkManager.IsSporkActive(SPORK_6_NEW_SIGS)) {
        uint256 hash = GetSignatureHash();
        if (!CHashSigner::VerifyHash(hash, pubKeyMasternode, vchSig, strError)) {
            LogPrint(BCLog::GOBJECT, "CGovernanceVote::IsValid -- VerifyMessage() failed, error: %s\n", strError);
            return false;  
        }
    } 
    return true;
}

bool CGovernanceVote::IsValid(bool fSignatureCheck) const
{
    if(nTime > GetAdjustedTime() + (60*60)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceVote::IsValid -- vote is too far ahead of current time - %s - nTime %lli - Max Time %lli\n", GetHash().ToString(), nTime, GetAdjustedTime() + (60*60));
        return false;
    }

    // support up to MAX_SUPPORTED_VOTE_SIGNAL, can be extended
    if(nVoteSignal > MAX_SUPPORTED_VOTE_SIGNAL)
    {
        LogPrint(BCLog::GOBJECT, "CGovernanceVote::IsValid -- Client attempted to vote on invalid signal(%d) - %s\n", nVoteSignal, GetHash().ToString());
        return false;
    }

    // 0=none, 1=yes, 2=no, 3=abstain. Beyond that reject votes
    if(nVoteOutcome > 3)
    {
        LogPrint(BCLog::GOBJECT, "CGovernanceVote::IsValid -- Client attempted to vote on invalid outcome(%d) - %s\n", nVoteSignal, GetHash().ToString());
        return false;
    }

    masternode_info_t infoMn;
    if(!mnodeman.GetMasternodeInfo(masternodeOutpoint, infoMn)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceVote::IsValid -- Unknown Masternode - %s\n", masternodeOutpoint.ToStringShort());
        return false;
    }

    if(!fSignatureCheck) return true;

    return CheckSignature(infoMn.pubKeyMasternode);
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
    if(!fResult) {
        return false;
    }
    fResult = (vote1.masternodeOutpoint == vote2.masternodeOutpoint);

    fResult = fResult && (vote1.nParentHash < vote2.nParentHash);
    if(!fResult) {
        return false;
    }
    fResult = fResult && (vote1.nParentHash == vote2.nParentHash);

    fResult = fResult && (vote1.nVoteOutcome < vote2.nVoteOutcome);
    if(!fResult) {
        return false;
    }
    fResult = fResult && (vote1.nVoteOutcome == vote2.nVoteOutcome);

    fResult = fResult && (vote1.nVoteSignal == vote2.nVoteSignal);
    if(!fResult) {
        return false;
    }
    fResult = fResult && (vote1.nVoteSignal == vote2.nVoteSignal);

    fResult = fResult && (vote1.nTime < vote2.nTime);

    return fResult;
}
