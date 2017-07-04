// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/server.h"
#include "rpc/client.h"
#include "rpc/rpcutil.h"
#include "util.h"

#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>

UniValue CallRPC(std::string args)
{
    std::vector<std::string> vArgs;
    boost::split(vArgs, args, boost::is_any_of(" \t"));
    std::string strMethod = vArgs[0];
    vArgs.erase(vArgs.begin());
    JSONRPCRequest request;
    request.strMethod = strMethod;
    request.params = RPCConvertValues(strMethod, vArgs);
    
    request.fHelp = false;
    const CRPCCommand *cmd = tableRPC[strMethod];
    
    if (!cmd)
        throw std::runtime_error(strprintf("CallRPC Unknown command, %s.", strMethod));
    rpcfn_type method = tableRPC[strMethod]->actor;
    try {
        return (*method)(request);
    }
    catch (const UniValue& objError) {
        throw std::runtime_error(find_value(objError, "message").get_str());
    }
};
