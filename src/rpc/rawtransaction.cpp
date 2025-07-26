// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/amount.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <evo/creditpool.h>
#include <index/txindex.h>
#include <init.h>
#include <key_io.h>
#include <node/blockstorage.h>
#include <node/coin.h>
#include <node/context.h>
#include <node/psbt.h>
#include <node/transaction.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <primitives/transaction.h>
#include <psbt.h>
#include <rpc/blockchain.h>
#include <rpc/index_util.h>
#include <rpc/rawtransaction_util.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/bip32.h>
#include <util/check.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/translation.h>
#include <validation.h>
#include <validationinterface.h>
#include <util/irange.h>

#include <evo/cbtx.h>
#include <evo/specialtx.h>
#include <instantsend/lock.h>
#include <instantsend/instantsend.h>
#include <llmq/chainlocks.h>
#include <llmq/context.h>

#include <numeric>
#include <stdint.h>

#include <univalue.h>

using node::AnalyzePSBT;
using node::DEFAULT_MAX_RAW_TX_FEE_RATE;
using node::GetTransaction;
using node::NodeContext;
using node::PSBTAnalysis;

void TxToJSON(const CTransaction& tx, const uint256 hashBlock, const  CTxMemPool& mempool, const CChainState& active_chainstate, const llmq::CChainLocksHandler& clhandler, const llmq::CInstantSendManager& isman, UniValue& entry, TxVerbosity verbosity = TxVerbosity::SHOW_DETAILS)
{
    CHECK_NONFATAL(verbosity >= TxVerbosity::SHOW_DETAILS);
    LOCK(::cs_main);
    // Call into TxToUniv() in bitcoin-common to decode the transaction hex.
    //
    // Blockchain contextual information (confirmations and blocktime) is not
    // available to code in bitcoin-common, so we query them here and push the
    // data into the returned UniValue.

    uint256 txid = tx.GetHash();
    CSpentIndexTxInfo *txSpentInfoPtr{nullptr};

    // Add spent information if spentindex is enabled
    CSpentIndexTxInfo txSpentInfo;
    if (node::fSpentIndex) {
        txSpentInfo = CSpentIndexTxInfo{};
        for (const auto& txin : tx.vin) {
            if (!tx.IsCoinBase()) {
                CSpentIndexValue spentInfo;
                CSpentIndexKey spentKey(txin.prevout.hash, txin.prevout.n);
                if (GetSpentIndex(*active_chainstate.m_blockman.m_block_tree_db, mempool, spentKey, spentInfo)) {
                    txSpentInfo.mSpentInfo.emplace(spentKey, spentInfo);
                }
            }
        }
        for (unsigned int i = 0; i < tx.vout.size(); i++) {
            CSpentIndexValue spentInfo;
            CSpentIndexKey spentKey(txid, i);
            if (GetSpentIndex(*active_chainstate.m_blockman.m_block_tree_db, mempool, spentKey, spentInfo)) {
                txSpentInfo.mSpentInfo.emplace(spentKey, spentInfo);
            }
        }
        txSpentInfoPtr = &txSpentInfo;
    }

    TxToUniv(tx, /*block_hash=*/uint256(), entry, /*include_hex=*/true, /*serialize_flags=*/0, /*txundo=*/nullptr, verbosity, txSpentInfoPtr);

    bool chainLock = false;
    if (!hashBlock.IsNull()) {
        entry.pushKV("blockhash", hashBlock.GetHex());
        const CBlockIndex* pindex = active_chainstate.m_blockman.LookupBlockIndex(hashBlock);
        if (pindex) {
            if (active_chainstate.m_chain.Contains(pindex)) {
                entry.pushKV("height", pindex->nHeight);
                entry.pushKV("confirmations", 1 + active_chainstate.m_chain.Height() - pindex->nHeight);
                entry.pushKV("time", pindex->GetBlockTime());
                entry.pushKV("blocktime", pindex->GetBlockTime());
                chainLock = clhandler.HasChainLock(pindex->nHeight, pindex->GetBlockHash());
            } else {
                entry.pushKV("height", -1);
                entry.pushKV("confirmations", 0);
            }
        }
    }

    bool fLocked = isman.IsLocked(txid);
    entry.pushKV("instantlock", fLocked || chainLock);
    entry.pushKV("instantlock_internal", fLocked);
    entry.pushKV("chainlock", chainLock);
}

static std::vector<RPCArg> CreateTxDoc()
{
    return {
        {"inputs", RPCArg::Type::ARR, RPCArg::Optional::NO, "The inputs",
            {
                {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                    {
                        {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                        {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                        {"sequence", RPCArg::Type::NUM, RPCArg::DefaultHint{"depends on the value of the 'locktime' argument"}, "The sequence number"},
                    },
                },
            },
        },
        {"outputs", RPCArg::Type::ARR, RPCArg::Optional::NO, "The outputs (key-value pairs), where none of the keys are duplicated.\n"
                "That is, each address can only appear once and there can only be one 'data' object.\n"
                "For compatibility reasons, a dictionary, which holds the key-value pairs directly, is also\n"
                "                             accepted as second parameter.",
            {
                {"", RPCArg::Type::OBJ_USER_KEYS, RPCArg::Optional::OMITTED, "",
                    {
                        {"address", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "A key-value pair. The key (string) is the Dash address, the value (float or string) is the amount in " + CURRENCY_UNIT},
                    },
                },
                {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                    {
                        {"data", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "A key-value pair. The key must be \"data\", the value is hex-encoded data"},
                    },
                },
            },
        },
        {"locktime", RPCArg::Type::NUM, RPCArg::Default{0}, "Raw locktime. Non-0 value also locktime-activates inputs"},
    };
}

static RPCHelpMan getrawtransaction()
{
    return RPCHelpMan{
                "getrawtransaction",
                "\nReturn the raw transaction data.\n"

                "\nBy default, this call only returns a transaction if it is in the mempool. If -txindex is enabled\n"
                "and no blockhash argument is passed, it will return the transaction if it is in the mempool or any block.\n"
                "If a blockhash argument is passed, it will return the transaction if\n"
                "the specified block is available and the transaction is in that block.\n"
                "\nHint: Use gettransaction for wallet transactions.\n"

            "\nIf verbose is 'true', returns an Object with information about 'txid'.\n"
            "If verbose is 'false' or omitted, returns a string that is serialized, hex-encoded data for 'txid'.\n",
                {
                    {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                    {"verbose", RPCArg::Type::BOOL, RPCArg::Default{false}, "If false, return a string, otherwise return a json object"},
                    {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED_NAMED_ARG, "The block in which to look for the transaction"},
                },
                {
                    RPCResult{"if verbose is not set or set to false",
                         RPCResult::Type::STR, "data", "The serialized, hex-encoded data for 'txid'"
                    },
                    RPCResult{"if verbose is set to true",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::BOOL, "in_active_chain", /* optional */ true, "Whether specified block is in the active chain or not (only present with explicit \"blockhash\" argument)"},
                            {RPCResult::Type::STR_HEX, "txid", "The transaction id (same as provided)"},
                            {RPCResult::Type::NUM, "size", "The serialized transaction size"},
                            {RPCResult::Type::NUM, "version", "The version"},
                            {RPCResult::Type::NUM, "type", "The type"},
                            {RPCResult::Type::NUM_TIME, "locktime", "The lock time"},
                            {RPCResult::Type::ARR, "vin", "",
                            {
                                {RPCResult::Type::OBJ, "", "",
                                {
                                    {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
                                    {RPCResult::Type::NUM, "vout", "The output number"},
                                    {RPCResult::Type::OBJ, "scriptSig", "The script",
                                    {
                                        {RPCResult::Type::STR, "asm", "asm"},
                                        {RPCResult::Type::STR_HEX, "hex", "hex"},
                                    }},
                                    {RPCResult::Type::NUM, "sequence", "The script sequence number"},
                                }},
                            }},
                            {RPCResult::Type::ARR, "vout", "",
                            {
                                {RPCResult::Type::OBJ, "", "",
                                {
                                    {RPCResult::Type::NUM, "value", "The value in " + CURRENCY_UNIT},
                                    {RPCResult::Type::NUM, "n", "index"},
                                    {RPCResult::Type::OBJ, "scriptPubKey", "",
                                    {
                                        {RPCResult::Type::STR, "asm", "the asm"},
                                        {RPCResult::Type::STR, "desc", "Inferred descriptor for the output"},
                                        {RPCResult::Type::STR, "hex", "the hex"},
                                        {RPCResult::Type::STR, "type", "The type, eg 'pubkeyhash'"},
                                        {RPCResult::Type::STR, "address", /* optional */ true, "The Dash address (only if a well-defined address exists)"},
                                    }},
                                }},
                            }},
                            {RPCResult::Type::NUM, "extraPayloadSize", true /*optional*/, "Size of DIP2 extra payload. Only present if it's a special TX"},
                            {RPCResult::Type::STR_HEX, "extraPayload", true /*optional*/, "Hex-encoded DIP2 extra payload data. Only present if it's a special TX"},
                            {RPCResult::Type::STR_HEX, "hex", "The serialized, hex-encoded data for 'txid'"},
                            {RPCResult::Type::STR_HEX, "blockhash", /* optional */ true, "the block hash"},
                            {RPCResult::Type::NUM, "height", "The block height"},
                            {RPCResult::Type::NUM, "confirmations", /* optional */ true, "The confirmations"},
                            {RPCResult::Type::NUM_TIME, "blocktime", /* optional */ true, "The block time expressed in " + UNIX_EPOCH_TIME},
                            {RPCResult::Type::NUM, "time", /* optional */ true, "Same as \"blocktime\""},
                            {RPCResult::Type::BOOL, "instantlock", "Current transaction lock state"},
                            {RPCResult::Type::BOOL, "instantlock_internal", "Current internal transaction lock state"},
                            {RPCResult::Type::BOOL, "chainlock", "The state of the corresponding block ChainLock"},
                        }
                    },
                },
                RPCExamples{
                    HelpExampleCli("getrawtransaction", "\"mytxid\"")
            + HelpExampleCli("getrawtransaction", "\"mytxid\" true")
            + HelpExampleRpc("getrawtransaction", "\"mytxid\", true")
            + HelpExampleCli("getrawtransaction", "\"mytxid\" false \"myblockhash\"")
            + HelpExampleCli("getrawtransaction", "\"mytxid\" true \"myblockhash\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    ChainstateManager& chainman = EnsureChainman(node);

    bool in_active_chain = true;
    uint256 hash = ParseHashV(request.params[0], "parameter 1");
    const CBlockIndex* blockindex = nullptr;

    if (hash == Params().GenesisBlock().hashMerkleRoot) {
        // Special exception for the genesis block coinbase transaction
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "The genesis block coinbase is not considered an ordinary transaction and cannot be retrieved");
    }

    // Accept either a bool (true) or a num (>=1) to indicate verbose output.
    bool fVerbose = false;
    if (!request.params[1].isNull()) {
        fVerbose = request.params[1].isNum() ? (request.params[1].get_int() != 0) : request.params[1].get_bool();
    }

    if (!request.params[2].isNull()) {
        LOCK(cs_main);

        uint256 blockhash = ParseHashV(request.params[2], "parameter 3");
        blockindex = chainman.m_blockman.LookupBlockIndex(blockhash);
        if (!blockindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block hash not found");
        }
        in_active_chain = chainman.ActiveChain().Contains(blockindex);
    }

    bool f_txindex_ready = false;
    if (g_txindex && !blockindex) {
        f_txindex_ready = g_txindex->BlockUntilSyncedToCurrentChain();
    }

    uint256 hash_block;
    const CTransactionRef tx = GetTransaction(blockindex, node.mempool.get(), hash, Params().GetConsensus(), hash_block);
    if (!tx) {
        std::string errmsg;
        if (blockindex) {
            const bool block_has_data = WITH_LOCK(::cs_main, return blockindex->nStatus & BLOCK_HAVE_DATA);
            if (!block_has_data) {
                throw JSONRPCError(RPC_MISC_ERROR, "Block not available");
            }
            errmsg = "No such transaction found in the provided block";
        } else if (!g_txindex) {
            errmsg = "No such mempool transaction. Use -txindex or provide a block hash to enable blockchain transaction queries";
        } else if (!f_txindex_ready) {
            errmsg = "No such mempool transaction. Blockchain transactions are still in the process of being indexed";
        } else {
            errmsg = "No such mempool or blockchain transaction";
        }
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, errmsg + ". Use gettransaction for wallet transactions.");
    }

    if (!fVerbose) {
        return EncodeHexTx(*tx);
    }

    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    const CTxMemPool& mempool = EnsureMemPool(node);

    UniValue result(UniValue::VOBJ);
    if (blockindex) result.pushKV("in_active_chain", in_active_chain);
    TxToJSON(*tx, hash_block, mempool, chainman.ActiveChainstate(), *llmq_ctx.clhandler, *llmq_ctx.isman, result);
    return result;
},
    };
}

static RPCHelpMan getrawtransactionmulti() {
    return RPCHelpMan{
            "getrawtransactionmulti",
            "\nReturns the raw transaction data for multiple transactions.\n"
            "\nThis call is an extension of getrawtransaction that supports multiple transactions.\n"
            "It accepts a map of block hashes to a list of transaction hashes.\n"
            "A block hash of 0 indicates transactions not yet mined or in the mempool.\n",
            {
                    {"transactions", RPCArg::Type::OBJ, RPCArg::Optional::NO,
                     "A JSON object with block hashes as keys and lists of transaction hashes as values (no more than 100 in total)",
                     {
                             {"blockhash", RPCArg::Type::ARR, RPCArg::Optional::OMITTED,
                              "The block hash and the list of transaction ids to fetch",
                              {
                                      {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The transaction id"},
                              }},
                     }},
                    {"verbose", RPCArg::Type::BOOL, RPCArg::Default{false},
                     "If false, return a string, otherwise return a json object"},
            },
            // TODO: replace RPCResults to proper annotation
            RPCResults{},
            RPCExamples{
                    HelpExampleCli("getrawtransactionmulti",
                                   R"('{"blockhash1":["txid1","txid2"], "0":["txid3"]}')")
                    + HelpExampleRpc("getrawtransactionmulti",
                                     R"('{"blockhash1":["txid1","txid2"], "0":["txid3"]})")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // Parse arguments
    UniValue transactions{request.params[0].get_obj()};
    // Accept either a bool (true) or a num (>=1) to indicate verbose output.
    bool fVerbose{false};
    if (!request.params[1].isNull()) {
        fVerbose = request.params[1].isNum() ? (request.params[1].get_int() != 0) : request.params[1].get_bool();
    }

    const NodeContext& node{EnsureAnyNodeContext(request.context)};
    const ChainstateManager& chainman{EnsureChainman(node)};
    const LLMQContext& llmq_ctx{EnsureLLMQContext(node)};
    CTxMemPool& mempool{EnsureMemPool(node)};

    if (transactions.size() > 100) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Up to 100 blocks and txids only");
    }

    size_t count{0};
    UniValue result(UniValue::VOBJ);
    for (const std::string& blockhash_str : transactions.getKeys()) {
        const uint256 blockhash{uint256S(blockhash_str)};
        const UniValue txids = transactions[blockhash_str].get_array();

        const CBlockIndex* blockindex{blockhash.IsNull() ? nullptr : WITH_LOCK(::cs_main, return chainman.m_blockman.LookupBlockIndex(blockhash))};
        if (blockindex == nullptr && !blockhash.IsNull()) {
            for (const auto idx : irange::range(txids.size())) {
                result.pushKV(txids[idx].get_str(), "None");
            }
            continue;
        }

        count += txids.size();
        if (count > 100) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Up to 100 txids in total");
        }
        for (const auto idx : irange::range(txids.size())) {
            const std::string txid_str = txids[idx].get_str();
            const uint256 txid = ParseHashV(txid_str, "transaction id");

            uint256 hash_block;
            const CTransactionRef tx = GetTransaction(blockindex, &mempool, txid, Params().GetConsensus(), hash_block);
            if (!tx) {
                result.pushKV(txid_str, "None");
            } else if (fVerbose) {
                UniValue tx_data{UniValue::VOBJ};
                TxToJSON(*tx, hash_block, mempool, chainman.ActiveChainstate(), *llmq_ctx.clhandler, *llmq_ctx.isman, tx_data);
                result.pushKV(txid_str, tx_data);
            } else {
                result.pushKV(txid_str, EncodeHexTx(*tx));
            }
        }
    }
    return result;
},
    };
}

