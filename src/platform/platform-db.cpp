// Copyright (c) 2014-2019 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "platform-db.h"
#include "platform/specialtx.h"
#include "main.h"

namespace Platform
{
    /*static*/ std::unique_ptr<PlatformDb> PlatformDb::s_instance;
    /*static*/ const char PlatformDb::DB_NFT = 'n';

    PlatformDb::PlatformDb(size_t nCacheSize, bool fMemory, bool fWipe)
    : m_db(GetDataDir() / "platform", nCacheSize, fMemory, fWipe)
    , m_dbTransaction(m_db)
    {
    }

    bool PlatformDb::LoadNftIndexGuts(std::function<bool(NfTokenIndex)> nftIndexHandler)
    {
        // Read NFTs from the DB to cache
        // if load the whole DB -> skip reading on read requests
        std::unique_ptr<leveldb::Iterator> dbIt(m_db.NewIterator());

        for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next())
        {
            boost::this_thread::interruption_point();

            if (dbIt->key().starts_with(std::string(1, DB_NFT)))
            {
                leveldb::Slice sliceValue = dbIt->value();
                CDataStream streamValue(sliceValue.data(), sliceValue.data() + sliceValue.size(), SER_DISK, CLIENT_VERSION);
                NfTokenDiskIndex nftDiskIndex;
                streamValue >> nftDiskIndex;

                NfTokenIndex nftIndex = NftDiskIndexToNftMemIndex(nftDiskIndex);
                if (nftIndex.IsNull())
                {
                    throw std::runtime_error("Cannot build an NFT record, reg tx hash: " + nftDiskIndex.RegTxHash().ToString());
                }

                if (!nftIndexHandler(std::move(nftIndex)))
                {
                    throw std::runtime_error("Cannot put nft index into memory, reg tx hash: " + nftDiskIndex.RegTxHash().ToString());
                }
            }
        }

        HandleError(dbIt->status());
    }

    void PlatformDb::WriteNftDiskIndex(const NfTokenDiskIndex & nftDiskIndex)
    {
        Write(std::make_tuple(PlatformDb::DB_NFT,
              nftDiskIndex.NfTokenPtr()->tokenProtocolId,
              nftDiskIndex.NfTokenPtr()->tokenId),
              nftDiskIndex
              );
    }

    NfTokenIndex PlatformDb::ReadNftIndex(const uint64_t &protocolId, const uint256 &tokenId)
    {
        NfTokenDiskIndex nftDiskIndex;
        bool readRes = Read(std::make_tuple(PlatformDb::DB_NFT, protocolId, tokenId), nftDiskIndex);
        if (readRes)
        {
            return NftDiskIndexToNftMemIndex(nftDiskIndex);
        }

        LogPrintf("%s: NFT index not found in the database", __func__);
        return NfTokenIndex();
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