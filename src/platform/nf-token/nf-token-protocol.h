// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKEN_PROTOCOL_H
#define CROWN_PLATFORM_NF_TOKEN_PROTOCOL_H

#include "serialize.h"
#include "uint256.h"
#include "pubkey.h"

namespace Platform
{
    enum class NftRegSign : uint8_t
    {
        SelfSign = 1,
        SignByCreator = 2,
        SignAny = 3
    };

    class NfTokenProtocol
    {
    public:
        /// NF token protocol unique symbol/identifier, can be an a abbreviated name describing this NF token type
        /// Represented as a base32 string, can only contain characters: .abcdefghijklmnopqrstuvwxyz12345
        /// Minimum length 3 symbols, maximum length 12 symbols
        uint64_t tokenProtocolId;

        /// Full name for this NF token type/protocol
        /// Minimum length 3 symbols, maximum length 24 symbols
        std::string tokenProtocolName;

        /// URI to schema (json/xml/binary) describing metadata format
        std::string tokenMetadataSchemaUri;

        /// MIME type describing metadata content type
        std::string tokenMetadataMimeType;

        /// Defines if this NF token type can be transferred
        bool isTokenTransferable;

        /// Defines if this NF token id can be changed during token lifetime
        bool isTokenImmutable;

        /// Defines if metadata is embedded or contains a URI
        /// It's recommended to use embedded metadata only if it's shorter than URI
        bool isMetadataEmbedded;

        /// Defines who must sign an NFT registration transaction
        uint8_t nftRegSign;

        /// Owner of the NF token protocol
        CKeyID tokenProtocolOwnerId;
        //TODO: add admin key to the protocol structure. add option to use setup admin key rights including tranfering ownership

    public:
        ADD_SERIALIZE_METHODS
        template<typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
        {
            READWRITE(tokenProtocolId);
            READWRITE(tokenProtocolName);
            READWRITE(tokenMetadataSchemaUri);
            READWRITE(tokenMetadataMimeType);
            READWRITE(isTokenTransferable);
            READWRITE(isTokenImmutable);
            READWRITE(isMetadataEmbedded);
            READWRITE(nftRegSign);
            READWRITE(tokenProtocolOwnerId);
        }
    };

    /* Examples:
    tokenProtocolId = nfg;
    tokenProtocolName = nftoken.glbl;
    tokenMetadataSchemaUri = null;
    tokenMetadataMimeType = null;
    isTokenImmutable = false;
    isTokenTransferable = false;
    isMetadataEmbedded = false;
    nftRegSign = NftRegSign::SignAny;

    tokenProtocolId = cks;
    tokenProtocolName = ercS21.kitts;
    tokenMetadataSchemaUri = https://erc721-json-schema-url;
    tokenMetadataMimeType = text/json;
    isTokenImmutable = false;
    isTokenTransferable = false;
    isMetadataEmbedded = false;
    nftRegSign = NftRegSign::SignByCreator;

    tokenProtocolId = crd;
    tokenProtocolName = crown.id;
    tokenMetadataSchemaUri = https://binary-schema;
    tokenMetadataMimeType = application/x-binary;
    isTokenImmutable = false;
    isTokenTransferable = false;
    isMetadataEmbedded = false;
    nftRegSign = NftRegSign::SelfSign;

    tokenProtocolSymbol = doc;
    tokenProtocolName = docproof;
    tokenMetadataSchemaUri = https://document-desciption-schema;
    tokenMetadataMimeType = text/plain;
    isTokenImmutable = true;
    isTokenTransferable = true;
    isMetadataEmbedded = true;
    nftRegSign = NftRegSign::SignAny; */
}

#endif // CROWN_PLATFORM_NF_TOKEN_PROTOCOL_H
