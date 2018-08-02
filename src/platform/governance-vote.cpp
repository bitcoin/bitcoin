// Copyright (c) 2014-2018 Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "governance-vote.h"
#include "specialtx.h"

#include "sync.h"
#include "main.h"

bool CheckVoteTx(const CTransaction& tx, const CBlockIndex* pindex, CValidationState& state)
{
    AssertLockHeld(cs_main);

    VoteTx vtx;
    if (!GetTxPayload(tx, vtx))
        return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

    return true;
}