static RPCHelpMan getislocks()
{
    return RPCHelpMan{"getislocks",
        "\nReturns the raw InstantSend lock data for each txids. Returns Null if there is no known IS yet.",
        {
            {"txids", RPCArg::Type::ARR, RPCArg::Optional::NO, "The transaction ids (no more than 100)",
                {
                    {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A transaction hash"},
                },
            },
        },
        RPCResult{
            RPCResult::Type::ARR, "", "Response is an array with the same size as the input txids",
                {{RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
                    {RPCResult::Type::ARR, "inputs", "The inputs",
                    {
                        {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
                                {RPCResult::Type::NUM, "vout", "The output number"},
                            },
                        },
                    }},
                    {RPCResult::Type::STR_HEX, "id", "Request ID"},
                    {RPCResult::Type::STR_HEX, "cycleHash", "The Cycle Hash"},
                    {RPCResult::Type::STR_HEX, "signature", "The InstantSend's BLS signature"},
                    {RPCResult::Type::STR_HEX, "hex", "The serialized, hex-encoded data for 'txid'"},
                }},
                RPCResult{"if no InstantSend Lock is known for specified txid",
                     RPCResult::Type::STR, "data", "Just 'None' string"
                },
            }},
        RPCExamples{
            HelpExampleCli("getislocks", "'[\"txid\",...]'")
            + HelpExampleRpc("getislocks", "'[\"txid\",...]'")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);

    UniValue result_arr(UniValue::VARR);
    UniValue txids = request.params[0].get_array();
    if (txids.size() > 100) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Up to 100 txids only");
    }

    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    for (const auto idx : irange::range(txids.size())) {
        const uint256 txid(ParseHashV(txids[idx], "txid"));

        if (const instantsend::InstantSendLockPtr islock = llmq_ctx.isman->GetInstantSendLockByTxid(txid); islock != nullptr) {
            UniValue objIS(UniValue::VOBJ);
            objIS.pushKV("txid", islock->txid.ToString());
            UniValue inputs(UniValue::VARR);
            for (const auto out : islock->inputs) {
                UniValue outpoint(UniValue::VOBJ);
                outpoint.pushKV("txid", out.hash.ToString());
                outpoint.pushKV("vout", static_cast<int64_t>(out.n));
                inputs.push_back(outpoint);
            }
            objIS.pushKV("inputs", inputs);
            objIS.pushKV("id", islock->GetRequestId().ToString());
            objIS.pushKV("cycleHash", islock->cycleHash.ToString());
            objIS.pushKV("signature", islock->sig.ToString());
            {
                CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
                ssTx << *islock;
                objIS.pushKV("hex", HexStr(ssTx));
            }
            result_arr.push_back(objIS);
        } else {
            result_arr.push_back("None");
        }
    }
    return result_arr;

},
    };
}

static RPCHelpMan gettxchainlocks()
{
    return RPCHelpMan{
        "gettxchainlocks",
        "\nReturns the block height at which each transaction was mined, and indicates whether it is in the mempool, ChainLocked, or neither.\n",
        {
            {"txids", RPCArg::Type::ARR, RPCArg::Optional::NO, "The transaction ids (no more than 100)",
                {
                    {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A transaction hash"},
                },
            },
        },
        RPCResult{
            RPCResult::Type::ARR, "", "Response is an array with the same size as the input txids",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::NUM, "height", "The block height"},
                    {RPCResult::Type::BOOL, "chainlock", "The state of the corresponding block ChainLock"},
                    {RPCResult::Type::BOOL, "mempool", "Mempool status for the transaction"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("gettxchainlocks", "'[\"mytxid\",...]'")
        + HelpExampleRpc("gettxchainlocks", "[\"mytxid\",...]")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    const ChainstateManager& chainman = EnsureChainman(node);
    const CChainState& active_chainstate = chainman.ActiveChainstate();

    UniValue result_arr(UniValue::VARR);
    UniValue txids = request.params[0].get_array();
    if (txids.size() > 100) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Up to 100 txids only");
    }

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    for (const auto idx : irange::range(txids.size())) {
        UniValue result(UniValue::VOBJ);
        const uint256 txid(ParseHashV(txids[idx], "txid"));
        if (txid == Params().GenesisBlock().hashMerkleRoot) {
            // Special exception for the genesis block coinbase transaction
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "The genesis block coinbase is not considered an ordinary transaction and cannot be retrieved");
        }

        uint256 hash_block;
        int height{-1};
        bool chainLock{false};

        const auto tx_ref = GetTransaction(nullptr, node.mempool.get(), txid, Params().GetConsensus(), hash_block);

        if (tx_ref == nullptr) {
            result.pushKV("height", -1);
            result.pushKV("chainlock", false);
            result.pushKV("mempool", false);
            result_arr.push_back(result);
            continue;
        }

        if (!hash_block.IsNull()) {
            LOCK(cs_main);
            const CBlockIndex* pindex = active_chainstate.m_blockman.LookupBlockIndex(hash_block);
            if (pindex && active_chainstate.m_chain.Contains(pindex)) {
                height = pindex->nHeight;
            }
        }
        if (height != -1) {
            chainLock = llmq_ctx.clhandler->HasChainLock(height, hash_block);
        }
        result.pushKV("height", height);
        result.pushKV("chainlock", chainLock);
        result.pushKV("mempool", height == -1);
        result_arr.push_back(result);
    }
    return result_arr;
},
    };
}

