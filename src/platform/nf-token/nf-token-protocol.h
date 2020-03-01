// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKEN_PROTOCOL_H
#define CROWN_PLATFORM_NF_TOKEN_PROTOCOL_H

#include "serialize.h"
#include "uint256.h"
#include "pubkey.h"

namespace Platform
{
    enum NftRegSign : uint8_t
    {
        SelfSign = 1,
        SignByCreator = 2,
        SignPayer = 3,
        NftRegSignMin = SelfSign,
        NftRegSignMax = SignPayer
    };

    std::string NftRegSignToString(NftRegSign nftRegSign);

    class NfTokenProtocol
    {
    public:
        /// NF token protocol unique symbol/identifier, can be an a abbreviated name describing this NF token type
        /// Represented as a base32 string, can only contain characters: .abcdefghijklmnopqrstuvwxyz12345
        /// Dots(.) are not allowed as a first and last characters
        /// Minimum length 3 symbols, maximum length 12 symbols
        uint64_t tokenProtocolId;

        /// Full name for this NF token type/protocol
        /// Minimum length 3 symbols, maximum length 24 symbols
        std::string tokenProtocolName;

        /// URI to schema (json/xml/binary) describing metadata format
        std::string tokenMetadataSchemaUri;

        /// MIME type describing metadata content type
        std::string tokenMetadataMimeType{"text/plain"};

        /// Defines if this NF token type can be transferred
        bool isTokenTransferable{true};

        /// Defines if metadata is embedded or contains a URI
        /// It's recommended to use embedded metadata only if it's shorter than URI
        bool isMetadataEmbedded{false};

        /// Defines who must sign an NFT registration transaction
        uint8_t nftRegSign{static_cast<uint8_t>(SignByCreator)};

        /// Defines the maximum size of the NFT metadata
        uint8_t maxMetadataSize{TOKEN_METADATA_ABSOLUTE_MAX};

        /// Owner of the NF token protocol
        CKeyID tokenProtocolOwnerId;
        //TODO: add admin key to the protocol structure. add option to use setup admin key rights including tranfering ownership

        static const unsigned TOKEN_PROTOCOL_ID_MIN = 3;
        static const unsigned TOKEN_PROTOCOL_ID_MAX = 12;
        static const unsigned TOKEN_PROTOCOL_NAME_MIN = 3;
        static const unsigned TOKEN_PROTOCOL_NAME_MAX = 24;
        static const unsigned TOKEN_METADATA_SCHEMA_URI_MAX = 128;
        static const unsigned TOKEN_METADATA_MIMETYPE_MAX = 32;
        static const unsigned TOKEN_METADATA_ABSOLUTE_MAX = 255;

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
            READWRITE(isMetadataEmbedded);
            READWRITE(nftRegSign);
            READWRITE(maxMetadataSize);
            READWRITE(tokenProtocolOwnerId);
        }
    };

    /* Examples:
    tokenProtocolId = nfg;
    tokenProtocolName = nftoken.glbl;
    tokenMetadataSchemaUri = null;
    tokenMetadataMimeType = null;
    isTokenTransferable = false;
    isMetadataEmbedded = false;
    nftRegSign = NftRegSign::SignPayer;

    tokenProtocolId = cks;
    tokenProtocolName = ercS21.kitts;
    tokenMetadataSchemaUri = https://erc721-json-schema-url;
    tokenMetadataMimeType = text/json;
    isTokenTransferable = false;
    isMetadataEmbedded = false;
    nftRegSign = NftRegSign::SignByCreator;

    tokenProtocolId = crd;
    tokenProtocolName = crown.id;
    tokenMetadataSchemaUri = https://binary-schema;
    tokenMetadataMimeType = application/x-binary;
    isTokenTransferable = false;
    isMetadataEmbedded = false;
    nftRegSign = NftRegSign::SelfSign;

    tokenProtocolSymbol = doc;
    tokenProtocolName = docproof;
    tokenMetadataSchemaUri = https://document-desciption-schema;
    tokenMetadataMimeType = text/plain;
    isTokenTransferable = true;
    isMetadataEmbedded = true;
    nftRegSign = NftRegSign::SignPayer; */
}

#endif // CROWN_PLATFORM_NF_TOKEN_PROTOCOL_H
