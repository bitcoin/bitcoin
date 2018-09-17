// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKEN_REGISTRATION_TX_BUILDER_H
#define CROWN_PLATFORM_NF_TOKEN_REGISTRATION_TX_BUILDER_H

#include "rpcserver.h"
#include "platform/rpc/specialtx-rpc-utils.h"
#include "nf-token-registration-tx.h"

namespace Platform
{
    class NfTokenRegistrationTxBuilder
    {
    public:
        NfTokenRegistrationTxBuilder & SetTokenType(const json_spirit::Value & tokenTypeSymbol)
        {
            // convert string to the token type id
            m_nfToken.tokenTypeId = -1; //TODO: use as a common token type for now
            return *this;
        }

        NfTokenRegistrationTxBuilder & SetTokenId(const json_spirit::Value & tokenIdHexValue)
        {
            m_nfToken.tokenId = ParseHashV(tokenIdHexValue.get_str(), "nfTokenId");
            return *this;
        }

        NfTokenRegistrationTxBuilder & SetTokenOwnerKey(const json_spirit::Value & tokenOwnerKeyOrAddress, CKey & ownerPrivKey)
        {
            ownerPrivKey = ParsePrivKeyOrAddress(tokenOwnerKeyOrAddress.get_str());
            m_nfToken.tokenOwnerKeyId = ownerPrivKey.GetPubKey().GetID();
            return *this;
        }

        NfTokenRegistrationTxBuilder & SetMetadataAdminKey(const json_spirit::Value & metadataAdminAddress)
        {
            if (!metadataAdminAddress.get_str().empty() && metadataAdminAddress.get_str() != "0")
            {
                m_nfToken.metadataAdminKeyId = ParsePubKeyIDFromAddress(metadataAdminAddress.get_str(), "nfTokenMetadataAdminAddr");
            }
            return *this;
        }

        NfTokenRegistrationTxBuilder & SetMetadata(const json_spirit::Value & metadata)
        {
            //TODO: convert to vector, if binary then also convert from hex
            //m_nfToken.metadata = metadata.get_array();
            return *this;
        }

        NfTokenRegistrationTx BuildTx()
        {
            if (m_nfToken.metadataAdminKeyId.IsNull())
            {
                m_nfToken.metadataAdminKeyId = m_nfToken.tokenOwnerKeyId;
            }
            NfTokenRegistrationTx regTx(m_nfToken);
            //regTx.Sign(m_ownerKey, m_ownerPubKey);
            return regTx;
        }

    private:
        NfToken m_nfToken;
        //CKey m_ownerKey;
        //CPubKey m_ownerPubKey;
    };
}

#endif // CROWN_PLATFORM_NF_TOKEN_REGISTRATION_TX_BUILDER_H
