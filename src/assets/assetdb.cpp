// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util.h>
#include <consensus/params.h>
#include <script/ismine.h>
#include <tinyformat.h>
#include "assetdb.h"
#include "assets.h"
#include "validation.h"

#include <boost/thread.hpp>

static const char ASSET_FLAG = 'A';
static const char ASSET_ADDRESS_QUANTITY_FLAG = 'B';
static const char MY_ASSET_FLAG = 'M';
static const char BLOCK_ASSET_UNDO_DATA = 'U';
static const char MEMPOOL_REISSUED_TX = 'Z';

CAssetsDB::CAssetsDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "assets", nCacheSize, fMemory, fWipe) {
}

bool CAssetsDB::WriteAssetData(const CNewAsset &asset)
{
    return Write(std::make_pair(ASSET_FLAG, asset.strName), asset);
}

bool CAssetsDB::WriteMyAssetsData(const std::string &strName, const std::set<COutPoint>& setOuts)
{
    return Write(std::make_pair(MY_ASSET_FLAG, strName), setOuts);
}

bool CAssetsDB::WriteAssetAddressQuantity(const std::string &assetName, const std::string &address, const CAmount &quantity)
{
    return Write(std::make_pair(ASSET_ADDRESS_QUANTITY_FLAG, std::make_pair(assetName, address)), quantity);
}

bool CAssetsDB::ReadAssetData(const std::string& strName, CNewAsset& asset)
{
    return Read(std::make_pair(ASSET_FLAG, strName), asset);
}

bool CAssetsDB::ReadMyAssetsData(const std::string &strName, std::set<COutPoint>& setOuts)
{
    return Read(std::make_pair(MY_ASSET_FLAG, strName), setOuts);
}

bool CAssetsDB::ReadAssetAddressQuantity(const std::string& assetName, const std::string& address, CAmount& quantity)
{
    return Read(std::make_pair(ASSET_ADDRESS_QUANTITY_FLAG, std::make_pair(assetName, address)), quantity);
}

bool CAssetsDB::EraseAssetData(const std::string& assetName)
{
    return Erase(std::make_pair(ASSET_FLAG, assetName));
}

bool CAssetsDB::EraseMyAssetData(const std::string& assetName)
{
    return Erase(std::make_pair(MY_ASSET_FLAG, assetName));
}

bool CAssetsDB::EraseAssetAddressQuantity(const std::string &assetName, const std::string &address) {
    return Erase(std::make_pair(ASSET_ADDRESS_QUANTITY_FLAG, std::make_pair(assetName, address)));
}

bool CAssetsDB::EraseMyOutPoints(const std::string& assetName)
{
    if (!EraseMyAssetData(assetName))
        return error("%s : Failed to erase my asset outpoints from database.", __func__);

    return true;
}

bool CAssetsDB::WriteBlockUndoAssetData(const uint256& blockhash, const std::vector<std::pair<std::string, CBlockAssetUndo> >& assetUndoData)
{
    return Write(std::make_pair(BLOCK_ASSET_UNDO_DATA, blockhash), assetUndoData);
}

bool CAssetsDB::ReadBlockUndoAssetData(const uint256 &blockhash, std::vector<std::pair<std::string, CBlockAssetUndo> > &assetUndoData)
{
    // If it exists, return the read value.
    if (Exists(std::make_pair(BLOCK_ASSET_UNDO_DATA, blockhash)))
           return Read(std::make_pair(BLOCK_ASSET_UNDO_DATA, blockhash), assetUndoData);

    // If it doesn't exist, we just return true because we don't want to fail just because it didn't exist in the db
    return true;
}

bool CAssetsDB::WriteReissuedMempoolState()
{
    return Write(MEMPOOL_REISSUED_TX, mapReissuedAssets);
}

bool CAssetsDB::ReadReissuedMempoolState()
{
    mapReissuedAssets.clear();
    mapReissuedTx.clear();
    // If it exists, return the read value.
    bool rv = Read(MEMPOOL_REISSUED_TX, mapReissuedAssets);
    if (rv) {
        for (auto pair : mapReissuedAssets)
            mapReissuedTx.insert(std::make_pair(pair.second, pair.first));
    }
    return rv;
}

