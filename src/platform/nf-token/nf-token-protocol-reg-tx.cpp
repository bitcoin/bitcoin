// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "nf-token-protocol-reg-tx.h"
#include "primitives/transaction.h"
#include "platform/specialtx.h"

#include "sync.h"
#include "main.h"

namespace Platform
{
    bool NfTokenProtocolRegTx::CheckTx(const CTransaction& tx, const CBlockIndex* pIndex, CValidationState& state)
    {
        AssertLockHeld(cs_main);

        NfTokenProtocolRegTx nfTokenProtoRegTx;
        if (!GetTxPayload(tx, nfTokenProtoRegTx))
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

        return true;
    }
}
