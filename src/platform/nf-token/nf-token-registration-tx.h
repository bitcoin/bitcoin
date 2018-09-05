// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKEN_REGISTRATION_TX_H
#define CROWN_PLATFORM_NF_TOKEN_REGISTRATION_TX_H

#include "serialize.h"
#include "uint256.h"
#include "pubkey.h"

class CTransaction;
class CBlockIndex;
class CValidationState;

namespace Platform
{
    class NfTokenRegistrationTx
    {
    public:
        static const int CURRENT_VERSION = 1;

    public:
        uint16_t version;
        uint256 token;
        CKeyID tokenOwnerKeyId;
        CKeyID metadataAdminKeyId; // if metadata key id is null, the onwer key is used instead
        std::vector<unsigned char> metadata;
        std::vector<unsigned char> signature; // owner signature

    public:
        ADD_SERIALIZE_METHODS

        template<typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
        {
            READWRITE(version);
            READWRITE(token);
            READWRITE(tokenOwnerKeyId);
            READWRITE(metadataAdminKeyId);
            READWRITE(metadata);
            READWRITE(signature);
        }

        std::string ToString() const;
    };

    bool CheckNfTokenRegistrationTx(const CTransaction& tx, const CBlockIndex* pIndex, CValidationState& state);
}


#endif // CROWN_PLATFORM_NF_TOKEN_REGISTRATION_TX_H