static RPCHelpMan getassetunlockstatuses()
{
    return RPCHelpMan{
            "getassetunlockstatuses",
            "\nReturns the status of given Asset Unlock indexes at the tip of the chain or at a specific block height if specified.\n",
            {
                    {"indexes", RPCArg::Type::ARR, RPCArg::Optional::NO, "The Asset Unlock indexes (no more than 100)",
                     {
                             {"index", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "An Asset Unlock index"},
                     },
                    },
                    {"height", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The maximum block height to check"},
            },
            RPCResult{
                    RPCResult::Type::ARR, "", "Response is an array with the same size as the input txids",
                    {
                            {RPCResult::Type::OBJ, "", "",
                             {
                                {RPCResult::Type::NUM, "index", "The Asset Unlock index"},
                                {RPCResult::Type::STR, "status", "Status of the Asset Unlock index: {chainlocked|mined|mempooled|unknown}"},
                             }},
                    }
            },
            RPCExamples{
                    HelpExampleCli("getassetunlockstatuses", "'[\"myindex\",...]'")
                    + HelpExampleRpc("getassetunlockstatuses", "[\"myindex\",...]")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const CTxMemPool& mempool = EnsureMemPool(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    const ChainstateManager& chainman = EnsureChainman(node);

    UniValue result_arr(UniValue::VARR);
    const UniValue str_indexes = request.params[0].get_array();
    if (str_indexes.size() > 100) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Up to 100 indexes only");
    }

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    const CBlockIndex* pTipBlockIndex{WITH_LOCK(cs_main, return chainman.ActiveChain().Tip())};

    if (!pTipBlockIndex) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No blocks in chain");
    }

    std::optional<CCreditPool> poolCL{std::nullopt};
    std::optional<CCreditPool> poolOnTip{std::nullopt};
    std::optional<int> nSpecificCoreHeight{std::nullopt};

    if (!request.params[1].isNull()) {
        nSpecificCoreHeight = request.params[1].get_int();
        if (nSpecificCoreHeight.value() < 0 || nSpecificCoreHeight.value() > chainman.ActiveChain().Height()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
        }
        poolCL = std::make_optional(CHECK_NONFATAL(node.cpoolman)->GetCreditPool(chainman.ActiveChain()[nSpecificCoreHeight.value()], Params().GetConsensus()));
    }
    else {
        const auto pBlockIndexBestCL = [&]() -> const CBlockIndex* {
            if (!llmq_ctx.clhandler->GetBestChainLock().IsNull()) {
                return pTipBlockIndex->GetAncestor(llmq_ctx.clhandler->GetBestChainLock().getHeight());
            }
            // If no CL info is available, try to use CbTx CL information
            if (const auto cbtx_best_cl = GetNonNullCoinbaseChainlock(pTipBlockIndex)) {
                return pTipBlockIndex->GetAncestor(pTipBlockIndex->nHeight - cbtx_best_cl->second - 1);
            }
            // no CL info, no CbTx CL
            return nullptr;
        }();

        // We need in 2 credit pools: at tip of chain and on best CL to know if tx is mined or chainlocked
        // Sometimes that's two different blocks, sometimes not and we need to initialize 2nd creditPoolManager
        poolCL = pBlockIndexBestCL ?
                 std::make_optional(node.cpoolman->GetCreditPool(pBlockIndexBestCL, Params().GetConsensus())) :
                 std::nullopt;

        poolOnTip = [&]() -> std::optional<CCreditPool> {
            if (pTipBlockIndex != pBlockIndexBestCL) {
                return std::make_optional(node.cpoolman->GetCreditPool(pTipBlockIndex, Params().GetConsensus()));
            }
            return std::nullopt;
        }();
    }

    for (const auto i : irange::range(str_indexes.size())) {
        UniValue obj(UniValue::VOBJ);
        uint64_t index{};
        if (!ParseUInt64(str_indexes[i].get_str(), &index)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid index");
        }
        obj.pushKV("index", index);
        auto status_to_push = [&]() -> std::string {
            if (poolCL.has_value() && poolCL->indexes.Contains(index)) {
                return "chainlocked";
            }
            if (poolOnTip.has_value() && poolOnTip->indexes.Contains(index)) {
                return "mined";
            }
            bool is_mempooled = [&]() {
                LOCK(mempool.cs);
                return std::any_of(mempool.mapTx.begin(), mempool.mapTx.end(), [index](const CTxMemPoolEntry &e) {
                    if (e.GetTx().nType == CAssetUnlockPayload::SPECIALTX_TYPE) {
                        if (auto opt_assetUnlockTx = GetTxPayload<CAssetUnlockPayload>(e.GetTx())) {
                            return index == opt_assetUnlockTx->getIndex();
                        } else {
                            throw JSONRPCError(RPC_TRANSACTION_ERROR, "bad-assetunlocktx-payload");
                        }
                    }
                    return false;
                });
            }();
            return is_mempooled && !nSpecificCoreHeight.has_value() ? "mempooled" : "unknown";
        };
        obj.pushKV("status", status_to_push());
        result_arr.push_back(obj);
    }

    return result_arr;
},
    };
}

static RPCHelpMan createrawtransaction()
{
    return RPCHelpMan{"createrawtransaction",
        "\nCreate a transaction spending the given inputs and creating new outputs.\n"
        "Outputs can be addresses or data.\n"
        "Returns hex-encoded raw transaction.\n"
        "Note that the transaction's inputs are not signed, and\n"
        "it is not stored in the wallet or transmitted to the network.\n",
        CreateTxDoc(),
        RPCResult{
            RPCResult::Type::STR_HEX, "transaction", "hex string of the transaction"
        },
        RPCExamples{
            HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"[{\\\"address\\\":0.01}]\"")
    + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"[{\\\"data\\\":\\\"00010203\\\"}]\"")
    + HelpExampleRpc("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"[{\\\"address\\\":0.01}]\"")
    + HelpExampleRpc("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"[{\\\"data\\\":\\\"00010203\\\"}]\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {
        UniValue::VARR,
        UniValueType(), // ARR or OBJ, checked later
        UniValue::VNUM,
        }, true
    );

    CMutableTransaction rawTx = ConstructTransaction(request.params[0], request.params[1], request.params[2]);

    return EncodeHexTx(CTransaction(rawTx));
},
    };
}

static RPCHelpMan decoderawtransaction()
{
    return RPCHelpMan{"decoderawtransaction",
                "\nReturn a JSON object representing the serialized, hex-encoded transaction.\n",
                {
                    {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction hex string"},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
                        {RPCResult::Type::NUM, "size", "The transaction size"},
                        {RPCResult::Type::NUM, "version", "The version"},
                        {RPCResult::Type::NUM, "type", "The type"},
                        {RPCResult::Type::NUM_TIME, "locktime", "The lock time"},
                        {RPCResult::Type::ARR, "vin", "",
                        {
                            {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR_HEX, "coinbase", /* optional */ true, ""},
                                {RPCResult::Type::STR_HEX, "txid", /* optional */ true, "The transaction id"},
                                {RPCResult::Type::NUM, "vout", /* optional */ true, "The output number"},
                                {RPCResult::Type::OBJ, "scriptSig", /* optional */ true, "The script",
                                {
                                    {RPCResult::Type::STR, "asm", "asm"},
                                    {RPCResult::Type::STR_HEX, "hex", "hex"},
                                }},
                                {RPCResult::Type::NUM, "sequence", "The script sequence number"},
                            }},
                        }},
                        {RPCResult::Type::ARR, "vout", "",
                        {
                            {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::NUM, "value", "The value in " + CURRENCY_UNIT},
                                {RPCResult::Type::NUM, "n", "index"},
                                {RPCResult::Type::OBJ, "scriptPubKey", "",
                                {
                                    {RPCResult::Type::STR, "asm", "the asm"},
                                    {RPCResult::Type::STR, "desc", "Inferred descriptor for the output"},
                                    {RPCResult::Type::STR_HEX, "hex", "the hex"},
                                    {RPCResult::Type::STR, "type", "The type, eg 'pubkeyhash'"},
                                    {RPCResult::Type::STR, "address", /* optional */ true, "The Dash address (only if a well-defined address exists)"},
                                }},
                            }},
                        }},
                        {RPCResult::Type::NUM, "extraPayloadSize", true /*optional*/, "Size of DIP2 extra payload. Only present if it's a special TX"},
                        {RPCResult::Type::STR_HEX, "extraPayload", true /*optional*/, "Hex-encoded DIP2 extra payload data. Only present if it's a special TX"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("decoderawtransaction", "\"hexstring\"")
            + HelpExampleRpc("decoderawtransaction", "\"hexstring\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {UniValue::VSTR});

    CMutableTransaction mtx;

    if (!DecodeHexTx(mtx, request.params[0].get_str()))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");

    UniValue result(UniValue::VOBJ);
    TxToUniv(CTransaction(std::move(mtx)), /*block_hash=*/uint256(), /*entry=*/result, /*include_hex=*/false);

    return result;
},
    };
}

