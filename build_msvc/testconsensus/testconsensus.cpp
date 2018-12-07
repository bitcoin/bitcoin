#include <iostream>

// bitcoin includes.
#include <..\src\script\bitcoinconsensus.h>
#include <..\src\primitives\transaction.h>
#include <..\src\script\script.h>
#include <..\src\streams.h>
#include <..\src\version.h>

#include <..\src\factories\transactions.h>

int main()
{
    std::cout << "bitcoinconsensus version: " << bitcoinconsensus_version() << std::endl;

    CScript pubKeyScript;
    pubKeyScript << OP_1 << OP_0 << OP_1;

    int amount = 0; // 600000000;

    CScript scriptSig;
    CScriptWitness scriptWitness;
    CTransaction vanillaSpendTx = BuildSpendingTransaction(scriptSig, scriptWitness, amount);
    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << vanillaSpendTx;

    bitcoinconsensus_error err;
    auto op0Result = bitcoinconsensus_verify_script_with_amount(pubKeyScript.data(), pubKeyScript.size(), amount, (const unsigned char*)&stream[0], stream.size(), 0, bitcoinconsensus_SCRIPT_FLAGS_VERIFY_ALL, &err);
    std::cout << "Op0 result: " << op0Result << ", error code " << err << std::endl;

    getchar();

    return 0;
}
