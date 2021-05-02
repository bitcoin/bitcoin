// Copyright (c) 2019 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef XBIT_TEST_UTIL_WALLET_H
#define XBIT_TEST_UTIL_WALLET_H

#include <string>

class CWallet;

// Constants //

extern const std::string ADDRESS_BCRT1_UNSPENDABLE;

// RPC-like //

/** Import the address to the wallet */
void importaddress(CWallet& wallet, const std::string& address);
/** Returns a new address from the wallet */
std::string getnewaddress(CWallet& w);


#endif // XBIT_TEST_UTIL_WALLET_H
