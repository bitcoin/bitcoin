// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/thread.hpp>
#include "platform-utils.h"
#include "platform-db.h"
#include "platform/specialtx.h"
#include "platform/nf-token/nf-token-protocol-reg-tx.h"
#include "platform/nf-token/nf-tokens-manager.h"
#include "main.h"

namespace Platform
{
    /*static*/ std::unique_ptr<PlatformDb> PlatformDb::s_instance;
    /*static*/ const char PlatformDb::DB_NFT = 'n';
    /*static*/ const char PlatformDb::DB_NFT_TOTAL = 't';
    /*static*/ const char PlatformDb::DB_NFT_PROTO = 'p';
    /*static*/ const char PlatformDb::DB_NFT_PROTO_TOTAL = 'c';

    PlatformDb::PlatformDb(size_t nCacheSize, PlatformOpt optSetting, bool fMemory, bool fWipe)
    : TransactionLevelDBWrapper("platform", nCacheSize, fMemory, fWipe)
    {
        m_optSetting = optSetting;
    }

    void PlatformDb::ProcessPlatformDbGuts(std::function<bool(const leveldb::Iterator &)> processor)
    {
        std::unique_ptr<leveldb::Iterator> dbIt(m_db.NewIterator());

        for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next())
        {
            boost::this_thread::interruption_point();

            if (!processor(*dbIt))
            {
                LogPrintf("%s : Cannot process a platform db record - %s", __func__, dbIt->key().ToString());
                continue;
            }
        }

