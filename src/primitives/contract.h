#pragma once


#include <stdint.h>
#include <amount.h>
#include <uint256.h>
#include <sqlite/sqlite3.h>



class CContract
{


public:

    std::string contractAddress;
    std::string contractTxnID;
    uint64_t blockHeight;
    std::string code;
    std::string createdBy;
    


    CContract();
};



