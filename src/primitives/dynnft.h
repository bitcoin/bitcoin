#pragma once
#include <string>


class CNFTAssetClass
{
private:

    std::string hash;
    std::string metaData;
    std::string owner;
    std::string txnID;
    uint64_t maxCount;

public:
    CNFTAssetClass();


};


class CNFTAsset
{
private:
    std::string hash;
    std::string assetClassHash;
    std::string metaData;
    std::string binaryData;
    std::string owner;
    std::string txnID;
    uint64_t serial;

public:
    CNFTAsset();

};
