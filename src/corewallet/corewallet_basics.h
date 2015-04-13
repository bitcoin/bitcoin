// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COREWALLET_COREWALLET_BASICS_H
#define BITCOIN_COREWALLET_COREWALLET_BASICS_H

#include "pubkey.h"
#include "serialize.h"

#include <stdint.h>

// basic/common corewallet classes

namespace CoreWallet
{
    class CAddressBookMetadata
    {
    public:
        static const int CURRENT_VERSION=1;
        int nVersion;
        std::string label;
        std::string purpose;
        
        CAddressBookMetadata()
        {
            SetNull();
        }

        ADD_SERIALIZE_METHODS;
        
        template <typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
            READWRITE(this->nVersion);
            nVersion = this->nVersion;
            READWRITE(label);
            READWRITE(purpose);
        }
        
        void SetNull()
        {
            nVersion = CURRENT_VERSION;
        }
    };
    
    class CKeyMetadata
    {
    public:
        static const int CURRENT_VERSION=1;
        int nVersion;
        int64_t nCreateTime; // 0 means unknown
        std::string label;
        std::string purpose;
        
        // BIP32 metadata.
        CKeyID keyidParent;
        uint32_t nDerivationIndex;
        int nDepth;
        
        CKeyMetadata()
        {
            SetNull();
        }
        CKeyMetadata(int64_t nCreateTime_)
        {
            nVersion = CKeyMetadata::CURRENT_VERSION;
            nCreateTime = nCreateTime_;
        }
        
        ADD_SERIALIZE_METHODS;
        
        template <typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
            READWRITE(this->nVersion);
            nVersion = this->nVersion;
            READWRITE(nCreateTime);
            READWRITE(label);
            READWRITE(purpose);
            READWRITE(keyidParent);
            READWRITE(nDerivationIndex);
            READWRITE(nDepth);
        }
        
        void SetNull()
        {
            nVersion = CKeyMetadata::CURRENT_VERSION;
            nCreateTime = 0;
            keyidParent = CKeyID();
            nDerivationIndex = 0;
            nDepth = 0;
        }
    };
}; // end namespace CoreWallet

#endif