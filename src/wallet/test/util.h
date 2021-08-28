// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_WALLET_TEST_UTIL_H
#define SYSCOIN_WALLET_TEST_UTIL_H

#include <memory>

class CChain;
class CKey;
class CWallet;
namespace interfaces {
class Chain;
} // namespace interfaces

std::unique_ptr<CWallet> CreateSyncedWallet(interfaces::Chain& chain, CChain& cchain, const CKey& key);

#endif // SYSCOIN_WALLET_TEST_UTIL_H
