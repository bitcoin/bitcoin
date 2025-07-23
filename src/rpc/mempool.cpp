// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/blockchain.h>

#include <core_io.h>
#include <fs.h>
#include <policy/settings.h>
#include <primitives/transaction.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <txmempool.h>
#include <univalue.h>
#include <validation.h>

#include <instantsend/instantsend.h>
#include <llmq/context.h>

using node::NodeContext;

static std::vector<RPCResult> MempoolEntryDescription() { return {
    RPCResult{RPCResult::Type::NUM, "vsize", "virtual transaction size. This can be different from actual serialized size for high-sigop transactions."},
    RPCResult{RPCResult::Type::STR_AMOUNT, "fee", /*optional=*/true,
              "transaction fee, denominated in " + CURRENCY_UNIT + " (DEPRECATED, returned only if config option -deprecatedrpc=fees is passed)"},
    RPCResult{RPCResult::Type::STR_AMOUNT, "modifiedfee", /*optional=*/true,
              "transaction fee with fee deltas used for mining priority, denominated in " + CURRENCY_UNIT +
                  " (DEPRECATED, returned only if config option -deprecatedrpc=fees is passed)"},
    RPCResult{RPCResult::Type::NUM_TIME, "time", "local time transaction entered pool in " + UNIX_EPOCH_TIME},
    RPCResult{RPCResult::Type::NUM, "height", "block height when transaction entered pool"},
    RPCResult{RPCResult::Type::NUM, "descendantcount", "number of in-mempool descendant transactions (including this one)"},
    RPCResult{RPCResult::Type::NUM, "descendantsize", "size of in-mempool descendants (including this one)"},
    RPCResult{RPCResult::Type::STR_AMOUNT, "descendantfees", /*optional=*/true,
              "transaction fees of in-mempool descendants (including this one) with fee deltas used for mining priority, denominated in " +
                  CURRENCY_ATOM + "s (DEPRECATED, returned only if config option -deprecatedrpc=fees is passed)"},
    RPCResult{RPCResult::Type::NUM, "ancestorcount", "number of in-mempool ancestor transactions (including this one)"},
    RPCResult{RPCResult::Type::NUM, "ancestorsize", "size of in-mempool ancestors (including this one)"},
    RPCResult{RPCResult::Type::STR_AMOUNT, "ancestorfees", /*optional=*/true,
              "transaction fees of in-mempool ancestors (including this one) with fee deltas used for mining priority, denominated in " +
                  CURRENCY_ATOM + "s (DEPRECATED, returned only if config option -deprecatedrpc=fees is passed)"},
    RPCResult{RPCResult::Type::OBJ, "fees", "",
    {
        RPCResult{RPCResult::Type::STR_AMOUNT, "base", "transaction fee, denominated in " + CURRENCY_UNIT},
        RPCResult{RPCResult::Type::STR_AMOUNT, "modified", "transaction fee with fee deltas used for mining priority, denominated in " + CURRENCY_UNIT},
        RPCResult{RPCResult::Type::STR_AMOUNT, "ancestor", "transaction fees of in-mempool ancestors (including this one) with fee deltas used for mining priority, denominated in " + CURRENCY_UNIT},
        RPCResult{RPCResult::Type::STR_AMOUNT, "descendant", "transaction fees of in-mempool descendants (including this one) with fee deltas used for mining priority, denominated in " + CURRENCY_UNIT},
    }},
    RPCResult{RPCResult::Type::ARR, "depends", "unconfirmed transactions used as inputs for this transaction",
        {RPCResult{RPCResult::Type::STR_HEX, "transactionid", "parent transaction id"}}},
    RPCResult{RPCResult::Type::ARR, "spentby", "unconfirmed transactions spending outputs from this transaction",
        {RPCResult{RPCResult::Type::STR_HEX, "transactionid", "child transaction id"}}},
    RPCResult{RPCResult::Type::BOOL, "instantsend", "True if this transaction was locked via InstantSend"},
    RPCResult{RPCResult::Type::BOOL, "unbroadcast", "Whether this transaction is currently unbroadcast (initial broadcast not yet acknowledged by any peers)"}
};}

