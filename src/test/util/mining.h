// Copyright (c) 2019 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef XBIT_TEST_UTIL_MINING_H
#define XBIT_TEST_UTIL_MINING_H

#include <memory>
#include <string>

class CBlock;
class CScript;
class CTxIn;
struct NodeContext;

/** Returns the generated coin */
CTxIn MineBlock(const NodeContext&, const CScript& coinbase_scriptPubKey);

/** Prepare a block to be mined */
std::shared_ptr<CBlock> PrepareBlock(const NodeContext&, const CScript& coinbase_scriptPubKey);

/** RPC-like helper function, returns the generated coin */
CTxIn generatetoaddress(const NodeContext&, const std::string& address);

#endif // XBIT_TEST_UTIL_MINING_H
