#include "dynvm.h"




void CDynVM::exec(uint256 caller, int64_t sentAmount, unsigned char* data, unsigned char* method, CContract* iContract)
{

    contract = iContract;

    //find method entry point and set programCounter
    programCounter = 0;

    bVMError = false;
    bVMDone = false;

    //run until we are done or get an error
    while ((!bVMError) && (!bVMDone)) {

        unsigned char opcode = byteCode[programCounter];

        if (opcode == OP_SEND)
            processOP_SEND();

        else if (opcode == OP_AMT_SENT)
            processOP_AMT_SENT();

        else if (opcode == OP_SENDER)
            processOP_SENDER(caller);

    }


}


void CDynVM::processOP_SEND()
{
}


void CDynVM::processOP_AMT_SENT()
{

}

void CDynVM::processOP_SENDER(uint256 caller)
{
    storeI256Value(caller, programCounter + 1);
}


void CDynVM::storeI256Value(uint256 value, uint16_t destStart) {

    if (byteCode[destStart] == 0x00) {
        bVMError = true;
        errorMessage = "Cannot store to immediate value";
    }

    else if (byteCode[destStart] == 0x01) {
    }

    else if (byteCode[destStart] == 0x02) {
        int16_t address = byteCode[destStart + 1] * 256 + byteCode[destStart];
        if (address > 65535 - 32) {
            bVMError = true;
            errorMessage = "Attempt to write past end of RAM";
        }
        for ( int i = 0; i < 32; i++)
            ram[address+i] = *(value.begin() + i);
    }

    else if (byteCode[destStart] == 0x03) {
    }

    else {
        bVMError = true;
        errorMessage = "Illegal destination opcode";
    }


}
