// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NFT_PROTOCOL_REG_TX_BUILDER_H
#define CROWN_PLATFORM_NFT_PROTOCOL_REG_TX_BUILDER_H

#include "utilstrencodings.h"
#include "rpcserver.h"
#include "rpcprotocol.h"
#include "platform/platform-utils.h"
#include "platform/rpc/specialtx-rpc-utils.h"
#include "nf-token-protocol-reg-tx.h"
#include "nf-token.h"

namespace Platform
{
    class NftProtocolRegTxBuilder
    {
    public:
        NftProtocolRegTxBuilder & SetTokenProtocol(const json_spirit::Value & tokenProtocolId)
        {
            auto nftProtoStr = tokenProtocolId.get_str();
            if (nftProtoStr.size() < NfTokenProtocol::TOKEN_PROTOCOL_ID_MIN || nftProtoStr.size() > NfTokenProtocol::TOKEN_PROTOCOL_ID_MAX)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "NFT protocol ID must be between 3 and 12 symbols long");
            m_nftProto.tokenProtocolId = StringToProtocolName(nftProtoStr.c_str());
            if (m_nftProto.tokenProtocolId == NfToken::UNKNOWN_TOKEN_PROTOCOL)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "NFT protocol ID contains invalid characters");
            return *this;
        }

        NftProtocolRegTxBuilder & SetTokenProtocolName(const json_spirit::Value & tokenProtocolName)
        {
            m_nftProto.tokenProtocolName = tokenProtocolName.get_str();
            if (m_nftProto.tokenProtocolName.size() < NfTokenProtocol::TOKEN_PROTOCOL_NAME_MIN
            || m_nftProto.tokenProtocolName.size() > NfTokenProtocol::TOKEN_PROTOCOL_NAME_MAX)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "NFT Protocol name must be between 3 and 24 symbols long");
            return *this;
        }

        NftProtocolRegTxBuilder & SetTokenProtocolOwnerKey(const json_spirit::Value & nftProtoOwnerAddress, CKey & ownerPrivKey)
        {
            ownerPrivKey = PullPrivKeyFromWallet(nftProtoOwnerAddress.get_str(), "nftProtoOwnerAddress");
            m_nftProto.tokenProtocolOwnerId = ownerPrivKey.GetPubKey().GetID();
            return *this;
        }

        NftProtocolRegTxBuilder & SetMetadataSchemaUri(const json_spirit::Value & schemaUri)
        {
            m_nftProto.tokenMetadataSchemaUri = schemaUri.get_str();
            if (m_nftProto.tokenMetadataSchemaUri.size() > NfTokenProtocol::TOKEN_METADATA_SCHEMA_URI_MAX)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "metadata schema URI is longer than permitted");
            return *this;
        }

        NftProtocolRegTxBuilder & SetMetadataMimeType(const json_spirit::Value & mimeType)
        {
            m_nftProto.tokenMetadataMimeType = mimeType.get_str();
            if (m_nftProto.tokenMetadataMimeType.size() > NfTokenProtocol::TOKEN_METADATA_MIMETYPE_MAX)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "metadata mimetype is longer than permitted");
            return *this;
        }

        NftProtocolRegTxBuilder & SetNftRegSign(const json_spirit::Value & nftRegSign)
        {
            m_nftProto.nftRegSign = ParseInt32V(nftRegSign, "nftRegSign");
            if (m_nftProto.nftRegSign < static_cast<uint8_t>(NftRegSignMin) || m_nftProto.nftRegSign > static_cast<uint8_t>(NftRegSignMax))
            {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Unsupported NftRegSign type");
            }
            return *this;
        }

        NftProtocolRegTxBuilder & SetIsTokenTransferable(const json_spirit::Value & value)
        {
            m_nftProto.isTokenTransferable = ParseBoolV(value, "isTokenTransferable");
            return *this;
        }

        NftProtocolRegTxBuilder & SetIsMetadataEmbedded(const json_spirit::Value & value)
        {
            m_nftProto.isMetadataEmbedded = ParseBoolV(value, "isMetadataEmbedded");
            return *this;
        }

        NftProtocolRegTxBuilder & SetMaxMetadataSize(const json_spirit::Value & value)
        {
            m_nftProto.maxMetadataSize = ParseUInt8V(value, "maxMetadataSize");
            return *this;
        }

        NfTokenProtocolRegTx BuildTx()
        {
            NfTokenProtocolRegTx regTx(std::move(m_nftProto));
            return regTx;
        }

    private:
        NfTokenProtocol m_nftProto;
    };
}

#endif // CROWN_PLATFORM_NFT_PROTOCOL_REG_TX_BUILDER_H
