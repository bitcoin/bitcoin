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

    // stores the last timestamp for each asset class hash or asset hash requested
    std::map<std::string, time_t> requestAssetClass;
    std::map<std::string, time_t> requestAsset;
    RecursiveMutex requestLock;

};
