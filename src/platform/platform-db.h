// Copyright (c) 2014-2019 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_DB_H
#define CROWN_PLATFORM_DB_H

#include "uint256.h"
#include "leveldbwrapper.h"
#include "sync.h"
#include "platform/nf-token/nf-token-index.h"

namespace Platform
{
    enum class PlatformOpt
    {
        OptSpeed,
        OptRam
    };

    class PlatformDb
    {
    public:
        static PlatformDb & CreateInstance(size_t nCacheSize, bool fMemory = false, bool fWipe = false)
        {
            if (s_instance == nullptr)
                s_instance.reset(new PlatformDb(nCacheSize, fMemory, fWipe));
            return *s_instance;
        }

        static void DestroyInstance()
        {
            s_instance.reset();
        }

        static PlatformDb & Instance()
        {
            return *s_instance;
        }

        static NfTokenIndex NftDiskIndexToNftMemIndex(const NfTokenDiskIndex &nftDiskIndex);

    public:
        std::unique_ptr<CScopedDBTransaction> BeginTransaction()
        {
            LOCK(m_cs);
            auto t = CScopedDBTransaction::Begin(m_dbTransaction);
            return t;
        }

        template<typename K, typename V>
        bool Read(const K& key, V& value)
        {
            LOCK(m_cs);
            return m_dbTransaction.Read(key, value);
        }


        template<typename K, typename V>
        void Write(const K& key, const V& value)
        {
            LOCK(m_cs);
            m_dbTransaction.Write(key, value);
        }

        template <typename K>
        bool Exists(const K& key)
        {
            LOCK(m_cs);
            return m_dbTransaction.Exists(key);
        }

        template <typename K>
        void Erase(const K& key)
        {
            LOCK(m_cs);
            m_dbTransaction.Erase(key);
        }

        CLevelDBWrapper& GetRawDB() { return m_db; }

        bool OptimizeRam() const { return m_optSetting == PlatformOpt::OptRam; }
        bool OptimizeSpeed() const { return m_optSetting == PlatformOpt::OptSpeed; }

        bool ProcessNftIndexGuts(std::function<bool(NfTokenIndex)> nftIndexHandler);

        void WriteNftDiskIndex(const NfTokenDiskIndex & nftDiskIndex);
        void EraseNftDiskIndex(const uint64_t &protocolId, const uint256 &tokenId);
        NfTokenIndex ReadNftIndex(const uint64_t &protocolId, const uint256 &tokenId);

    private:
        explicit PlatformDb(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    public:
        static const char DB_NFT;

    private:
        CCriticalSection m_cs;
        CLevelDBWrapper m_db;
        CDBTransaction m_dbTransaction;
        PlatformOpt m_optSetting = PlatformOpt::OptSpeed;

        static std::unique_ptr<PlatformDb> s_instance;
    };
}

#endif //CROWN_PLATFORM_DB_H
