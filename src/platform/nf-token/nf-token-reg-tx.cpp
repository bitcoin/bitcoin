// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "nf-token-reg-tx.h"
#include "primitives/transaction.h"
#include "platform/specialtx.h"
#include "platform/nf-token/nf-tokens-manager.h"

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

    bool NfTokenRegTx::CheckTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
    {
        AssertLockHeld(cs_main);

        NfTokenRegTx nfTokenRegTx;
        if (!GetTxPayload(tx, nfTokenRegTx))
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

        const NfToken & nfToken = nfTokenRegTx.GetNfToken();

        if (nfTokenRegTx.m_version != NfTokenRegTx::CURRENT_VERSION)
            return state.DoS(100, false, REJECT_INVALID, "bad-token-reg-tx-version");

        if (nfToken.tokenId.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-token-reg-tx-token");

        if (nfToken.tokenOwnerKeyId.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-token-reg-tx-owner-key-null");

        if (nfToken.metadataAdminKeyId.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-token-reg-tx-metadata-admin-key-null");

        //TODO: Validate metadata

        if (pindexPrev != nullptr)
        {
            if (NfTokensManager::Instance().ContainsAtHeight(nfToken.tokenProtocolId, nfToken.tokenId, pindexPrev->nHeight))
                return state.DoS(10, false, REJECT_DUPLICATE, "bad-token-reg-tx-dup-token");
        }

        return CheckNfTokenTxSignature(tx, nfTokenRegTx, nfTokenRegTx.GetNfToken().tokenOwnerKeyId, state);
    }

    bool NfTokenRegTx::ProcessSpecialTx(const CTransaction &tx, const CBlockIndex *pindex, CValidationState &state)
    {
        NfTokenRegTx nfTokenRegTx;
        bool result = GetTxPayload(tx, nfTokenRegTx);
        // should have been checked already
        assert(result);

        if (!result)
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

        if (!NfTokensManager::Instance().AddNfToken(nfTokenRegTx.GetNfToken(), tx, pindex))
            return state.DoS(100, false, REJECT_DUPLICATE/*REJECT_CONFLICT*/, "token-reg-tx-conflit");
    }
}