static RPCHelpMan decodescript()
{
    return RPCHelpMan{
        "decodescript",
        "\nDecode a hex-encoded script.\n",
        {
            {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "the hex-encoded script"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "asm", "Script public key"},
                {RPCResult::Type::STR, "desc", "Inferred descriptor for the script"},
                {RPCResult::Type::STR, "type", "The output type (e.g. " + GetAllOutputTypes() + ")"},
                {RPCResult::Type::STR, "address", /* optional */ true, "The Dash address (only if a well-defined address exists)"},
            },
        },
        RPCExamples{
            HelpExampleCli("decodescript", "\"hexstring\"")
          + HelpExampleRpc("decodescript", "\"hexstring\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {UniValue::VSTR});

    UniValue r(UniValue::VOBJ);
    CScript script;
    if (request.params[0].get_str().size() > 0){
        std::vector<unsigned char> scriptData(ParseHexV(request.params[0], "argument"));
        script = CScript(scriptData.begin(), scriptData.end());
    } else {
        // Empty scripts are valid
    }
    ScriptToUniv(script, /*out=*/r, /*include_hex=*/false, /*include_address=*/true);

    std::vector<std::vector<unsigned char>> solutions_data;
    const TxoutType which_type{Solver(script, solutions_data)};

    if (which_type != TxoutType::SCRIPTHASH) {
        // P2SH cannot be wrapped in a P2SH. If this script is already a P2SH,
        // don't return the address for a P2SH of the P2SH.
        r.pushKV("p2sh", EncodeDestination(ScriptHash(script)));
    }

    return r;
},
    };
}

static RPCHelpMan combinerawtransaction()
{
    return RPCHelpMan{"combinerawtransaction",
        "\nCombine multiple partially signed transactions into one transaction.\n"
        "The combined transaction may be another partially signed transaction or a \n"
        "fully signed transaction.",
        {
            {"txs", RPCArg::Type::ARR, RPCArg::Optional::NO, "The of hex strings of partially signed transactions",
                {
                    {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A hex-encoded raw transaction"},
                },
                },
        },
        RPCResult{
            RPCResult::Type::STR, "", "The hex-encoded raw transaction with signature(s)"
        },
        RPCExamples{
            HelpExampleCli("combinerawtransaction", R"('["myhex1", "myhex2", "myhex3"]')")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    UniValue txs = request.params[0].get_array();
    std::vector<CMutableTransaction> txVariants(txs.size());

    for (unsigned int idx = 0; idx < txs.size(); idx++) {
        if (!DecodeHexTx(txVariants[idx], txs[idx].get_str())) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed for tx %d. Make sure the tx has at least one input.", idx));
        }
    }

    if (txVariants.empty()) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Missing transactions");
    }

    // mergedTx will end up with all the signatures; it
    // starts as a clone of the rawtx:
    CMutableTransaction mergedTx(txVariants[0]);

    // Fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        const NodeContext& node = EnsureAnyNodeContext(request.context);
        const CTxMemPool& mempool = EnsureMemPool(node);
        ChainstateManager& chainman = EnsureChainman(node);
        LOCK2(cs_main, mempool.cs);
        CCoinsViewCache &viewChain = chainman.ActiveChainstate().CoinsTip();
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        for (const CTxIn& txin : mergedTx.vin) {
            view.AccessCoin(txin.prevout); // Load entries from viewChain into view; can fail.
        }

        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }

    // Use CTransaction for the constant parts of the
    // transaction to avoid rehashing.
    const CTransaction txConst(mergedTx);
    // Sign what we can:
    for (unsigned int i = 0; i < mergedTx.vin.size(); i++) {
        CTxIn& txin = mergedTx.vin[i];
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) {
            throw JSONRPCError(RPC_VERIFY_ERROR, "Input not found or already spent");
        }
        SignatureData sigdata;

        // ... and merge in other signatures:
        for (const CMutableTransaction& txv : txVariants) {
            if (txv.vin.size() > i) {
                sigdata.MergeSignatureData(DataFromTransaction(txv, i, coin.out));
            }
        }
        ProduceSignature(DUMMY_SIGNING_PROVIDER, MutableTransactionSignatureCreator(&mergedTx, i, coin.out.nValue, 1), coin.out.scriptPubKey, sigdata);

        UpdateInput(txin, sigdata);
    }

    return EncodeHexTx(CTransaction(mergedTx));
},
    };
}

static RPCHelpMan signrawtransactionwithkey()
{
    return RPCHelpMan{"signrawtransactionwithkey",
        "\nSign inputs for raw transaction (serialized, hex-encoded).\n"
        "The second argument is an array of base58-encoded private\n"
        "keys that will be the only keys used to sign the transaction.\n"
        "The third optional argument (may be null) is an array of previous transaction outputs that\n"
        "this transaction depends on but may not yet be in the block chain.\n",
        {
            {"hexstring", RPCArg::Type::STR, RPCArg::Optional::NO, "The transaction hex string"},
            {"privkeys", RPCArg::Type::ARR, RPCArg::Optional::NO, "The base58-encoded private keys for signing",
                {
                    {"privatekey", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "private key in base58-encoding"},
                },
                },
            {"prevtxs", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "The previous dependent transaction outputs",
                {
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                        {
                            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                            {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                            {"scriptPubKey", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "script key"},
                            {"redeemScript", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "(required for P2SH or P2WSH) redeem script"},
                            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "The amount spent"},
                        },
                    },
                },
            },
            {"sighashtype", RPCArg::Type::STR, RPCArg::Default{"ALL"}, "The signature hash type. Must be one of:\n"
    "       \"ALL\"\n"
    "       \"NONE\"\n"
    "       \"SINGLE\"\n"
    "       \"ALL|ANYONECANPAY\"\n"
    "       \"NONE|ANYONECANPAY\"\n"
    "       \"SINGLE|ANYONECANPAY\"\n"
            },
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "hex", "The hex-encoded raw transaction with signature(s)"},
                {RPCResult::Type::BOOL, "complete", "If the transaction has a complete set of signatures"},
                {RPCResult::Type::ARR, "errors", /* optional */ true, "Script verification errors (if there are any)",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR_HEX, "txid", "The hash of the referenced, previous transaction"},
                        {RPCResult::Type::NUM, "vout", "The index of the output to spent and used as input"},
                        {RPCResult::Type::STR_HEX, "scriptSig", "The hex-encoded signature script"},
                        {RPCResult::Type::NUM, "sequence", "Script sequence number"},
                        {RPCResult::Type::STR, "error", "Verification or signing error related to the input"},
                    }},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("signrawtransactionwithkey", "\"myhex\" \"[\\\"key1\\\",\\\"key2\\\"]\"")
    + HelpExampleRpc("signrawtransactionwithkey", "\"myhex\", \"[\\\"key1\\\",\\\"key2\\\"]\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VARR, UniValue::VARR, UniValue::VSTR}, true);

    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed. Make sure the tx has at least one input.");
    }

    FillableSigningProvider keystore;
    const UniValue& keys = request.params[1].get_array();
    for (unsigned int idx = 0; idx < keys.size(); ++idx) {
        UniValue k = keys[idx];
        CKey key = DecodeSecret(k.get_str());
        if (!key.IsValid()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
        }
        keystore.AddKey(key);
    }

    // Fetch previous transactions (inputs):
    std::map<COutPoint, Coin> coins;
    for (const CTxIn& txin : mtx.vin) {
        coins[txin.prevout]; // Create empty map entry keyed by prevout.
    }
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    FindCoins(node, coins);

    // Parse the prevtxs array
    ParsePrevouts(request.params[2], &keystore, coins);

    UniValue result(UniValue::VOBJ);
    SignTransaction(mtx, &keystore, coins, request.params[3], result);
    return result;
},
    };
}

RPCHelpMan sendrawtransaction()
{
    return RPCHelpMan{"sendrawtransaction",                "\nSubmit a raw transaction (serialized, hex-encoded) to local node and network.\n"
                "\nThe transaction will be sent unconditionally to all peers, so using sendrawtransaction\n"
                "for manual rebroadcast may degrade privacy by leaking the transaction's origin, as\n"
                "nodes will normally not rebroadcast non-wallet transactions already in their mempool.\n"
                "\nA specific exception, RPC_TRANSACTION_ALREADY_IN_CHAIN, may throw if the transaction cannot be added to the mempool.\n"
                "\nRelated RPCs: createrawtransaction, signrawtransactionwithkey\n",
                {
                    {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex string of the raw transaction"},
                    {"maxfeerate", RPCArg::Type::AMOUNT, RPCArg::Default{FormatMoney(DEFAULT_MAX_RAW_TX_FEE_RATE.GetFeePerK())},
                        "Reject transactions whose fee rate is higher than the specified value, expressed in " + CURRENCY_UNIT +
                            "/kB.\nSet to 0 to accept any fee rate.\n"},
                    {"instantsend", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Deprecated and ignored"},
                    {"bypasslimits", RPCArg::Type::BOOL, RPCArg::Default{false}, "Bypass transaction policy limits"},
                },
                RPCResult{
                    RPCResult::Type::STR_HEX, "", "The transaction hash in hex"
                },
                RPCExamples{
            "\nCreate a transaction\n"
            + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\" : \\\"mytxid\\\",\\\"vout\\\":0}]\" \"{\\\"myaddress\\\":0.01}\"") +
            "Sign the transaction, and get back the hex\n"
            + HelpExampleCli("signrawtransactionwithwallet", "\"myhex\"") +
            "\nSend the transaction (signed hex)\n"
            + HelpExampleCli("sendrawtransaction", "\"signedhex\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("sendrawtransaction", "\"signedhex\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {
        UniValue::VSTR,
        UniValueType(), // VNUM or VSTR, checked inside AmountFromValue()
        UniValue::VBOOL
    });

    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed. Make sure the tx has at least one input.");
    }
    CTransactionRef tx(MakeTransactionRef(std::move(mtx)));

    const CFeeRate max_raw_tx_fee_rate = request.params[1].isNull() ?
                                             DEFAULT_MAX_RAW_TX_FEE_RATE :
                                             CFeeRate(AmountFromValue(request.params[1]));

    int64_t virtual_size = GetVirtualTransactionSize(*tx);
    CAmount max_raw_tx_fee = max_raw_tx_fee_rate.GetFee(virtual_size);

    bool bypass_limits = false;
    if (!request.params[3].isNull()) bypass_limits = request.params[3].get_bool();
    bilingual_str err_string;
    AssertLockNotHeld(cs_main);
    NodeContext& node = EnsureAnyNodeContext(request.context);
    const TransactionError err = BroadcastTransaction(node, tx, err_string, max_raw_tx_fee, /* relay */ true, /* wait_callback */ true, bypass_limits);
    if (TransactionError::OK != err) {
        throw JSONRPCTransactionError(err, err_string.original);
    }

    return tx->GetHash().GetHex();
},
    };
}

