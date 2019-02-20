// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "nf-token-protocol-reg-tx.h"
#include "primitives/transaction.h"
#include "platform/platform-utils.h"
#include "platform/specialtx.h"

#include "sync.h"
#include "main.h"

namespace Platform
{
    bool NfTokenProtocolRegTx::CheckTx(const CTransaction& tx, const CBlockIndex* pndexPrev, CValidationState& state)
    {
        AssertLockHeld(cs_main);

        NfTokenProtocolRegTx nfTokenProtoRegTx;
        if (!GetTxPayload(tx, nfTokenProtoRegTx))
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

        return true;
    }

    std::string NfTokenProtocolRegTx::ToString() const
    {
        std::ostringstream out;
        out << "NfTokenProtocolRegTx(nft protocol ID=" << ProtocolName{m_nfTokenProtocol.tokenProtocolId}.ToString()
        << ", nft protocol name=" << m_nfTokenProtocol.tokenProtocolName << ", nft metadata schema url=" << m_nfTokenProtocol.tokenMetadataSchemaUri
        << ", nft metadata mimetype=" << m_nfTokenProtocol.tokenMetadataMimeType << ", transferable=" << m_nfTokenProtocol.isTokenTransferable
        << ", immutable=" << m_nfTokenProtocol.isTokenImmutable << ", nft owner ID=" << CBitcoinAddress(m_nfTokenProtocol.tokenProtocolOwnerId).ToString() << ")";
        return out.str();
    }
}
