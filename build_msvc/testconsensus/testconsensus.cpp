// Copyright (c) 2018-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>

// bitcoin includes.
#include <..\src\script\bitcoinconsensus.h>
#include <..\src\primitives\transaction.h>
#include <..\src\script\script.h>
#include <..\src\streams.h>
#include <..\src\version.h>

CMutableTransaction BuildSpendingTransaction(const CScript& scriptSig, const CScriptWitness& scriptWitness, int nValue = 0)
{
    CMutableTransaction txSpend;
    txSpend.nVersion = 1;
    txSpend.nLockTime = 0;
    txSpend.vin.resize(1);
    txSpend.vout.resize(1);
    txSpend.vin[0].scriptWitness = scriptWitness;
    txSpend.vin[0].prevout.hash = uint256();
    txSpend.vin[0].prevout.n = 0;
    txSpend.vin[0].scriptSig = scriptSig;
    txSpend.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    txSpend.vout[0].scriptPubKey = CScript();
    txSpend.vout[0].nValue = nValue;

    return txSpend;
}

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
    auto op0Result = bitcoinconsensus_verify_script_with_amount(pubKeyScript.data(), pubKeyScript.size(), amount, stream.data(), stream.size(), 0, bitcoinconsensus_SCRIPT_FLAGS_VERIFY_ALL, &err);
    std::cout << "Op0 result: " << op0Result << ", error code " << err << std::endl;

    getchar();

    return 0;
}
