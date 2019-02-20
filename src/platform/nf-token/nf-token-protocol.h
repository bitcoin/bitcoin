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
    class NfTokenProtocol
    {
    public:
        /// NF token protocol unique symbol/identifier, can be an a abbreviated name describing this NF token type
        /// Represented as a base32 string, can only contain characters: .abcdefghijklmnopqrstuvwxyz12345
        /// Minimum length 3 symbols, maximum length 12 symbols
        uint64_t tokenProtocolId;

        /// Full name for this NF token type/protocol
        /// Minimum length 3 symbols, maximum length 24(TODO:?) symbols
        /// Characters TODO:
        std::string tokenProtocolName;

        /// URI to schema (json/xml/binary) describing metadata format
        std::string tokenMetadataSchemaUri;

        /// MIME type describing metadata content type
        std::string tokenMetadataMimeType;

        /// Defines if this NF token type can be transferred
        bool isTokenTransferable;

        /// Defines if this NF token id can be changed during token lifetime
        bool isTokenImmutable;

        /// Owner of the NF token protocol
        CKeyID tokenProtocolOwnerId;

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

    tokenProtocolId = cks;
    tokenProtocolName = ercS21.kitts;
    tokenMetadataSchemaUri = https://erc721-json-schema-url;
    tokenMetadataMimeType = text/uri-list;
    isTokenImmutable = false;
    isTokenTransferable = false;

    tokenProtocolId = crd;
    tokenProtocolName = crown.id;
    tokenMetadataSchemaUri = https://binary-schema;
    tokenMetadataMimeType = application/x-binary;
    isTokenImmutable = false;
    isTokenTransferable = false;

    tokenProtocolSymbol = doc;
    tokenProtocolName = docproof;
    tokenMetadataSchemaUri = https://document-desciption-schema;
    tokenMetadataMimeType = text/plain;
    isTokenImmutable = true;
    isTokenTransferable = true; */
}

#endif // CROWN_PLATFORM_NF_TOKEN_PROTOCOL_H
