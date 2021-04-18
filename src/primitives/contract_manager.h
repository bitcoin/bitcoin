#pragma once

#include <stdint.h>
#include <amount.h>
#include <uint256.h>
#include <sqlite/sqlite3.h>
#include <primitives/contract.h>


class CContractManager
{
private:

    sqlite3* contractDB;

public:
    CContractManager();

    void CreateOrOpenDatabase(std::string dataDirectory);

    uint32_t execScalar(char* sql);

    void addContract(CContract* contract);


};
