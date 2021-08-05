#pragma once

#include <amount.h>
#include <primitives/dynnft.h>
#include <sqlite/sqlite3.h>
#include <stdint.h>
#include <uint256.h>
#include <map>
#include <util/system.h>


//stores items needed to implement hysterisis on requesting NFT data
//starts at 10 second retry interval
//after 3 requsests the retry interval doubles
struct sCacheTiming {
    time_t lastAttempt;
    int numRequests;
    int checkInterval;
};

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
    bool addAssetToCache(CNFTAsset* assetClass);

    bool assetSerialExists(std::string assetClassHash, uint64_t assetSerial);

    CNFTAssetClass* retrieveAssetClassFromCache(std::string hash);
    CNFTAsset* retrieveAssetFromCache(std::string hash);

    CNFTAssetClass* retrieveAssetClassFromDatabase(std::string hash);
    CNFTAsset* retrieveAssetFromDatabase(std::string hash);

    void updateAssetClassOwner(std::string hash, std::string newOwner);
    void updateAssetOwner(std::string hash, std::string newOwner);


    // stores the last timestamp for each asset class hash or asset hash requested
    std::map<std::string, sCacheTiming> requestAssetClass;
    std::map<std::string, sCacheTiming> requestAsset;
    RecursiveMutex requestLock;

    // if we are not storing the NFT database, keep a cache of NFT data so that we can relay to requesting peers
    std::map<std::string, CNFTAssetClass*> assetClassCache;
    std::map<std::string, CNFTAsset*> assetCache;
    std::map<std::string, time_t> lastCacheAccessAssetClass;
    std::map<std::string, time_t> lastCacheAccessAsset;
    RecursiveMutex cacheLock;


};
