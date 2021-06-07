#pragma once

#include <amount.h>
#include <primitives/dynnft.h>
#include <sqlite/sqlite3.h>
#include <stdint.h>
#include <uint256.h>

#include <fstream>
#include <iostream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

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
};
