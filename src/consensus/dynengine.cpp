
#pragma once
#include "dynengine.h"



void CDynEngine::createContract(std::string txnID, std::string blockHash, std::string createdBy, int64_t sentAmount, std::string contractCode)
{


    //generate contract address
    FastRandomContext rng(true);
    uint256 iContractAddress = rng.rand256();

    unsigned char checkSum = 0;
    for (int i = 0; i < 15; i++)
        checkSum += iContractAddress.begin()[i];
    iContractAddress.begin()[15] = checkSum;

    CContract* contract = new CContract(txnID, blockHash, contractCode, createdBy, sentAmount, iContractAddress.GetHex().substr(0, 63));

    g_contractMgr->addContract(contract);       //save to database

}

void CDynEngine::execContract(sAddressType contractAddress, sAddressType caller, int64_t sentAmount, unsigned char* data, unsigned char* method)
{



    CDynVM* vm = new CDynVM();
    vm->exec()

    free(vm);


}