bool CAssetsDB::LoadAssets()
{
    std::unique_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(std::make_pair(ASSET_FLAG, std::string()));

    // Load assets
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, std::string> key;
        if (pcursor->GetKey(key) && key.first == ASSET_FLAG) {
            CNewAsset asset;
            if (pcursor->GetValue(asset)) {
                passetsCache->Put(asset.strName, asset);
                pcursor->Next();
            } else {
                return error("%s: failed to read asset", __func__);
            }
        } else {
            break;
        }
    }

    std::unique_ptr<CDBIterator> pcursor2(NewIterator());
    pcursor2->Seek(std::make_pair(MY_ASSET_FLAG, std::string()));
    // Load mapMyUnspentAssets
    while (pcursor2->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, std::string> key;
        if (pcursor2->GetKey(key) && key.first == MY_ASSET_FLAG) {
            std::set<COutPoint> outs;
            if (pcursor2->GetValue(outs)) {
                passets->mapMyUnspentAssets.insert(std::make_pair(key.second, outs));
                pcursor2->Next();
            } else {
                return error("%s: failed to read my assets", __func__);
            }
        } else {
            break;
        }
    }

    std::unique_ptr<CDBIterator> pcursor3(NewIterator());
    pcursor3->Seek(std::make_pair(ASSET_ADDRESS_QUANTITY_FLAG, std::make_pair(std::string(), std::string())));
    // Load mapMyUnspentAssets
    while (pcursor3->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, std::pair<std::string, std::string> > key; // <Asset Name, Address> -> Quantity
        if (pcursor3->GetKey(key) && key.first == ASSET_ADDRESS_QUANTITY_FLAG) {
            CAmount value;
            if (pcursor3->GetValue(value)) {
                if (!passets->mapAssetsAddresses.count(key.second.first)) {
                    std::set<std::string> setAddresses;
                    passets->mapAssetsAddresses.insert(std::make_pair(key.second.first, setAddresses));
                }
                if (!passets->mapAssetsAddresses[key.second.first].insert(key.second.second).second)
                    return error("%s: failed to read my address quantity from database", __func__);
                passets->mapAssetsAddressAmount.insert(std::make_pair(std::make_pair(key.second.first, key.second.second), value));
                pcursor3->Next();
            } else {
                return error("%s: failed to read my address quantity from database", __func__);
            }
        } else {
            break;
        }
    }

    return true;
}

bool CAssetsDB::AssetDir(std::vector<CNewAsset>& assets, const std::string filter, const size_t count, const long start)
{
    FlushStateToDisk();

    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->Seek(std::make_pair(ASSET_FLAG, std::string()));

    auto prefix = filter;
    bool wildcard = prefix.back() == '*';
    if (wildcard)
        prefix.pop_back();

    size_t skip = 0;
    if (start >= 0) {
        skip = start;
    }
    else {
        // compute table size for backwards offset
        long table_size = 0;
        while (pcursor->Valid()) {
            boost::this_thread::interruption_point();

            std::pair<char, std::string> key;
            if (pcursor->GetKey(key) && key.first == ASSET_FLAG) {
                if (prefix == "" ||
                    (wildcard && key.second.find(prefix) == 0) ||
                    (!wildcard && key.second == prefix)) {
                    table_size += 1;
                }
            }
            pcursor->Next();
        }
        skip = table_size + start;
        pcursor->SeekToFirst();
    }


    size_t loaded = 0;
    size_t offset = 0;

    // Load assets
    while (pcursor->Valid() && loaded < count) {
        boost::this_thread::interruption_point();

        std::pair<char, std::string> key;
        if (pcursor->GetKey(key) && key.first == ASSET_FLAG) {
            if (prefix == "" ||
                    (wildcard && key.second.find(prefix) == 0) ||
                    (!wildcard && key.second == prefix)) {
                if (offset < skip) {
                    offset += 1;
                }
                else {
                    CNewAsset asset;
                    if (pcursor->GetValue(asset)) {
                        assets.push_back(asset);
                        loaded += 1;
                    } else {
                        return error("%s: failed to read asset", __func__);
                    }
                }
            }
            pcursor->Next();
        } else {
            break;
        }
    }

    return true;
}

bool CAssetsDB::AssetDir(std::vector<CNewAsset>& assets)
{
    return CAssetsDB::AssetDir(assets, "*", MAX_SIZE, 0);
}