static void entryToJSON(const CTxMemPool& pool, UniValue& info, const CTxMemPoolEntry& e, const llmq::CInstantSendManager* isman) EXCLUSIVE_LOCKS_REQUIRED(pool.cs)
{
    AssertLockHeld(pool.cs);

    info.pushKV("vsize", (int)e.GetTxSize());
    // TODO: top-level fee fields are deprecated. deprecated_fee_fields_enabled blocks should be removed in v24
    const bool deprecated_fee_fields_enabled{IsDeprecatedRPCEnabled("fees")};
    if (deprecated_fee_fields_enabled) {
        info.pushKV("fee", ValueFromAmount(e.GetFee()));
        info.pushKV("modifiedfee", ValueFromAmount(e.GetModifiedFee()));
    }
    info.pushKV("time", count_seconds(e.GetTime()));
    info.pushKV("height", (int)e.GetHeight());
    info.pushKV("descendantcount", e.GetCountWithDescendants());
    info.pushKV("descendantsize", e.GetSizeWithDescendants());
    if (deprecated_fee_fields_enabled) {
        info.pushKV("descendantfees", e.GetModFeesWithDescendants());
    }
    info.pushKV("ancestorcount", e.GetCountWithAncestors());
    info.pushKV("ancestorsize", e.GetSizeWithAncestors());
    if (deprecated_fee_fields_enabled) {
        info.pushKV("ancestorfees", e.GetModFeesWithAncestors());
    }

    UniValue fees(UniValue::VOBJ);
    fees.pushKV("base", ValueFromAmount(e.GetFee()));
    fees.pushKV("modified", ValueFromAmount(e.GetModifiedFee()));
    fees.pushKV("ancestor", ValueFromAmount(e.GetModFeesWithAncestors()));
    fees.pushKV("descendant", ValueFromAmount(e.GetModFeesWithDescendants()));
    info.pushKV("fees", fees);

    const CTransaction& tx = e.GetTx();
    std::set<std::string> setDepends;
    for (const CTxIn& txin : tx.vin)
    {
        if (pool.exists(txin.prevout.hash))
            setDepends.insert(txin.prevout.hash.ToString());
    }

    UniValue depends(UniValue::VARR);
    for (const std::string& dep : setDepends)
    {
        depends.push_back(dep);
    }

    info.pushKV("depends", depends);

    UniValue spent(UniValue::VARR);
    const CTxMemPool::txiter& it = pool.mapTx.find(tx.GetHash());
    const CTxMemPoolEntry::Children& children = it->GetMemPoolChildrenConst();
    for (const CTxMemPoolEntry& child : children) {
        spent.push_back(child.GetTx().GetHash().ToString());
    }

    info.pushKV("spentby", spent);
    info.pushKV("instantlock", isman ? (isman->IsLocked(tx.GetHash()) ? "true" : "false") : "unknown");
    info.pushKV("unbroadcast", pool.IsUnbroadcastTx(tx.GetHash()));
}

UniValue MempoolToJSON(const CTxMemPool& pool, const llmq::CInstantSendManager* isman, bool verbose, bool include_mempool_sequence)
{
    if (verbose) {
        if (include_mempool_sequence) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Verbose results cannot contain mempool sequence values.");
        }
        LOCK(pool.cs);
        UniValue o(UniValue::VOBJ);
        for (const CTxMemPoolEntry& e : pool.mapTx) {
            const uint256& hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(pool, info, e, isman);
            // Mempool has unique entries so there is no advantage in using
            // UniValue::pushKV, which checks if the key already exists in O(N).
            // UniValue::__pushKV is used instead which currently is O(1).
            o.__pushKV(hash.ToString(), info);
        }
        return o;
    } else {
        uint64_t mempool_sequence;
        std::vector<uint256> vtxid;
        {
            LOCK(pool.cs);
            pool.queryHashes(vtxid);
            mempool_sequence = pool.GetSequence();
        }
        UniValue a(UniValue::VARR);
        for (const uint256& hash : vtxid)
            a.push_back(hash.ToString());

        if (!include_mempool_sequence) {
            return a;
        } else {
            UniValue o(UniValue::VOBJ);
            o.pushKV("txids", a);
            o.pushKV("mempool_sequence", mempool_sequence);
            return o;
        }
    }
}

