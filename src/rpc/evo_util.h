// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_EVO_UTIL_H
#define BITCOIN_RPC_EVO_UTIL_H

class UniValue;

/** Process setting (legacy) Core network information field based on ProTx version */
template <typename ProTx>
void ProcessNetInfoCore(ProTx& ptx, const UniValue& input, const bool optional);

/** Process setting (legacy) Platform network information fields based on ProTx version */
template <typename ProTx>
void ProcessNetInfoPlatform(ProTx& ptx, const UniValue& input_p2p, const UniValue& input_http);

#endif // BITCOIN_RPC_EVO_UTIL_H