static RPCHelpMan testmempoolaccept()
{
    return RPCHelpMan{"testmempoolaccept",
                "\nReturns result of mempool acceptance tests indicating if raw transaction (serialized, hex-encoded) would be accepted by mempool.\n"
                "\nIf multiple transactions are passed in, parents must come before children and package policies apply: the transactions cannot conflict with any mempool transactions or each other.\n"
                "\nIf one transaction fails, other transactions may not be fully validated (the 'allowed' key will be blank).\n"
                "\nThe maximum number of transactions allowed is " + ToString(MAX_PACKAGE_COUNT) + ".\n"
                "\nThis checks if transactions violate the consensus or policy rules.\n"
                "\nSee sendrawtransaction call.\n",
                {
                    {"rawtxs", RPCArg::Type::ARR, RPCArg::Optional::NO, "An array of hex strings of raw transactions.",
                        {
                            {"rawtx", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, ""},
                        },
                        },
                    {"maxfeerate", RPCArg::Type::AMOUNT, RPCArg::Default{FormatMoney(DEFAULT_MAX_RAW_TX_FEE_RATE.GetFeePerK())},
                     "Reject transactions whose fee rate is higher than the specified value, expressed in " + CURRENCY_UNIT + "/kB\n"},
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "The result of the mempool acceptance test for each raw transaction in the input array.\n"
                        "Returns results for each transaction in the same order they were passed in.\n"
                        "Transactions that cannot be fully validated due to failures in other transactions will not contain an 'allowed' result.\n",
                    {
                        {RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR_HEX, "txid", "The transaction hash in hex"},
                            {RPCResult::Type::STR, "package-error", /* optional */ true, "Package validation error, if any (only possible if rawtxs had more than 1 transaction)."},
                            {RPCResult::Type::BOOL, "allowed", /* optional */ true, "Whether this tx would be accepted to the mempool and pass client-specified maxfeerate. "
                                                               "If not present, the tx was not fully validated due to a failure in another tx in the list."},
                            {RPCResult::Type::NUM, "vsize", /* optional */ true, "Transaction size."},
                            {RPCResult::Type::OBJ, "fees", /* optional */ true, "Transaction fees (only present if 'allowed' is true)",
                            {
                                {RPCResult::Type::STR_AMOUNT, "base", "transaction fee in " + CURRENCY_UNIT},
                            }},
                            {RPCResult::Type::STR, "reject-reason", /* optional */ true, "Rejection string (only present when 'allowed' is false)"},
                        }},
                    }
                },
                RPCExamples{
            "\nCreate a transaction\n"
            + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\" : \\\"mytxid\\\",\\\"vout\\\":0}]\" \"{\\\"myaddress\\\":0.01}\"") +
            "Sign the transaction, and get back the hex\n"
            + HelpExampleCli("signrawtransactionwithwallet", "\"myhex\"") +
            "\nTest acceptance of the transaction (signed hex)\n"
            + HelpExampleCli("testmempoolaccept", R"('["signedhex"]')") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("testmempoolaccept", "[\"signedhex\"]")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {
        UniValue::VARR,
        UniValueType(), // VNUM or VSTR, checked inside AmountFromValue()
    });
    const UniValue raw_transactions = request.params[0].get_array();
    if (raw_transactions.size() < 1 || raw_transactions.size() > MAX_PACKAGE_COUNT) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "Array must contain between 1 and " + ToString(MAX_PACKAGE_COUNT) + " transactions.");
    }

    const CFeeRate max_raw_tx_fee_rate = request.params[1].isNull() ?
                                             DEFAULT_MAX_RAW_TX_FEE_RATE :
                                             CFeeRate(AmountFromValue(request.params[1]));

    std::vector<CTransactionRef> txns;
    txns.reserve(raw_transactions.size());
    for (const auto& rawtx : raw_transactions.getValues()) {
        CMutableTransaction mtx;
        if (!DecodeHexTx(mtx, rawtx.get_str())) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR,
                               "TX decode failed: " + rawtx.get_str() + " Make sure the tx has at least one input.");
        }
        txns.emplace_back(MakeTransactionRef(std::move(mtx)));
    }

    NodeContext& node = EnsureAnyNodeContext(request.context);
    CTxMemPool& mempool = EnsureMemPool(node);
    ChainstateManager& chainman = EnsureChainman(node);
    CChainState& chainstate = chainman.ActiveChainstate();
    const PackageMempoolAcceptResult package_result = [&] {
        LOCK(::cs_main);
        if (txns.size() > 1) return ProcessNewPackage(chainstate, mempool, txns, /* test_accept */ true);
        return PackageMempoolAcceptResult(txns[0]->GetHash(),
               chainman.ProcessTransaction(txns[0], /*test_accept=*/ true));
    }();

    UniValue rpc_result(UniValue::VARR);
    // We will check transaction fees while we iterate through txns in order. If any transaction fee
    // exceeds maxfeerate, we will leave the rest of the validation results blank, because it
    // doesn't make sense to return a validation result for a transaction if its ancestor(s) would
    // not be submitted.
    bool exit_early{false};
    for (const auto& tx : txns) {
        UniValue result_inner(UniValue::VOBJ);
        result_inner.pushKV("txid", tx->GetHash().GetHex());
        if (package_result.m_state.GetResult() == PackageValidationResult::PCKG_POLICY) {
            result_inner.pushKV("package-error", package_result.m_state.GetRejectReason());
        }
        auto it = package_result.m_tx_results.find(tx->GetHash());
        if (exit_early || it == package_result.m_tx_results.end()) {
            // Validation unfinished. Just return the txid.
            rpc_result.push_back(result_inner);
            continue;
        }
        const auto& tx_result = it->second;
        // Package testmempoolaccept doesn't allow transactions to already be in the mempool.
        CHECK_NONFATAL(tx_result.m_result_type != MempoolAcceptResult::ResultType::MEMPOOL_ENTRY);
        if (tx_result.m_result_type == MempoolAcceptResult::ResultType::VALID) {
            const CAmount fee = tx_result.m_base_fees.value();
            // Check that fee does not exceed maximum fee
            const int64_t virtual_size = tx_result.m_vsize.value();
            const CAmount max_raw_tx_fee = max_raw_tx_fee_rate.GetFee(virtual_size);
            if (max_raw_tx_fee && fee > max_raw_tx_fee) {
                result_inner.pushKV("allowed", false);
                result_inner.pushKV("reject-reason", "max-fee-exceeded");
                exit_early = true;
            } else {
                // Only return the fee and vsize if the transaction would pass ATMP.
                // These can be used to calculate the feerate.
                result_inner.pushKV("allowed", true);
                result_inner.pushKV("vsize", virtual_size);
                UniValue fees(UniValue::VOBJ);
                fees.pushKV("base", ValueFromAmount(fee));
                result_inner.pushKV("fees", fees);
            }
        } else {
            result_inner.pushKV("allowed", false);
            const TxValidationState state = tx_result.m_state;
            if (state.GetResult() == TxValidationResult::TX_MISSING_INPUTS) {
                result_inner.pushKV("reject-reason", "missing-inputs");
            } else {
                result_inner.pushKV("reject-reason", state.GetRejectReason());
            }
        }
        rpc_result.push_back(result_inner);
    }
    return rpc_result;
},
    };
}

