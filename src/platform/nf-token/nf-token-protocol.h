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
        /// NF token protocol unique name/identifier, should be a meaningful name descrbing this NF token type
        /// Represented as a base32 string, can only contain characters: .abcdefghijklmnopqrstuvwxyz12345
        /// Minimum length 6(TODO:?) symbols, maximum length 12 symbols
        /// The minimum length requirement comes from the Token/Token Protocol Capture Attack (working title)
        uint64_t tokenProtocolName;

        /// Abbreviated name for this NF token type/protocol
        /// Minimum length 3 symbols, maximum length 6/12(TODO:?) symbols
        std::string tokenProtocolSymbol;

        /// Optional full NF token type name, maximum length is 260(TODO:?) symbols
        std::string tokenProtocolLongName;

        /// URI to schema (json/xml/binary) descibing metadata format
        std::string tokenMetadataSchemaUri;

        /// MIME type describing metadata content type
        std::string tokenMetadataMimeType;

        /// Defines if this NF token type can be transferred
        bool isTokenTransferrable;

        /// Defines if this NF token id can be changed during token lifetime
        bool isTokenImmutable;

        /// Owner of the NF token protocol
        CKeyID tokenProtocolOwnerId;

    public:
        ADD_SERIALIZE_METHODS
        template<typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
        {
            READWRITE(tokenProtocolName);
            READWRITE(tokenProtocolSymbol);
            READWRITE(tokenProtocolLongName);
            READWRITE(tokenMetadataSchemaUri);
            READWRITE(tokenMetadataMimeType);
            READWRITE(isTokenTransferrable);
            READWRITE(isTokenImmutable);
            READWRITE(tokenProtocolOwnerId);
        }
    };

    /* Examples:
    tokenProtocolName = nftoken.glbl;
    tokenProtocolSymbol = nfg;
    tokenProtocolLongName = nftoken-global;
    tokenMetadataSchemaUri = null;
    tokenMetadataMimeType = null;
    tokenImmutable = false;
    tokenTransferrable = false;

    tokenProtocolName = ercS21.kitts;
    tokenProtocolSymbol = cks;
    tokenProtocolLongName = erc721-cryptokitties;
    tokenMetadataSchemaUri = https://erc721-json-schema-url;
    tokenMetadataMimeType = text/uri-list;
    tokenImmutable = false;
    tokenTransferrable = false;

    tokenProtocolName = crown.id;
    tokenProtocolSymbol = crd;
    tokenProtocolLongName = crown-id;
    tokenMetadataSchemaUri = https://binary-schema;
    tokenMetadataMimeType = application/x-binary;
    tokenImmutable = false;
    tokenTransferrable = false;

    tokenProtocolName = docproof;
    tokenProtocolSymbol = doc;
    tokenProtocolLongName = Document Proofs;
    tokenMetadataSchemaUri = https://document-desciption-schema;
    tokenMetadataMimeType = text/plain;
    tokenImmutable = true;
    tokenTransferrable = true; */
}

#endif // CROWN_PLATFORM_NF_TOKEN_PROTOCOL_H
