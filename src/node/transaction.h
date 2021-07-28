// Copyright (c) 2017-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TRANSACTION_H
#define BITCOIN_NODE_TRANSACTION_H

#include <attributes.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <util/error.h>

class CBlockIndex;
class CTxMemPool;
struct NodeContext;
namespace Consensus {
struct Params;
}

/** Maximum fee rate for sendrawtransaction and testmempoolaccept RPC calls.
 * Also used by the GUI when broadcasting a completed PSBT.
 * By default, a transaction with a fee rate higher than this will be rejected
 * by these RPCs and the GUI. This can be overridden with the maxfeerate argument.
 */
static const CFeeRate DEFAULT_MAX_RAW_TX_FEE_RATE{COIN / 10};

/**
 * Submit a transaction to the mempool and (optionally) relay it to all P2P peers.
 *
 * Mempool submission can be synchronous (will await mempool entry notification
 * over the CValidationInterface) or asynchronous (will submit and not wait for
 * notification), depending on the value of wait_callback. wait_callback MUST
 * NOT be set while cs_main, cs_mempool or cs_wallet are held to avoid
 * deadlock.
 *
 * @param[in]  node reference to node context
 * @param[in]  tx the transaction to broadcast
 * @param[out] err_string reference to std::string to fill with error string if available
 * @param[in]  max_tx_fee reject txs with fees higher than this (if 0, accept any fee)
 * @param[in]  relay flag if both mempool insertion and p2p relay are requested
 * @param[in]  wait_callback wait until callbacks have been processed to avoid stale result due to a sequentially RPC.
 * return error
 */
[[nodiscard]] TransactionError BroadcastTransaction(NodeContext& node, CTransactionRef tx, std::string& err_string, const CAmount& highfee, bool relay, bool wait_callback, bool bypass_limits = false);

/**
 * Return transaction with a given hash.
 * If mempool is provided and block_index is not provided, check it first for the tx.
 * If -txindex is available, check it next for the tx.
 * Finally, if block_index is provided, check for tx by reading entire block from disk.
 *
 * @param[in]  block_index     The block to read from disk, or nullptr
 * @param[in]  mempool         If provided, check mempool for tx
 * @param[in]  hash            The txid
 * @param[in]  consensusParams The params
 * @param[out] hashBlock       The block hash, if the tx was found via -txindex or block_index
 * @returns                    The tx if found, otherwise nullptr
 */
CTransactionRef GetTransaction(const CBlockIndex* const block_index, const CTxMemPool* const mempool, const uint256& hash, const Consensus::Params& consensusParams, uint256& hashBlock);

#endif // BITCOIN_NODE_TRANSACTION_H
