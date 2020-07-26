// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_DB_H
#define CROWN_PLATFORM_DB_H

#include "uint256.h"
#include "leveldbwrapper.h"
#include "sync.h"
#include "platform/nf-token/nf-token-index.h"
#include "platform/nf-token/nf-token-protocol-index.h"

class BlockIndex;

namespace Platform
{
    enum class PlatformOpt
    {
        OptSpeed,
        OptRam
    };

    class PlatformDb : public TransactionLevelDBWrapper
    {
    public:
        static PlatformDb & CreateInstance(
                size_t nCacheSize,
                PlatformOpt optSetting = PlatformOpt::OptSpeed,
                bool fMemory = false,
                bool fWipe = false)
        {
            if (s_instance == nullptr)
                s_instance.reset(new PlatformDb(nCacheSize, optSetting, fMemory, fWipe));
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
        static NftProtoIndex NftProtoDiskIndexToNftProtoMemIndex(const NftProtoDiskIndex &protoDiskIndex);
        static BlockIndex * FindBlockIndex(const uint256 & blockHash);

        bool OptimizeRam() const { return m_optSetting == PlatformOpt::OptRam; }
        bool OptimizeSpeed() const { return m_optSetting == PlatformOpt::OptSpeed; }

        void ProcessPlatformDbGuts(std::function<bool(const leveldb::Iterator &)> processor);
        void ProcessNftIndexGutsOnly(std::function<bool(NfTokenIndex)> nftIndexHandler);
        void ProcessNftProtoIndexGutsOnly(std::function<bool(NftProtoIndex)> protoIndexHandler);
        bool ProcessNftIndex(const leveldb::Iterator & dbIt, std::function<bool(NfTokenIndex)> nftIndexHandler);
        bool ProcessNftProtoIndex(const leveldb::Iterator & dbIt, std::function<bool(NftProtoIndex)> protoIndexHandler);
        bool ProcessNftSupply(const leveldb::Iterator & dbIt, std::function<bool(uint64_t, std::size_t)> protoSupplyHandler);

        bool IsNftIndexEmpty();
        void WriteNftDiskIndex(const NfTokenDiskIndex & nftDiskIndex);
        void EraseNftDiskIndex(const uint64_t &protocolId, const uint256 &tokenId);
        NfTokenIndex ReadNftIndex(const uint64_t &protocolId, const uint256 &tokenId);

        void WriteTotalSupply(std::size_t count, uint64_t nftProtocolId = NfToken::UNKNOWN_TOKEN_PROTOCOL);
        bool ReadTotalSupply(std::size_t & count, uint64_t nftProtocolId = NfToken::UNKNOWN_TOKEN_PROTOCOL);

        void WriteNftProtoDiskIndex(const NftProtoDiskIndex & nftDiskIndex);
        void EraseNftProtoDiskIndex(const uint64_t &protocolId);
        NftProtoIndex ReadNftProtoIndex(const uint64_t &protocolId);

        void WriteTotalProtocolCount(std::size_t count);
        bool ReadTotalProtocolCount(std::size_t & count);
        bool CleanupDb();
        void Reindex();

    private:
        explicit PlatformDb(
                size_t nCacheSize,
                PlatformOpt optSetting = PlatformOpt::OptSpeed,
                bool fMemory = false,
                bool fWipe = false
                );

    public:
        static const char DB_NFT;
        static const char DB_NFT_TOTAL;
        static const char DB_NFT_PROTO;
        static const char DB_NFT_PROTO_TOTAL;

    private:
        PlatformOpt m_optSetting = PlatformOpt::OptSpeed;

        static std::unique_ptr<PlatformDb> s_instance;
    };
}

#endif //CROWN_PLATFORM_DB_H
