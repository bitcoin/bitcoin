// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_RAWTRANSACTION_H
#define BITCOIN_RPC_RAWTRANSACTION_H

#include <string>

struct RPCResult;

std::vector<RPCResult> DecodeTxDoc(const std::string& txid_field_doc);

#endif // BITCOIN_RPC_RAWTRANSACTION_H
