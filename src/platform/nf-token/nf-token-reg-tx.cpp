// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sync.h"
#include "main.h"

#include "primitives/transaction.h"
#include "platform/platform-utils.h"
#include "platform/specialtx.h"
#include "platform/rpc/specialtx-rpc-utils.h"
#include "nf-tokens-manager.h"
#include "nf-token-reg-tx.h"
#include "nft-protocols-manager.h"

namespace Platform
{
    bool NfTokenRegTx::CheckTx(const CTransaction& tx, const CBlockIndex* pindexLast, CValidationState& state)
    {
        AssertLockHeld(cs_main);

        NfTokenRegTx nfTokenRegTx;
        if (!GetTxPayload(tx, nfTokenRegTx))
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

        const NfToken & nfToken = nfTokenRegTx.GetNfToken();

        if (nfTokenRegTx.m_version != NfTokenRegTx::CURRENT_VERSION)
            return state.DoS(100, false, REJECT_INVALID, "bad-nf-token-reg-tx-version");

        bool containsProto;
        if (pindexLast != nullptr)
            containsProto = NftProtocolsManager::Instance().Contains(nfToken.tokenProtocolId, pindexLast->nHeight);
        else
            containsProto = NftProtocolsManager::Instance().Contains(nfToken.tokenProtocolId);

        if (!containsProto)
            return state.DoS(10, false, REJECT_INVALID, "bad-nf-token-reg-tx-unknown-token-protocol");

        auto nftProtoIndex = NftProtocolsManager::Instance().GetNftProtoIndex(nfToken.tokenProtocolId);

        if (pindexLast != nullptr)
        {
            int protoDepth = pindexLast->nHeight - nftProtoIndex.BlockIndex()->nHeight;
            if (protoDepth < TX_CONFIRMATIONS_NUM)
                return state.DoS(10, false, REJECT_INVALID, "bad-nf-token-reg-tx-nft-proto-immature");
        }

        CKeyID signerKeyId;
        switch (nftProtoIndex.NftProtoPtr()->nftRegSign)
        {
        case SignByCreator:
            signerKeyId = nftProtoIndex.NftProtoPtr()->tokenProtocolOwnerId;
            break;
        case SelfSign:
            signerKeyId = nfToken.tokenOwnerKeyId;
            break;
        case SignPayer:
        {
            if (!GetPayerPubKeyIdForNftTx(tx, signerKeyId))
            {
                return state.DoS(10, false, REJECT_INVALID, "bad-nf-token-reg-tx-cant-get-payer-key");
            }
            break;
        }
        default:
            return state.DoS(10, false, REJECT_INVALID, "bad-nf-token-reg-tx-unknown-nft-reg-sign");
        }

        if (nfToken.tokenId.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-nf-token-reg-tx-token");

        if (nfToken.tokenOwnerKeyId.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-nf-token-reg-tx-owner-key-null");

        if (nfToken.metadataAdminKeyId.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-nf-token-reg-tx-metadata-admin-key-null");

        if (nfToken.metadata.size() > nftProtoIndex.NftProtoPtr()->maxMetadataSize)
            return state.DoS(10, false, REJECT_INVALID, "bad-nf-token-reg-tx-metadata-is-too-long");

        if (pindexLast != nullptr)
        {
            if (NfTokensManager::Instance().Contains(nfToken.tokenProtocolId, nfToken.tokenId, pindexLast->nHeight))
                return state.DoS(10, false, REJECT_DUPLICATE, "bad-nf-token-reg-tx-dup-token");
        }

        if (!CheckInputsHashAndSig(tx, nfTokenRegTx, signerKeyId, state))
            return state.DoS(50, false, REJECT_INVALID, "bad-nf-token-reg-tx-invalid-signature");

        return true;
    }

    bool NfTokenRegTx::ProcessTx(const CTransaction &tx, const CBlockIndex *pindex, CValidationState &state)
    {
        NfTokenRegTx nfTokenRegTx;
        bool result = GetTxPayload(tx, nfTokenRegTx);
        // should have been checked already
        assert(result);

        auto nfToken = nfTokenRegTx.GetNfToken();

        if (!NfTokensManager::Instance().AddNfToken(nfToken, tx, pindex))
            return state.DoS(100, false, REJECT_DUPLICATE/*TODO: REJECT_CONFLICT*/, "token-reg-tx-conflict");
        return true;
    }

    bool NfTokenRegTx::UndoTx(const CTransaction& tx, const CBlockIndex * pindex)
    {
        NfTokenRegTx nfTokenRegTx;
        bool result = GetTxPayload(tx, nfTokenRegTx);
        // should have been checked already
        assert(result);

        auto nfToken = nfTokenRegTx.GetNfToken();
        return NfTokensManager::Instance().Delete(nfToken.tokenProtocolId, nfToken.tokenId, pindex->nHeight);
    }

    void NfTokenRegTx::ToJson(json_spirit::Object & result) const
    {
        result.push_back(json_spirit::Pair("version", m_version));
        result.push_back(json_spirit::Pair("nftProtocolId", ProtocolName{m_nfToken.tokenProtocolId}.ToString()));
        result.push_back(json_spirit::Pair("nftId", m_nfToken.tokenId.ToString()));
        result.push_back(json_spirit::Pair("nftOwnerKeyId", CBitcoinAddress(m_nfToken.tokenOwnerKeyId).ToString()));
        result.push_back(json_spirit::Pair("metadataAdminKeyId", CBitcoinAddress(m_nfToken.metadataAdminKeyId).ToString()));
        result.push_back(json_spirit::Pair("metadata", std::string(m_nfToken.metadata.begin(), m_nfToken.metadata.end())));
    }

    std::string NfTokenRegTx::ToString() const
    {
        std::ostringstream out;
        out << "NfTokenRegTx(version=" << m_version
            << ", NFT protocol ID=" << ProtocolName{m_nfToken.tokenProtocolId}.ToString()
            << ", NFT ID=" << m_nfToken.tokenId.ToString()
            << ", NFT owner address=" << CBitcoinAddress(m_nfToken.tokenOwnerKeyId).ToString()
            << ", metadata admin address=" << CBitcoinAddress(m_nfToken.metadataAdminKeyId).ToString()
            << ", metadata" << std::string(m_nfToken.metadata.begin(), m_nfToken.metadata.end()) << ")";
        return out.str();
    }
}
