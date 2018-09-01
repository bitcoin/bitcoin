// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/rpccategory.h>

#include <cassert>

namespace rpccategory
{

std::string Label(const RPCCategory category)
{
    switch (category) {
        case RPCCategory::BLOCKCHAIN: return "Blockchain";
        case RPCCategory::CONTROL: return "Control";
        case RPCCategory::GENERATING: return "Generating";
        case RPCCategory::HIDDEN: return "Hidden";
        case RPCCategory::MINING: return "Mining";
        case RPCCategory::NETWORK: return "Network";
        case RPCCategory::RAWTRANSACTIONS: return "Rawtransactions";
        case RPCCategory::UTIL: return "Util";
        case RPCCategory::WALLET: return "Wallet";
        case RPCCategory::ZMQ: return "Zmq";
        case RPCCategory::TEST: return "Test";
    }
    assert(false);
}

} // namespace rpccategory
