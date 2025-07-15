// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_EVO_UTIL_H
#define BITCOIN_RPC_EVO_UTIL_H

class UniValue;

template <typename T1>
void ProcessNetInfoCore(T1& ptx, const UniValue& input, const bool optional);

template <typename T1>
void ProcessNetInfoPlatform(T1& ptx, const UniValue& input_p2p, const UniValue& input_http);

#endif // BITCOIN_RPC_EVO_UTIL_H
