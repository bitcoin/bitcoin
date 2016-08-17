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



std::string CGovernanceVoting::ConvertOutcomeToString(int nOutcome)
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

std::string CGovernanceVoting::ConvertSignalToString(int nSignal)
{
    switch(nSignal)
    {
        case VOTE_SIGNAL_NONE:
            return "NONE"; break;
        case VOTE_SIGNAL_FUNDING:
            return "FUNDING"; break;
        case VOTE_SIGNAL_VALID:
            return "VALID"; break;
        case VOTE_SIGNAL_DELETE:
            return "DELETE"; break;
        case VOTE_SIGNAL_ENDORSED:
            return "ENDORSED"; break;
        case VOTE_SIGNAL_NOOP1:
            return "NOOP1"; break;
        case VOTE_SIGNAL_NOOP2:
            return "NOOP2"; break;
        case VOTE_SIGNAL_NOOP3:
            return "NOOP3"; break;
        case VOTE_SIGNAL_NOOP4:
            return "NOOP4"; break;
        case VOTE_SIGNAL_NOOP5:
            return "NOOP5"; break;
        case VOTE_SIGNAL_NOOP6:
            return "NOOP6"; break;
        case VOTE_SIGNAL_NOOP7:
            return "NOOP7"; break;
        case VOTE_SIGNAL_NOOP8:
            return "NOOP8"; break;
        case VOTE_SIGNAL_NOOP9:
            return "NOOP9"; break;
        case VOTE_SIGNAL_NOOP10:
            return "NOOP10"; break;
        case VOTE_SIGNAL_NOOP11:
            return "NOOP11"; break;
        case VOTE_SIGNAL_CUSTOM_START:
            return "CUSTOM_START"; break;
        case VOTE_SIGNAL_CUSTOM_END:
            return "CUSTOM_END"; break;
    }

    return "error";
}


int CGovernanceVoting::ConvertVoteOutcome(std::string strVoteOutcome)
{
    int nVote = -1;
    if(strVoteOutcome == "yes") nVote = VOTE_OUTCOME_YES;
    if(strVoteOutcome == "no") nVote = VOTE_OUTCOME_NO;
    if(strVoteOutcome == "abstain") nVote = VOTE_OUTCOME_ABSTAIN;
    if(strVoteOutcome == "none") nVote = VOTE_OUTCOME_NONE;
    return nVote;
}

int CGovernanceVoting::ConvertVoteSignal(std::string strVoteSignal)
{
    if(strVoteSignal == "none") return VOTE_SIGNAL_NONE;         // 0
    if(strVoteSignal == "funding") return VOTE_SIGNAL_FUNDING;   // 1
    if(strVoteSignal == "valid") return VOTE_SIGNAL_VALID;       // 2
    if(strVoteSignal == "delete") return VOTE_SIGNAL_DELETE;     // 3
    if(strVoteSignal == "endorsed") return VOTE_SIGNAL_ENDORSED; // 4

    // ID FIVE THROUGH CUSTOM_START ARE TO BE USED BY GOVERNANCE ENGINE / TRIGGER SYSTEM

    // convert custom sentinel outcomes to integer and store
    try {
        int  i = boost::lexical_cast<int>(strVoteSignal);
        if(i < VOTE_SIGNAL_CUSTOM_START || i > VOTE_SIGNAL_CUSTOM_END) return -1;
        return i;
    }
    catch(std::exception const & e)
    {
         cout<<"error : " << e.what() <<endl;
    }

    return -1;
}

CGovernanceVote::CGovernanceVote()
{
    vinMasternode = CTxIn();
    nParentHash = uint256();
    nVoteSignal = VOTE_SIGNAL_NONE;
    nVoteOutcome = VOTE_OUTCOME_NONE;
    nTime = 0;
    fValid = true;
    fSynced = false;
    vchSig.clear();
}

CGovernanceVote::CGovernanceVote(CTxIn vinMasternodeIn, uint256 nParentHashIn, int nVoteSignalIn, int nVoteOutcomeIn)
{
    vinMasternode = vinMasternodeIn;
    nParentHash = nParentHashIn;
    nVoteSignal = nVoteSignalIn;
    nVoteOutcome = nVoteOutcomeIn;
    nTime = GetAdjustedTime();
    fValid = true;
    fSynced = false;
}

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

    std::string errorMessage;
    std::string strMessage = vinMasternode.prevout.ToStringShort() + "|" + nParentHash.ToString() + "|" +
        boost::lexical_cast<std::string>(nVoteSignal) + "|" + boost::lexical_cast<std::string>(nVoteOutcome) + "|" + boost::lexical_cast<std::string>(nTime);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode)) {
        LogPrintf("CGovernanceVote::Sign - Error upon calling SignMessage");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrintf("CGovernanceVote::Sign - Error upon calling VerifyMessage");
        return false;
    }

    return true;
}

bool CGovernanceVote::IsValid(bool fSignatureCheck)
{
    if(nTime > GetTime() + (60*60)){
        LogPrint("gobject", "CGovernanceVote::IsValid() - vote is too far ahead of current time - %s - nTime %lli - Max Time %lli\n", GetHash().ToString(), nTime, GetTime() + (60*60));
        return false;
    }

    // support up to 50 actions (implemented in sentinel)
    if(nVoteSignal > 50)
    {
        LogPrint("gobject", "CGovernanceVote::IsValid() - Client attempted to vote on invalid action(%d) - %s\n", nVoteSignal, GetHash().ToString());
        return false;
    }

    // 0=none, 1=yes, 2=no, 3=abstain. Beyond that reject votes
    if(nVoteOutcome > 3)
    {
        LogPrint("gobject", "CGovernanceVote::IsValid() - Client attempted to vote on invalid outcome(%d) - %s\n", nVoteSignal, GetHash().ToString());
        return false;   
    }

    CMasternode* pmn = mnodeman.Find(vinMasternode);
    if(pmn == NULL)
    {
        LogPrint("gobject", "CGovernanceVote::IsValid() - Unknown Masternode - %s\n", vinMasternode.ToString());
        return false;
    }

    if(!fSignatureCheck) return true;

    std::string errorMessage;
    std::string strMessage = vinMasternode.prevout.ToStringShort() + "|" + nParentHash.ToString() + "|" +
        boost::lexical_cast<std::string>(nVoteSignal) + "|" + boost::lexical_cast<std::string>(nVoteOutcome) + "|" + boost::lexical_cast<std::string>(nTime);

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage)) {
        LogPrintf("CGovernanceVote::IsValid() - Verify message failed - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}
