// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKEN_PROTOCOL_REG_TX_H
#define CROWN_PLATFORM_NF_TOKEN_PROTOCOL_REG_TX_H

#include "key.h"
#include "serialize.h"
#include "nf-token-protocol.h"

class CTransaction;
class CMutableTransaction;
class CBlockIndex;
class CValidationState;

namespace Platform
{
    class NfTokenProtocolRegTx //TODO: create common nf token tx interface
    {
    public:
        NfTokenProtocolRegTx()
        {}

        explicit NfTokenProtocolRegTx(NfTokenProtocol nfTokenProtocol)
            : m_nfTokenProtocol(std::move(nfTokenProtocol))
        {}

        bool Sign(CKey & privKey, CPubKey & pubKey);

        std::string ToString() const;

        static bool CheckTx(const CTransaction& tx, const CBlockIndex* pIndex, CValidationState& state);

    public:
        ADD_SERIALIZE_METHODS
        template<typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
        {
            READWRITE(m_version);
            READWRITE(m_nfTokenProtocol);
            READWRITE(signature);
        }

    public:
        static const int CURRENT_VERSION = 1;
        std::vector<unsigned char> signature; // TODO: temp public to conform the template signing function

    private:
        uint16_t m_version{CURRENT_VERSION};
        NfTokenProtocol m_nfTokenProtocol;
        // TODO: std::vector<unsigned char> m_signature;
    };
}

#endif // CROWN_PLATFORM_NF_TOKEN_PROTOCOL_REG_TX_H
