// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKEN_REG_TX_BUILDER_H
#define CROWN_PLATFORM_NF_TOKEN_REG_TX_BUILDER_H

#include "rpcserver.h"
#include "platform/rpc/specialtx-rpc-utils.h"
#include "nf-token-reg-tx.h"

namespace Platform
{
    class NfTokenRegTxBuilder
    {
    public:
        NfTokenRegTxBuilder & SetTokenProtocol(const json_spirit::Value & tokenProtocolName)
        {
            //TODO: convert base 32 string to the token type name/id
            //m_nfToken.tokenProtocolName;
            return *this;
        }

        NfTokenRegTxBuilder & SetTokenId(const json_spirit::Value & tokenIdHexValue)
        {
            m_nfToken.tokenId = ParseHashV(tokenIdHexValue.get_str(), "nfTokenId");
            return *this;
        }

        NfTokenRegTxBuilder & SetTokenOwnerKey(const json_spirit::Value & tokenOwnerKeyOrAddress, CKey & ownerPrivKey)
        {
            ownerPrivKey = ParsePrivKeyOrAddress(tokenOwnerKeyOrAddress.get_str());
            m_nfToken.tokenOwnerKeyId = ownerPrivKey.GetPubKey().GetID();
            return *this;
        }

        NfTokenRegTxBuilder & SetMetadataAdminKey(const json_spirit::Value & metadataAdminAddress)
        {
            if (!metadataAdminAddress.get_str().empty() && metadataAdminAddress.get_str() != "0")
            {
                m_nfToken.metadataAdminKeyId = ParsePubKeyIDFromAddress(metadataAdminAddress.get_str(), "nfTokenMetadataAdminAddr");
            }
            return *this;
        }

        NfTokenRegTxBuilder & SetMetadata(const json_spirit::Value & metadata)
        {
            //TODO: convert to vector, if binary then also convert from hex
            //m_nfToken.metadata = metadata.get_array();
            return *this;
        }

        NfTokenRegTx BuildTx()
        {
            if (m_nfToken.metadataAdminKeyId.IsNull())
            {
                m_nfToken.metadataAdminKeyId = m_nfToken.tokenOwnerKeyId;
            }
            NfTokenRegTx regTx(std::move(m_nfToken));
            //regTx.Sign(m_ownerKey, m_ownerPubKey);
            return regTx;
        }

    private:
        NfToken m_nfToken;
        //CKey m_ownerKey;
        //CPubKey m_ownerPubKey;
    };
}

#endif // CROWN_PLATFORM_NF_TOKEN_REG_TX_BUILDER_H
