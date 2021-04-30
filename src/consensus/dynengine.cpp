
#pragma once
#include "dynengine.h"
#include "dynvm.h"


void CDynEngine::createContract(std::string txnID, std::string blockHash, std::string createdBy, int64_t sentAmount, std::string contractCode)
{



    //generate contract address
    FastRandomContext rng(true);
    uint256 iContractAddress = rng.rand256();
     
    CContract* contract = new CContract(txnID, blockHash, contractCode, createdBy, sentAmount, iContractAddress.GetHex().substr(0, 63));

    g_contractMgr->addContract(contract);       //save to database


    contracts[iContractAddress] = contract;

}

void CDynEngine::execContract(uint256 contractAddress, uint256 caller, int64_t sentAmount, unsigned char* data, unsigned char* method)
{

    CContract* contract = contracts[contractAddress];

    CDynVM* vm = new CDynVM();

    vm->exec(caller, sentAmount, data, method, contract);

    free(vm);


}
