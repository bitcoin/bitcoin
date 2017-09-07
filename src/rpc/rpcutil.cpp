// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/server.h"
#include "rpc/client.h"
#include "rpc/rpcutil.h"
#include "util.h"

#include "support/events.h"



UniValue CallRPC(std::string args, std::string wallet)
{
    if (args.empty())
        throw std::runtime_error("No input.");
    
    std::vector<std::string> vArgs;
    
    bool fInQuotes = false;
    std::string s;
    for (size_t i = 0; i < args.size(); ++i)
    {
        char c = args[i];
        if (!fInQuotes
            && !s.empty()
            && (c == ' ' || c == '\t'))
        {
            vArgs.push_back(part::TrimQuotes(s));
            s.clear();
            continue;
        };
        
        if (c == '"' && (i == 0 || args[i-1] != '\\'))
            fInQuotes = !fInQuotes;
        
        s.push_back(c);
    };
    if (!s.empty())
        vArgs.push_back(part::TrimQuotes(s));
    
    
    std::string strMethod = vArgs[0];
    vArgs.erase(vArgs.begin());
    JSONRPCRequest request;
    request.strMethod = strMethod;
    request.params = RPCConvertValues(strMethod, vArgs);
    
    if (!wallet.empty()) {
        char *encodedURI = evhttp_uriencode(wallet.c_str(), wallet.size(), false);
        if (encodedURI) {
            request.URI = "/wallet/"+ std::string(encodedURI);
            free(encodedURI);
        } else {
            throw std::runtime_error("uri-encode failed");
        }
    }
    
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
