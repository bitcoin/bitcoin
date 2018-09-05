// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "nf-token-registration-tx.h"
#include "primitives/transaction.h"
#include "specialtx.h"

#include "sync.h"
#include "main.h"

namespace Platform
{

    template <typename NfTokenTx>
    static bool CheckNfTokenTxSignature(const CTransaction &tx, const NfTokenTx& nfTokenTx, const CKeyID& keyId, CValidationState& state)
    {
        //uint256 inputsHash = CalcTxInputsHash(tx);
        //if (inputsHash != proTx.inputsHash)
        //    return state.DoS(100, false, REJECT_INVALID, "bad-protx-inputs-hash");

        std::string strError;
        if (!CHashSigner::VerifyHash(::SerializeHash(nfTokenTx), keyId, nfTokenTx.signature, strError))
            return state.DoS(100, false, REJECT_INVALID, "bad-token-reg-tx-sig", false, strError);

        return true;
    }

    bool CheckNfTokenRegistrationTx(const CTransaction& tx, const CBlockIndex* pIndex, CValidationState& state)
    {
        AssertLockHeld(cs_main);

        NfTokenRegistrationTx nfTokenRegTx;
        if (!GetTxPayload(tx, tokenRegTX))
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

        if (nfTokenRegTx.version != NfTokenRegistrationTx::CURRENT_VERSION)
            return stage.DoS(100, false, REJECT_INVALID, "bad-token-reg-tx-version");

        if (nfTokenRegTx.token.IsNull())
            return stage.DoS(10, false, REJECT_INVALID, "bad-token-reg-tx-token");

        if (nfTokenRegTx.tokenOwnerKeyId.IsNull())
            return stage.DoS(10, false, REJECT_INVALID, "bad-token-reg-tx-owner-key-null");

        return CheckNfTokenTxSignature(tx, nfTokenRegTx, nfTokenRegTx.tokenOwnerKeyId, state);
    }
}
