// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core_io.h"
#include "main.h"
#include "init.h"

#include "flat-database.h"
#include "governance.h"
#include "masternode.h"
#include "governance.h"
#include "darksend.h"
#include "masternodeman.h"
#include "masternode-sync.h"
#include "util.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>



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
    string strReturn = "NONE";
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
        case VOTE_SIGNAL_NOOP1:
            strReturn = "NOOP1";
            break;
        case VOTE_SIGNAL_NOOP2:
            strReturn = "NOOP2";
            break;
        case VOTE_SIGNAL_NOOP3:
            strReturn = "NOOP3";
            break;
        case VOTE_SIGNAL_NOOP4:
            strReturn = "NOOP4";
            break;
        case VOTE_SIGNAL_NOOP5:
            strReturn = "NOOP5";
            break;
        case VOTE_SIGNAL_NOOP6:
            strReturn = "NOOP6";
            break;
        case VOTE_SIGNAL_NOOP7:
            strReturn = "NOOP7";
            break;
        case VOTE_SIGNAL_NOOP8:
            strReturn = "NOOP8";
            break;
        case VOTE_SIGNAL_NOOP9:
            strReturn = "NOOP9";
            break;
        case VOTE_SIGNAL_NOOP10:
            strReturn = "NOOP10";
            break;
        case VOTE_SIGNAL_NOOP11:
            strReturn = "NOOP11";
            break;
        case VOTE_SIGNAL_CUSTOM1:
            strReturn = "CUSTOM1";
            break;
        case VOTE_SIGNAL_CUSTOM2:
            strReturn = "CUSTOM2";
            break;
        case VOTE_SIGNAL_CUSTOM3:
            strReturn = "CUSTOM3";
            break;
        case VOTE_SIGNAL_CUSTOM4:
            strReturn = "CUSTOM4";
            break;
        case VOTE_SIGNAL_CUSTOM5:
            strReturn = "CUSTOM5";
            break;
        case VOTE_SIGNAL_CUSTOM6:
            strReturn = "CUSTOM6";
            break;
        case VOTE_SIGNAL_CUSTOM7:
            strReturn = "CUSTOM7";
            break;
        case VOTE_SIGNAL_CUSTOM8:
            strReturn = "CUSTOM8";
            break;
        case VOTE_SIGNAL_CUSTOM9:
            strReturn = "CUSTOM9";
            break;
        case VOTE_SIGNAL_CUSTOM10:
            strReturn = "CUSTOM10";
            break;
        case VOTE_SIGNAL_CUSTOM11:
            strReturn = "CUSTOM11";
            break;
        case VOTE_SIGNAL_CUSTOM12:
            strReturn = "CUSTOM12";
            break;
        case VOTE_SIGNAL_CUSTOM13:
            strReturn = "CUSTOM13";
            break;
        case VOTE_SIGNAL_CUSTOM14:
            strReturn = "CUSTOM14";
            break;
        case VOTE_SIGNAL_CUSTOM15:
            strReturn = "CUSTOM15";
            break;
        case VOTE_SIGNAL_CUSTOM16:
            strReturn = "CUSTOM16";
            break;
        case VOTE_SIGNAL_CUSTOM17:
            strReturn = "CUSTOM17";
            break;
        case VOTE_SIGNAL_CUSTOM18:
            strReturn = "CUSTOM18";
            break;
        case VOTE_SIGNAL_CUSTOM19:
            strReturn = "CUSTOM19";
            break;
        case VOTE_SIGNAL_CUSTOM20:
            strReturn = "CUSTOM20";
            break;
    }

    return strReturn;
}


vote_outcome_enum_t CGovernanceVoting::ConvertVoteOutcome(std::string strVoteOutcome)
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

