#pragma once
#include <map>
#include <vector>
#include "global.h"
#include <primitives/contract.h>
#include "primitives/contract_manager.h"



class CDynEngine
{
public:

    std::map<uint256, CContract*> contracts;

    void createContract(std::string txnID, std::string blockHash, std::string createdBy, int64_t sentAmount, std::string contractCode);

    void execContract(uint256 contractAddress, uint256 caller, int64_t sentAmount, unsigned char* data, unsigned char* method);
};
