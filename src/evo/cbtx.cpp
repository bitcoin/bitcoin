// Copyright (c) 2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cbtx.h"
#include "specialtx.h"
#include "deterministicmns.h"

#include "validation.h"
#include "univalue.h"

bool CheckCbTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    AssertLockHeld(cs_main);

    if (!tx.IsCoinBase())
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-invalid");

    CCbTx cbTx;
    if (!GetTxPayload(tx, cbTx))
        return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

    if (cbTx.nVersion > CCbTx::CURRENT_VERSION)
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-version");

    if (pindexPrev) {
        if (pindexPrev->nHeight + 1 != cbTx.nHeight)
            return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-height");
    }

    return true;
}

std::string CCbTx::ToString() const
{
    return strprintf("CCbTx(nHeight=%d, nVersion=%d, merkleRootMNList=%s)",
        nVersion, nHeight, merkleRootMNList.ToString());
}

void CCbTx::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.push_back(Pair("version", (int)nVersion));
    obj.push_back(Pair("height", (int)nHeight));
    obj.push_back(Pair("merkleRootMNList", merkleRootMNList.ToString()));
}
