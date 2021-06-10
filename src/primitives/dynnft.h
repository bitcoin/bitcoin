#pragma once
#include <string>
#include <vector>


class CNFTAssetClass
{

public:
    CNFTAssetClass();

    std::string hash;
    std::vector<unsigned char> metaData;
    std::string owner;
    std::string txnID;
    uint64_t maxCount;

    std::vector<unsigned char> strSerialData;
    bool serialDataCreated;

    void createSerialData();
    void loadFromSerialData(std::vector<unsigned char> data);

};


class CNFTAsset
{

public:
    CNFTAsset();

    std::string hash;
    std::string assetClassHash;
    std::vector<unsigned char> metaData;
    std::vector<unsigned char> binaryData;
    std::string owner;
    std::string txnID;
    uint64_t serial;
};
