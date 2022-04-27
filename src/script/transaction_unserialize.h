// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_TRANSACTION_UNSERIALIZE_H
#define BITCOIN_SCRIPT_TRANSACTION_UNSERIALIZE_H

#include <primitives/transaction.h>

#include <cstddef>

namespace bitcoinconsensus {
CTransaction UnserializeTx(const unsigned char* txTo, unsigned int txToLen);
size_t TxSize(const CTransaction& tx);
} // namespace bitcoinconsensus

#endif // BITCOIN_SCRIPT_TRANSACTION_UNSERIALIZE_H
