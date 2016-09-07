// Copyright (c) 2016 Tom Zander <tomz@freedommail.ch>
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_TRANSACTION_UTILS_H
#define BITCOIN_TEST_TRANSACTION_UTILS_H

class CScript;
class CMutableTransaction;

namespace TxUtils {
    void RandomScript(CScript &script);
    void RandomTransaction(CMutableTransaction &tx, bool single);
}

#endif
