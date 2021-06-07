#pragma once
#include <string>


class CNFTAssetClass
{

public:
    CNFTAssetClass();

    std::string hash;
    std::string metaData;
    std::string owner;
    std::string txnID;
    uint64_t maxCount;

};


class CNFTAsset
{

public:
    CNFTAsset();

    std::string hash;
    std::string assetClassHash;
    std::string metaData;
    std::string binaryData;
    std::string owner;
    std::string txnID;
    uint64_t serial;
};
