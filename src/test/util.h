// Copyright (c) 2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_TEST_UTIL_H
#define SYSCOIN_TEST_UTIL_H

#include <memory>
#include <string>

class CBlock;
class CScript;
class CTxIn;
class CWallet;

// Constants //

extern const std::string ADDRESS_BCRT1_UNSPENDABLE;

// Lower-level utils //

/** Returns the generated coin */
CTxIn MineBlock(const CScript& coinbase_scriptPubKey);
/** Prepare a block to be mined */
std::shared_ptr<CBlock> PrepareBlock(const CScript& coinbase_scriptPubKey);


// RPC-like //

/** Import the address to the wallet */
void importaddress(CWallet& wallet, const std::string& address);
/** Returns a new address from the wallet */
std::string getnewaddress(CWallet& w);
/** Returns the generated coin */
CTxIn generatetoaddress(const std::string& address);


#endif // SYSCOIN_TEST_UTIL_H