static RPCHelpMan decodepsbt()
{
    return RPCHelpMan{
        "decodepsbt",
        "Return a JSON object representing the serialized, base64-encoded partially signed blockchain transaction.",
        {
            {"psbt", RPCArg::Type::STR, RPCArg::Optional::NO, "The PSBT base64 string"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::OBJ, "tx", "The decoded network-serialized unsigned transaction.",
                {
                    {RPCResult::Type::ELISION, "", "The layout is the same as the output of decoderawtransaction."},
                }},
                {RPCResult::Type::ARR, "global_xpubs", "",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "xpub", "The extended public key this path corresponds to"},
                        {RPCResult::Type::STR_HEX, "master_fingerprint", "The fingerprint of the master key"},
                        {RPCResult::Type::STR, "path", "The path"},
                    }},
                }},
                {RPCResult::Type::NUM, "psbt_version", "The PSBT version number. Not to be confused with the unsigned transaction version"},
                {RPCResult::Type::ARR, "proprietary", "The global proprietary map",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR_HEX, "identifier", "The hex string for the proprietary identifier"},
                        {RPCResult::Type::NUM, "subtype", "The number for the subtype"},
                        {RPCResult::Type::STR_HEX, "key", "The hex for the key"},
                        {RPCResult::Type::STR_HEX, "value", "The hex for the value"},
                    }},
                }},
                {RPCResult::Type::OBJ_DYN, "unknown", "The unknown global fields",
                {
                     {RPCResult::Type::STR_HEX, "key", "(key-value pair) An unknown key-value pair"},
                }},
                {RPCResult::Type::ARR, "inputs", "",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::OBJ, "non_witness_utxo", /* optional */ true, "Decoded network transaction for non-witness UTXOs",
                        {
                            {RPCResult::Type::ELISION, "",""},
                        }},
                        {RPCResult::Type::OBJ_DYN, "partial_signatures", /* optional */ true, "",
                        {
                            {RPCResult::Type::STR, "pubkey", "The public key and signature that corresponds to it."},
                        }},
                        {RPCResult::Type::STR, "sighash", /* optional */ true, "The sighash type to be used"},
                        {RPCResult::Type::OBJ, "redeem_script", /* optional */ true, "",
                        {
                            {RPCResult::Type::STR, "asm", "The asm"},
                            {RPCResult::Type::STR_HEX, "hex", "The hex"},
                            {RPCResult::Type::STR, "type", "The type, eg 'pubkeyhash'"},
                        }},
                        {RPCResult::Type::ARR, "bip32_derivs", /* optional */ true, "",
                        {
                            {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR, "pubkey", "The public key with the derivation path as the value."},
                                {RPCResult::Type::STR, "master_fingerprint", "The fingerprint of the master key"},
                                {RPCResult::Type::STR, "path", "The path"},
                            }},
                        }},
                        {RPCResult::Type::OBJ, "final_scriptSig", /* optional */ true, "",
                        {
                            {RPCResult::Type::STR, "asm", "The asm"},
                            {RPCResult::Type::STR, "hex", "The hex"},
                        }},
                        {RPCResult::Type::OBJ_DYN, "ripemd160_preimages", /*optional=*/ true, "",
                        {
                            {RPCResult::Type::STR, "hash", "The hash and preimage that corresponds to it."},
                        }},
                        {RPCResult::Type::OBJ_DYN, "sha256_preimages", /*optional=*/ true, "",
                        {
                            {RPCResult::Type::STR, "hash", "The hash and preimage that corresponds to it."},
                        }},
                        {RPCResult::Type::OBJ_DYN, "hash160_preimages", /*optional=*/ true, "",
                        {
                            {RPCResult::Type::STR, "hash", "The hash and preimage that corresponds to it."},
                        }},
                        {RPCResult::Type::OBJ_DYN, "hash256_preimages", /*optional=*/ true, "",
                        {
                            {RPCResult::Type::STR, "hash", "The hash and preimage that corresponds to it."},
                        }},
                        {RPCResult::Type::OBJ_DYN, "unknown", /*optional=*/ true, "The unknown input fields",
                        {
                            {RPCResult::Type::STR_HEX, "key", "(key-value pair) An unknown key-value pair"},
                        }},
                        {RPCResult::Type::ARR, "proprietary", /*optional=*/ true, "The input proprietary map",
                        {
                            {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR_HEX, "identifier", "The hex string for the proprietary identifier"},
                                {RPCResult::Type::NUM, "subtype", "The number for the subtype"},
                                {RPCResult::Type::STR_HEX, "key", "The hex for the key"},
                                {RPCResult::Type::STR_HEX, "value", "The hex for the value"},
                            }},
                        }},
                    }},
                }},
                {RPCResult::Type::ARR, "outputs", "",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::OBJ, "redeem_script", /* optional */ true, "",
                        {
                            {RPCResult::Type::STR, "asm", "The asm"},
                            {RPCResult::Type::STR_HEX, "hex", "The hex"},
                            {RPCResult::Type::STR, "type", "The type, eg 'pubkeyhash'"},
                        }},
                        {RPCResult::Type::ARR, "bip32_derivs", /* optional */ true, "",
                        {
                            {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR, "pubkey", "The public key this path corresponds to"},
                                {RPCResult::Type::STR, "master_fingerprint", "The fingerprint of the master key"},
                                {RPCResult::Type::STR, "path", "The path"},
                            }},
                        }},
                        {RPCResult::Type::OBJ_DYN, "unknown", /* optional */ true, "The unknown global fields",
                        {
                            {RPCResult::Type::STR_HEX, "key", "(key-value pair) An unknown key-value pair"},
                        }},
                        {RPCResult::Type::ARR, "proprietary", /*optional=*/true, "The output proprietary map",
                        {
                            {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR_HEX, "identifier", "The hex string for the proprietary identifier"},
                                {RPCResult::Type::NUM, "subtype", "The number for the subtype"},
                                {RPCResult::Type::STR_HEX, "key", "The hex for the key"},
                                {RPCResult::Type::STR_HEX, "value", "The hex for the value"},
                            }},
                        }},
                    }},
                }},
                {RPCResult::Type::STR_AMOUNT, "fee", /* optional */ true, "The transaction fee paid if all UTXOs slots in the PSBT have been filled."},
            }
        },
        RPCExamples{
            HelpExampleCli("decodepsbt", "\"psbt\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {UniValue::VSTR});

    // Unserialize the transactions
    PartiallySignedTransaction psbtx;
    std::string error;
    if (!DecodeBase64PSBT(psbtx, request.params[0].get_str(), error)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed %s", error));
    }

    UniValue result(UniValue::VOBJ);

    // Add the decoded tx
    UniValue tx_univ(UniValue::VOBJ);
    TxToUniv(CTransaction(*psbtx.tx), /*block_hash=*/uint256(), /*entry=*/tx_univ, /*include_hex=*/false);
    result.pushKV("tx", tx_univ);

    // Add the global xpubs
    UniValue global_xpubs(UniValue::VARR);
    for (std::pair<KeyOriginInfo, std::set<CExtPubKey>> xpub_pair : psbtx.m_xpubs) {
        for (auto& xpub : xpub_pair.second) {
            std::vector<unsigned char> ser_xpub;
            ser_xpub.assign(BIP32_EXTKEY_WITH_VERSION_SIZE, 0);
            xpub.EncodeWithVersion(ser_xpub.data());

            UniValue keypath(UniValue::VOBJ);
            keypath.pushKV("xpub", EncodeBase58Check(ser_xpub));
            keypath.pushKV("master_fingerprint", HexStr(Span<unsigned char>(xpub_pair.first.fingerprint, xpub_pair.first.fingerprint + 4)));
            keypath.pushKV("path", WriteHDKeypath(xpub_pair.first.path));
            global_xpubs.push_back(keypath);
        }
    }
    result.pushKV("global_xpubs", global_xpubs);

    // PSBT version
    result.pushKV("psbt_version", static_cast<uint64_t>(psbtx.GetVersion()));

    // Proprietary
    UniValue proprietary(UniValue::VARR);
    for (const auto& entry : psbtx.m_proprietary) {
        UniValue this_prop(UniValue::VOBJ);
        this_prop.pushKV("identifier", HexStr(entry.identifier));
        this_prop.pushKV("subtype", entry.subtype);
        this_prop.pushKV("key", HexStr(entry.key));
        this_prop.pushKV("value", HexStr(entry.value));
        proprietary.push_back(this_prop);
    }
    result.pushKV("proprietary", proprietary);

    // Unknown data
    UniValue unknowns(UniValue::VOBJ);
    for (auto entry : psbtx.unknown) {
        unknowns.pushKV(HexStr(entry.first), HexStr(entry.second));
    }
    result.pushKV("unknown", unknowns);

    // inputs
    CAmount total_in = 0;
    bool have_all_utxos = true;
    UniValue inputs(UniValue::VARR);
    for (unsigned int i = 0; i < psbtx.inputs.size(); ++i) {
        const PSBTInput& input = psbtx.inputs[i];
        UniValue in(UniValue::VOBJ);
        // UTXOs
        bool have_a_utxo = false;
        CTxOut txout;
        if (input.non_witness_utxo) {
            txout = input.non_witness_utxo->vout[psbtx.tx->vin[i].prevout.n];

            UniValue non_wit(UniValue::VOBJ);
            TxToUniv(*input.non_witness_utxo, /*block_hash=*/uint256(), /*entry=*/non_wit, /*include_hex=*/false);
            in.pushKV("non_witness_utxo", non_wit);

            have_a_utxo = true;
        }
        if (have_a_utxo) {
            if (MoneyRange(txout.nValue) && MoneyRange(total_in + txout.nValue)) {
                total_in += txout.nValue;
            } else {
                // Hack to just not show fee later
                have_all_utxos = false;
            }
        } else {
            have_all_utxos = false;
        }

        // Partial sigs
        if (!input.partial_sigs.empty()) {
            UniValue partial_sigs(UniValue::VOBJ);
            for (const auto& sig : input.partial_sigs) {
                partial_sigs.pushKV(HexStr(sig.second.first), HexStr(sig.second.second));
            }
            in.pushKV("partial_signatures", partial_sigs);
        }

        // Sighash
        if (input.sighash_type != std::nullopt) {
            in.pushKV("sighash", SighashToStr((unsigned char)*input.sighash_type));
        }

        // Redeem script
        if (!input.redeem_script.empty()) {
            UniValue r(UniValue::VOBJ);
            ScriptToUniv(input.redeem_script, /*out=*/r);
            in.pushKV("redeem_script", r);
        }

        // keypaths
        if (!input.hd_keypaths.empty()) {
            UniValue keypaths(UniValue::VARR);
            for (auto entry : input.hd_keypaths) {
                UniValue keypath(UniValue::VOBJ);
                keypath.pushKV("pubkey", HexStr(entry.first));

                keypath.pushKV("master_fingerprint", strprintf("%08x", ReadBE32(entry.second.fingerprint)));
                keypath.pushKV("path", WriteHDKeypath(entry.second.path));
                keypaths.push_back(keypath);
            }
            in.pushKV("bip32_derivs", keypaths);
        }

        // Final scriptSig
        if (!input.final_script_sig.empty()) {
            UniValue scriptsig(UniValue::VOBJ);
            scriptsig.pushKV("asm", ScriptToAsmStr(input.final_script_sig, true));
            scriptsig.pushKV("hex", HexStr(input.final_script_sig));
            in.pushKV("final_scriptSig", scriptsig);
        }

        // Ripemd160 hash preimages
        if (!input.ripemd160_preimages.empty()) {
            UniValue ripemd160_preimages(UniValue::VOBJ);
            for (const auto& [hash, preimage] : input.ripemd160_preimages) {
                ripemd160_preimages.pushKV(HexStr(hash), HexStr(preimage));
            }
            in.pushKV("ripemd160_preimages", ripemd160_preimages);
        }

        // Sha256 hash preimages
        if (!input.sha256_preimages.empty()) {
            UniValue sha256_preimages(UniValue::VOBJ);
            for (const auto& [hash, preimage] : input.sha256_preimages) {
                sha256_preimages.pushKV(HexStr(hash), HexStr(preimage));
            }
            in.pushKV("sha256_preimages", sha256_preimages);
        }

        // Hash160 hash preimages
        if (!input.hash160_preimages.empty()) {
            UniValue hash160_preimages(UniValue::VOBJ);
            for (const auto& [hash, preimage] : input.hash160_preimages) {
                hash160_preimages.pushKV(HexStr(hash), HexStr(preimage));
            }
            in.pushKV("hash160_preimages", hash160_preimages);
        }

        // Hash256 hash preimages
        if (!input.hash256_preimages.empty()) {
            UniValue hash256_preimages(UniValue::VOBJ);
            for (const auto& [hash, preimage] : input.hash256_preimages) {
                hash256_preimages.pushKV(HexStr(hash), HexStr(preimage));
            }
            in.pushKV("hash256_preimages", hash256_preimages);
        }

        // Proprietary
        if (!input.m_proprietary.empty()) {
            UniValue proprietary(UniValue::VARR);
            for (const auto& entry : input.m_proprietary) {
                UniValue this_prop(UniValue::VOBJ);
                this_prop.pushKV("identifier", HexStr(entry.identifier));
                this_prop.pushKV("subtype", entry.subtype);
                this_prop.pushKV("key", HexStr(entry.key));
                this_prop.pushKV("value", HexStr(entry.value));
                proprietary.push_back(this_prop);
            }
            in.pushKV("proprietary", proprietary);
        }

        // Unknown data
        if (input.unknown.size() > 0) {
            UniValue unknowns(UniValue::VOBJ);
            for (auto entry : input.unknown) {
                unknowns.pushKV(HexStr(entry.first), HexStr(entry.second));
            }
            in.pushKV("unknown", unknowns);
        }

        inputs.push_back(in);
    }
    result.pushKV("inputs", inputs);

    // outputs
    CAmount output_value = 0;
    UniValue outputs(UniValue::VARR);
    for (unsigned int i = 0; i < psbtx.outputs.size(); ++i) {
        const PSBTOutput& output = psbtx.outputs[i];
        UniValue out(UniValue::VOBJ);
        // Redeem script
        if (!output.redeem_script.empty()) {
            UniValue r(UniValue::VOBJ);
            ScriptToUniv(output.redeem_script, /*out=*/r);
            out.pushKV("redeem_script", r);
        }

        // keypaths
        if (!output.hd_keypaths.empty()) {
            UniValue keypaths(UniValue::VARR);
            for (auto entry : output.hd_keypaths) {
                UniValue keypath(UniValue::VOBJ);
                keypath.pushKV("pubkey", HexStr(entry.first));
                keypath.pushKV("master_fingerprint", strprintf("%08x", ReadBE32(entry.second.fingerprint)));
                keypath.pushKV("path", WriteHDKeypath(entry.second.path));
                keypaths.push_back(keypath);
            }
            out.pushKV("bip32_derivs", keypaths);
        }

        // Proprietary
        if (!output.m_proprietary.empty()) {
            UniValue proprietary(UniValue::VARR);
            for (const auto& entry : output.m_proprietary) {
                UniValue this_prop(UniValue::VOBJ);
                this_prop.pushKV("identifier", HexStr(entry.identifier));
                this_prop.pushKV("subtype", entry.subtype);
                this_prop.pushKV("key", HexStr(entry.key));
                this_prop.pushKV("value", HexStr(entry.value));
                proprietary.push_back(this_prop);
            }
            out.pushKV("proprietary", proprietary);
        }

        // Unknown data
        if (output.unknown.size() > 0) {
            UniValue unknowns(UniValue::VOBJ);
            for (auto entry : output.unknown) {
                unknowns.pushKV(HexStr(entry.first), HexStr(entry.second));
            }
            out.pushKV("unknown", unknowns);
        }

        outputs.push_back(out);

        // Fee calculation
        if (MoneyRange(psbtx.tx->vout[i].nValue) && MoneyRange(output_value + psbtx.tx->vout[i].nValue)) {
            output_value += psbtx.tx->vout[i].nValue;
        } else {
            // Hack to just not show fee later
            have_all_utxos = false;
        }
    }
    result.pushKV("outputs", outputs);
    if (have_all_utxos) {
        result.pushKV("fee", ValueFromAmount(total_in - output_value));
    }

    return result;
},
    };
}