        HandleError(dbIt->status());
    }

    void PlatformDb::ProcessNftIndexGutsOnly(std::function<bool(NfTokenIndex)> nftIndexHandler)
    {
        std::unique_ptr<leveldb::Iterator> dbIt(m_db.NewIterator());

        for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next())
        {
            boost::this_thread::interruption_point();

            if (!ProcessNftIndex(*dbIt, nftIndexHandler))
            {
                LogPrintf("%s : Cannot process a platform db record - %s", __func__, dbIt->key().ToString());
                continue;
            }
        }

        HandleError(dbIt->status());
    }

    void PlatformDb::ProcessNftProtoIndexGutsOnly(std::function<bool(NftProtoIndex)> protoIndexHandler)
    {
        std::unique_ptr<leveldb::Iterator> dbIt(m_db.NewIterator());

        for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next())
        {
            boost::this_thread::interruption_point();

            if (!ProcessNftProtoIndex(*dbIt, protoIndexHandler))
            {
                LogPrintf("%s : Cannot process a platform db record - %s", __func__, dbIt->key().ToString());
                continue;
            }
        }

        HandleError(dbIt->status());
    }

    bool PlatformDb::IsNftIndexEmpty()
    {
        std::unique_ptr<leveldb::Iterator> dbIt(m_db.NewIterator());

        for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next())
        {
            boost::this_thread::interruption_point();

            if (dbIt->key().starts_with(std::string(1, DB_NFT)))
            {
                HandleError(dbIt->status());
                return false;
            }
        }

        HandleError(dbIt->status());
        return true;
    }

    bool PlatformDb::ProcessNftIndex(const leveldb::Iterator & dbIt, std::function<bool(NfTokenIndex)> nftIndexHandler)
    {
        if (dbIt.key().starts_with(std::string(1, DB_NFT)))
        {
            leveldb::Slice sliceValue = dbIt.value();
            CDataStream streamValue(sliceValue.data(), sliceValue.data() + sliceValue.size(), SER_DISK, CLIENT_VERSION);
            NfTokenDiskIndex nftDiskIndex;

            try
            {
                streamValue >> nftDiskIndex;
            }
            catch (const std::exception & ex)
            {
                LogPrintf("%s : Deserialize or I/O error - %s", __func__, ex.what());
                return false;
            }

            NfTokenIndex nftIndex = NftDiskIndexToNftMemIndex(nftDiskIndex);
            if (nftIndex.IsNull())
            {
                LogPrintf("%s : Cannot build an NFT record, reg tx hash: %s", __func__, nftDiskIndex.RegTxHash().ToString());
                return false;
            }

            if (!nftIndexHandler(std::move(nftIndex)))
            {
                LogPrintf("%s : Cannot process an NFT index, reg tx hash: %s", __func__, nftDiskIndex.RegTxHash().ToString());
                return false;
            }
        }
        return true;
    }

    bool PlatformDb::ProcessNftProtoIndex(const leveldb::Iterator & dbIt, std::function<bool(NftProtoIndex)> protoIndexHandler)
    {
        if (dbIt.key().starts_with(std::string(1, DB_NFT_PROTO)))
        {
            leveldb::Slice sliceValue = dbIt.value();
            CDataStream streamValue(sliceValue.data(), sliceValue.data() + sliceValue.size(), SER_DISK, CLIENT_VERSION);
            NftProtoDiskIndex protoDiskIndex;

            try
            {
                streamValue >> protoDiskIndex;
            }
            catch (const std::exception & ex)
            {
                LogPrintf("%s : Deserialize or I/O error - %s", __func__, ex.what());
                return false;
            }

            NftProtoIndex protoIndex = NftProtoDiskIndexToNftProtoMemIndex(protoDiskIndex);
            if (protoIndex.IsNull())
            {
                LogPrintf("%s : Cannot build an NFT proto record, reg tx hash: %s", __func__, protoDiskIndex.RegTxHash().ToString());
                return false;
            }

            if (!protoIndexHandler(std::move(protoIndex)))
            {
                LogPrintf("%s : Cannot process an NFT proto index, reg tx hash: %s", __func__, protoDiskIndex.RegTxHash().ToString());
                return false;
            }
        }
        return true;
    }

    bool PlatformDb::ProcessNftSupply(const leveldb::Iterator & dbIt, std::function<bool(uint64_t, std::size_t)> protoSupplyHandler)
    {
        if (dbIt.key().starts_with(std::string(1, DB_NFT_TOTAL)))
        {
            leveldb::Slice sliceKey = dbIt.key();
            CDataStream streamKey(sliceKey.data(), sliceKey.data() + sliceKey.size(), SER_DISK, CLIENT_VERSION);
            std::pair<char, uint64_t> key;

            leveldb::Slice sliceValue = dbIt.value();
            CDataStream streamValue(sliceValue.data(), sliceValue.data() + sliceValue.size(), SER_DISK, CLIENT_VERSION);
            std::size_t protoSupply = 0;

            try
            {
                streamKey >> key;
                streamValue >> protoSupply;
            }
            catch (const std::exception & ex)
            {
                LogPrintf("%s : Deserialize or I/O error - %s", __func__, ex.what());
                return false;
            }

            if (!protoSupplyHandler(key.second, protoSupply))
            {
                LogPrintf("%s : Cannot process protocol supply: %s", __func__, ProtocolName(key.second).ToString());
                return false;
            }
        }
        return true;
    }

    void PlatformDb::WriteNftDiskIndex(const NfTokenDiskIndex & nftDiskIndex)
    {
        this->Write(std::make_tuple(DB_NFT,
              nftDiskIndex.NfTokenPtr()->tokenProtocolId,
              nftDiskIndex.NfTokenPtr()->tokenId),
              nftDiskIndex
              );
    }

    void PlatformDb::EraseNftDiskIndex(const uint64_t &protocolId, const uint256 &tokenId)
    {
        this->Erase(std::make_tuple(DB_NFT, protocolId, tokenId));
    }

    NfTokenIndex PlatformDb::ReadNftIndex(const uint64_t &protocolId, const uint256 &tokenId)
    {
        NfTokenDiskIndex nftDiskIndex;
        if (this->Read(std::make_tuple(DB_NFT, protocolId, tokenId), nftDiskIndex))
        {
            return NftDiskIndexToNftMemIndex(nftDiskIndex);
        }
        return NfTokenIndex();
    }

    void PlatformDb::WriteNftProtoDiskIndex(const NftProtoDiskIndex & protoDiskIndex)
    {
        this->Write(std::make_pair(DB_NFT_PROTO, protoDiskIndex.NftProtoPtr()->tokenProtocolId), protoDiskIndex);
    }

    void PlatformDb::EraseNftProtoDiskIndex(const uint64_t &protocolId)
    {
        this->Erase(std::make_pair(DB_NFT_PROTO, protocolId));
    }

    NftProtoIndex PlatformDb::ReadNftProtoIndex(const uint64_t &protocolId)
    {
        NftProtoDiskIndex protoDiskIndex;
        if (this->Read(std::make_pair(DB_NFT_PROTO, protocolId), protoDiskIndex))
        {
            return NftProtoDiskIndexToNftProtoMemIndex(protoDiskIndex);
        }
        return NftProtoIndex();
    }

    void PlatformDb::WriteTotalSupply(std::size_t count, uint64_t nftProtocolId)
    {
        this->Write(std::make_pair(DB_NFT_TOTAL, nftProtocolId), count);
    }

    bool PlatformDb::ReadTotalSupply(std::size_t & count, uint64_t nftProtocolId)
    {
        return this->Read(std::make_pair(DB_NFT_TOTAL, nftProtocolId), count);
    }

    void PlatformDb::WriteTotalProtocolCount(std::size_t count)
    {
        this->Write(DB_NFT_PROTO_TOTAL, count);   
    }

    bool PlatformDb::ReadTotalProtocolCount(std::size_t & count)
    {
        return this->Read(DB_NFT_PROTO_TOTAL, count);
    }

    BlockIndex * PlatformDb::FindBlockIndex(const uint256 & blockHash)
    {
        auto blockIndexIt = mapBlockIndex.find(blockHash);
        if (blockIndexIt != mapBlockIndex.end())
            blockIndexIt->second;
        return nullptr;
    }

    NfTokenIndex PlatformDb::NftDiskIndexToNftMemIndex(const NfTokenDiskIndex &nftDiskIndex)
    {
        auto blockIndexIt = mapBlockIndex.find(nftDiskIndex.BlockHash());
        if (blockIndexIt == mapBlockIndex.end())
        {
            LogPrintf("%s: Block index for NFT transaction cannot be found, block hash: %s, tx hash: %s",
                      __func__, nftDiskIndex.BlockHash().ToString(), nftDiskIndex.RegTxHash().ToString());
            return NfTokenIndex();
        }

        /// Not sure if this case is even possible
        if (nftDiskIndex.NfTokenPtr() == nullptr)
        {
            CTransaction tx;
            uint256 txBlockHash;
            if (!GetTransaction(nftDiskIndex.RegTxHash(), tx, txBlockHash, true))
            {
                LogPrintf("%s: Transaction for NFT cannot be found, block hash: %s, tx hash: %s",
                          __func__, nftDiskIndex.BlockHash().ToString(), nftDiskIndex.RegTxHash().ToString());
                return NfTokenIndex();
            }
            assert(txBlockHash == nftDiskIndex.BlockHash());

            NfTokenRegTx nftRegTx;
            bool res = GetTxPayload(tx, nftRegTx);
            assert(res);

            std::shared_ptr<NfToken> nfTokenPtr(new NfToken(nftRegTx.GetNfToken()));
            return {blockIndexIt->second, nftDiskIndex.RegTxHash(), nfTokenPtr};
        }

        return {blockIndexIt->second, nftDiskIndex.RegTxHash(), nftDiskIndex.NfTokenPtr()};
    }

    NftProtoIndex PlatformDb::NftProtoDiskIndexToNftProtoMemIndex(const NftProtoDiskIndex &protoDiskIndex)
    {
        auto blockIndexIt = mapBlockIndex.find(protoDiskIndex.BlockHash());
        if (blockIndexIt == mapBlockIndex.end())
        {
            LogPrintf("%s: Block index for NFT proto transaction cannot be found, block hash: %s, tx hash: %s",
                      __func__, protoDiskIndex.BlockHash().ToString(), protoDiskIndex.RegTxHash().ToString());
            return NftProtoIndex();
        }

        /// Not sure if this case is even possible
        if (protoDiskIndex.NftProtoPtr() == nullptr)
        {
            CTransaction tx;
            uint256 txBlockHash;
            if (!GetTransaction(protoDiskIndex.RegTxHash(), tx, txBlockHash, true))
            {
                LogPrintf("%s: Transaction for NFT proto cannot be found, block hash: %s, tx hash: %s",
                          __func__, protoDiskIndex.BlockHash().ToString(), protoDiskIndex.RegTxHash().ToString());
                return NftProtoIndex();
            }
            assert(txBlockHash == protoDiskIndex.BlockHash());

            NfTokenProtocolRegTx protoRegTx;
            bool res = GetTxPayload(tx, protoRegTx);
            assert(res);

            std::shared_ptr<NfTokenProtocol> nftProtoPtr(new NfTokenProtocol(protoRegTx.GetNftProto()));
            return {blockIndexIt->second, protoDiskIndex.RegTxHash(), nftProtoPtr};
        }

        return {blockIndexIt->second, protoDiskIndex.RegTxHash(), protoDiskIndex.NftProtoPtr()};
    }

    bool PlatformDb::CleanupDb()
    {
        LogPrintf("%s", __func__);
        std::unique_ptr<leveldb::Iterator> dbIt(m_db.NewIterator());
        for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next())
        {
            if (dbIt->key().starts_with(std::string(1, DB_NFT)))
            {
                leveldb::Slice sliceValue = dbIt->value();
                leveldb::Slice sliceKey = dbIt->key();
                CDataStream streamValue(sliceValue.data(), sliceValue.data() + sliceValue.size(), SER_DISK, CLIENT_VERSION);
                CDataStream keyStreamValue(sliceKey.data(), sliceKey.data() + sliceKey.size(), SER_DISK, CLIENT_VERSION);
                NfTokenDiskIndex nftDiskIndex;
                std::tuple<char, uint64_t, uint256> keyTuple;

                try
                {
                    streamValue >> nftDiskIndex;
                    keyStreamValue >> keyTuple;
                }
                catch (const std::exception & ex)
                {
                    LogPrintf("%s : Deserialize or I/O error - %s", __func__, ex.what());
                    return false;
                }

                auto blockIndexIt = mapBlockIndex.find(nftDiskIndex.BlockHash());
                if (blockIndexIt == mapBlockIndex.end())
                {
                    LogPrintf("Block index not found for nftDiskIndex. Removing from platform db\n");
                    {
                        auto platformDbTx = BeginTransaction();
                        EraseNftDiskIndex(std::get<1>(keyTuple), std::get<2>(keyTuple));
                        platformDbTx->Commit();
                    }

                    {
                        auto platformDbTx = BeginTransaction();
                        std::size_t totalCount = 0;
                        this->Read(std::make_pair(DB_NFT_TOTAL, std::get<1>(keyTuple)), totalCount);
                        this->Write(std::make_pair(DB_NFT_TOTAL, std::get<1>(keyTuple)), --totalCount);
                        platformDbTx->Commit();
                    }
                }
            }
            else if (dbIt->key().starts_with(std::string(1, DB_NFT_PROTO)))
            {
                leveldb::Slice sliceValue = dbIt->value();
                leveldb::Slice sliceKey = dbIt->key();
                CDataStream streamValue(sliceValue.data(), sliceValue.data() + sliceValue.size(), SER_DISK, CLIENT_VERSION);
                CDataStream keyStreamValue(sliceKey.data(), sliceKey.data() + sliceKey.size(), SER_DISK, CLIENT_VERSION);
                NftProtoDiskIndex protoDiskIndex;
                std::pair<char, uint64_t> keyTuple;

                try
                {
                    streamValue >> protoDiskIndex;
                    keyStreamValue >> keyTuple;
                }
                catch (const std::exception & ex)
                {
                    LogPrintf("%s : Deserialize or I/O error - %s", __func__, ex.what());
                    return false;
                }
                auto blockIndexIt = mapBlockIndex.find(protoDiskIndex.BlockHash());
                if (blockIndexIt == mapBlockIndex.end())
                {
                    LogPrintf("Block index not found for protoDiskIndex. Removing from platform db\n");
                    {
                        auto platformDbTx = BeginTransaction();
                        EraseNftProtoDiskIndex(std::get<1>(keyTuple));
                        platformDbTx->Commit();
                    }

                    {
                        std::size_t totalProtoCount = 0;
                        ReadTotalProtocolCount(totalProtoCount);

                        auto platformDbTx = BeginTransaction();
                        WriteTotalProtocolCount(--totalProtoCount);
                        platformDbTx->Commit();
                    }
                }
            }
        }
        return true;
    }

    void PlatformDb::Reindex()
    {
        int height = chainActive.Height();
        LogPrintf("%s : Height - %d\n", __func__, height);
        for (int i = 2800000; i < height + 1; i++)
        {
            CBlockIndex* index = chainActive[i];

            CValidationState state;
            CBlock block;

            // Reconsider last 50 blocks
            if (i > height - 50)
            {
                ReconsiderBlock(state, index);
            }

            if(!ReadBlockFromDisk(block, index))
            {
                LogPrintf("%s : Failed to read block from disk %d", __func__, i);
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");
            }

            auto platformDbTx = BeginTransaction();
            if (!Platform::ProcessSpecialTxsInBlock(false, block, index, state))
            {
                LogPrintf("%s : Failed to process special transaction %d\n", __func__, i);
            }
            bool committed = platformDbTx->Commit();
        }
        LogPrintf("%s : Reindex done\n", __func__);
    }
}
