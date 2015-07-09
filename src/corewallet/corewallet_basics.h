// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COREWALLET_COREWALLET_BASICS_H
#define BITCOIN_COREWALLET_COREWALLET_BASICS_H

#include "base58.h"
#include "pubkey.h"
#include "serialize.h"

#include <stdint.h>

// basic/common corewallet classes

namespace CoreWallet
{
    class Wallet;

    //keyvalue store (database/logdb) keys
    static const std::string kvs_keymetadata_key                = "keymeta";
    static const std::string kvs_key_key                        = "key";
    static const std::string kvs_hd_master_seed_key             = "hdmasterseed";
    static const std::string kvs_hd_hdchain_key                 = "hdchain"; //chain of keys metadata like m/44'/0'... (CHDChain)
    static const std::string kvs_hd_encrypted_master_seed_key   = "hdcryptedmasterseed";
    static const std::string kvs_hdpubkey_key                   = "hdpubkey"; //a pubkey with some hd metadata (CHDPubKey)
    static const std::string kvs_hdactivechain_key              = "hdactivechain"; //the current active hd chain of keys (uint256)
    static const std::string kvs_wtx_key                        = "tx";
    static const std::string kvs_address_book_metadata_key      = "adrmeta";


    enum CREDIT_DEBIT_TYPE
    {
        CREDIT_DEBIT_TYPE_AVAILABLE = 0,
        CREDIT_DEBIT_TYPE_UNCONFIRMED = 1,
        CREDIT_DEBIT_TYPE_IMMATURE = 2,
        CREDIT_DEBIT_TYPE_DEBIT = 3
    };

    class CAddressBookMetadata
    {
    public:
        static const int CURRENT_VERSION=1;

        int nVersion;
        std::string label;
        std::string purpose;
        std::map<std::string, std::string> destdata; //flexible key wallet store for the UI layer

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
            READWRITE(destdata);
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
        static const uint8_t KEY_ORIGIN_UNSET         = 0x0000;
        static const uint8_t KEY_ORIGIN_UNKNOWN       = 0x0001;
        static const uint8_t KEY_ORIGIN_IMPORTED      = 0x0002;
        static const uint8_t KEY_ORIGIN_UNENC_WALLET  = 0x0004;
        static const uint8_t KEY_ORIGIN_ENC_WALLET    = 0x0008;

        int nVersion;
        int64_t nCreateTime; // 0 means unknown
        uint8_t keyFlags;
        std::string label;
        std::string purpose;
        
        CKeyMetadata()
        {
            SetNull();
        }
        CKeyMetadata(int64_t nCreateTime_)
        {
            SetNull();
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
            READWRITE(keyFlags);
        }
        
        void SetNull()
        {
            nVersion = CKeyMetadata::CURRENT_VERSION;
            nCreateTime = 0;
            keyFlags = KEY_ORIGIN_UNSET;
        }

        std::string KeyflagsAsString() const
        {
            std::string keyFlagsStr;
            if (keyFlags & CKeyMetadata::KEY_ORIGIN_UNKNOWN)
                keyFlagsStr = "unknown";
            if (keyFlags & CKeyMetadata::KEY_ORIGIN_ENC_WALLET)
                keyFlagsStr = "created_in_encrypted_wallet";
            else if (keyFlags & CKeyMetadata::KEY_ORIGIN_UNENC_WALLET)
                keyFlagsStr = "created_in_unencrypted_wallet";
            if (keyFlags & CKeyMetadata::KEY_ORIGIN_IMPORTED)
                keyFlagsStr = "imported";

            return keyFlagsStr;
        }
    };

    struct CRecipient
    {
        CScript scriptPubKey;
        CAmount nAmount;
        bool fSubtractFeeFromAmount;
    };

    // WalletModel: a wallet metadata class
    class WalletModel
    {
    public:
        static const int CURRENT_VERSION=1;
        int nVersion;

        Wallet* pWallet; //no persistance
        std::string walletID; //only A-Za-z0-9._-
        std::string strWalletFilename;
        int64_t nCreateTime; // 0 means unknown

        WalletModel()
        {
            SetNull();
        }

        WalletModel(const std::string& filenameIn, Wallet *pWalletIn)
        {
            SetNull();

            strWalletFilename = filenameIn;
            pWallet = pWalletIn;
        }

        void SetNull()
        {
            nVersion = CURRENT_VERSION;
            nCreateTime = 0;
            pWallet = NULL;
        }

        ADD_SERIALIZE_METHODS;

        template <typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
            READWRITE(this->nVersion);
            nVersion = this->nVersion;
            READWRITE(nCreateTime);
            READWRITE(strWalletFilename);
        }
    };
}; // end namespace CoreWallet

#endif