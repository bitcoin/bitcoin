// Copyright (c) 2014-2018 Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "governance-vote.h"
#include "agent.h"
#include "specialtx.h"

#include "sync.h"
#include "main.h"

namespace Platform
{
    bool CheckVoteTx(const CTransaction& tx, const CBlockIndex* pindex, CValidationState& state)
    {
        AssertLockHeld(cs_main);

        VoteTx vtx;
        if (!GetTxPayload(tx, vtx))
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

        auto vote = vtx.GetVote();

        CMasternode* pmn = mnodeman.Find(vote.VoterId());
        if(pmn == nullptr)
            return state.DoS(10, false, REJECT_INVALID, "bad-vote-tx-no-masternode");

        if (!vote.Verify(pmn->pubkey2))
            return state.DoS(50, false, REJECT_INVALID, "bad-vote-tx-invalid-signature");

        return true;
    }

    bool ProcessVoteTx(const CTransaction& tx, const CBlockIndex* pindex, CValidationState& state)
    {
        try
        {
            VoteTx vtx;
            GetTxPayload(tx, vtx);

            AgentsVoting().AcceptVote(vtx.GetVote());

            return true;
        }
        catch (const std::exception& )
        {
            return state.DoS(1, false, REJECT_INVALID, "vote-tx-process-fail");
        }

    }
}
