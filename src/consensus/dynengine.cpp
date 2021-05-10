
#pragma once
#include "dynengine.h"
#include "dynvm.h"
#include <crypto/sha256.h>


void CDynEngine::createContract(std::string txnID, std::string blockHash, std::string createdBy, int64_t sentAmount, std::string contractCode)
{



    //generate contract address - catenation of block hash and creator
    uint256 iContractAddress;
    CSHA256 ctx;
    ctx.Write((unsigned char*)(blockHash + createdBy).c_str(), blockHash.length() + createdBy.length());
    ctx.Finalize(iContractAddress.begin());

     
    CContract* contract = new CContract(txnID, blockHash, contractCode, createdBy, sentAmount, iContractAddress.GetHex().substr(0, 63));

    g_contractMgr->addContract(contract);       //save to database


    contracts[iContractAddress] = contract;

}

void CDynEngine::execContract(uint256 contractAddress, uint256 caller, int64_t sentAmount, unsigned char* data, unsigned char* method)
{

    CContract* contract = contracts[contractAddress];

    CDynVM* vm = new CDynVM();

    vm->exec(caller, sentAmount, data, method, contract);

    delete vm;


}
