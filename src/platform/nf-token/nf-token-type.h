// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKEN_TYPE_H
#define CROWN_PLATFORM_NF_TOKEN_TYPE_H

#include "uint256.h"
#include "pubkey.h"


namespace Platform
{
    class NfTokenType
    {
    public:
        uint32_t tokenTypeId;

        std::string tokenTypeName;
        std::string tokenTypeSymbol;
        std::string tokenMetadtaSchemaUrl;
        std::string tokenMetadataMimeType;

        bool isTokenTransferrable;
        bool isTokenImmutable;

        CKeyID tokenProtocolOwnerId;
        std::vector<unsigned char> signature;
    };

    /* Examples:
    tokenTypeId = 0;
    tokenTypeName = nftoken-global;
    tokenTypeSymbol = nfg;
    tokenMetadtaSchemaUrl = null;
    tokenMetadataMimeType = null;
    tokenImmutable = false;
    tokenTransferrable = false;

    tokenTypeId = 1
    tokenTypeName = erc721-cryptokitties
    tokenTypeSymbol = cks
    tokenMetadtaSchemaUrl = https://erc721-json-schema-url
    tokenMetadataMimeType = text/uri-list
    tokenImmutable = false;
    tokenTransferrable = false;

    tokenTypeId = 2;
    tokenTypeName = crown-id;
    tokenTypeSymbol = crd;
    tokenMetadataSchemaUrl = https://binary-schema;
    tokenMetadataMimeType = application/x-binary;
    tokenImmutable = false;
    tokenTransferrable = false;

    tokenTypeId = 3;
    tokenTypeName = notary;
    tokenTypeSymbol = ntr;
    tokenMetadtaSchemaUrl = https://document-desciption-schema;
    tokenMetadataMimeType = text/plain;
    tokenImmutable = true;
    tokenTransferrable = true; */
}

#endif // CROWN_PLATFORM_NF_TOKEN_TYPE_H
