// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPCCLIENT_H
#define BITCOIN_RPCCLIENT_H

#include "univalue/univalue.h"

UniValue RPCConvertValues(const std::string& strMethod, const std::vector<std::string>& strParams);
/** Non-RFC4627 JSON parser, accepts internal values (such as numbers, true, false, null)
 * as well as objects and arrays.
 */
UniValue ParseNonRFCJSONValue(const std::string& strVal);
/**
 * Add new conversion to the JSON RPC conversion table
 * @param method name of the method
 * @param idx    0-based index of the paramter
 */
void RPCAddConversion(const std::string& method, int idx);

/**
 * Convert strings to command-specific RPC representation
 **/
UniValue RPCConvertValues(const std::string& strMethod, const std::vector<std::string>& strParams);

#endif // BITCOIN_RPCCLIENT_H
