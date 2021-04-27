#pragma once
#include <map>
#include <vector>
#include "global.h"
#include <primitives/contract.h>
#include "primitives/contract_manager.h"

struct sAddressType {
    unsigned char addr[32];
};


class CDynEngine
{
public:

    std::map<sAddressType, CContract> contracts;

    void createContract(std::string txnID, std::string blockHash, std::string createdBy, int64_t sentAmount, std::string contractCode);

    void execContract(sAddressType contractAddress, sAddressType caller, int64_t sentAmount, unsigned char* data, unsigned char* method);
};
