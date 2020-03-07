// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKEN_H
#define CROWN_PLATFORM_NF_TOKEN_H

#include "serialize.h"
#include "uint256.h"
#include "pubkey.h"

namespace Platform
{
    class NfToken
    {
    public:
        /// NF token protocol unique symbol/identifier, can be represented as a base32 string, 12 symbols max
        uint64_t tokenProtocolId{UNKNOWN_TOKEN_PROTOCOL};

        /// NF token unique identifier, can be a sha256 hash, tokenProtocolId + tokenId must be globally unique
        uint256 tokenId;

        /// Public key id of the owner of the nf-token
        CKeyID tokenOwnerKeyId;

        /// Public key id of the metadata operator, may be null
        CKeyID metadataAdminKeyId;

        /// Metadata buffer
        std::vector<unsigned char> metadata;

        static const uint64_t UNKNOWN_TOKEN_PROTOCOL = static_cast<uint64_t >(-1);

    public:
        ADD_SERIALIZE_METHODS
        template<typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
        {
            //TODO: maybe optimize writing on disk of token-id and protocol-id since it's a key in the db (or reg-tx-hash depending on the key)
            READWRITE(tokenProtocolId);
            READWRITE(tokenId);
            READWRITE(tokenOwnerKeyId);
            READWRITE(metadataAdminKeyId);
            READWRITE(metadata);
        }
    };
}

#endif // CROWN_PLATFORM_NF_TOKEN_H
