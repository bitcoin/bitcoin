#pragma once
#include <string>
#include <vector>


class CNFTAssetClass
{

public:
    CNFTAssetClass();

    std::string hash;
    std::string metaData;
    std::string owner;
    std::string txnID;
    uint64_t maxCount;

    std::vector<unsigned char> strSerialData;
    bool serialDataCreated;

    void writeString(std::string data);
    void writeVector(std::vector<unsigned char> data);
    void createSerialData();
    void loadFromSerialData(std::vector<unsigned char> data);

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
