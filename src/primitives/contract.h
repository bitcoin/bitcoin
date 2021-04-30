#pragma once


#include <stdint.h>
#include <amount.h>
#include <sqlite/sqlite3.h>
#include "random.h"



class CContract
{


public:

    std::string contractAddress;
    std::string contractTxnID;
    std::string blockHash;
    std::string code;
    std::string createdBy;
    int64_t balance;


    CContract(std::string txnID, std::string hash, std::string iCode, std::string iCreatedBy, int64_t initialBalance, std::string address);
};



