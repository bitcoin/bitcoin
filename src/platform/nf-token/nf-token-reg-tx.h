// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKEN_REG_TX_H
#define CROWN_PLATFORM_NF_TOKEN_REG_TX_H

#include <json/json_spirit_value.h>
#include "key.h"
#include "serialize.h"
#include "nf-token.h"

class CTransaction;
class CMutableTransaction;
class CBlockIndex;
class CValidationState;

namespace Platform
{
    class NfTokenRegTx
    {
    public:
        NfTokenRegTx() = default;
        explicit NfTokenRegTx(NfToken nfToken)
            : m_nfToken(std::move(nfToken))
        {}

        inline const NfToken & GetNfToken() const
        {
            return m_nfToken;
        }

        // TODO: encapsulate signing
        // bool Sign(CKey & privKey, CPubKey & pubKey);

        std::string ToString() const;
        void ToJson(json_spirit::Object & result) const;

        static bool CheckTx(const CTransaction & tx, const CBlockIndex * pindexLast, CValidationState & state);
        static bool ProcessTx(const CTransaction & tx, const CBlockIndex * pindex, CValidationState & state);
        static bool UndoTx(const CTransaction & tx, const CBlockIndex * pindex);

    public:
        ADD_SERIALIZE_METHODS
        template<typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
        {
            READWRITE(m_version);
            READWRITE(m_nfToken);
            if (!(s.nType & SER_GETHASH))
            {
                READWRITE(signature);
            }
        }

    public:
        static const int CURRENT_VERSION = 1;
        std::vector<unsigned char> signature; // TODO: temp public to conform the template signing function

    private:
        uint16_t m_version{CURRENT_VERSION};
        NfToken m_nfToken;
        // TODO: std::vector<unsigned char> m_signature;

        static const int TX_CONFIRMATIONS_NUM = 6;
    };
}


#endif // CROWN_PLATFORM_NF_TOKEN_REG_TX_H
