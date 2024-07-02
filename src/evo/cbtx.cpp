// Copyright (c) 2017-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/cbtx.h>
#include <evo/deterministicmns.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_commitment.h>
#include <evo/specialtx.h>

#include <chainparams.h>
#include <consensus/merkle.h>
#include <univalue.h>
#include <validation.h>
#include <util/time.h>
#include <logging.h>
bool CheckCbTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, bool fJustCheck)
{
    if (tx.nVersion != SYSCOIN_TX_VERSION_MN_COINBASE) {
        return FormatSyscoinErrorMessage(state, "bad-cbtx-type", fJustCheck);
    }

    if (!tx.IsCoinBase()) {
        return FormatSyscoinErrorMessage(state, "bad-cbtx-invalid", fJustCheck);
    }

    CCbTx cbTx;
    if (!GetTxPayload(tx, cbTx)) {
        return FormatSyscoinErrorMessage(state, "bad-cbtx-payload", fJustCheck);
    }

    if (cbTx.nVersion == 0 || cbTx.nVersion > CCbTx::CURRENT_VERSION) {
        return FormatSyscoinErrorMessage(state, "bad-cbtx-version", fJustCheck);
    }

    if (pindexPrev && pindexPrev->nHeight + 1 != cbTx.nHeight) {
        return FormatSyscoinErrorMessage(state, "bad-cbtx-height", fJustCheck);
    }

    if (pindexPrev) {
        if (cbTx.nVersion < 2) {
            return FormatSyscoinErrorMessage(state, "bad-cbtx-version", fJustCheck);
        }
    }

    return true;
}
bool CheckCbTx(const CCbTx &cbTx, const CBlockIndex* pindexPrev, TxValidationState& state, bool fJustCheck)
{
    if (cbTx.nVersion == 0 || cbTx.nVersion > CCbTx::CURRENT_VERSION) {
        return FormatSyscoinErrorMessage(state, "bad-cbtx-version", fJustCheck);
    }

    if (pindexPrev && pindexPrev->nHeight + 1 != cbTx.nHeight) {
        return FormatSyscoinErrorMessage(state, "bad-cbtx-height", fJustCheck);
    }

    if (pindexPrev) {
        if (cbTx.nVersion < 2) {
            return FormatSyscoinErrorMessage(state, "bad-cbtx-version", fJustCheck);
        }
    }

    return true;
}

std::string CCbTx::ToString() const
{
    return strprintf("CCbTx(nVersion=%d, nHeight=%d)",
        nVersion, nHeight);
}