static RPCHelpMan combinepsbt()
{
    return RPCHelpMan{"combinepsbt",
        "\nCombine multiple partially signed blockchain transactions into one transaction.\n"
        "Implements the Combiner role.\n",
        {
            {"txs", RPCArg::Type::ARR, RPCArg::Optional::NO, "The base64 strings of partially signed transactions",
                {
                    {"psbt", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "A base64 string of a PSBT"},
                },
                },
        },
        RPCResult{
            RPCResult::Type::STR, "", "The base64-encoded partially signed transaction"
        },
        RPCExamples{
            HelpExampleCli("combinepsbt", R"('["mybase64_1", "mybase64_2", "mybase64_3"]')")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {UniValue::VARR}, true);

    // Unserialize the transactions
    std::vector<PartiallySignedTransaction> psbtxs;
    UniValue txs = request.params[0].get_array();
    if (txs.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Parameter 'txs' cannot be empty");
    }
    for (unsigned int i = 0; i < txs.size(); ++i) {
        PartiallySignedTransaction psbtx;
        std::string error;
        if (!DecodeBase64PSBT(psbtx, txs[i].get_str(), error)) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed %s", error));
        }
        psbtxs.push_back(psbtx);
    }

    PartiallySignedTransaction merged_psbt;
    const TransactionError error = CombinePSBTs(merged_psbt, psbtxs);
    if (error != TransactionError::OK) {
        throw JSONRPCTransactionError(error);
    }

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << merged_psbt;
    return EncodeBase64(ssTx);
},
    };
}

static RPCHelpMan finalizepsbt()
{
    return RPCHelpMan{"finalizepsbt",
                "Finalize the inputs of a PSBT. If the transaction is fully signed, it will produce a\n"
                "network serialized transaction which can be broadcast with sendrawtransaction. Otherwise a PSBT will be\n"
                "created which has the final_scriptSig field filled for inputs that are complete.\n"
                "Implements the Finalizer and Extractor roles.\n",
                {
                    {"psbt", RPCArg::Type::STR, RPCArg::Optional::NO, "A base64 string of a PSBT"},
                    {"extract", RPCArg::Type::BOOL, RPCArg::Default{true}, "If true and the transaction is complete,\n"
            "                             extract and return the complete transaction in normal network serialization instead of the PSBT."},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "psbt", /* optional */ true, "The base64-encoded partially signed transaction if not extracted"},
                        {RPCResult::Type::STR_HEX, "hex", /* optional */ true, "The hex-encoded network transaction if extracted"},
                        {RPCResult::Type::BOOL, "complete", "If the transaction has a complete set of signatures"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("finalizepsbt", "\"psbt\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VBOOL}, true);

    // Unserialize the transactions
    PartiallySignedTransaction psbtx;
    std::string error;
    if (!DecodeBase64PSBT(psbtx, request.params[0].get_str(), error)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed %s", error));
    }

    bool extract = request.params[1].isNull() || (!request.params[1].isNull() && request.params[1].get_bool());

    CMutableTransaction mtx;
    bool complete = FinalizeAndExtractPSBT(psbtx, mtx);

    UniValue result(UniValue::VOBJ);
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    std::string result_str;

    if (complete && extract) {
        ssTx << mtx;
        result_str = HexStr(ssTx);
        result.pushKV("hex", result_str);
    } else {
        ssTx << psbtx;
        result_str = EncodeBase64(ssTx.str());
        result.pushKV("psbt", result_str);
    }
    result.pushKV("complete", complete);

    return result;
},
    };
}

static RPCHelpMan createpsbt()
{
    return RPCHelpMan{"createpsbt",
        "\nCreates a transaction in the Partially Signed Transaction format.\n"
        "Implements the Creator role.\n",
        CreateTxDoc(),
        RPCResult{
            RPCResult::Type::STR, "", "The resulting raw transaction (base64-encoded string)"
        },
        RPCExamples{
            HelpExampleCli("createpsbt", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"[{\\\"data\\\":\\\"00010203\\\"}]\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    RPCTypeCheck(request.params, {
        UniValue::VARR,
        UniValueType(), // ARR or OBJ, checked later
        UniValue::VNUM,
        }, true
    );

    CMutableTransaction rawTx = ConstructTransaction(request.params[0], request.params[1], request.params[2]);

    // Make a blank psbt
    PartiallySignedTransaction psbtx;
    psbtx.tx = rawTx;
    for (unsigned int i = 0; i < rawTx.vin.size(); ++i) {
        psbtx.inputs.push_back(PSBTInput());
    }
    for (unsigned int i = 0; i < rawTx.vout.size(); ++i) {
        psbtx.outputs.push_back(PSBTOutput());
    }

    // Serialize the PSBT
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << psbtx;

    return EncodeBase64(ssTx);
},
    };
}

static RPCHelpMan converttopsbt()
{
    return RPCHelpMan{"converttopsbt",
                "\nConverts a network serialized transaction to a PSBT. This should be used only with createrawtransaction and fundrawtransaction\n"
                "createpsbt and walletcreatefundedpsbt should be used for new applications.\n",
                {
                    {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex string of a raw transaction"},
                    {"permitsigdata", RPCArg::Type::BOOL, RPCArg::Default{false}, "If true, any signatures in the input will be discarded and conversion\n"
                            "                              will continue. If false, RPC will fail if any signatures are present."},
                },
                RPCResult{
                    RPCResult::Type::STR, "", "The resulting raw transaction (base64-encoded string)"
                },
                RPCExamples{
                            "\nCreate a transaction\n"
                            + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"[{\\\"data\\\":\\\"00010203\\\"}]\"") +
                            "\nConvert the transaction to a PSBT\n"
                            + HelpExampleCli("converttopsbt", "\"rawtransaction\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VBOOL, UniValue::VBOOL}, true);

    // parse hex string from parameter
    CMutableTransaction tx;
    bool permitsigdata = request.params[1].isNull() ? false : request.params[1].get_bool();
    if (!DecodeHexTx(tx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    // Remove all scriptSigs from inputs
    for (CTxIn& input : tx.vin) {
        if (!input.scriptSig.empty() && !permitsigdata) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Inputs must not have scriptSigs");
        }
        input.scriptSig.clear();
    }

    // Make a blank psbt
    PartiallySignedTransaction psbtx;
    psbtx.tx = tx;
    for (unsigned int i = 0; i < tx.vin.size(); ++i) {
        psbtx.inputs.push_back(PSBTInput());
    }
    for (unsigned int i = 0; i < tx.vout.size(); ++i) {
        psbtx.outputs.push_back(PSBTOutput());
    }

    // Serialize the PSBT
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << psbtx;

    return EncodeBase64(ssTx);
},
    };
}

static RPCHelpMan utxoupdatepsbt()
{
    return RPCHelpMan{"utxoupdatepsbt",
    "\nUpdates a PSBT with data from output descriptors, UTXOs retrieved from the UTXO set or the mempool.\n",
    {
        {"psbt", RPCArg::Type::STR, RPCArg::Optional::NO, "A base64 string of a PSBT"},
        {"descriptors", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "An array of either strings or objects", {
            {"", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "An output descriptor"},
            {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "An object with an output descriptor and extra information", {
                 {"desc", RPCArg::Type::STR, RPCArg::Optional::NO, "An output descriptor"},
                 {"range", RPCArg::Type::RANGE, RPCArg::Default{1000}, "Up to what index HD chains should be explored (either end or [begin,end])"},
            }},
        }},
    },
    RPCResult {
            RPCResult::Type::STR, "", "The base64-encoded partially signed transaction with inputs updated"
    },
    RPCExamples {
        HelpExampleCli("utxoupdatepsbt", "\"psbt\"")
    },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VARR}, true);

    // Unserialize the transactions
    PartiallySignedTransaction psbtx;
    std::string error;
    if (!DecodeBase64PSBT(psbtx, request.params[0].get_str(), error)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed %s", error));
    }

    // Parse descriptors, if any.
    FlatSigningProvider provider;
    if (!request.params[1].isNull()) {
        auto descs = request.params[1].get_array();
        for (size_t i = 0; i < descs.size(); ++i) {
            EvalDescriptorStringOrObject(descs[i], provider);
        }
    }
    // We don't actually need private keys further on; hide them as a precaution.
    HidingSigningProvider public_provider(&provider, /*hide_secret=*/true, /*hide_origin=*/false);

    // Fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        const NodeContext& node = EnsureAnyNodeContext(request.context);
        const CTxMemPool& mempool = EnsureMemPool(node);
        ChainstateManager& chainman = EnsureChainman(node);
        LOCK2(cs_main, mempool.cs);
        CCoinsViewCache &viewChain = chainman.ActiveChainstate().CoinsTip();
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        for (const CTxIn& txin : psbtx.tx->vin) {
            view.AccessCoin(txin.prevout); // Load entries from viewChain into view; can fail.
        }

        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }

    // Fill the inputs
    const PrecomputedTransactionData txdata = PrecomputePSBTData(psbtx);
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        PSBTInput& input = psbtx.inputs.at(i);

        if (input.non_witness_utxo) {
            continue;
        }

        // Update script/keypath information using descriptor data.
        // Note that SignPSBTInput does a lot more than just constructing ECDSA signatures
        // we don't actually care about those here, in fact.
        SignPSBTInput(public_provider, psbtx, i, &txdata, /*sighash=*/SIGHASH_ALL);
    }

    // Update script/keypath information using descriptor data.
    for (unsigned int i = 0; i < psbtx.tx->vout.size(); ++i) {
        UpdatePSBTOutput(public_provider, psbtx, i);
    }

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << psbtx;
    return EncodeBase64(ssTx);
},
    };
}

