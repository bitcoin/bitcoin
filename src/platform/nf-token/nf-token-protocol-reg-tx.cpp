// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "nf-token-protocol-reg-tx.h"
#include "primitives/transaction.h"
#include "platform/platform-utils.h"
#include "platform/specialtx.h"
#include "nft-protocols-manager.h"
#include "nf-tokens-manager.h"

#include "sync.h"
#include "main.h"

namespace Platform
{
    bool NfTokenProtocolRegTx::CheckTx(const CTransaction& tx, const CBlockIndex* pindexLast, CValidationState& state)
    {
        AssertLockHeld(cs_main);

        NfTokenProtocolRegTx nftProtoRegTx;
        if (!GetTxPayload(tx, nftProtoRegTx))
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

        const NfTokenProtocol & nftProto = nftProtoRegTx.GetNftProto();

        if (nftProtoRegTx.m_version != NfTokenProtocolRegTx::CURRENT_VERSION)
            return state.DoS(100, false, REJECT_INVALID, "bad-nft-proto-reg-tx-version");

        if (nftProto.tokenProtocolId == NfToken::UNKNOWN_TOKEN_PROTOCOL)
            return state.DoS(10, false, REJECT_INVALID, "bad-nft-proto-reg-tx-token-protocol");

        if (nftProto.tokenProtocolOwnerId.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-nft-proto-reg-tx-owner-key-null");

        if (nftProto.nftRegSign < static_cast<uint8_t>(NftRegSignMin) || nftProto.nftRegSign > static_cast<uint8_t>(NftRegSignMax))
            return state.DoS(10, false, REJECT_INVALID, "bad-nft-proto-reg-tx-reg-sign");

        if (nftProto.tokenProtocolName.size() < NfTokenProtocol::TOKEN_PROTOCOL_NAME_MIN
        || nftProto.tokenProtocolName.size() > NfTokenProtocol::TOKEN_PROTOCOL_NAME_MAX)
            return state.DoS(10, false, REJECT_INVALID, "bad-nft-proto-reg-tx-proto-name");

        if (nftProto.tokenMetadataMimeType.size() > NfTokenProtocol::TOKEN_METADATA_MIMETYPE_MAX)
            return state.DoS(10, false, REJECT_INVALID, "bad-nft-proto-reg-tx-metadata-mime-type");

        if (nftProto.tokenMetadataSchemaUri.size() > NfTokenProtocol::TOKEN_METADATA_SCHEMA_URI_MAX)
            return state.DoS(10, false, REJECT_INVALID, "bad-nft-proto-reg-tx-metadata-schema-uri");

        if (nftProto.maxMetadataSize > NfTokenProtocol::TOKEN_METADATA_ABSOLUTE_MAX)
            return state.DoS(10, false, REJECT_INVALID, "bad-nft-proto-reg-tx-metadata-max-size-too-big");

        if (pindexLast != nullptr)
        {
            if (NftProtocolsManager::Instance().Contains(nftProto.tokenProtocolId, pindexLast->nHeight))
                return state.DoS(10, false, REJECT_DUPLICATE, "bad-nft-proto-reg-tx-dup-protocol");
        }

        if (!CheckInputsHashAndSig(tx, nftProtoRegTx, nftProto.tokenProtocolOwnerId, state))
            return state.DoS(50, false, REJECT_INVALID, "bad-nft-proto-reg-tx-invalid-signature");

        return true;
    }

    bool NfTokenProtocolRegTx::ProcessTx(const CTransaction & tx, const CBlockIndex * pindex, CValidationState & state)
    {
        NfTokenProtocolRegTx nftProtoRegTx;
        bool result = GetTxPayload(tx, nftProtoRegTx);
        // should have been checked already
        assert(result);

        auto nftProto = nftProtoRegTx.GetNftProto();

        if (!NftProtocolsManager::Instance().AddNftProto(nftProto, tx, pindex))
            return state.DoS(100, false, REJECT_DUPLICATE/*TODO: REJECT_CONFLICT*/, "nft-proto-reg-tx-conflict");
        NfTokensManager::Instance().OnNewProtocolRegistered(nftProto.tokenProtocolId);
        return true;
    }

    bool NfTokenProtocolRegTx::UndoTx(const CTransaction & tx, const CBlockIndex * pindex)
    {
        NfTokenProtocolRegTx nftProtoRegTx;
        bool result = GetTxPayload(tx, nftProtoRegTx);
        // should have been checked already
        assert(result);

        auto nftProto = nftProtoRegTx.GetNftProto();
        return NftProtocolsManager::Instance().Delete(nftProto.tokenProtocolId, pindex->nHeight);
    }

    void NfTokenProtocolRegTx::ToJson(json_spirit::Object & result) const
    {
        result.push_back(json_spirit::Pair("version", m_version));
        result.push_back(json_spirit::Pair("nftProtocolId", ProtocolName{m_nfTokenProtocol.tokenProtocolId}.ToString()));
        result.push_back(json_spirit::Pair("tokenProtocolName", m_nfTokenProtocol.tokenProtocolName));
        result.push_back(json_spirit::Pair("tokenMetadataSchemaUri", m_nfTokenProtocol.tokenMetadataSchemaUri));
        result.push_back(json_spirit::Pair("tokenMetadataMimeType", m_nfTokenProtocol.tokenMetadataMimeType));
        result.push_back(json_spirit::Pair("isTokenTransferable", m_nfTokenProtocol.isTokenTransferable));
        result.push_back(json_spirit::Pair("isMetadataEmbedded", m_nfTokenProtocol.isMetadataEmbedded));
        auto nftRegSignStr = NftRegSignToString(static_cast<NftRegSign>(m_nfTokenProtocol.nftRegSign));
        result.push_back(json_spirit::Pair("nftRegSign", nftRegSignStr));
        result.push_back(json_spirit::Pair("maxMetadataSize", m_nfTokenProtocol.maxMetadataSize));
        result.push_back(json_spirit::Pair("tokenProtocolOwnerId", CBitcoinAddress(m_nfTokenProtocol.tokenProtocolOwnerId).ToString()));
    }


    std::string NfTokenProtocolRegTx::ToString() const
    {
        std::ostringstream out;
        out << "NfTokenProtocolRegTx(nft protocol ID=" << ProtocolName{m_nfTokenProtocol.tokenProtocolId}.ToString()
        << ", nft protocol name=" << m_nfTokenProtocol.tokenProtocolName << ", nft metadata schema url=" << m_nfTokenProtocol.tokenMetadataSchemaUri
        << ", nft metadata mimetype=" << m_nfTokenProtocol.tokenMetadataMimeType << ", transferable=" << m_nfTokenProtocol.isTokenTransferable
        << ", is metadata embedded" << m_nfTokenProtocol.isMetadataEmbedded << ", max metadata size=" << m_nfTokenProtocol.maxMetadataSize
        << ", nft Protocol owner ID=" << CBitcoinAddress(m_nfTokenProtocol.tokenProtocolOwnerId).ToString() << ")";
        return out.str();
    }
}
