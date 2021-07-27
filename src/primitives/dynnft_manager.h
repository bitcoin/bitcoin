#pragma once

#include <amount.h>
#include <primitives/dynnft.h>
#include <sqlite/sqlite3.h>
#include <stdint.h>
#include <uint256.h>
#include <map>
#include <util/system.h>


class CNFTManager
{
private:
    sqlite3* nftDB;

public:
    CNFTManager();

    void CreateOrOpenDatabase(std::string dataDirectory);

    uint32_t execScalar(char* sql);

    void addNFTAssetClass(CNFTAssetClass* assetClass);
    void addNFTAsset(CNFTAsset* asset);

    bool assetClassInDatabase(std::string assetClassHash);
    bool assetInDatabase(std::string assetHash);

    void queueAssetClassRequest(std::string hash);
    void queueAssetRequest(std::string hash);

    bool assetClassInCache(std::string hash);
    bool assetInCache(std::string hash);

    bool addAssetClassToCache(CNFTAssetClass* assetClass);
    void addAssetToCache(CNFTAsset* assetClass);

    CNFTAssetClass* retrieveAssetClassFromCache(std::string hash);
    CNFTAsset* retrieveAssetFromCache(std::string hash);

    CNFTAssetClass* retrieveAssetClassFromDatabase(std::string hash);


    // stores the last timestamp for each asset class hash or asset hash requested
    std::map<std::string, time_t> requestAssetClass;
    std::map<std::string, time_t> requestAsset;
    RecursiveMutex requestLock;

    // if we are not storing the NFT database, keep a cache of NFT data so that we can relay to requesting peers
    std::map<std::string, CNFTAssetClass*> assetClassCache;
    std::map<std::string, CNFTAsset*> assetCache;
    std::map<std::string, time_t> lastCacheAccessAssetClass;
    std::map<std::string, time_t> lastCacheAccessAsset;
    RecursiveMutex cacheLock;


};