static RPCHelpMan joinpsbts()
{
    return RPCHelpMan{"joinpsbts",
    "\nJoins multiple distinct PSBTs with different inputs and outputs into one PSBT with inputs and outputs from all of the PSBTs\n"
    "No input in any of the PSBTs can be in more than one of the PSBTs.\n",
    {
        {"txs", RPCArg::Type::ARR, RPCArg::Optional::NO, "The base64 strings of partially signed transactions",
            {
                {"psbt", RPCArg::Type::STR, RPCArg::Optional::NO, "A base64 string of a PSBT"}
            }}
    },
    RPCResult {
            RPCResult::Type::STR, "", "The base64-encoded partially signed transaction"
    },
    RPCExamples {
        HelpExampleCli("joinpsbts", "\"psbt\"")
    },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {UniValue::VARR}, true);

    // Unserialize the transactions
    std::vector<PartiallySignedTransaction> psbtxs;
    UniValue txs = request.params[0].get_array();

    if (txs.size() <= 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "At least two PSBTs are required to join PSBTs.");
    }

    uint16_t best_version = 1;
    uint32_t best_locktime = 0xffffffff;
    for (unsigned int i = 0; i < txs.size(); ++i) {
        PartiallySignedTransaction psbtx;
        std::string error;
        if (!DecodeBase64PSBT(psbtx, txs[i].get_str(), error)) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed %s", error));
        }
        psbtxs.push_back(psbtx);
        // Choose the highest version number
        if (static_cast<uint16_t>(psbtx.tx->nVersion) > best_version) {
            best_version = static_cast<uint16_t>(psbtx.tx->nVersion);
        }
        // Choose the lowest lock time
        if (psbtx.tx->nLockTime < best_locktime) {
            best_locktime = psbtx.tx->nLockTime;
        }
    }

    // Create a blank psbt where everything will be added
    PartiallySignedTransaction merged_psbt;
    merged_psbt.tx = CMutableTransaction();
    merged_psbt.tx->nVersion = static_cast<int16_t>(best_version);
    merged_psbt.tx->nLockTime = best_locktime;

    // Merge
    for (auto& psbt : psbtxs) {
        for (unsigned int i = 0; i < psbt.tx->vin.size(); ++i) {
            if (!merged_psbt.AddInput(psbt.tx->vin[i], psbt.inputs[i])) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Input %s:%d exists in multiple PSBTs", psbt.tx->vin[i].prevout.hash.ToString(), psbt.tx->vin[i].prevout.n));
            }
        }
        for (unsigned int i = 0; i < psbt.tx->vout.size(); ++i) {
            merged_psbt.AddOutput(psbt.tx->vout[i], psbt.outputs[i]);
        }
        for (auto& xpub_pair : psbt.m_xpubs) {
            if (merged_psbt.m_xpubs.count(xpub_pair.first) == 0) {
                merged_psbt.m_xpubs[xpub_pair.first] = xpub_pair.second;
            } else {
                merged_psbt.m_xpubs[xpub_pair.first].insert(xpub_pair.second.begin(), xpub_pair.second.end());
            }
        }
        merged_psbt.unknown.insert(psbt.unknown.begin(), psbt.unknown.end());
    }

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << merged_psbt;
    return EncodeBase64(ssTx);
},
    };
}

static RPCHelpMan analyzepsbt()
{
    return RPCHelpMan{"analyzepsbt",
    "\nAnalyzes and provides information about the current status of a PSBT and its inputs\n",
    {
        {"psbt", RPCArg::Type::STR, RPCArg::Optional::NO, "A base64 string of a PSBT"}
    },
    RPCResult {
        RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::ARR, "inputs", /* optional */ true, "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::BOOL, "has_utxo", "Whether a UTXO is provided"},
                    {RPCResult::Type::BOOL, "is_final", "Whether the input is finalized"},
                    {RPCResult::Type::OBJ, "missing", /* optional */ true, "Things that are missing that are required to complete this input",
                    {
                        {RPCResult::Type::ARR, "pubkeys", /* optional */ true, "",
                        {
                            {RPCResult::Type::STR_HEX, "keyid", "Public key ID, hash160 of the public key, of a public key whose BIP 32 derivation path is missing"},
                        }},
                        {RPCResult::Type::ARR, "signatures", /* optional */ true, "",
                        {
                            {RPCResult::Type::STR_HEX, "keyid", "Public key ID, hash160 of the public key, of a public key whose signature is missing"},
                        }},
                        {RPCResult::Type::STR_HEX, "redeemscript", /* optional */ true, "Hash160 of the redeemScript that is missing"},
                    }},
                    {RPCResult::Type::STR, "next", /* optional */ true, "Role of the next person that this input needs to go to"},
                }},
            }},
            {RPCResult::Type::NUM, "estimated_vsize", /* optional */ true, "Estimated vsize of the final signed transaction"},
            {RPCResult::Type::STR_AMOUNT, "estimated_feerate", /* optional */ true, "Estimated feerate of the final signed transaction in " + CURRENCY_UNIT + "/kB. Shown only if all UTXO slots in the PSBT have been filled"},
            {RPCResult::Type::STR_AMOUNT, "fee", /* optional */ true, "The transaction fee paid. Shown only if all UTXO slots in the PSBT have been filled"},
            {RPCResult::Type::STR, "next", "Role of the next person that this psbt needs to go to"},
            {RPCResult::Type::STR, "error", /* optional */ true, "Error message if there is one"},
        }
    },
    RPCExamples {
        HelpExampleCli("analyzepsbt", "\"psbt\"")
    },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {UniValue::VSTR});

    // Unserialize the transaction
    PartiallySignedTransaction psbtx;
    std::string error;
    if (!DecodeBase64PSBT(psbtx, request.params[0].get_str(), error)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed %s", error));
    }

    PSBTAnalysis psbta = AnalyzePSBT(psbtx);

    UniValue result(UniValue::VOBJ);
    UniValue inputs_result(UniValue::VARR);
    for (const auto& input : psbta.inputs) {
        UniValue input_univ(UniValue::VOBJ);
        UniValue missing(UniValue::VOBJ);

        input_univ.pushKV("has_utxo", input.has_utxo);
        input_univ.pushKV("is_final", input.is_final);
        input_univ.pushKV("next", PSBTRoleName(input.next));

        if (!input.missing_pubkeys.empty()) {
            UniValue missing_pubkeys_univ(UniValue::VARR);
            for (const CKeyID& pubkey : input.missing_pubkeys) {
                missing_pubkeys_univ.push_back(HexStr(pubkey));
            }
            missing.pushKV("pubkeys", missing_pubkeys_univ);
        }
        if (!input.missing_redeem_script.IsNull()) {
            missing.pushKV("redeemscript", HexStr(input.missing_redeem_script));
        }
        if (!input.missing_sigs.empty()) {
            UniValue missing_sigs_univ(UniValue::VARR);
            for (const CKeyID& pubkey : input.missing_sigs) {
                missing_sigs_univ.push_back(HexStr(pubkey));
            }
            missing.pushKV("signatures", missing_sigs_univ);
        }
        if (!missing.getKeys().empty()) {
            input_univ.pushKV("missing", missing);
        }
        inputs_result.push_back(input_univ);
    }
    if (!inputs_result.empty()) result.pushKV("inputs", inputs_result);

    if (psbta.estimated_vsize) {
        result.pushKV("estimated_vsize", (int)*psbta.estimated_vsize);
    }
    if (psbta.estimated_feerate) {
        result.pushKV("estimated_feerate", ValueFromAmount(psbta.estimated_feerate->GetFeePerK()));
    }
    if (psbta.fee) {
        result.pushKV("fee", ValueFromAmount(*psbta.fee));
    }
    result.pushKV("next", PSBTRoleName(psbta.next));
    if (!psbta.error.empty()) {
        result.pushKV("error", psbta.error);
    }

    return result;
},
    };
}

void RegisterRawTransactionRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category               actor (function)
  //  ---------------------  -----------------------
    { "rawtransactions",     &getassetunlockstatuses,     },
    { "rawtransactions",     &getrawtransaction,          },
    { "rawtransactions",     &getrawtransactionmulti,     },
    { "rawtransactions",     &getislocks,                 },
    { "rawtransactions",     &gettxchainlocks,            },
    { "rawtransactions",     &createrawtransaction,       },
    { "rawtransactions",     &decoderawtransaction,       },
    { "rawtransactions",     &decodescript,               },
    { "rawtransactions",     &sendrawtransaction,         },
    { "rawtransactions",     &combinerawtransaction,      },
    { "rawtransactions",     &signrawtransactionwithkey,  },
    { "rawtransactions",     &testmempoolaccept,          },
    { "rawtransactions",     &decodepsbt,                 },
    { "rawtransactions",     &combinepsbt,                },
    { "rawtransactions",     &finalizepsbt,               },
    { "rawtransactions",     &createpsbt,                 },
    { "rawtransactions",     &converttopsbt,              },
    { "rawtransactions",     &utxoupdatepsbt,             },
    { "rawtransactions",     &joinpsbts,                  },
    { "rawtransactions",     &analyzepsbt,                },
};
// clang-format on
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
