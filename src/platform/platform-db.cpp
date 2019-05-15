// Copyright (c) 2014-2019 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "platform-utils.h"
#include "platform-db.h"
#include "platform/specialtx.h"
#include "main.h"

namespace Platform
{
    /*static*/ std::unique_ptr<PlatformDb> PlatformDb::s_instance;
    /*static*/ const char PlatformDb::DB_NFT = 'n';
    /*static*/ const char PlatformDb::DB_PROTO_TOTAL = 't';

    PlatformDb::PlatformDb(size_t nCacheSize, bool fMemory, bool fWipe)
    : TransactionLevelDBWrapper("platform", nCacheSize, fMemory, fWipe)
    {
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

            if (ProcessNftIndex(*dbIt, nftIndexHandler))
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

    bool PlatformDb::ProcessNftProtosSupply(const leveldb::Iterator & dbIt, std::function<bool(uint64_t, std::size_t)> protoSupplyHandler)
    {
        if (dbIt.key().starts_with(std::string(1, DB_PROTO_TOTAL)))
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

    void PlatformDb::WriteTotalSupply(std::size_t count, uint64_t nftProtocolId)
    {
        this->Write(std::make_pair(DB_PROTO_TOTAL, nftProtocolId), count);
    }

    bool PlatformDb::ReadTotalSupply(std::size_t & count, uint64_t nftProtocolId)
    {
        return this->Read(std::make_pair(DB_PROTO_TOTAL, nftProtocolId), count);
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

        /// Not sure if this case is even possible, TODO: test it
        if (nftDiskIndex.NfTokenPtr() == nullptr)
        {
            CTransaction tx;
            uint256 txBlockHash;
            if (!GetTransaction(nftDiskIndex.RegTxHash(), tx, txBlockHash, true))
            {
                LogPrintf("%s: Transaction for NFT cannot found, block hash: %s, tx hash: %s",
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
}