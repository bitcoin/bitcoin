// Copyright (c) 2014-2018 Crown Core developers
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
        /// NF token protocol unique name/identifier, can be represented as a base32 string, 12 symbols max
        uint64_t tokenProtocolName{UKNOWN_TOKEN_PROTOCOL};

        /// NF token unique identifier, can be a sha256 hash, tokenProtocolName + tokenId must be globally unique
        uint256 tokenId;

        /// Public key id of the owner of the nf-token
        CKeyID tokenOwnerKeyId;

        /// Public key id of th metadata operator, may be null
        CKeyID metadataAdminKeyId;

        /// Metadata buffer
        std::vector<unsigned char> metadata;

        static const uint64_t UKNOWN_TOKEN_PROTOCOL = -1;

    public:
        ADD_SERIALIZE_METHODS
        template<typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
        {
            READWRITE(tokenProtocolName);
            READWRITE(tokenId);
            READWRITE(tokenOwnerKeyId);
            READWRITE(metadataAdminKeyId);
            READWRITE(metadata);
        }
    };
}

#endif // CROWN_PLATFORM_NF_TOKEN_H
