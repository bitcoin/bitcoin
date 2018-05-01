// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_RPCCLIENT_H
#define SYSCOIN_RPCCLIENT_H

#include <univalue.h>
class JSONRPCRequest;
/** Convert positional arguments to command-specific RPC representation */
UniValue RPCConvertValues(const std::string& strMethod, const std::vector<std::string>& strParams);

/** Convert named arguments to command-specific RPC representation */
UniValue RPCConvertNamedValues(const std::string& strMethod, const std::vector<std::string>& strParams);

/** Non-RFC4627 JSON parser, accepts internal values (such as numbers, true, false, null)
 * as well as objects and arrays.
 */
UniValue ParseNonRFCJSONValue(const std::string& strVal);
extern UniValue getaddressbalance(const JSONRPCRequest& request);
extern UniValue getaddressutxos(const JSONRPCRequest& request);
#endif // SYSCOIN_RPCCLIENT_H
