
#include <primitives/contract.h>

CContract::CContract(std::string txnID, std::string hash, std::string iCode, std::string iCreatedBy, int64_t initialBalance, std::string address)

{
    contractTxnID = txnID;
    blockHash = hash;
    code = iCode;
    createdBy = iCreatedBy;
    balance = initialBalance;
    contractAddress = address;


}

