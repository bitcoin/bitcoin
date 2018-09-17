// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKEN_H
#define CROWN_PLATFORM_NF_TOKEN_H

#include "uint256.h"
#include "pubkey.h"

namespace Platform
{
    class NfToken
    {
    public:
        uint32_t tokenTypeId;                 // nf-token protocol unique identifier
        uint256 tokenId;                      // nf-token unique identifier, can be a sha256 hash, tokenProtocolId + tokenId must be globally unique
        CKeyID tokenOwnerKeyId;               // public key id of the owner of the nf-token
        CKeyID metadataAdminKeyId;            // public key id of th metadata operator, may be null
        std::vector<unsigned char> metadata;  // metadata buffer
    };
}

#endif // CROWN_PLATFORM_NF_TOKEN_H
