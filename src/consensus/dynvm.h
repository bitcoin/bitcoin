#pragma once
#include <vector>
#include <consensus/dynengine.h>



    //0109test_send0000   74 65 73 74 5f 73 65 6e 64
//28 02 00 00             get address of DYN sent from into memory pos 0
//27 02 00 20             get amount of DYN sent into contract
//23 02 00 00 02 00 20    send dyn back to same addr in same amt

//0109746573745f73656e640000280200002702002023020000020020

#define OP_SEND 0x22            //send DYN from contract balance
#define OP_AMT_SENT 0x27        //amt of DYN sent in atoms
#define OP_SENDER 0x28          //get address of caller



class CDynVM
{
    std::vector<uint256> stack;
    std::vector<uint256> registers;
    std::vector<unsigned char> ram;

    uint16_t programCounter;
    CContract* contract;        //to avoid passing as parameter everywhere
    std::vector<unsigned char> byteCode;

    bool bVMError = false;
    bool bVMDone = false;
    std::string errorMessage;


public:

    void exec(uint256 caller, int64_t sentAmount, unsigned char* data, unsigned char* method, CContract* iContract);

    void processOP_SEND();
    void processOP_AMT_SENT();
    void processOP_SENDER(uint256 caller);

    void storeI256Value(uint256 value, uint16_t destStart);


};