RPCHelpMan getrawmempool()
{
    return RPCHelpMan{"getrawmempool",
        "\nReturns all transaction ids in memory pool as a json array of string transaction ids.\n"
        "\nHint: use getmempoolentry to fetch a specific transaction from the mempool.\n",
        {
            {"verbose", RPCArg::Type::BOOL, RPCArg::Default{false}, "True for a json object, false for array of transaction ids"},
            {"mempool_sequence", RPCArg::Type::BOOL, RPCArg::Default{false}, "If verbose=false, returns a json object with transaction list and mempool sequence number attached."},
        },
        {
            RPCResult{"for verbose = false",
                RPCResult::Type::ARR, "", "",
                {
                    {RPCResult::Type::STR_HEX, "", "The transaction id"},
                }},
            RPCResult{"for verbose = true",
                RPCResult::Type::OBJ_DYN, "", "",
                {
                    {RPCResult::Type::OBJ, "transactionid", "", MempoolEntryDescription()},
                }},
            RPCResult{"for verbose = false and mempool_sequence = true",
                RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::ARR, "txids", "",
                    {
                        {RPCResult::Type::STR_HEX, "", "The transaction id"},
                    }},
                    {RPCResult::Type::NUM, "mempool_sequence", "The mempool sequence value."},
                }},
        },
        RPCExamples{
            HelpExampleCli("getrawmempool", "true")
            + HelpExampleRpc("getrawmempool", "true")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    bool fVerbose = false;
    if (!request.params[0].isNull())
        fVerbose = request.params[0].get_bool();

    bool include_mempool_sequence = false;
    if (!request.params[1].isNull()) {
        include_mempool_sequence = request.params[1].get_bool();
    }

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const CTxMemPool& mempool = EnsureMemPool(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    return MempoolToJSON(mempool, llmq_ctx.isman.get(), fVerbose, include_mempool_sequence);
},
    };
}