vote_signal_enum_t CGovernanceVoting::ConvertVoteSignal(std::string strVoteSignal)
{
    vote_signal_enum_t eSignal = VOTE_SIGNAL_NONE;
    if(strVoteSignal == "funding") {
        eSignal = VOTE_SIGNAL_FUNDING;
    }
    else if(strVoteSignal == "valid") {
        eSignal = VOTE_SIGNAL_VALID;
    }
    if(strVoteSignal == "delete") {
        eSignal = VOTE_SIGNAL_DELETE;
    }
    if(strVoteSignal == "endorsed") {
        eSignal = VOTE_SIGNAL_ENDORSED;
    }

    if(eSignal != VOTE_SIGNAL_NONE)  {
        return eSignal;
    }

    // ID FIVE THROUGH CUSTOM_START ARE TO BE USED BY GOVERNANCE ENGINE / TRIGGER SYSTEM

    // convert custom sentinel outcomes to integer and store
    try {
        int i = boost::lexical_cast<int>(strVoteSignal);
        if(i < VOTE_SIGNAL_CUSTOM1 || i > VOTE_SIGNAL_CUSTOM20) {
            eSignal = VOTE_SIGNAL_NONE;
        }
        else  {
            eSignal = vote_signal_enum_t(i);
        }
    }
    catch(std::exception const & e)
    {
        std::ostringstream ostr;
        ostr << "CGovernanceVote::ConvertVoteSignal: error : " << e.what() << std::endl;
        LogPrintf(ostr.str().c_str());
    }

    return eSignal;
}

CGovernanceVote::CGovernanceVote()
    : fValid(true),
      fSynced(false),
      nVoteSignal(int(VOTE_SIGNAL_NONE)),
      vinMasternode(),
      nParentHash(),
      nVoteOutcome(int(VOTE_OUTCOME_NONE)),
      nTime(0),
      vchSig()
{}

CGovernanceVote::CGovernanceVote(CTxIn vinMasternodeIn, uint256 nParentHashIn, vote_signal_enum_t eVoteSignalIn, vote_outcome_enum_t eVoteOutcomeIn)
    : fValid(true),
      fSynced(false),
      nVoteSignal(eVoteSignalIn),
      vinMasternode(vinMasternodeIn),
      nParentHash(nParentHashIn),
      nVoteOutcome(eVoteOutcomeIn),
      nTime(GetAdjustedTime()),
      vchSig()
{}

void CGovernanceVote::Relay()
{
    CInv inv(MSG_GOVERNANCE_OBJECT_VOTE, GetHash());
    RelayInv(inv, MSG_GOVERNANCE_PEER_PROTO_VERSION);
}

bool CGovernanceVote::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    // Choose coins to use
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;

    std::string strError;
    std::string strMessage = vinMasternode.prevout.ToStringShort() + "|" + nParentHash.ToString() + "|" +
        boost::lexical_cast<std::string>(nVoteSignal) + "|" + boost::lexical_cast<std::string>(nVoteOutcome) + "|" + boost::lexical_cast<std::string>(nTime);

    if(!darkSendSigner.SignMessage(strMessage, vchSig, keyMasternode)) {
        LogPrintf("CGovernanceVote::Sign -- SignMessage() failed\n");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, strError)) {
        LogPrintf("CGovernanceVote::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool CGovernanceVote::IsValid(bool fSignatureCheck)
{
    if(nTime > GetTime() + (60*60)){
        LogPrint("gobject", "CGovernanceVote::IsValid -- vote is too far ahead of current time - %s - nTime %lli - Max Time %lli\n", GetHash().ToString(), nTime, GetTime() + (60*60));
        return false;
    }

    // support up to 50 actions (implemented in sentinel)
    if(nVoteSignal > 50)
    {
        LogPrint("gobject", "CGovernanceVote::IsValid -- Client attempted to vote on invalid signal(%d) - %s\n", nVoteSignal, GetHash().ToString());
        return false;
    }

    // 0=none, 1=yes, 2=no, 3=abstain. Beyond that reject votes
    if(nVoteOutcome > 3)
    {
        LogPrint("gobject", "CGovernanceVote::IsValid -- Client attempted to vote on invalid outcome(%d) - %s\n", nVoteSignal, GetHash().ToString());
        return false;
    }

    CMasternode* pmn = mnodeman.Find(vinMasternode);
    if(pmn == NULL)
    {
        LogPrint("gobject", "CGovernanceVote::IsValid -- Unknown Masternode - %s\n", vinMasternode.prevout.ToStringShort());
        return false;
    }

    if(!fSignatureCheck) return true;

    std::string strError;
    std::string strMessage = vinMasternode.prevout.ToStringShort() + "|" + nParentHash.ToString() + "|" +
        boost::lexical_cast<std::string>(nVoteSignal) + "|" + boost::lexical_cast<std::string>(nVoteOutcome) + "|" + boost::lexical_cast<std::string>(nTime);

    if(!darkSendSigner.VerifyMessage(pmn->pubKeyMasternode, vchSig, strMessage, strError)) {
        LogPrintf("CGovernanceVote::IsValid -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}
