// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_WALLET_RPC_SPEND_H
#define SYSCOIN_WALLET_RPC_SPEND_H
#include <vector>
class RPCHelpMan;
class UniValue;
namespace wallet {
    class CWallet;
    class CCoinControl;
RPCHelpMan signrawtransactionwithwallet();
// SYSCOIN
RPCHelpMan send();
UniValue SendMoney(CWallet& wallet, const CCoinControl &coin_control, std::vector<CRecipient> &recipients, mapValue_t map_value, bool verbose);
}
#endif // SYSCOIN_WALLET_RPC_SPEND_H
