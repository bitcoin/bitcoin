// Copyright (c) 2013 The Bitcoin Core developers
// Copyright (c) 2016 Tom Zander <tomz@freedommail.ch>
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "transaction_utils.h"
#include <primitives/transaction.h>
#include <random.h>

void TxUtils::RandomScript(CScript &script) {
    static const opcodetype oplist[] = {OP_FALSE, OP_1, OP_2, OP_3, OP_CHECKSIG, OP_IF, OP_VERIF, OP_RETURN, OP_CODESEPARATOR};
    script = CScript();
    int ops = (insecure_rand() % 10);
    for (int i=-5; i<ops; i++)
        script << oplist[insecure_rand() % (sizeof(oplist)/sizeof(oplist[0]))];
}

void TxUtils::RandomTransaction(CMutableTransaction &tx, bool fSingle) {
    tx.nVersion = 1;
    tx.vin.clear();
    tx.vout.clear();
    tx.nLockTime = (insecure_rand() % 2) ? insecure_rand() : 0;
    int ins = (insecure_rand() % 4) + 1;
    int outs = fSingle ? ins : (insecure_rand() % 4) + 1;
    for (int in = 0; in < ins; in++) {
        tx.vin.push_back(CTxIn());
        CTxIn &txin = tx.vin.back();
        txin.prevout.hash = GetRandHash();
        txin.prevout.n = insecure_rand() % 4;
        RandomScript(txin.scriptSig);
        txin.nSequence = (insecure_rand() % 2) ? insecure_rand() : (unsigned int)-1;
    }
    for (int out = 0; out < outs; out++) {
        tx.vout.push_back(CTxOut());
        CTxOut &txout = tx.vout.back();
        txout.nValue = insecure_rand() % 100000000;
        RandomScript(txout.scriptPubKey);
    }
}
