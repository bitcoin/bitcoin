// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "nf-token-registration-tx.h"
#include "primitives/transaction.h"
#include "platform/specialtx.h"

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

    bool NfTokenRegistrationTx::CheckTx(const CTransaction& tx, const CBlockIndex* pIndex, CValidationState& state)
    {
        AssertLockHeld(cs_main);

        NfTokenRegistrationTx nfTokenRegTx;
        if (!GetTxPayload(tx, nfTokenRegTx))
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

        if (nfTokenRegTx.m_version != NfTokenRegistrationTx::CURRENT_VERSION)
            return state.DoS(100, false, REJECT_INVALID, "bad-token-reg-tx-version");

        if (nfTokenRegTx.m_nfToken.tokenId.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-token-reg-tx-token");

        if (nfTokenRegTx.m_nfToken.tokenOwnerKeyId.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-token-reg-tx-owner-key-null");

        if (nfTokenRegTx.m_nfToken.metadataAdminKeyId.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-token-reg-tx-metadata-admin-key-null");

        return CheckNfTokenTxSignature(tx, nfTokenRegTx, nfTokenRegTx.m_nfToken.tokenOwnerKeyId, state);
    }
}