RPCHelpMan getmempoolancestors()
{
    return RPCHelpMan{"getmempoolancestors",
        "\nIf txid is in the mempool, returns all in-mempool ancestors.\n",
        {
            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id (must be in mempool)"},
            {"verbose", RPCArg::Type::BOOL, RPCArg::Default{false}, "True for a json object, false for array of transaction ids"},
        },
        {
            RPCResult{"for verbose = false",
                RPCResult::Type::ARR, "", "",
                {{RPCResult::Type::STR_HEX, "", "The transaction id of an in-mempool ancestor transaction"}}},
            RPCResult{"for verbose = true",
                RPCResult::Type::OBJ_DYN, "", "",
                {
                    {RPCResult::Type::OBJ, "transactionid", "", MempoolEntryDescription()},
                }},
        },
        RPCExamples{
            HelpExampleCli("getmempoolancestors", "\"mytxid\"")
            + HelpExampleRpc("getmempoolancestors", "\"mytxid\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    bool fVerbose = false;
    if (!request.params[1].isNull())
        fVerbose = request.params[1].get_bool();

    uint256 hash(ParseHashV(request.params[0], "parameter 1"));

    const NodeContext& node = EnsureAnyNodeContext(request.context);

    const CTxMemPool& mempool = EnsureMemPool(node);
    LOCK(mempool.cs);

    CTxMemPool::txiter it = mempool.mapTx.find(hash);
    if (it == mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    CTxMemPool::setEntries setAncestors;
    uint64_t noLimit = std::numeric_limits<uint64_t>::max();
    std::string dummy;
    mempool.CalculateMemPoolAncestors(*it, setAncestors, noLimit, noLimit, noLimit, noLimit, dummy, false);

    if (!fVerbose) {
        UniValue o(UniValue::VARR);
        for (CTxMemPool::txiter ancestorIt : setAncestors) {
            o.push_back(ancestorIt->GetTx().GetHash().ToString());
        }
        return o;
    } else {
        UniValue o(UniValue::VOBJ);
        const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
        for (CTxMemPool::txiter ancestorIt : setAncestors) {
            const CTxMemPoolEntry &e = *ancestorIt;
            const uint256& _hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(mempool, info, e, llmq_ctx.isman.get());
            o.pushKV(_hash.ToString(), info);
        }
        return o;
    }
},
    };
}

RPCHelpMan getmempooldescendants()
{
    return RPCHelpMan{"getmempooldescendants",
        "\nIf txid is in the mempool, returns all in-mempool descendants.\n",
        {
            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id (must be in mempool)"},
            {"verbose", RPCArg::Type::BOOL, RPCArg::Default{false}, "True for a json object, false for array of transaction ids"},
        },
        {
            RPCResult{"for verbose = false",
                RPCResult::Type::ARR, "", "",
                {{RPCResult::Type::STR_HEX, "", "The transaction id of an in-mempool descendant transaction"}}},
            RPCResult{"for verbose = true",
                RPCResult::Type::OBJ_DYN, "", "",
                {
                    {RPCResult::Type::OBJ, "transactionid", "", MempoolEntryDescription()},
                }},
        },
        RPCExamples{
            HelpExampleCli("getmempooldescendants", "\"mytxid\"")
            + HelpExampleRpc("getmempooldescendants", "\"mytxid\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    bool fVerbose = false;
    if (!request.params[1].isNull())
        fVerbose = request.params[1].get_bool();

    uint256 hash(ParseHashV(request.params[0], "parameter 1"));

    const NodeContext& node = EnsureAnyNodeContext(request.context);

    const CTxMemPool& mempool = EnsureMemPool(node);
    LOCK(mempool.cs);

    CTxMemPool::txiter it = mempool.mapTx.find(hash);
    if (it == mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    CTxMemPool::setEntries setDescendants;
    mempool.CalculateDescendants(it, setDescendants);
    // CTxMemPool::CalculateDescendants will include the given tx
    setDescendants.erase(it);

    if (!fVerbose) {
        UniValue o(UniValue::VARR);
        for (CTxMemPool::txiter descendantIt : setDescendants) {
            o.push_back(descendantIt->GetTx().GetHash().ToString());
        }

        return o;
    } else {
        UniValue o(UniValue::VOBJ);
        const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
        for (CTxMemPool::txiter descendantIt : setDescendants) {
            const CTxMemPoolEntry &e = *descendantIt;
            const uint256& _hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(mempool, info, e, llmq_ctx.isman.get());
            o.pushKV(_hash.ToString(), info);
        }
        return o;
    }
},
    };
}

RPCHelpMan getmempoolentry()
{
    return RPCHelpMan{"getmempoolentry",
        "\nReturns mempool data for given transaction\n",
        {
            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id (must be in mempool)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "", MempoolEntryDescription()},
        RPCExamples{
            HelpExampleCli("getmempoolentry", "\"mytxid\"")
            + HelpExampleRpc("getmempoolentry", "\"mytxid\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    uint256 hash(ParseHashV(request.params[0], "parameter 1"));

    const NodeContext& node = EnsureAnyNodeContext(request.context);

    const CTxMemPool& mempool = EnsureMemPool(node);
    LOCK(mempool.cs);

    CTxMemPool::txiter it = mempool.mapTx.find(hash);
    if (it == mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    const CTxMemPoolEntry &e = *it;
    UniValue info(UniValue::VOBJ);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    entryToJSON(mempool, info, e, llmq_ctx.isman.get());
    return info;
},
    };
}

UniValue MempoolInfoToJSON(const CTxMemPool& pool, const llmq::CInstantSendManager& isman)
{
    // Make sure this call is atomic in the pool.
    LOCK(pool.cs);
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("loaded", pool.IsLoaded());
    ret.pushKV("size", (int64_t)pool.size());
    ret.pushKV("bytes", (int64_t)pool.GetTotalTxSize());
    ret.pushKV("usage", (int64_t)pool.DynamicMemoryUsage());
    ret.pushKV("total_fee", ValueFromAmount(pool.GetTotalFee()));
    int64_t maxmempool{gArgs.GetIntArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000};
    ret.pushKV("maxmempool", maxmempool);
    ret.pushKV("mempoolminfee", ValueFromAmount(std::max(pool.GetMinFee(maxmempool), ::minRelayTxFee).GetFeePerK()));
    ret.pushKV("minrelaytxfee", ValueFromAmount(::minRelayTxFee.GetFeePerK()));
    ret.pushKV("instantsendlocks", isman.GetInstantSendLockCount());
    ret.pushKV("unbroadcastcount", pool.GetUnbroadcastTxs().size());
    return ret;
}

RPCHelpMan getmempoolinfo()
{
    return RPCHelpMan{"getmempoolinfo",
        "\nReturns details on the active state of the TX memory pool.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "loaded", "True if the mempool is fully loaded"},
                {RPCResult::Type::NUM, "size", "Current tx count"},
                {RPCResult::Type::NUM, "bytes", "Sum of all transaction sizes"},
                {RPCResult::Type::NUM, "usage", "Total memory usage for the mempool"},
                {RPCResult::Type::STR_AMOUNT, "total_fee", "Total fees for the mempool in " + CURRENCY_UNIT + ", ignoring modified fees through prioritisetransaction"},
                {RPCResult::Type::NUM, "maxmempool", "Maximum memory usage for the mempool"},
                {RPCResult::Type::STR_AMOUNT, "mempoolminfee", "Minimum fee rate in " + CURRENCY_UNIT + "/kB for tx to be accepted. Is the maximum of minrelaytxfee and minimum mempool fee"},
                {RPCResult::Type::STR_AMOUNT, "minrelaytxfee", "Current minimum relay fee for transactions"},
                {RPCResult::Type::NUM, "instantsendlocks", "Number of unconfirmed InstantSend locks"},
                {RPCResult::Type::NUM, "unbroadcastcount", "Current number of transactions that haven't passed initial broadcast yet"}
            }},
        RPCExamples{
            HelpExampleCli("getmempoolinfo", "")
            + HelpExampleRpc("getmempoolinfo", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const CTxMemPool& mempool = EnsureMemPool(node);
    const LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    return MempoolInfoToJSON(mempool, *llmq_ctx.isman);
},
    };
}

RPCHelpMan savemempool()
{
    return RPCHelpMan{"savemempool",
        "\nDumps the mempool to disk. It will fail until the previous dump is fully loaded.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "filename", "the directory and file where the mempool was saved"},
            }},
        RPCExamples{
            HelpExampleCli("savemempool", "")
            + HelpExampleRpc("savemempool", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const ArgsManager& args{EnsureAnyArgsman(request.context)};
    const CTxMemPool& mempool = EnsureAnyMemPool(request.context);

    if (!mempool.IsLoaded()) {
        throw JSONRPCError(RPC_MISC_ERROR, "The mempool was not loaded yet");
    }

    if (!DumpMempool(mempool)) {
        throw JSONRPCError(RPC_MISC_ERROR, "Unable to dump mempool to disk");
    }

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("filename", fs::path((args.GetDataDirNet() / "mempool.dat")).utf8string());

    return ret;
},
    };
}

void RegisterMempoolRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        // category     actor (function)
        // --------     ----------------
        {"blockchain", &getmempoolancestors},
        {"blockchain", &getmempooldescendants},
        {"blockchain", &getmempoolentry},
        {"blockchain", &getmempoolinfo},
        {"blockchain", &getrawmempool},
        {"blockchain", &savemempool},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
