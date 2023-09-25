// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2014-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/blockchain.h>

#include <amount.h>
#include <blockfilter.h>
#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <core_io.h>
#include <consensus/validation.h>
#include <index/blockfilterindex.h>
#include <index/coinstatsindex.h>
#include <index/txindex.h>
#include <llmq/context.h>
#include <node/blockstorage.h>
#include <node/coinstats.h>
#include <node/context.h>
#include <node/utxo_snapshot.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <script/descriptor.h>
#include <streams.h>
#include <sync.h>
#include <txmempool.h>
#include <undo.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <validation.h>
#include <validationinterface.h>
#include <versionbitsinfo.h>
#include <warnings.h>

#include <evo/specialtx.h>
#include <evo/cbtx.h>
#include <evo/evodb.h>

#include <llmq/chainlocks.h>
#include <llmq/instantsend.h>

#include <stdint.h>

#include <univalue.h>

#include <merkleblock.h>

#include <atomic>
#include <condition_variable>
#include <mutex>

struct CUpdatedBlock
{
    uint256 hash;
    int height;
};

static Mutex cs_blockchange;
static std::condition_variable cond_blockchange;
static CUpdatedBlock latestblock GUARDED_BY(cs_blockchange);

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, CTxMemPool& mempool, CChainState& active_chainstate, llmq::CChainLocksHandler& clhandler, llmq::CInstantSendManager& isman, UniValue& entry);

NodeContext& EnsureAnyNodeContext(const CoreContext& context)
{
    auto* const node_context = GetContext<NodeContext>(context);
    if (!node_context) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Node context not found");
    }
    return *node_context;
}

CTxMemPool& EnsureMemPool(const NodeContext& node)
{
    if (!node.mempool) {
        throw JSONRPCError(RPC_CLIENT_MEMPOOL_DISABLED, "Mempool disabled or instance not found");
    }
    return *node.mempool;
}

CTxMemPool& EnsureAnyMemPool(const CoreContext& context)
{
    return EnsureMemPool(EnsureAnyNodeContext(context));
}

ChainstateManager& EnsureChainman(const NodeContext& node)
{
    if (!node.chainman) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Node chainman not found");
    }
    WITH_LOCK(::cs_main, CHECK_NONFATAL(std::addressof(g_chainman) == std::addressof(*node.chainman)));
    return *node.chainman;
}

ChainstateManager& EnsureAnyChainman(const CoreContext& context)
{
    return EnsureChainman(EnsureAnyNodeContext(context));
}

CBlockPolicyEstimator& EnsureFeeEstimator(const NodeContext& node)
{
    if (!node.fee_estimator) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Fee estimation disabled");
    }
    return *node.fee_estimator;
}

CBlockPolicyEstimator& EnsureAnyFeeEstimator(const CoreContext& context)
{
    return EnsureFeeEstimator(EnsureAnyNodeContext(context));
}

LLMQContext& EnsureLLMQContext(const NodeContext& node)
{
    if (!node.llmq_ctx) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Node LLMQ context not found");
    }
    return *node.llmq_ctx;
}

LLMQContext& EnsureAnyLLMQContext(const CoreContext& context)
{
    return EnsureLLMQContext(EnsureAnyNodeContext(context));
}

/* Calculate the difficulty for a given block index.
 */
double GetDifficulty(const CBlockIndex* blockindex)
{
    CHECK_NONFATAL(blockindex);

    return ConvertBitsToDouble(blockindex->nBits);
}

static int ComputeNextBlockAndDepth(const CBlockIndex* tip, const CBlockIndex* blockindex, const CBlockIndex*& next)
{
    next = tip->GetAncestor(blockindex->nHeight + 1);
    if (next && next->pprev == blockindex) {
        return tip->nHeight - blockindex->nHeight + 1;
    }
    next = nullptr;
    return blockindex == tip ? 1 : -1;
}

CBlockIndex* ParseHashOrHeight(const UniValue& param, ChainstateManager& chainman) {
    LOCK(::cs_main);
    CChain& active_chain = chainman.ActiveChain();

    if (param.isNum()) {
        const int height{param.get_int()};
        if (height < 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Target block height %d is negative", height));
        }
        const int current_tip{active_chain.Height()};
        if (height > current_tip) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Target block height %d after current tip %d", height, current_tip));
        }

        return active_chain[height];
    } else {
        const uint256 hash{ParseHashV(param, "hash_or_height")};
        CBlockIndex* pindex = chainman.m_blockman.LookupBlockIndex(hash);

        if (!pindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }

        return pindex;
    }
}

UniValue blockheaderToJSON(const CBlockIndex* tip, const CBlockIndex* blockindex, llmq::CChainLocksHandler& clhandler, llmq::CInstantSendManager& isman)
{
    // Serialize passed information without accessing chain state of the active chain!
    AssertLockNotHeld(cs_main); // For performance reasons

    UniValue result(UniValue::VOBJ);
    result.pushKV("hash", blockindex->GetBlockHash().GetHex());
    const CBlockIndex* pnext;
    int confirmations = ComputeNextBlockAndDepth(tip, blockindex, pnext);
    result.pushKV("confirmations", confirmations);
    result.pushKV("height", blockindex->nHeight);
    result.pushKV("version", blockindex->nVersion);
    result.pushKV("versionHex", strprintf("%08x", blockindex->nVersion));
    result.pushKV("merkleroot", blockindex->hashMerkleRoot.GetHex());
    result.pushKV("time", (int64_t)blockindex->nTime);
    result.pushKV("mediantime", (int64_t)blockindex->GetMedianTimePast());
    result.pushKV("nonce", (uint64_t)blockindex->nNonce);
    result.pushKV("bits", strprintf("%08x", blockindex->nBits));
    result.pushKV("difficulty", GetDifficulty(blockindex));
    result.pushKV("chainwork", blockindex->nChainWork.GetHex());
    result.pushKV("nTx", (uint64_t)blockindex->nTx);

    if (blockindex->pprev)
        result.pushKV("previousblockhash", blockindex->pprev->GetBlockHash().GetHex());
    if (pnext)
        result.pushKV("nextblockhash", pnext->GetBlockHash().GetHex());

    result.pushKV("chainlock", clhandler.HasChainLock(blockindex->nHeight, blockindex->GetBlockHash()));

    return result;
}

UniValue blockToJSON(const CBlock& block, const CBlockIndex* tip, const CBlockIndex* blockindex, llmq::CChainLocksHandler& clhandler, llmq::CInstantSendManager& isman, bool txDetails)
{
    // Serialize passed information without accessing chain state of the active chain!
    AssertLockNotHeld(cs_main); // For performance reasons

    UniValue result(UniValue::VOBJ);
    result.pushKV("hash", blockindex->GetBlockHash().GetHex());
    const CBlockIndex* pnext;
    int confirmations = ComputeNextBlockAndDepth(tip, blockindex, pnext);
    result.pushKV("confirmations", confirmations);
    result.pushKV("size", (int)::GetSerializeSize(block, PROTOCOL_VERSION));
    result.pushKV("height", blockindex->nHeight);
    result.pushKV("version", block.nVersion);
    result.pushKV("versionHex", strprintf("%08x", block.nVersion));
    result.pushKV("merkleroot", block.hashMerkleRoot.GetHex());
    bool chainLock = clhandler.HasChainLock(blockindex->nHeight, blockindex->GetBlockHash());
    UniValue txs(UniValue::VARR);
    for(const auto& tx : block.vtx)
    {
        if(txDetails)
        {
            UniValue objTx(UniValue::VOBJ);
            TxToUniv(*tx, uint256(), objTx, true);
            bool fLocked = isman.IsLocked(tx->GetHash());
            objTx.pushKV("instantlock", fLocked || chainLock);
            objTx.pushKV("instantlock_internal", fLocked);
            txs.push_back(objTx);
        }
        else
            txs.push_back(tx->GetHash().GetHex());
    }
    result.pushKV("tx", txs);
    if (!block.vtx[0]->vExtraPayload.empty()) {
        CCbTx cbTx;
        if (GetTxPayload(block.vtx[0]->vExtraPayload, cbTx)) {
            UniValue cbTxObj;
            cbTx.ToJson(cbTxObj);
            result.pushKV("cbTx", cbTxObj);
        }
    }
    result.pushKV("time", block.GetBlockTime());
    result.pushKV("mediantime", (int64_t)blockindex->GetMedianTimePast());
    result.pushKV("nonce", (uint64_t)block.nNonce);
    result.pushKV("bits", strprintf("%08x", block.nBits));
    result.pushKV("difficulty", GetDifficulty(blockindex));
    result.pushKV("chainwork", blockindex->nChainWork.GetHex());
    result.pushKV("nTx", (uint64_t)blockindex->nTx);

    if (blockindex->pprev)
        result.pushKV("previousblockhash", blockindex->pprev->GetBlockHash().GetHex());
    if (pnext)
        result.pushKV("nextblockhash", pnext->GetBlockHash().GetHex());

    result.pushKV("chainlock", chainLock);

    return result;
}

static UniValue getblockcount(const JSONRPCRequest& request)
{
    RPCHelpMan{"getblockcount",
        "\nReturns the height of the most-work fully-validated chain.\n"
        "The genesis block has height 0.\n",
        {},
        RPCResult{
            RPCResult::Type::NUM, "", "The current block count"},
        RPCExamples{
            HelpExampleCli("getblockcount", "")
    + HelpExampleRpc("getblockcount", "")
        },
    }.Check(request);

    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    LOCK(cs_main);
    return chainman.ActiveChain().Height();
}

static UniValue getbestblockhash(const JSONRPCRequest& request)
{
    RPCHelpMan{"getbestblockhash",
        "\nReturns the hash of the best (tip) block in the most-work fully-validated chain.\n",
        {},
        RPCResult{
            RPCResult::Type::STR_HEX, "", "the block hash, hex-encoded"},
        RPCExamples{
            HelpExampleCli("getbestblockhash", "")
    + HelpExampleRpc("getbestblockhash", "")
        },
    }.Check(request);

    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    LOCK(cs_main);
    return chainman.ActiveChain().Tip()->GetBlockHash().GetHex();
}

static UniValue getbestchainlock(const JSONRPCRequest& request)
{
    RPCHelpMan{"getbestchainlock",
        "\nReturns information about the best chainlock. Throws an error if there is no known chainlock yet.",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "hash", "The block hash hex-encoded"},
                {RPCResult::Type::NUM, "height", "The block height or index"},
                {RPCResult::Type::STR_HEX, "signature", "The chainlock's BLS signature"},
                {RPCResult::Type::BOOL, "known_block", "True if the block is known by our node"},
            }},
        RPCExamples{
            HelpExampleCli("getbestchainlock", "")
            + HelpExampleRpc("getbestchainlock", "")
        },
    }.Check(request);
    UniValue result(UniValue::VOBJ);

    const NodeContext& node = EnsureAnyNodeContext(request.context);

    LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    llmq::CChainLockSig clsig = llmq_ctx.clhandler->GetBestChainLock();
    if (clsig.IsNull()) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Unable to find any chainlock");
    }
    result.pushKV("blockhash", clsig.getBlockHash().GetHex());
    result.pushKV("height", clsig.getHeight());
    result.pushKV("signature", clsig.getSig().ToString());

    ChainstateManager& chainman = EnsureChainman(node);
    LOCK(cs_main);
    result.pushKV("known_block", chainman.m_blockman.LookupBlockIndex(clsig.getBlockHash()) != nullptr);
    return result;
}

void RPCNotifyBlockChange(bool ibd, const CBlockIndex * pindex)
{
    if(pindex) {
        LOCK(cs_blockchange);
        latestblock.hash = pindex->GetBlockHash();
        latestblock.height = pindex->nHeight;
    }
    cond_blockchange.notify_all();
}

static UniValue waitfornewblock(const JSONRPCRequest& request)
{
    RPCHelpMan{"waitfornewblock",
        "\nWaits for a specific new block and returns useful info about it.\n"
        "\nReturns the current block on timeout or exit.\n",
        {
            {"timeout", RPCArg::Type::NUM, /* default */ "0", "Time in milliseconds to wait for a response. 0 indicates no timeout."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "hash", "The blockhash"},
                {RPCResult::Type::NUM, "height", "Block height"},
            }},
        RPCExamples{
            HelpExampleCli("waitfornewblock", "1000")
    + HelpExampleRpc("waitfornewblock", "1000")
        },
    }.Check(request);
    int timeout = 0;
    if (!request.params[0].isNull())
        timeout = request.params[0].get_int();

    CUpdatedBlock block;
    {
        WAIT_LOCK(cs_blockchange, lock);
        block = latestblock;
        if(timeout)
            cond_blockchange.wait_for(lock, std::chrono::milliseconds(timeout), [&block]() EXCLUSIVE_LOCKS_REQUIRED(cs_blockchange) {return latestblock.height != block.height || latestblock.hash != block.hash || !IsRPCRunning(); });
        else
            cond_blockchange.wait(lock, [&block]() EXCLUSIVE_LOCKS_REQUIRED(cs_blockchange) {return latestblock.height != block.height || latestblock.hash != block.hash || !IsRPCRunning(); });
        block = latestblock;
    }
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("hash", block.hash.GetHex());
    ret.pushKV("height", block.height);
    return ret;
}

static UniValue waitforblock(const JSONRPCRequest& request)
{
    RPCHelpMan{"waitforblock",
        "\nWaits for a specific new block and returns useful info about it.\n"
        "\nReturns the current block on timeout or exit.\n",
        {
            {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Block hash to wait for."},
            {"timeout", RPCArg::Type::NUM, /* default */ "0", "Time in milliseconds to wait for a response. 0 indicates no timeout."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "hash", "The blockhash"},
                {RPCResult::Type::NUM, "height", "Block height"},
            }},
        RPCExamples{
            HelpExampleCli("waitforblock", "\"0000000000079f8ef3d2c688c244eb7a4570b24c9ed7b4a8c619eb02596f8862\" 1000")
    + HelpExampleRpc("waitforblock", "\"0000000000079f8ef3d2c688c244eb7a4570b24c9ed7b4a8c619eb02596f8862\", 1000")
        },
    }.Check(request);
    int timeout = 0;

    uint256 hash(ParseHashV(request.params[0], "blockhash"));

    if (!request.params[1].isNull())
        timeout = request.params[1].get_int();

    CUpdatedBlock block;
    {
        WAIT_LOCK(cs_blockchange, lock);
        if(timeout)
            cond_blockchange.wait_for(lock, std::chrono::milliseconds(timeout), [&hash]() EXCLUSIVE_LOCKS_REQUIRED(cs_blockchange) {return latestblock.hash == hash || !IsRPCRunning();});
        else
            cond_blockchange.wait(lock, [&hash]() EXCLUSIVE_LOCKS_REQUIRED(cs_blockchange) {return latestblock.hash == hash || !IsRPCRunning(); });
        block = latestblock;
    }

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("hash", block.hash.GetHex());
    ret.pushKV("height", block.height);
    return ret;
}

static UniValue waitforblockheight(const JSONRPCRequest& request)
{
    RPCHelpMan{"waitforblockheight",
        "\nWaits for (at least) block height and returns the height and hash\n"
        "of the current tip.\n"
        "\nReturns the current block on timeout or exit.\n",
        {
            {"height", RPCArg::Type::NUM, RPCArg::Optional::NO, "Block height to wait for."},
            {"timeout", RPCArg::Type::NUM, /* default */ "0", "Time in milliseconds to wait for a response. 0 indicates no timeout."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "hash", "The blockhash"},
                {RPCResult::Type::NUM, "height", "Block height"},
            }},
        RPCExamples{
            HelpExampleCli("waitforblockheight", "100 1000")
    + HelpExampleRpc("waitforblockheight", "100, 1000")
        },
    }.Check(request);
    int timeout = 0;

    int height = request.params[0].get_int();

    if (!request.params[1].isNull())
        timeout = request.params[1].get_int();

    CUpdatedBlock block;
    {
        WAIT_LOCK(cs_blockchange, lock);
        if(timeout)
            cond_blockchange.wait_for(lock, std::chrono::milliseconds(timeout), [&height]() EXCLUSIVE_LOCKS_REQUIRED(cs_blockchange) {return latestblock.height >= height || !IsRPCRunning();});
        else
            cond_blockchange.wait(lock, [&height]() EXCLUSIVE_LOCKS_REQUIRED(cs_blockchange) {return latestblock.height >= height || !IsRPCRunning(); });
        block = latestblock;
    }
    // TODO: Backport g_utxosetscan and associated logic from #16127
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("hash", block.hash.GetHex());
    ret.pushKV("height", block.height);
    return ret;
}

static UniValue syncwithvalidationinterfacequeue(const JSONRPCRequest& request)
{
    RPCHelpMan{"syncwithvalidationinterfacequeue",
        "\nWaits for the validation interface queue to catch up on everything that was there when we entered this function.\n",
        {},
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
            HelpExampleCli("syncwithvalidationinterfacequeue","")
    + HelpExampleRpc("syncwithvalidationinterfacequeue","")
        },
    }.Check(request);
    SyncWithValidationInterfaceQueue();
    return NullUniValue;
}

static UniValue getdifficulty(const JSONRPCRequest& request)
{
    RPCHelpMan{"getdifficulty",
        "\nReturns the proof-of-work difficulty as a multiple of the minimum difficulty.\n",
        {},
        RPCResult{
            RPCResult::Type::NUM, "", "the proof-of-work difficulty as a multiple of the minimum difficulty."},
        RPCExamples{
            HelpExampleCli("getdifficulty", "")
    + HelpExampleRpc("getdifficulty", "")
        },
    }.Check(request);

    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    LOCK(cs_main);
    return GetDifficulty(chainman.ActiveChain().Tip());
}

static std::vector<RPCResult> MempoolEntryDescription() { return {
    RPCResult{RPCResult::Type::NUM, "vsize", "virtual transaction size. This can be different from actual serialized size for high-sigop transactions."},
    RPCResult{RPCResult::Type::STR_AMOUNT, "fee", "transaction fee in " + CURRENCY_UNIT + " (DEPRECATED)"},
    RPCResult{RPCResult::Type::STR_AMOUNT, "modifiedfee", "transaction fee with fee deltas used for mining priority (DEPRECATED)"},
    RPCResult{RPCResult::Type::NUM_TIME, "time", "local time transaction entered pool in " + UNIX_EPOCH_TIME},
    RPCResult{RPCResult::Type::NUM, "height", "block height when transaction entered pool"},
    RPCResult{RPCResult::Type::NUM, "descendantcount", "number of in-mempool descendant transactions (including this one)"},
    RPCResult{RPCResult::Type::NUM, "descendantsize", "size of in-mempool descendants (including this one)"},
    RPCResult{RPCResult::Type::STR_AMOUNT, "descendantfees", "modified fees (see above) of in-mempool descendants (including this one) (DEPRECATED)"},
    RPCResult{RPCResult::Type::NUM, "ancestorcount", "number of in-mempool ancestor transactions (including this one)"},
    RPCResult{RPCResult::Type::NUM, "ancestorsize", "size of in-mempool ancestors (including this one)"},
    RPCResult{RPCResult::Type::STR_AMOUNT, "ancestorfees", "modified fees (see above) of in-mempool ancestors (including this one) (DEPRECATED)"},
    RPCResult{RPCResult::Type::OBJ, "fees", "",
    {
        RPCResult{RPCResult::Type::STR_AMOUNT, "base", "transaction fee in " + CURRENCY_UNIT},
        RPCResult{RPCResult::Type::STR_AMOUNT, "modified", "transaction fee with fee deltas used for mining priority in " + CURRENCY_UNIT},
        RPCResult{RPCResult::Type::STR_AMOUNT, "ancestor", "transaction fees of in-mempool ancestors (including this one) in " + CURRENCY_UNIT},
        RPCResult{RPCResult::Type::STR_AMOUNT, "descendant", "transaction fees of in-mempool descendants (including this one) in " + CURRENCY_UNIT},
    }},
    RPCResult{RPCResult::Type::ARR, "depends", "unconfirmed transactions used as inputs for this transaction",
        {RPCResult{RPCResult::Type::STR_HEX, "transactionid", "parent transaction id"}}},
    RPCResult{RPCResult::Type::ARR, "spentby", "unconfirmed transactions spending outputs from this transaction",
        {RPCResult{RPCResult::Type::STR_HEX, "transactionid", "child transaction id"}}},
    RPCResult{RPCResult::Type::BOOL, "instantsend", "True if this transaction was locked via InstantSend"},
    RPCResult{RPCResult::Type::BOOL, "unbroadcast", "Whether this transaction is currently unbroadcast (initial broadcast not yet acknowledged by any peers)"}
};}

static void entryToJSON(const CTxMemPool& pool, UniValue& info, const CTxMemPoolEntry& e, llmq::CInstantSendManager* isman) EXCLUSIVE_LOCKS_REQUIRED(pool.cs)
{
    AssertLockHeld(pool.cs);

    UniValue fees(UniValue::VOBJ);
    fees.pushKV("base", ValueFromAmount(e.GetFee()));
    fees.pushKV("modified", ValueFromAmount(e.GetModifiedFee()));
    fees.pushKV("ancestor", ValueFromAmount(e.GetModFeesWithAncestors()));
    fees.pushKV("descendant", ValueFromAmount(e.GetModFeesWithDescendants()));
    info.pushKV("fees", fees);

    info.pushKV("vsize", (int)e.GetTxSize());
    info.pushKV("fee", ValueFromAmount(e.GetFee()));
    info.pushKV("modifiedfee", ValueFromAmount(e.GetModifiedFee()));
    info.pushKV("time", count_seconds(e.GetTime()));
    info.pushKV("height", (int)e.GetHeight());
    info.pushKV("descendantcount", e.GetCountWithDescendants());
    info.pushKV("descendantsize", e.GetSizeWithDescendants());
    info.pushKV("descendantfees", e.GetModFeesWithDescendants());
    info.pushKV("ancestorcount", e.GetCountWithAncestors());
    info.pushKV("ancestorsize", e.GetSizeWithAncestors());
    info.pushKV("ancestorfees", e.GetModFeesWithAncestors());
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
    const CTxMemPool::setEntries& setChildren = pool.GetMemPoolChildren(it);
    for (CTxMemPool::txiter childiter : setChildren) {
        spent.push_back(childiter->GetTx().GetHash().ToString());
    }

    info.pushKV("spentby", spent);
    info.pushKV("instantlock", isman ? (isman->IsLocked(tx.GetHash()) ? "true" : "false") : "unknown");
    info.pushKV("unbroadcast", pool.IsUnbroadcastTx(tx.GetHash()));
}

UniValue MempoolToJSON(const CTxMemPool& pool, llmq::CInstantSendManager* isman, bool verbose)
{
    if (verbose) {
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
        std::vector<uint256> vtxid;
        pool.queryHashes(vtxid);

        UniValue a(UniValue::VARR);
        for (const uint256& hash : vtxid)
            a.push_back(hash.ToString());

        return a;
    }
}

static UniValue getrawmempool(const JSONRPCRequest& request)
{
    RPCHelpMan{"getrawmempool",
        "\nReturns all transaction ids in memory pool as a json array of string transaction ids.\n"
        "\nHint: use getmempoolentry to fetch a specific transaction from the mempool.\n",
        {
            {"verbose", RPCArg::Type::BOOL, /* default */ "false", "True for a json object, false for array of transaction ids"},
        },
        {
            RPCResult{"for verbose = false",
                RPCResult::Type::ARR, "", "",
                {
                    {RPCResult::Type::STR_HEX, "", "The transaction id"},
                }},
            RPCResult{"for verbose = true",
                RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::OBJ_DYN, "transactionid", "", MempoolEntryDescription()},
                }},
        },
        RPCExamples{
            HelpExampleCli("getrawmempool", "true")
    + HelpExampleRpc("getrawmempool", "true")
        },
    }.Check(request);

    bool fVerbose = false;
    if (!request.params[0].isNull())
        fVerbose = request.params[0].get_bool();

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const CTxMemPool& mempool = EnsureMemPool(node);
    LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    return MempoolToJSON(mempool, llmq_ctx.isman, fVerbose);
}

static UniValue getmempoolancestors(const JSONRPCRequest& request)
{
    RPCHelpMan{"getmempoolancestors",
        "\nIf txid is in the mempool, returns all in-mempool ancestors.\n",
        {
            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id (must be in mempool)"},
            {"verbose", RPCArg::Type::BOOL, /* default */ "false", "True for a json object, false for array of transaction ids"},
        },
        {
            RPCResult{"for verbose = false",
                RPCResult::Type::ARR, "", "",
                {{RPCResult::Type::STR_HEX, "", "The transaction id of an in-mempool ancestor transaction"}}},
            RPCResult{"for verbose = true",
                RPCResult::Type::OBJ_DYN, "transactionid", "", MempoolEntryDescription()
            },
        },
        RPCExamples{
            HelpExampleCli("getmempoolancestors", "\"mytxid\"")
    + HelpExampleRpc("getmempoolancestors", "\"mytxid\"")
        },
    }.Check(request);

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
        LLMQContext& llmq_ctx = EnsureLLMQContext(node);
        for (CTxMemPool::txiter ancestorIt : setAncestors) {
            const CTxMemPoolEntry &e = *ancestorIt;
            const uint256& _hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(mempool, info, e, llmq_ctx.isman);
            o.pushKV(_hash.ToString(), info);
        }
        return o;
    }
}

static UniValue getmempooldescendants(const JSONRPCRequest& request)
{
    RPCHelpMan{"getmempooldescendants",
        "\nIf txid is in the mempool, returns all in-mempool descendants.\n",
        {
            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id (must be in mempool)"},
            {"verbose", RPCArg::Type::BOOL, /* default */ "false", "True for a json object, false for array of transaction ids"},
        },
        {
            RPCResult{"for verbose = false",
                RPCResult::Type::ARR, "", "",
                {{RPCResult::Type::STR_HEX, "", "The transaction id of an in-mempool descendant transaction"}}},
            RPCResult{"for verbose = true",
                RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::OBJ_DYN, "transactionid", "", MempoolEntryDescription()},
                }},
        },
        RPCExamples{
            HelpExampleCli("getmempooldescendants", "\"mytxid\"")
    + HelpExampleRpc("getmempooldescendants", "\"mytxid\"")
        },
    }.Check(request);

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
        LLMQContext& llmq_ctx = EnsureLLMQContext(node);
        for (CTxMemPool::txiter descendantIt : setDescendants) {
            const CTxMemPoolEntry &e = *descendantIt;
            const uint256& _hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(mempool, info, e, llmq_ctx.isman);
            o.pushKV(_hash.ToString(), info);
        }
        return o;
    }
}

static UniValue getmempoolentry(const JSONRPCRequest& request)
{
    RPCHelpMan{"getmempoolentry",
        "\nReturns mempool data for given transaction\n",
        {
            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id (must be in mempool)"},
        },
        RPCResult{
            RPCResult::Type::OBJ_DYN, "", "", MempoolEntryDescription()},
        RPCExamples{
            HelpExampleCli("getmempoolentry", "\"mytxid\"")
    + HelpExampleRpc("getmempoolentry", "\"mytxid\"")
        },
    }.Check(request);

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
    LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    entryToJSON(mempool, info, e, llmq_ctx.isman);
    return info;
}

static UniValue getblockhashes(const JSONRPCRequest& request)
{
    RPCHelpMan{"getblockhashes",
        "\nReturns array of hashes of blocks within the timestamp range provided.\n",
        {
            {"high", RPCArg::Type::NUM, RPCArg::Optional::NO, "The newer block timestamp"},
            {"low", RPCArg::Type::NUM, RPCArg::Optional::NO, "The older block timestamp"},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {{RPCResult::Type::STR_HEX, "", "The block hash"}}},
        RPCExamples{
            HelpExampleCli("getblockhashes", "1231614698 1231024505")
            + HelpExampleRpc("getblockhashes", "1231614698, 1231024505")
        },
    }.Check(request);

    unsigned int high = request.params[0].get_int();
    unsigned int low = request.params[1].get_int();
    std::vector<uint256> blockHashes;

    if (!GetTimestampIndex(high, low, blockHashes)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for block hashes");
    }

    UniValue result(UniValue::VARR);
    for (const auto& hash : blockHashes) {
        result.push_back(hash.GetHex());
    }

    return result;
}

static UniValue getblockhash(const JSONRPCRequest& request)
{
    RPCHelpMan{"getblockhash",
        "\nReturns hash of block in best-block-chain at height provided.\n",
        {
            {"height", RPCArg::Type::NUM, RPCArg::Optional::NO, "The height index"},
        },
        RPCResult{
            RPCResult::Type::STR_HEX, "", "The block hash"},
        RPCExamples{
            HelpExampleCli("getblockhash", "1000")
    + HelpExampleRpc("getblockhash", "1000")
        },
    }.Check(request);

    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    LOCK(cs_main);
    const CChain& active_chain = chainman.ActiveChain();

    int nHeight = request.params[0].get_int();
    if (nHeight < 0 || nHeight > active_chain.Height())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");

    CBlockIndex* pblockindex = active_chain[nHeight];
    return pblockindex->GetBlockHash().GetHex();
}

static UniValue getblockheader(const JSONRPCRequest& request)
{
    RPCHelpMan{"getblockheader",
        "\nIf verbose is false, returns a string that is serialized, hex-encoded data for blockheader 'hash'.\n"
        "If verbose is true, returns an Object with information about blockheader <hash>.\n",
        {
            {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The block hash"},
            {"verbose", RPCArg::Type::BOOL, /* default */ "true", "true for a json object, false for the hex-encoded data"},
        },
        {
            RPCResult{"for verbose = true",
                RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "hash", "the block hash (same as provided)"},
                    {RPCResult::Type::NUM, "confirmations", "The number of confirmations, or -1 if the block is not on the main chain"},
                    {RPCResult::Type::NUM, "height", "The block height or index"},
                    {RPCResult::Type::NUM, "version", "The block version"},
                    {RPCResult::Type::STR_HEX, "versionHex", "The block version formatted in hexadecimal"},
                    {RPCResult::Type::STR_HEX, "merkleroot", "The merkle root"},
                    {RPCResult::Type::NUM_TIME, "time", "The block time expressed in " + UNIX_EPOCH_TIME},
                    {RPCResult::Type::NUM_TIME, "mediantime", "The median block time expressed in " + UNIX_EPOCH_TIME},
                    {RPCResult::Type::NUM, "nonce", "The nonce"},
                    {RPCResult::Type::STR_HEX, "bits", "The bits"},
                    {RPCResult::Type::NUM, "difficulty", "The difficulty"},
                    {RPCResult::Type::STR_HEX, "chainwork", "Expected number of hashes required to produce the current chain"},
                    {RPCResult::Type::NUM, "nTx", "The number of transactions in the block"},
                    {RPCResult::Type::STR_HEX, "previousblockhash", /* optional */ true, "The hash of the previous block (if available)"},
                    {RPCResult::Type::STR_HEX, "nextblockhash", /* optional */ true, "The hash of the next block (if available)"},
                }},
            RPCResult{"for verbose=false",
                RPCResult::Type::STR_HEX, "", "A string that is serialized, hex-encoded data for block 'hash'"},
        },
        RPCExamples{
            HelpExampleCli("getblockheader", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
    + HelpExampleRpc("getblockheader", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
        },
    }.Check(request);

    uint256 hash(ParseHashV(request.params[0], "hash"));
    const NodeContext& node = EnsureAnyNodeContext(request.context);

    bool fVerbose = true;
    if (!request.params[1].isNull())
        fVerbose = request.params[1].get_bool();

    const CBlockIndex* pblockindex;
    const CBlockIndex* tip;
    {
        ChainstateManager& chainman = EnsureChainman(node);
        LOCK(cs_main);
        pblockindex = chainman.m_blockman.LookupBlockIndex(hash);
        tip = chainman.ActiveChain().Tip();
    }

    if (!pblockindex) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
    }

    if (!fVerbose)
    {
        CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
        ssBlock << pblockindex->GetBlockHeader();
        std::string strHex = HexStr(ssBlock);
        return strHex;
    }

    LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    return blockheaderToJSON(tip, pblockindex, *llmq_ctx.clhandler, *llmq_ctx.isman);
}

static UniValue getblockheaders(const JSONRPCRequest& request)
{
    RPCHelpMan{"getblockheaders",
        "\nReturns an array of items with information about <count> blockheaders starting from <hash>.\n"
        "\nIf verbose is false, each item is a string that is serialized, hex-encoded data for a single blockheader.\n"
        "If verbose is true, each item is an Object with information about a single blockheader.\n",
        {
            {"hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The block hash"},
            {"count", RPCArg::Type::NUM, /* default */ strprintf("%s", MAX_HEADERS_RESULTS), ""},
            {"verbose", RPCArg::Type::BOOL, /* default */ "true", "true for a json object, false for the hex-encoded data"},
        },
        {
            RPCResult{"for verbose = true",
                RPCResult::Type::ARR, "", "",
                    {{RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR_HEX, "hash", "the block hash (same as provided)"},
                        {RPCResult::Type::NUM, "confirmations", "The number of confirmations, or -1 if the block is not on the main chain"},
                        {RPCResult::Type::NUM, "height", "The block height or index"},
                        {RPCResult::Type::NUM, "version", "The block version"},
                        {RPCResult::Type::STR_HEX, "versionHex", "The block version formatted in hexadecimal"},
                        {RPCResult::Type::STR_HEX, "merkleroot", "The merkle root"},
                        {RPCResult::Type::NUM_TIME, "time", "The block time expressed in " + UNIX_EPOCH_TIME},
                        {RPCResult::Type::NUM_TIME, "mediantime", "The median block time expressed in " + UNIX_EPOCH_TIME},
                        {RPCResult::Type::NUM, "nonce", "The nonce"},
                        {RPCResult::Type::STR_HEX, "bits", "The bits"},
                        {RPCResult::Type::NUM, "difficulty", "The difficulty"},
                        {RPCResult::Type::STR_HEX, "chainwork", "Expected number of hashes required to produce the current chain"},
                        {RPCResult::Type::NUM, "nTx", "The number of transactions in the block"},
                        {RPCResult::Type::STR_HEX, "previousblockhash", /* optional */ true, "The hash of the previous block (if available)"},
                        {RPCResult::Type::STR_HEX, "nextblockhash", /* optional */ true, "The hash of the next block (if available)"},
                    }},
                }},
            RPCResult{"for verbose=false",
                RPCResult::Type::ARR, "", "",
                    {{RPCResult::Type::STR_HEX, "", "A string that is serialized, hex-encoded data for block 'hash'"}}},
        },
        RPCExamples{
            HelpExampleCli("getblockheaders", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\" 2000")
    + HelpExampleRpc("getblockheaders", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\" 2000")
        },
    }.Check(request);

    uint256 hash(ParseHashV(request.params[0], "blockhash"));

    const NodeContext& node = EnsureAnyNodeContext(request.context);

    ChainstateManager& chainman = EnsureChainman(node);

    CChainState& active_chainstate = chainman.ActiveChainstate();
    CChain& active_chain = active_chainstate.m_chain;

    const CBlockIndex* pblockindex;
    const CBlockIndex* tip;
    {
        LOCK(cs_main);
        pblockindex = chainman.m_blockman.LookupBlockIndex(hash);
        tip = active_chain.Tip();
    }

    if (!pblockindex) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
    }

    int nCount = MAX_HEADERS_RESULTS;
    if (!request.params[1].isNull())
        nCount = request.params[1].get_int();

    if (nCount <= 0 || nCount > (int)MAX_HEADERS_RESULTS)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Count is out of range");

    bool fVerbose = true;
    if (!request.params[2].isNull())
        fVerbose = request.params[2].get_bool();

    UniValue arrHeaders(UniValue::VARR);

    if (!fVerbose)
    {
        for (; pblockindex; pblockindex = active_chain.Next(pblockindex))
        {
            CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
            ssBlock << pblockindex->GetBlockHeader();
            std::string strHex = HexStr(ssBlock);
            arrHeaders.push_back(strHex);
            if (--nCount <= 0)
                break;
        }
        return arrHeaders;
    }

    LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    for (; pblockindex; pblockindex = active_chain.Next(pblockindex))
    {
        arrHeaders.push_back(blockheaderToJSON(tip, pblockindex, *llmq_ctx.clhandler, *llmq_ctx.isman));
        if (--nCount <= 0)
            break;
    }

    return arrHeaders;
}

static CBlock GetBlockChecked(const CBlockIndex* pblockindex)
{
    CBlock block;
    if (IsBlockPruned(pblockindex)) {
        throw JSONRPCError(RPC_MISC_ERROR, "Block not available (pruned data)");
    }

    if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) {
        // Block not found on disk. This could be because we have the block
        // header in our index but not yet have the block or did not accept the
        // block.
        throw JSONRPCError(RPC_MISC_ERROR, "Block not found on disk");
    }

    return block;
}

static CBlockUndo GetUndoChecked(const CBlockIndex* pblockindex)
{
    CBlockUndo blockUndo;
    if (IsBlockPruned(pblockindex)) {
        throw JSONRPCError(RPC_MISC_ERROR, "Undo data not available (pruned data)");
    }

    if (!UndoReadFromDisk(blockUndo, pblockindex)) {
        throw JSONRPCError(RPC_MISC_ERROR, "Can't read undo data from disk");
    }

    return blockUndo;
}

static UniValue getmerkleblocks(const JSONRPCRequest& request)
{
    RPCHelpMan{"getmerkleblocks",
        "\nReturns an array of hex-encoded merkleblocks for <count> blocks starting from <hash> which match <filter>.\n",
        {
            {"filter", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex-encoded bloom filter"},
            {"hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The block hash"},
            {"count", RPCArg::Type::NUM, /* default */ strprintf("%s", MAX_HEADERS_RESULTS), ""},
        },
        RPCResult{
                RPCResult::Type::ARR, "", "",
                    {{RPCResult::Type::STR_HEX, "", "A string that is serialized, hex-encoded data for a merkleblock"}}},
        RPCExamples{
            HelpExampleCli("getmerkleblocks", "\"2303028005802040100040000008008400048141010000f8400420800080025004000004130000000000000001\" \"00000000007e1432d2af52e8463278bf556b55cf5049262f25634557e2e91202\" 2000")
    + HelpExampleRpc("getmerkleblocks", "\"2303028005802040100040000008008400048141010000f8400420800080025004000004130000000000000001\" \"00000000007e1432d2af52e8463278bf556b55cf5049262f25634557e2e91202\" 2000")
        },
    }.Check(request);

    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    LOCK(cs_main);

    CBloomFilter filter;
    std::string strFilter = request.params[0].get_str();
    CDataStream ssBloomFilter(ParseHex(strFilter), SER_NETWORK, PROTOCOL_VERSION);
    ssBloomFilter >> filter;
    if (!filter.IsWithinSizeConstraints()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Filter is not within size constraints");
    }

    uint256 hash(ParseHashV(request.params[1], "blockhash"));

    const CBlockIndex* pblockindex = chainman.m_blockman.LookupBlockIndex(hash);
    if (!pblockindex) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
    }

    int nCount = MAX_HEADERS_RESULTS;
    if (!request.params[2].isNull())
        nCount = request.params[2].get_int();

    if (nCount <= 0 || nCount > (int)MAX_HEADERS_RESULTS) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Count is out of range");
    }

    CBlock block = GetBlockChecked(pblockindex);

    UniValue arrMerkleBlocks(UniValue::VARR);

    for (; pblockindex; pblockindex = chainman.ActiveChain().Next(pblockindex))
    {
        if (--nCount < 0) {
            break;
        }

        if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) {
            // this shouldn't happen, we already checked pruning case earlier
            throw JSONRPCError(RPC_MISC_ERROR, "Block not found on disk");
        }

        CMerkleBlock merkleblock(block, filter);
        if (merkleblock.vMatchedTxn.empty()) {
            // ignore blocks that do not match the filter
            continue;
        }

        CDataStream ssMerkleBlock(SER_NETWORK, PROTOCOL_VERSION);
        ssMerkleBlock << merkleblock;
        std::string strHex = HexStr(ssMerkleBlock);
        arrMerkleBlocks.push_back(strHex);
    }
    return arrMerkleBlocks;
}

static UniValue getblock(const JSONRPCRequest& request)
{
    RPCHelpMan{"getblock",
                "\nIf verbosity is 0, returns a string that is serialized, hex-encoded data for block 'hash'.\n"
                "If verbosity is 1, returns an Object with information about block <hash>.\n"
                "If verbosity is 2, returns an Object with information about block <hash> and information about each transaction. \n",
                {
                    {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The block hash"},
                    {"verbosity|verbose", RPCArg::Type::NUM, /* default */ "1", "0 for hex-encoded data, 1 for a json object, and 2 for json object with transaction data"},
                },
                {
                    RPCResult{"for verbosity = 0",
                        RPCResult::Type::STR_HEX, "", "A string that is serialized, hex-encoded data for block 'hash'"},
                    RPCResult{"for verbosity = 1",
                        RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR_HEX, "hash", "the block hash (same as provided)"},
                                {RPCResult::Type::NUM, "confirmations", "The number of confirmations, or -1 if the block is not on the main chain"},
                                {RPCResult::Type::NUM, "size", "The block size"},
                                {RPCResult::Type::NUM, "height", "The block height or index"},
                                {RPCResult::Type::NUM, "version", "The block version"},
                                {RPCResult::Type::STR_HEX, "versionHex", "The block version formatted in hexadecimal"},
                                {RPCResult::Type::STR_HEX, "merkleroot", "The merkle root"},
                                {RPCResult::Type::ARR, "tx", "The transaction ids",
                                    {{RPCResult::Type::STR_HEX, "", "The transaction id"}}},
                                {RPCResult::Type::OBJ, "cbTx", "The coinbase special transaction",
                                    {
                                        {RPCResult::Type::NUM, "version", "The coinbase special transaction version"},
                                        {RPCResult::Type::STR_HEX, "height", "The block height"},
                                        {RPCResult::Type::STR_HEX, "merkleRootMNList", "The merkle root of the masternode list"},
                                        {RPCResult::Type::STR_HEX, "merkleRootQuorums", "The merkle root of the quorum list"},
                                    }},
                                {RPCResult::Type::NUM_TIME, "time",       "The block time expressed in " + UNIX_EPOCH_TIME},
                                {RPCResult::Type::NUM_TIME, "mediantime", "The median block time expressed in " + UNIX_EPOCH_TIME},
                                {RPCResult::Type::NUM, "nonce", "The nonce"},
                                {RPCResult::Type::STR_HEX, "bits", "The bits"},
                                {RPCResult::Type::NUM, "difficulty", "The difficulty"},
                                {RPCResult::Type::STR_HEX, "chainwork", "Expected number of hashes required to produce the chain up to this block (in hex)"},
                                {RPCResult::Type::NUM, "nTx", "The number of transactions in the block"},
                                {RPCResult::Type::STR_HEX, "previousblockhash", "The hash of the previous block"},
                                {RPCResult::Type::STR_HEX, "nextblockhash", "The hash of the next block"},
                            }},
                    RPCResult{"for verbosity = 2",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::ELISION, "", "Same output as verbosity = 1"},
                            {RPCResult::Type::ARR, "tx", "",
                            {
                                {RPCResult::Type::OBJ, "", "",
                                {
                                    {RPCResult::Type::ELISION, "", "The transactions in the format of the getrawtransaction RPC. Different from verbosity = 1 \"tx\" result"},
                                }},
                            }},
                        }},
                },
                RPCExamples{
                    HelpExampleCli("getblock", "\"00000000000fd08c2fb661d2fcb0d49abb3a91e5f27082ce64feed3b4dede2e2\"")
            + HelpExampleRpc("getblock", "\"00000000000fd08c2fb661d2fcb0d49abb3a91e5f27082ce64feed3b4dede2e2\"")
                },
    }.Check(request);

    uint256 hash(ParseHashV(request.params[0], "blockhash"));

    int verbosity = 1;
    if (!request.params[1].isNull()) {
        if(request.params[1].isNum())
            verbosity = request.params[1].get_int();
        else
            verbosity = request.params[1].get_bool() ? 1 : 0;
    }

    const NodeContext& node = EnsureAnyNodeContext(request.context);

    CBlock block;
    const CBlockIndex* pblockindex;
    const CBlockIndex* tip;
    {
        ChainstateManager& chainman = EnsureChainman(node);
        LOCK(cs_main);
        pblockindex = chainman.m_blockman.LookupBlockIndex(hash);
        tip = chainman.ActiveChain().Tip();

        if (!pblockindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }

        block = GetBlockChecked(pblockindex);
    }

    if (verbosity <= 0)
    {
        CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
        ssBlock << block;
        std::string strHex = HexStr(ssBlock);
        return strHex;
    }

    LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    return blockToJSON(block, tip, pblockindex, *llmq_ctx.clhandler, *llmq_ctx.isman, verbosity >= 2);
}

static UniValue pruneblockchain(const JSONRPCRequest& request)
{
    RPCHelpMan{"pruneblockchain", "",
        {
            {"height", RPCArg::Type::NUM, RPCArg::Optional::NO, "The block height to prune up to. May be set to a discrete height, or to a " + UNIX_EPOCH_TIME + "\n"
    "                  to prune blocks whose block time is at least 2 hours older than the provided timestamp."},
        },
        RPCResult{
            RPCResult::Type::NUM, "", "Height of the last block pruned"},
        RPCExamples{
            HelpExampleCli("pruneblockchain", "1000")
    + HelpExampleRpc("pruneblockchain", "1000")
        },
    }.Check(request);

    if (!fPruneMode)
        throw JSONRPCError(RPC_MISC_ERROR, "Cannot prune blocks because node is not in prune mode.");

    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    LOCK(cs_main);
    CChainState& active_chainstate = chainman.ActiveChainstate();
    CChain& active_chain = active_chainstate.m_chain;

    int heightParam = request.params[0].get_int();
    if (heightParam < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative block height.");

    // Height value more than a billion is too high to be a block height, and
    // too low to be a block time (corresponds to timestamp from Sep 2001).
    if (heightParam > 1000000000) {
        // Add a 2 hour buffer to include blocks which might have had old timestamps
        CBlockIndex* pindex = active_chain.FindEarliestAtLeast(heightParam - TIMESTAMP_WINDOW, 0);
        if (!pindex) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Could not find block with at least the specified timestamp.");
        }
        heightParam = pindex->nHeight;
    }

    unsigned int height = (unsigned int) heightParam;
    unsigned int chainHeight = (unsigned int) active_chain.Height();
    if (chainHeight < Params().PruneAfterHeight())
        throw JSONRPCError(RPC_MISC_ERROR, "Blockchain is too short for pruning.");
    else if (height > chainHeight)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Blockchain is shorter than the attempted prune height.");
    else if (height > chainHeight - MIN_BLOCKS_TO_KEEP) {
        LogPrint(BCLog::RPC, "Attempt to prune blocks close to the tip.  Retaining the minimum number of blocks.\n");
        height = chainHeight - MIN_BLOCKS_TO_KEEP;
    }

    PruneBlockFilesManual(active_chainstate, height);
    const CBlockIndex* block = active_chain.Tip();
    CHECK_NONFATAL(block);
    while (block->pprev && (block->pprev->nStatus & BLOCK_HAVE_DATA)) {
        block = block->pprev;
    }
    return uint64_t(block->nHeight);
}

CoinStatsHashType ParseHashType(const std::string& hash_type_input)
{
    if (hash_type_input == "hash_serialized_2") {
        return CoinStatsHashType::HASH_SERIALIZED;
    } else if (hash_type_input == "muhash") {
        return CoinStatsHashType::MUHASH;
    } else if (hash_type_input == "none") {
        return CoinStatsHashType::NONE;
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s is not a valid hash_type", hash_type_input));
    }
}

static UniValue gettxoutsetinfo(const JSONRPCRequest& request)
{
    RPCHelpMan{"gettxoutsetinfo",
        "\nReturns statistics about the unspent transaction output set.\n"
        "Note this call may take some time if you are not using coinstatsindex.\n",
        {
            {"hash_type", RPCArg::Type::STR, /* default */ "hash_serialized_2", "Which UTXO set hash should be calculated. Options: 'hash_serialized_2' (the legacy algorithm), 'muhash', 'none'."},
            {"hash_or_height", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The block hash or height of the target height (only available with coinstatsindex)", "", {"", "string or numeric"}},
            {"use_index", RPCArg::Type::BOOL, /* default */ "true", "Use coinstatsindex, if available."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "height", "The current block height (index)"},
                {RPCResult::Type::STR_HEX, "bestblock", "The hash of the block at the tip of the chain"},
                {RPCResult::Type::NUM, "txouts", "The number of unspent transaction outputs"},
                {RPCResult::Type::NUM, "bogosize", "Database-independent, meaningless metric indicating the UTXO set size"},
                {RPCResult::Type::STR_HEX, "hash_serialized_2", /* optional */ true, "The serialized hash (only present if 'hash_serialized_2' hash_type is chosen)"},
                {RPCResult::Type::STR_HEX, "muhash", /* optional */ true, "The serialized hash (only present if 'muhash' hash_type is chosen)"},
                {RPCResult::Type::NUM, "transactions", "The number of transactions with unspent outputs (not available when coinstatsindex is used)"},
                {RPCResult::Type::NUM, "disk_size", "The estimated size of the chainstate on disk (not available when coinstatsindex is used)"},
                {RPCResult::Type::STR_AMOUNT, "total_amount", "The total amount"},
                {RPCResult::Type::STR_AMOUNT, "total_unspendable_amount", "The total amount of coins permanently excluded from the UTXO set (only available if coinstatsindex is used)"},
                {RPCResult::Type::OBJ, "block_info", "Info on amounts in the block at this block height (only available if coinstatsindex is used)",
                {
                    {RPCResult::Type::STR_AMOUNT, "prevout_spent", "Total amount of all prevouts spent in this block"},
                    {RPCResult::Type::STR_AMOUNT, "coinbase", "Coinbase subsidy amount of this block"},
                    {RPCResult::Type::STR_AMOUNT, "new_outputs_ex_coinbase", "Total amount of new outputs created by this block"},
                    {RPCResult::Type::STR_AMOUNT, "unspendable", "Total amount of unspendable outputs created in this block"},
                    {RPCResult::Type::OBJ, "unspendables", "Detailed view of the unspendable categories",
                    {
                        {RPCResult::Type::STR_AMOUNT, "genesis_block", "The unspendable amount of the Genesis block subsidy"},
                        {RPCResult::Type::STR_AMOUNT, "bip30", "Transactions overridden by duplicates (no longer possible with BIP30)"},
                        {RPCResult::Type::STR_AMOUNT, "scripts", "Amounts sent to scripts that are unspendable (for example OP_RETURN outputs)"},
                        {RPCResult::Type::STR_AMOUNT, "unclaimed_rewards", "Fee rewards that miners did not claim in their coinbase transaction"},
                    }}
                }},
            }},
        RPCExamples{
                HelpExampleCli("gettxoutsetinfo", "") +
                HelpExampleCli("gettxoutsetinfo", R"("none")") +
                HelpExampleCli("gettxoutsetinfo", R"("none" 1000)") +
                HelpExampleCli("gettxoutsetinfo", R"("none" '"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09"')") +
                HelpExampleRpc("gettxoutsetinfo", "") +
                HelpExampleRpc("gettxoutsetinfo", R"("none")") +
                HelpExampleRpc("gettxoutsetinfo", R"("none", 1000)") +
                HelpExampleRpc("gettxoutsetinfo", R"("none", "00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09")")
        },
    }.Check(request);

    UniValue ret(UniValue::VOBJ);

    CBlockIndex* pindex{nullptr};
    const CoinStatsHashType hash_type{request.params[0].isNull() ? CoinStatsHashType::HASH_SERIALIZED : ParseHashType(request.params[0].get_str())};
    CCoinsStats stats{hash_type};
    stats.index_requested = request.params[2].isNull() || request.params[2].get_bool();

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    ChainstateManager& chainman = EnsureChainman(node);
    CChainState& active_chainstate = chainman.ActiveChainstate();
    active_chainstate.ForceFlushStateToDisk();

    CCoinsView* coins_view;
    BlockManager* blockman;
    {
        LOCK(::cs_main);
        coins_view = &active_chainstate.CoinsDB();
        blockman = &active_chainstate.m_blockman;
        pindex = blockman->LookupBlockIndex(coins_view->GetBestBlock());
    }

    if (!request.params[1].isNull()) {
        if (!g_coin_stats_index) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Querying specific block heights requires coinstatsindex");
        }

        if (stats.m_hash_type == CoinStatsHashType::HASH_SERIALIZED) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "hash_serialized_2 hash type cannot be queried for a specific block");
        }

        pindex = ParseHashOrHeight(request.params[1], chainman);
    }

    if (stats.index_requested && g_coin_stats_index) {
        if (!g_coin_stats_index->BlockUntilSyncedToCurrentChain()) {
            const IndexSummary summary{g_coin_stats_index->GetSummary()};

            // If a specific block was requested and the index has already synced past that height, we can return the
            // data already even though the index is not fully synced yet.
            if (pindex->nHeight > summary.best_block_height) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Unable to get data because coinstatsindex is still syncing. Current height: %d", summary.best_block_height));
            }
        }
    }

    if (GetUTXOStats(coins_view, *blockman, stats, node.rpc_interruption_point, pindex)) {
        ret.pushKV("height", (int64_t)stats.nHeight);
        ret.pushKV("bestblock", stats.hashBlock.GetHex());
        ret.pushKV("txouts", (int64_t)stats.nTransactionOutputs);
        ret.pushKV("bogosize", (int64_t)stats.nBogoSize);
        if (hash_type == CoinStatsHashType::HASH_SERIALIZED) {
            ret.pushKV("hash_serialized_2", stats.hashSerialized.GetHex());
        }
        if (hash_type == CoinStatsHashType::MUHASH) {
            ret.pushKV("muhash", stats.hashSerialized.GetHex());
        }
        CHECK_NONFATAL(stats.total_amount.has_value());
        ret.pushKV("total_amount", ValueFromAmount(stats.total_amount.value()));
        if (!stats.index_used) {
            ret.pushKV("transactions", static_cast<int64_t>(stats.nTransactions));
            ret.pushKV("disk_size", stats.nDiskSize);
        } else {
            ret.pushKV("total_unspendable_amount", ValueFromAmount(stats.total_unspendable_amount));

            CCoinsStats prev_stats{hash_type};

            if (pindex->nHeight > 0) {
                GetUTXOStats(coins_view, *blockman, prev_stats, node.rpc_interruption_point, pindex->pprev);
            }

            UniValue block_info(UniValue::VOBJ);
            block_info.pushKV("prevout_spent", ValueFromAmount(stats.total_prevout_spent_amount - prev_stats.total_prevout_spent_amount));
            block_info.pushKV("coinbase", ValueFromAmount(stats.total_coinbase_amount - prev_stats.total_coinbase_amount));
            block_info.pushKV("new_outputs_ex_coinbase", ValueFromAmount(stats.total_new_outputs_ex_coinbase_amount - prev_stats.total_new_outputs_ex_coinbase_amount));
            block_info.pushKV("unspendable", ValueFromAmount(stats.total_unspendable_amount - prev_stats.total_unspendable_amount));

            UniValue unspendables(UniValue::VOBJ);
            unspendables.pushKV("genesis_block", ValueFromAmount(stats.total_unspendables_genesis_block - prev_stats.total_unspendables_genesis_block));
            unspendables.pushKV("bip30", ValueFromAmount(stats.total_unspendables_bip30 - prev_stats.total_unspendables_bip30));
            unspendables.pushKV("scripts", ValueFromAmount(stats.total_unspendables_scripts - prev_stats.total_unspendables_scripts));
            unspendables.pushKV("unclaimed_rewards", ValueFromAmount(stats.total_unspendables_unclaimed_rewards - prev_stats.total_unspendables_unclaimed_rewards));
            block_info.pushKV("unspendables", unspendables);

            ret.pushKV("block_info", block_info);
        }
    } else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Unable to read UTXO set");
    }
    return ret;
}

static UniValue gettxout(const JSONRPCRequest& request)
{
    RPCHelpMan{"gettxout",
        "\nReturns details about an unspent transaction output.\n",
        {
            {"txid", RPCArg::Type::STR, RPCArg::Optional::NO, "The transaction id"},
            {"n", RPCArg::Type::NUM, RPCArg::Optional::NO, "vout number"},
            {"include_mempool", RPCArg::Type::BOOL, /* default */ "true", "Whether to include the mempool. Note that an unspent output that is spent in the mempool won't appear."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "bestblock", "The hash of the block at the tip of the chain"},
                {RPCResult::Type::NUM, "confirmations", "The number of confirmations"},
                {RPCResult::Type::STR_AMOUNT, "value", "The transaction value in " + CURRENCY_UNIT},
                {RPCResult::Type::OBJ, "scriptPubKey", "",
                    {
                        {RPCResult::Type::STR_HEX, "asm", ""},
                        {RPCResult::Type::STR_HEX, "hex", ""},
                        {RPCResult::Type::NUM, "reqSigs", "Number of required signatures"},
                        {RPCResult::Type::STR_HEX, "type", "The type, eg pubkeyhash"},
                        {RPCResult::Type::ARR, "addresses", "array of dash addresses",
                            {{RPCResult::Type::STR, "address", "dash address"}}},
                    }},
                {RPCResult::Type::BOOL, "coinbase", "Coinbase or not"},
            }},
        RPCExamples{
    "\nGet unspent transactions\n"
    + HelpExampleCli("listunspent", "") +
    "\nView the details\n"
    + HelpExampleCli("gettxout", "\"txid\" 1") +
    "\nAs a JSON-RPC call\n"
    + HelpExampleRpc("gettxout", "\"txid\", 1")
        },
    }.Check(request);

    const NodeContext& node = EnsureAnyNodeContext(request.context);

    ChainstateManager& chainman = EnsureChainman(node);
    LOCK(cs_main);

    CChainState& active_chainstate = chainman.ActiveChainstate();

    UniValue ret(UniValue::VOBJ);

    uint256 hash(ParseHashV(request.params[0], "txid"));
    int n = request.params[1].get_int();
    COutPoint out(hash, n);
    bool fMempool = true;
    if (!request.params[2].isNull())
        fMempool = request.params[2].get_bool();

    Coin coin;
    CCoinsViewCache* coins_view = &active_chainstate.CoinsTip();

    if (fMempool) {
        const CTxMemPool& mempool = EnsureMemPool(node);
        LOCK(mempool.cs);
        CCoinsViewMemPool view(coins_view, mempool);
        if (!view.GetCoin(out, coin) || mempool.isSpent(out)) {
            return NullUniValue;
        }
    } else {
        if (!coins_view->GetCoin(out, coin)) {
            return NullUniValue;
        }
    }

    const CBlockIndex* pindex = active_chainstate.m_blockman.LookupBlockIndex(coins_view->GetBestBlock());
    ret.pushKV("bestblock", pindex->GetBlockHash().GetHex());
    if (coin.nHeight == MEMPOOL_HEIGHT) {
        ret.pushKV("confirmations", 0);
    } else {
        ret.pushKV("confirmations", (int64_t)(pindex->nHeight - coin.nHeight + 1));
    }
    ret.pushKV("value", ValueFromAmount(coin.out.nValue));
    UniValue o(UniValue::VOBJ);
    ScriptPubKeyToUniv(coin.out.scriptPubKey, o, true);
    ret.pushKV("scriptPubKey", o);
    ret.pushKV("coinbase", (bool)coin.fCoinBase);

    return ret;
}

static UniValue verifychain(const JSONRPCRequest& request)
{
    RPCHelpMan{"verifychain",
        "\nVerifies blockchain database.\n",
        {
            {"checklevel", RPCArg::Type::NUM, /* default */ strprintf("%d, range=0-4", DEFAULT_CHECKLEVEL),
                        strprintf("How thorough the block verification is:\n - %s", Join(CHECKLEVEL_DOC, "\n- "))},
            {"nblocks", RPCArg::Type::NUM, /* default */ strprintf("%d, 0=all", DEFAULT_CHECKBLOCKS), "The number of blocks to check."},
        },
        RPCResult{
            RPCResult::Type::BOOL, "", "Verified or not"},
        RPCExamples{
            HelpExampleCli("verifychain", "")
    + HelpExampleRpc("verifychain", "")
        },
    }.Check(request);

    const int check_level(request.params[0].isNull() ? DEFAULT_CHECKLEVEL : request.params[0].get_int());
    const int check_depth{request.params[1].isNull() ? DEFAULT_CHECKBLOCKS : request.params[1].get_int()};

    const NodeContext& node = EnsureAnyNodeContext(request.context);

    ChainstateManager& chainman = EnsureChainman(node);
    LOCK(cs_main);

    CChainState& active_chainstate = chainman.ActiveChainstate();
    return CVerifyDB().VerifyDB(
        active_chainstate, Params(), active_chainstate.CoinsTip(), *node.evodb, check_level, check_depth);
}

static void BuriedForkDescPushBack(UniValue& softforks, const std::string &name, int softfork_height, int tip_height) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    // For buried deployments.
    // A buried deployment is one where the height of the activation has been hardcoded into
    // the client implementation long after the consensus change has activated. See BIP 90.
    // Buried deployments with activation height value of
    // std::numeric_limits<int>::max() are disabled and thus hidden.
    if (softfork_height == std::numeric_limits<int>::max()) return;

    UniValue rv(UniValue::VOBJ);
    rv.pushKV("type", "buried");
    // getblockchaininfo reports the softfork as active from when the chain height is
    // one below the activation height
    rv.pushKV("active", tip_height + 1 >= softfork_height);
    rv.pushKV("height", softfork_height);
    softforks.pushKV(name, rv);
}

static void BIP9SoftForkDescPushBack(const CBlockIndex* active_chain_tip, UniValue& softforks, const std::string &name, const Consensus::Params& consensusParams, Consensus::DeploymentPos id) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    // For BIP9 deployments.
    // Deployments (e.g. testdummy) with timeout value before Jan 1, 2009 are hidden.
    // A timeout value of 0 guarantees a softfork will never be activated.
    // This is used when merging logic to implement a proposed softfork without a specified deployment schedule.
    if (consensusParams.vDeployments[id].nTimeout <= 1230768000) return;

    UniValue bip9(UniValue::VOBJ);
    const ThresholdState thresholdState = VersionBitsState(active_chain_tip, consensusParams, id, versionbitscache);
    switch (thresholdState) {
    case ThresholdState::DEFINED: bip9.pushKV("status", "defined"); break;
    case ThresholdState::STARTED: bip9.pushKV("status", "started"); break;
    case ThresholdState::LOCKED_IN: bip9.pushKV("status", "locked_in"); break;
    case ThresholdState::ACTIVE: bip9.pushKV("status", "active"); break;
    case ThresholdState::FAILED: bip9.pushKV("status", "failed"); break;
    }
    if (ThresholdState::STARTED == thresholdState)
    {
        bip9.pushKV("bit", consensusParams.vDeployments[id].bit);
    }
    bip9.pushKV("start_time", consensusParams.vDeployments[id].nStartTime);
    bip9.pushKV("timeout", consensusParams.vDeployments[id].nTimeout);
    int64_t since_height = VersionBitsStateSinceHeight(active_chain_tip, consensusParams, id, versionbitscache);
    bip9.pushKV("since", since_height);
    if (ThresholdState::STARTED == thresholdState)
    {
        UniValue statsUV(UniValue::VOBJ);
        BIP9Stats statsStruct = VersionBitsStatistics(active_chain_tip, consensusParams, id, versionbitscache);
        statsUV.pushKV("period", statsStruct.period);
        statsUV.pushKV("threshold", statsStruct.threshold);
        statsUV.pushKV("elapsed", statsStruct.elapsed);
        statsUV.pushKV("count", statsStruct.count);
        statsUV.pushKV("possible", statsStruct.possible);
        bip9.pushKV("statistics", statsUV);
    }

    UniValue rv(UniValue::VOBJ);
    rv.pushKV("type", "bip9");
    rv.pushKV("bip9", bip9);
    if (ThresholdState::ACTIVE == thresholdState) {
        rv.pushKV("height", since_height);
    }
    rv.pushKV("active", ThresholdState::ACTIVE == thresholdState);

    softforks.pushKV(name, rv);
}

UniValue getblockchaininfo(const JSONRPCRequest& request)
{
    RPCHelpMan{"getblockchaininfo",
        "Returns an object containing various state info regarding blockchain processing.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "chain", "current network name (main, test, regtest) and "
                                                "devnet or devnet-<name> for \"-devnet\" and \"-devnet=<name>\" respectively\n"},
                {RPCResult::Type::NUM, "blocks", "the height of the most-work fully-validated chain. The genesis block has height 0"},
                {RPCResult::Type::NUM, "headers", "the current number of headers we have validated"},
                {RPCResult::Type::STR, "bestblockhash", "the hash of the currently best block"},
                {RPCResult::Type::NUM, "difficulty", "the current difficulty"},
                {RPCResult::Type::NUM, "mediantime", "median time for the current best block"},
                {RPCResult::Type::NUM, "verificationprogress", "estimate of verification progress [0..1]"},
                {RPCResult::Type::BOOL, "initialblockdownload", "(debug information) estimate of whether this node is in Initial Block Download mode"},
                {RPCResult::Type::STR_HEX, "chainwork", "total amount of work in active chain, in hexadecimal"},
                {RPCResult::Type::NUM, "size_on_disk", "the estimated size of the block and undo files on disk"},
                {RPCResult::Type::BOOL, "pruned", "if the blocks are subject to pruning"},
                {RPCResult::Type::NUM, "pruneheight", "lowest-height complete block stored (only present if pruning is enabled)"},
                {RPCResult::Type::BOOL, "automatic_pruning", "whether automatic pruning is enabled (only present if pruning is enabled)"},
                {RPCResult::Type::NUM, "prune_target_size", "the target size used by pruning (only present if automatic pruning is enabled)"},
                {RPCResult::Type::OBJ, "softforks", "status of softforks in progress",
                {
                    {RPCResult::Type::STR, "type", "one of \"buried\", \"bip9\""},
                    {RPCResult::Type::OBJ, "bip9", "status of bip9 softforks (only for \"bip9\" type)",
                    {
                        {RPCResult::Type::STR, "status", "one of \"defined\", \"started\", \"locked_in\", \"active\", \"failed\""},
                        {RPCResult::Type::NUM, "bit", "the bit (0-28) in the block version field used to signal this softfork (only for \"started\" status)"},
                        {RPCResult::Type::NUM_TIME, "start_time", "the minimum median time past of a block at which the bit gains its meaning"},
                        {RPCResult::Type::NUM_TIME, "timeout", "the median time past of a block at which the deployment is considered failed if not yet locked in"},
                        {RPCResult::Type::NUM, "since", "height of the first block to which the status applies"},
                        {RPCResult::Type::OBJ, "statistics", "numeric statistics about BIP9 signalling for a softfork",
                        {
                            {RPCResult::Type::NUM, "period", "the length in blocks of the BIP9 signalling period"},
                            {RPCResult::Type::NUM, "threshold", "the number of blocks with the version bit set required to activate the feature"},
                            {RPCResult::Type::NUM, "elapsed", "the number of blocks elapsed since the beginning of the current period"},
                            {RPCResult::Type::NUM, "count", "the number of blocks with the version bit set in the current period"},
                            {RPCResult::Type::BOOL, "possible", "returns false if there are not enough blocks left in this period to pass activation threshold"},
                        }},
                    }},
                    {RPCResult::Type::NUM, "height", "height of the first block which the rules are or will be enforced (only for \"buried\" type, or \"bip9\" type with \"active\" status)"},
                    {RPCResult::Type::BOOL, "active", "true if the rules are enforced for the mempool and the next block"},
                }},
                {RPCResult::Type::STR, "warnings", "any network and blockchain warnings"},
            }},
        RPCExamples{
            HelpExampleCli("getblockchaininfo", "")
    + HelpExampleRpc("getblockchaininfo", "")
        },
    }.Check(request);

    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    LOCK(cs_main);
    CChainState& active_chainstate = chainman.ActiveChainstate();

    std::string strChainName = gArgs.IsArgSet("-devnet") ? gArgs.GetDevNetName() : Params().NetworkIDString();

    const CBlockIndex* tip = active_chainstate.m_chain.Tip();
    CHECK_NONFATAL(tip);
    const int height = tip->nHeight;
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("chain",                 strChainName);
    obj.pushKV("blocks",                height);
    obj.pushKV("headers",               pindexBestHeader ? pindexBestHeader->nHeight : -1);
    obj.pushKV("bestblockhash",         tip->GetBlockHash().GetHex());
    obj.pushKV("difficulty",            (double)GetDifficulty(tip));
    obj.pushKV("mediantime",            (int64_t)tip->GetMedianTimePast());
    obj.pushKV("verificationprogress",  GuessVerificationProgress(Params().TxData(), tip));
    obj.pushKV("initialblockdownload",  active_chainstate.IsInitialBlockDownload());
    obj.pushKV("chainwork",             tip->nChainWork.GetHex());
    obj.pushKV("size_on_disk",          CalculateCurrentUsage());
    obj.pushKV("pruned",                fPruneMode);
    if (fPruneMode) {
        const CBlockIndex* block = tip;
        CHECK_NONFATAL(block);
        while (block->pprev && (block->pprev->nStatus & BLOCK_HAVE_DATA)) {
            block = block->pprev;
        }

        obj.pushKV("pruneheight",        block->nHeight);

        // if 0, execution bypasses the whole if block.
        bool automatic_pruning = (gArgs.GetArg("-prune", 0) != 1);
        obj.pushKV("automatic_pruning",  automatic_pruning);
        if (automatic_pruning) {
            obj.pushKV("prune_target_size",  nPruneTarget);
        }
    }

    const Consensus::Params& consensusParams = Params().GetConsensus();
    UniValue softforks(UniValue::VOBJ);
    // sorted by activation block
    BuriedForkDescPushBack(softforks,"bip34", consensusParams.BIP34Height, height);
    BuriedForkDescPushBack(softforks,"bip66", consensusParams.BIP66Height, height);
    BuriedForkDescPushBack(softforks,"bip65", consensusParams.BIP65Height, height);
    BuriedForkDescPushBack(softforks,"bip147", consensusParams.BIP147Height, height);
    BuriedForkDescPushBack(softforks, "csv", consensusParams.CSVHeight, height);
    BuriedForkDescPushBack(softforks, "dip0001", consensusParams.DIP0001Height, height);
    BuriedForkDescPushBack(softforks, "dip0003", consensusParams.DIP0003Height, height);
    BuriedForkDescPushBack(softforks, "dip0008", consensusParams.DIP0008Height, height);
    BuriedForkDescPushBack(softforks, "dip0020", consensusParams.DIP0020Height, height);
    BuriedForkDescPushBack(softforks, "dip0024", consensusParams.DIP0024Height, height);
    BuriedForkDescPushBack(softforks, "realloc", consensusParams.BRRHeight, height);
    BuriedForkDescPushBack(softforks, "v19", consensusParams.V19Height, height);
    BIP9SoftForkDescPushBack(tip, softforks, "v20", consensusParams, Consensus::DEPLOYMENT_V20);
    BIP9SoftForkDescPushBack(tip, softforks, "mn_rr", consensusParams, Consensus::DEPLOYMENT_MN_RR);
    BIP9SoftForkDescPushBack(tip, softforks, "testdummy", consensusParams, Consensus::DEPLOYMENT_TESTDUMMY);

    obj.pushKV("softforks",             softforks);

    obj.pushKV("warnings", GetWarnings(false));
    return obj;
}

/** Comparison function for sorting the getchaintips heads.  */
struct CompareBlocksByHeight
{
    bool operator()(const CBlockIndex* a, const CBlockIndex* b) const
    {
        /* Make sure that unequal blocks with the same height do not compare
           equal. Use the pointers themselves to make a distinction. */

        if (a->nHeight != b->nHeight)
          return (a->nHeight > b->nHeight);

        return a < b;
    }
};

static UniValue getchaintips(const JSONRPCRequest& request)
{
    RPCHelpMan{"getchaintips",
        "Return information about all known tips in the block tree,"
        " including the main chain as well as orphaned branches.\n",
        {
            {"count", RPCArg::Type::NUM, /* default */ "", "only show this much of latest tips"},
            {"branchlen", RPCArg::Type::NUM, /* default */ "", "only show tips that have equal or greater length of branch"},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {{RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::NUM, "height", "height of the chain tip"},
                    {RPCResult::Type::STR_HEX, "hash", "block hash of the tip"},
                    {RPCResult::Type::NUM, "difficulty", "The difficulty"},
                    {RPCResult::Type::STR_HEX, "chainwork", "Expected number of hashes required to produce the current chain (in hex)"},
                    {RPCResult::Type::NUM, "branchlen", "zero for main chain, otherwise length of branch connecting the tip to the main chain"},
                    {RPCResult::Type::STR_HEX, "forkpoint", "same as \"hash\" for the main chain"},
                    {RPCResult::Type::STR, "status", "status of the chain, \"active\" for the main chain\n"
    "Possible values for status:\n"
    "1.  \"invalid\"               This branch contains at least one invalid block\n"
    "2.  \"headers-only\"          Not all blocks for this branch are available, but the headers are valid\n"
    "3.  \"valid-headers\"         All blocks are available for this branch, but they were never fully validated\n"
    "4.  \"valid-fork\"            This branch is not part of the active chain, but is fully validated\n"
    "5.  \"active\"                This is the tip of the active main chain, which is certainly valid\n"
    "6.  \"conflicting\"           This block or one of its ancestors is conflicting with ChainLocks."},
                }}}},
        RPCExamples{
            HelpExampleCli("getchaintips", "")
    + HelpExampleRpc("getchaintips", "")
        },
    }.Check(request);

    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    LOCK(cs_main);
    CChain& active_chain = chainman.ActiveChain();

    /*
     * Idea: The set of chain tips is the active chain tip, plus orphan blocks which do not have another orphan building off of them.
     * Algorithm:
     *  - Make one pass through BlockIndex(), picking out the orphan blocks, and also storing a set of the orphan block's pprev pointers.
     *  - Iterate through the orphan blocks. If the block isn't pointed to by another orphan, it is a chain tip.
     *  - Add the active chain tip
     */
    std::set<const CBlockIndex*, CompareBlocksByHeight> setTips;
    std::set<const CBlockIndex*> setOrphans;
    std::set<const CBlockIndex*> setPrevs;

    for (const std::pair<const uint256, CBlockIndex*>& item : chainman.BlockIndex()) {
        if (!active_chain.Contains(item.second)) {
            setOrphans.insert(item.second);
            setPrevs.insert(item.second->pprev);
        }
    }

    for (const auto& orphan : setOrphans) {
        if (setPrevs.erase(orphan) == 0) {
            setTips.insert(orphan);
        }
    }

    // Always report the currently active tip.
    setTips.insert(active_chain.Tip());

    int nBranchMin = -1;
    int nCountMax = INT_MAX;

    if(!request.params[0].isNull())
        nCountMax = request.params[0].get_int();

    if(!request.params[1].isNull())
        nBranchMin = request.params[1].get_int();

    /* Construct the output array.  */
    UniValue res(UniValue::VARR);
    for (const CBlockIndex* block : setTips)
    {
        const CBlockIndex* pindexFork = active_chain.FindFork(block);
        const int branchLen = block->nHeight - pindexFork->nHeight;
        if(branchLen < nBranchMin) continue;

        if(nCountMax-- < 1) break;

        UniValue obj(UniValue::VOBJ);
        obj.pushKV("height", block->nHeight);
        obj.pushKV("hash", block->phashBlock->GetHex());
        obj.pushKV("difficulty", GetDifficulty(block));
        obj.pushKV("chainwork", block->nChainWork.GetHex());
        obj.pushKV("branchlen", branchLen);
        obj.pushKV("forkpoint", pindexFork->phashBlock->GetHex());

        std::string status;
        if (active_chain.Contains(block)) {
            // This block is part of the currently active chain.
            status = "active";
        } else if (block->nStatus & BLOCK_FAILED_MASK) {
            // This block or one of its ancestors is invalid.
            status = "invalid";
        } else if (block->nStatus & BLOCK_CONFLICT_CHAINLOCK) {
            // This block or one of its ancestors is conflicting with ChainLocks.
            status = "conflicting";
        } else if (!block->HaveTxsDownloaded()) {
            // This block cannot be connected because full block data for it or one of its parents is missing.
            status = "headers-only";
        } else if (block->IsValid(BLOCK_VALID_SCRIPTS)) {
            // This block is fully validated, but no longer part of the active chain. It was probably the active block once, but was reorganized.
            status = "valid-fork";
        } else if (block->IsValid(BLOCK_VALID_TREE)) {
            // The headers for this block are valid, but it has not been validated. It was probably never part of the most-work chain.
            status = "valid-headers";
        } else {
            // No clue.
            status = "unknown";
        }
        obj.pushKV("status", status);

        res.push_back(obj);
    }

    return res;
}

UniValue MempoolInfoToJSON(const CTxMemPool& pool, llmq::CInstantSendManager& isman)
{
    // Make sure this call is atomic in the pool.
    LOCK(pool.cs);
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("loaded", pool.IsLoaded());
    ret.pushKV("size", (int64_t)pool.size());
    ret.pushKV("bytes", (int64_t)pool.GetTotalTxSize());
    ret.pushKV("usage", (int64_t)pool.DynamicMemoryUsage());
    size_t maxmempool = gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000;
    ret.pushKV("maxmempool", (int64_t) maxmempool);
    ret.pushKV("mempoolminfee", ValueFromAmount(std::max(pool.GetMinFee(maxmempool), ::minRelayTxFee).GetFeePerK()));
    ret.pushKV("minrelaytxfee", ValueFromAmount(::minRelayTxFee.GetFeePerK()));
    ret.pushKV("instantsendlocks", (int64_t)isman.GetInstantSendLockCount());
    ret.pushKV("unbroadcastcount", uint64_t{pool.GetUnbroadcastTxs().size()});
    return ret;
}

static UniValue getmempoolinfo(const JSONRPCRequest& request)
{
    RPCHelpMan{"getmempoolinfo",
        "\nReturns details on the active state of the TX memory pool.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "loaded", "True if the mempool is fully loaded"},
                {RPCResult::Type::NUM, "size", "Current tx count"},
                {RPCResult::Type::NUM, "bytes", "Sum of all virtual transaction sizes as defined in BIP 141. Differs from actual serialized size because witness data is discounted"},
                {RPCResult::Type::NUM, "usage", "Total memory usage for the mempool"},
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
    }.Check(request);

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const CTxMemPool& mempool = EnsureMemPool(node);
    LLMQContext& llmq_ctx = EnsureLLMQContext(node);
    return MempoolInfoToJSON(mempool, *llmq_ctx.isman);
}

static UniValue preciousblock(const JSONRPCRequest& request)
{
    RPCHelpMan{"preciousblock",
        "\nTreats a block as if it were received before others with the same work.\n"
        "\nA later preciousblock call can override the effect of an earlier one.\n"
        "\nThe effects of preciousblock are not retained across restarts.\n",
        {
            {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "the hash of the block to mark as precious"},
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
            HelpExampleCli("preciousblock", "\"blockhash\"")
    + HelpExampleRpc("preciousblock", "\"blockhash\"")
        },
    }.Check(request);

    uint256 hash(ParseHashV(request.params[0], "blockhash"));
    CBlockIndex* pblockindex;

    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    {
        LOCK(cs_main);
        pblockindex = chainman.m_blockman.LookupBlockIndex(hash);
        if (!pblockindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }
    }

    BlockValidationState state;
    chainman.ActiveChainstate().PreciousBlock(state, pblockindex);

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.ToString());
    }

    return NullUniValue;
}

static UniValue invalidateblock(const JSONRPCRequest& request)
{
    RPCHelpMan{"invalidateblock",
        "\nPermanently marks a block as invalid, as if it violated a consensus rule.\n",
        {
            {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "the hash of the block to mark as invalid"},
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
            HelpExampleCli("invalidateblock", "\"blockhash\"")
    + HelpExampleRpc("invalidateblock", "\"blockhash\"")
        },
    }.Check(request);

    uint256 hash(ParseHashV(request.params[0], "blockhash"));
    BlockValidationState state;

    CBlockIndex* pblockindex;
    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    {
        LOCK(cs_main);
        pblockindex = chainman.m_blockman.LookupBlockIndex(hash);
        if (!pblockindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }
    }

    CChainState& active_chainstate = chainman.ActiveChainstate();
    active_chainstate.InvalidateBlock(state, pblockindex);

    if (state.IsValid()) {
        active_chainstate.ActivateBestChain(state);
    }

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.ToString());
    }

    return NullUniValue;
}

static UniValue reconsiderblock(const JSONRPCRequest& request)
{
    RPCHelpMan{"reconsiderblock",
        "\nRemoves invalidity status of a block, its ancestors and its descendants, reconsider them for activation.\n"
        "This can be used to undo the effects of invalidateblock.\n",
        {
            {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "the hash of the block to reconsider"},
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
            HelpExampleCli("reconsiderblock", "\"blockhash\"")
    + HelpExampleRpc("reconsiderblock", "\"blockhash\"")
        },
    }.Check(request);

    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    CChainState& active_chainstate = chainman.ActiveChainstate();

    uint256 hash(ParseHashV(request.params[0], "blockhash"));

    {
        LOCK(cs_main);
        CBlockIndex* pblockindex = chainman.m_blockman.LookupBlockIndex(hash);
        if (!pblockindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }

        active_chainstate.ResetBlockFailureFlags(pblockindex);
    }

    BlockValidationState state;
    active_chainstate.ActivateBestChain(state);

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.ToString());
    }

    return NullUniValue;
}

static UniValue getchaintxstats(const JSONRPCRequest& request)
{
            RPCHelpMan{"getchaintxstats",
                "\nCompute statistics about the total number and rate of transactions in the chain.\n",
                {
                    {"nblocks", RPCArg::Type::NUM, /* default */ "one month", "Size of the window in number of blocks"},
                    {"blockhash", RPCArg::Type::STR_HEX, /* default */ "chain tip", "The hash of the block that ends the window."},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::NUM_TIME, "time", "The timestamp for the final block in the window, expressed in " + UNIX_EPOCH_TIME},
                        {RPCResult::Type::NUM, "txcount", "The total number of transactions in the chain up to that point"},
                        {RPCResult::Type::STR_HEX, "window_final_block_hash", "The hash of the final block in the window"},
                        {RPCResult::Type::NUM, "window_final_block_height", "The height of the final block in the window."},
                        {RPCResult::Type::NUM, "window_block_count", "Size of the window in number of blocks"},
                        {RPCResult::Type::NUM, "window_tx_count", /* optional */ true, "The number of transactions in the window. Only returned if \"window_block_count\" is > 0"},
                        {RPCResult::Type::NUM, "window_interval", /* optional */ true, "The elapsed time in the window in seconds. Only returned if \"window_block_count\" is > 0"},
                        {RPCResult::Type::NUM, "txrate", /* optional */ true, "The average rate of transactions per second in the window. Only returned if \"window_interval\" is > 0"},
                    }},
                RPCExamples{
                    HelpExampleCli("getchaintxstats", "")
            + HelpExampleRpc("getchaintxstats", "2016")
                },
            }.Check(request);

    ChainstateManager& chainman = EnsureAnyChainman(request.context);

    CChain& active_chain = chainman.ActiveChain();
    const CBlockIndex* pindex;
    int blockcount = 30 * 24 * 60 * 60 / Params().GetConsensus().nPowTargetSpacing; // By default: 1 month

    if (request.params[1].isNull()) {
        LOCK(cs_main);
        pindex = active_chain.Tip();
    } else {
        uint256 hash(ParseHashV(request.params[1], "blockhash"));
        LOCK(cs_main);
        pindex = chainman.m_blockman.LookupBlockIndex(hash);
        if (!pindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }
        if (!active_chain.Contains(pindex)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block is not in main chain");
        }
    }

    CHECK_NONFATAL(pindex != nullptr);

    if (request.params[0].isNull()) {
        blockcount = std::max(0, std::min(blockcount, pindex->nHeight - 1));
    } else {
        blockcount = request.params[0].get_int();

        if (blockcount < 0 || (blockcount > 0 && blockcount >= pindex->nHeight)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid block count: should be between 0 and the block's height - 1");
        }
    }

    const CBlockIndex* pindexPast = pindex->GetAncestor(pindex->nHeight - blockcount);
    int nTimeDiff = pindex->GetMedianTimePast() - pindexPast->GetMedianTimePast();
    int nTxDiff = pindex->nChainTx - pindexPast->nChainTx;

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("time", (int64_t)pindex->nTime);
    ret.pushKV("txcount", (int64_t)pindex->nChainTx);
    ret.pushKV("window_final_block_hash", pindex->GetBlockHash().GetHex());
    ret.pushKV("window_final_block_height", pindex->nHeight);
    ret.pushKV("window_block_count", blockcount);
    if (blockcount > 0) {
        ret.pushKV("window_tx_count", nTxDiff);
        ret.pushKV("window_interval", nTimeDiff);
        if (nTimeDiff > 0) {
            ret.pushKV("txrate", ((double)nTxDiff) / nTimeDiff);
        }
    }

    return ret;
}

template<typename T>
static T CalculateTruncatedMedian(std::vector<T>& scores)
{
    size_t size = scores.size();
    if (size == 0) {
        return 0;
    }

    std::sort(scores.begin(), scores.end());
    if (size % 2 == 0) {
        return (scores[size / 2 - 1] + scores[size / 2]) / 2;
    } else {
        return scores[size / 2];
    }
}

void CalculatePercentilesBySize(CAmount result[NUM_GETBLOCKSTATS_PERCENTILES], std::vector<std::pair<CAmount, int64_t>>& scores, int64_t total_size)
{
    if (scores.empty()) {
        return;
    }

    std::sort(scores.begin(), scores.end());

    // 10th, 25th, 50th, 75th, and 90th percentile weight units.
    const double weights[NUM_GETBLOCKSTATS_PERCENTILES] = {
        total_size / 10.0, total_size / 4.0, total_size / 2.0, (total_size * 3.0) / 4.0, (total_size * 9.0) / 10.0
    };

    int64_t next_percentile_index = 0;
    int64_t cumulative_weight = 0;
    for (const auto& element : scores) {
        cumulative_weight += element.second;
        while (next_percentile_index < NUM_GETBLOCKSTATS_PERCENTILES && cumulative_weight >= weights[next_percentile_index]) {
            result[next_percentile_index] = element.first;
            ++next_percentile_index;
        }
    }

    // Fill any remaining percentiles with the last value.
    for (int64_t i = next_percentile_index; i < NUM_GETBLOCKSTATS_PERCENTILES; i++) {
        result[i] = scores.back().first;
    }
}

template<typename T>
static inline bool SetHasKeys(const std::set<T>& set) {return false;}
template<typename T, typename Tk, typename... Args>
static inline bool SetHasKeys(const std::set<T>& set, const Tk& key, const Args&... args)
{
    return (set.count(key) != 0) || SetHasKeys(set, args...);
}

// outpoint (needed for the utxo index) + nHeight + fCoinBase
static constexpr size_t PER_UTXO_OVERHEAD = sizeof(COutPoint) + sizeof(uint32_t) + sizeof(bool);

static UniValue getblockstats(const JSONRPCRequest& request)
{
    RPCHelpMan{"getblockstats",
                "\nCompute per block statistics for a given window. All amounts are in duffs.\n"
                "It won't work for some heights with pruning.\n",
                {
                    {"hash_or_height", RPCArg::Type::NUM, RPCArg::Optional::NO, "The block hash or height of the target block", "", {"", "string or numeric"}},
                    {"stats", RPCArg::Type::ARR, /* default */ "all values", "Values to plot (see result below)",
                        {
                            {"height", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Selected statistic"},
                            {"time", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Selected statistic"},
                        },
                        "stats"},
                },
                RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "avgfee", "Average fee in the block"},
                {RPCResult::Type::NUM, "avgfeerate", "Average feerate (in satoshis per virtual byte)"},
                {RPCResult::Type::NUM, "avgtxsize", "Average transaction size"},
                {RPCResult::Type::STR_HEX, "blockhash", "The block hash (to check for potential reorgs)"},
                {RPCResult::Type::ARR_FIXED, "feerate_percentiles", "Feerates at the 10th, 25th, 50th, 75th, and 90th percentile weight unit (in duffs per byte)",
                {
                    {RPCResult::Type::NUM, "10th_percentile_feerate", "The 10th percentile feerate"},
                    {RPCResult::Type::NUM, "25th_percentile_feerate", "The 25th percentile feerate"},
                    {RPCResult::Type::NUM, "50th_percentile_feerate", "The 50th percentile feerate"},
                    {RPCResult::Type::NUM, "75th_percentile_feerate", "The 75th percentile feerate"},
                    {RPCResult::Type::NUM, "90th_percentile_feerate", "The 90th percentile feerate"},
                }},
                {RPCResult::Type::NUM, "height", "The height of the block"},
                {RPCResult::Type::NUM, "ins", "The number of inputs (excluding coinbase)"},
                {RPCResult::Type::NUM, "maxfee", "Maximum fee in the block"},
                {RPCResult::Type::NUM, "maxfeerate", "Maximum feerate (in satoshis per virtual byte)"},
                {RPCResult::Type::NUM, "maxtxsize", "Maximum transaction size"},
                {RPCResult::Type::NUM, "medianfee", "Truncated median fee in the block"},
                {RPCResult::Type::NUM, "mediantime", "The block median time past"},
                {RPCResult::Type::NUM, "mediantxsize", "Truncated median transaction size"},
                {RPCResult::Type::NUM, "minfee", "Minimum fee in the block"},
                {RPCResult::Type::NUM, "minfeerate", "Minimum feerate (in satoshis per virtual byte)"},
                {RPCResult::Type::NUM, "mintxsize", "Minimum transaction size"},
                {RPCResult::Type::NUM, "outs", "The number of outputs"},
                {RPCResult::Type::NUM, "subsidy", "The block subsidy"},
                {RPCResult::Type::NUM, "time", "The block time"},
                {RPCResult::Type::NUM, "total_out", "Total amount in all outputs (excluding coinbase and thus reward [ie subsidy + totalfee])"},
                {RPCResult::Type::NUM, "total_size", "Total size of all non-coinbase transactions"},
                {RPCResult::Type::NUM, "totalfee", "The fee total"},
                {RPCResult::Type::NUM, "txs", "The number of transactions (including coinbase)"},
                {RPCResult::Type::NUM, "utxo_increase", "The increase/decrease in the number of unspent outputs"},
                {RPCResult::Type::NUM, "utxo_size_inc", "The increase/decrease in size for the utxo index (not discounting op_return and similar)"},
            }},
                RPCExamples{
                    HelpExampleCli("getblockstats", R"('"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09"' '["minfeerate","avgfeerate"]')") +
                    HelpExampleCli("getblockstats", R"(1000 '["minfeerate","avgfeerate"]')") +
                    HelpExampleRpc("getblockstats", R"("00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09", ["minfeerate","avgfeerate"])") +
                    HelpExampleRpc("getblockstats", R"(1000, ["minfeerate","avgfeerate"])")
                },
    }.Check(request);

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    LOCK(cs_main);
    CBlockIndex* pindex{ParseHashOrHeight(request.params[0], chainman)};
    CHECK_NONFATAL(pindex != nullptr);

    std::set<std::string> stats;
    if (!request.params[1].isNull()) {
        const UniValue stats_univalue = request.params[1].get_array();
        for (unsigned int i = 0; i < stats_univalue.size(); i++) {
            const std::string stat = stats_univalue[i].get_str();
            stats.insert(stat);
        }
    }

    const CBlock block = GetBlockChecked(pindex);
    const CBlockUndo blockUndo = GetUndoChecked(pindex);

    const bool do_all = stats.size() == 0; // Calculate everything if nothing selected (default)
    const bool do_mediantxsize = do_all || stats.count("mediantxsize") != 0;
    const bool do_medianfee = do_all || stats.count("medianfee") != 0;
    const bool do_feerate_percentiles = do_all || stats.count("feerate_percentiles") != 0;
    const bool loop_inputs = do_all || do_medianfee || do_feerate_percentiles ||
        SetHasKeys(stats, "utxo_size_inc", "totalfee", "avgfee", "avgfeerate", "minfee", "maxfee", "minfeerate", "maxfeerate");
    const bool loop_outputs = do_all || loop_inputs || stats.count("total_out");
    const bool do_calculate_size = do_all || do_mediantxsize ||
        SetHasKeys(stats, "total_size", "avgtxsize", "mintxsize", "maxtxsize", "avgfeerate", "feerate_percentiles", "minfeerate", "maxfeerate");

    CAmount maxfee = 0;
    CAmount maxfeerate = 0;
    CAmount minfee = MAX_MONEY;
    CAmount minfeerate = MAX_MONEY;
    CAmount total_out = 0;
    CAmount totalfee = 0;
    int64_t inputs = 0;
    int64_t maxtxsize = 0;
    int64_t mintxsize = MaxBlockSize();
    int64_t outputs = 0;
    int64_t total_size = 0;
    int64_t utxo_size_inc = 0;
    std::vector<CAmount> fee_array;
    std::vector<std::pair<CAmount, int64_t>> feerate_array;
    std::vector<int64_t> txsize_array;

    for (size_t i = 0; i < block.vtx.size(); ++i) {
        const auto& tx = block.vtx.at(i);
        outputs += tx->vout.size();

        CAmount tx_total_out = 0;
        if (loop_outputs) {
            for (const CTxOut& out : tx->vout) {
                tx_total_out += out.nValue;
                utxo_size_inc += GetSerializeSize(out, PROTOCOL_VERSION) + PER_UTXO_OVERHEAD;
            }
        }

        if (tx->IsCoinBase()) {
            continue;
        }

        inputs += tx->vin.size(); // Don't count coinbase's fake input
        total_out += tx_total_out; // Don't count coinbase reward

        int64_t tx_size = 0;
        if (do_calculate_size) {

            tx_size = tx->GetTotalSize();
            if (do_mediantxsize) {
                txsize_array.push_back(tx_size);
            }
            maxtxsize = std::max(maxtxsize, tx_size);
            mintxsize = std::min(mintxsize, tx_size);
            total_size += tx_size;
        }

        if (loop_inputs) {
            CAmount tx_total_in = 0;
            const auto& txundo = blockUndo.vtxundo.at(i - 1);
            for (const Coin& coin: txundo.vprevout) {
                const CTxOut& prevoutput = coin.out;

                tx_total_in += prevoutput.nValue;
                utxo_size_inc -= GetSerializeSize(prevoutput, PROTOCOL_VERSION) + PER_UTXO_OVERHEAD;
            }

            CAmount txfee = tx_total_in - tx_total_out;
            CHECK_NONFATAL(MoneyRange(txfee));
            if (do_medianfee) {
                fee_array.push_back(txfee);
            }
            maxfee = std::max(maxfee, txfee);
            minfee = std::min(minfee, txfee);
            totalfee += txfee;

            CAmount feerate = tx_size ? txfee / tx_size : 0;
            if (do_feerate_percentiles) {
                feerate_array.emplace_back(std::make_pair(feerate, tx_size));
            }
            maxfeerate = std::max(maxfeerate, feerate);
            minfeerate = std::min(minfeerate, feerate);
        }
    }

    CAmount feerate_percentiles[NUM_GETBLOCKSTATS_PERCENTILES] = { 0 };
    CalculatePercentilesBySize(feerate_percentiles, feerate_array, total_size);

    UniValue feerates_res(UniValue::VARR);
    for (int64_t i = 0; i < NUM_GETBLOCKSTATS_PERCENTILES; i++) {
        feerates_res.push_back(feerate_percentiles[i]);
    }

    UniValue ret_all(UniValue::VOBJ);
    ret_all.pushKV("avgfee", (block.vtx.size() > 1) ? totalfee / (block.vtx.size() - 1) : 0);
    ret_all.pushKV("avgfeerate", total_size ? totalfee / total_size : 0); // Unit: sat/byte
    ret_all.pushKV("avgtxsize", (block.vtx.size() > 1) ? total_size / (block.vtx.size() - 1) : 0);
    ret_all.pushKV("blockhash", pindex->GetBlockHash().GetHex());
    ret_all.pushKV("feerate_percentiles", feerates_res);
    ret_all.pushKV("height", (int64_t)pindex->nHeight);
    ret_all.pushKV("ins", inputs);
    ret_all.pushKV("maxfee", maxfee);
    ret_all.pushKV("maxfeerate", maxfeerate);
    ret_all.pushKV("maxtxsize", maxtxsize);
    ret_all.pushKV("medianfee", CalculateTruncatedMedian(fee_array));
    ret_all.pushKV("mediantime", pindex->GetMedianTimePast());
    ret_all.pushKV("mediantxsize", CalculateTruncatedMedian(txsize_array));
    ret_all.pushKV("minfee", (minfee == MAX_MONEY) ? 0 : minfee);
    ret_all.pushKV("minfeerate", (minfeerate == MAX_MONEY) ? 0 : minfeerate);
    ret_all.pushKV("mintxsize", mintxsize == MaxBlockSize() ? 0 : mintxsize);
    ret_all.pushKV("outs", outputs);
    ret_all.pushKV("subsidy", GetBlockSubsidy(pindex, Params().GetConsensus()));
    ret_all.pushKV("time", pindex->GetBlockTime());
    ret_all.pushKV("total_out", total_out);
    ret_all.pushKV("total_size", total_size);
    ret_all.pushKV("totalfee", totalfee);
    ret_all.pushKV("txs", (int64_t)block.vtx.size());
    ret_all.pushKV("utxo_increase", outputs - inputs);
    ret_all.pushKV("utxo_size_inc", utxo_size_inc);

    if (do_all) {
        return ret_all;
    }

    UniValue ret(UniValue::VOBJ);
    for (const std::string& stat : stats) {
        const UniValue& value = ret_all[stat];
        if (value.isNull()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid selected statistic %s", stat));
        }
        ret.pushKV(stat, value);
    }
    return ret;
}

static UniValue getspecialtxes(const JSONRPCRequest& request)
{
    RPCHelpMan{"getspecialtxes",
        "Returns an array of special transactions found in the specified block\n"
        "\nIf verbosity is 0, returns tx hash for each transaction.\n"
        "If verbosity is 1, returns hex-encoded data for each transaction.\n"
        "If verbosity is 2, returns an Object with information for each transaction.\n",
        {
            {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The block hash"},
            {"type", RPCArg::Type::NUM, /* default */ "-1", "Filter special txes by type, -1 means all types"},
            {"count", RPCArg::Type::NUM, /* default */ "10", "The number of transactions to return"},
            {"skip", RPCArg::Type::NUM, /* default */ "0", "The number of transactions to skip"},
            {"verbosity", RPCArg::Type::NUM, /* default */ "0", "0 for hashes, 1 for hex-encoded data, and 2 for json object"},
        },
        {
            RPCResult{"for verbosity = 0",
                RPCResult::Type::ARR, "", "",
                    {{RPCResult::Type::STR_HEX, "", "The transaction id"}}},
            RPCResult{"for verbosity = 1",
                RPCResult::Type::ARR, "", "",
                    {{RPCResult::Type::STR_HEX, "data", "A string that is serialized, hex-encoded data for the transaction"}}},
            RPCResult{"for verbosity = 2",
                RPCResult::Type::ARR, "", "",
                    {{RPCResult::Type::ELISION, "", "The transactions in the format of the getrawtransaction RPC"}}},
        },
        RPCExamples{
            HelpExampleCli("getspecialtxes", "\"00000000000fd08c2fb661d2fcb0d49abb3a91e5f27082ce64feed3b4dede2e2\"")
    + HelpExampleRpc("getspecialtxes", "\"00000000000fd08c2fb661d2fcb0d49abb3a91e5f27082ce64feed3b4dede2e2\"")
        },
    }.Check(request);

    const NodeContext& node = EnsureAnyNodeContext(request.context);

    ChainstateManager& chainman = EnsureChainman(node);
    LOCK(cs_main);

    CTxMemPool& mempool = EnsureMemPool(node);
    LLMQContext& llmq_ctx = EnsureLLMQContext(node);

    uint256 hash(ParseHashV(request.params[0], "blockhash"));

    int nTxType = -1;
    if (!request.params[1].isNull()) {
        nTxType = request.params[1].get_int();
    }

    int nCount = 10;
    if (!request.params[2].isNull()) {
        nCount = request.params[2].get_int();
        if (nCount < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative count");
    }

    int nSkip = 0;
    if (!request.params[3].isNull()) {
        nSkip = request.params[3].get_int();
        if (nSkip < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative skip");
    }

    int nVerbosity = 0;
    if (!request.params[4].isNull()) {
        nVerbosity = request.params[4].get_int();
        if (nVerbosity < 0 || nVerbosity > 2) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Verbosity must be in range 0..2");
        }
    }

    const CBlockIndex* pblockindex = chainman.m_blockman.LookupBlockIndex(hash);
    if (!pblockindex) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
    }

    const CBlock block = GetBlockChecked(pblockindex);

    int nTxNum = 0;
    UniValue result(UniValue::VARR);

    for(const auto& tx : block.vtx)
    {
        if (tx->nVersion != 3 || tx->nType == TRANSACTION_NORMAL // ensure it's in fact a special tx
            || (nTxType != -1 && tx->nType != nTxType)) { // ensure special tx type matches filter, if given
                continue;
        }

        nTxNum++;
        if (nTxNum <= nSkip) continue;
        if (nTxNum > nSkip + nCount) break;

        switch (nVerbosity)
        {
            case 0 : result.push_back(tx->GetHash().GetHex()); break;
            case 1 : result.push_back(EncodeHexTx(*tx)); break;
            case 2 :
                {
                    UniValue objTx(UniValue::VOBJ);
                    TxToJSON(*tx, uint256(), mempool, chainman.ActiveChainstate(), *llmq_ctx.clhandler, *llmq_ctx.isman, objTx);
                    result.push_back(objTx);
                    break;
                }
            default : throw JSONRPCError(RPC_INTERNAL_ERROR, "Unsupported verbosity");
        }
    }

    return result;
}

static UniValue savemempool(const JSONRPCRequest& request)
{
    RPCHelpMan{"savemempool",
        "\nDumps the mempool to disk. It will fail until the previous dump is fully loaded.\n",
        {},
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
            HelpExampleCli("savemempool", "")
    + HelpExampleRpc("savemempool", "")
        },
    }.Check(request);

    const CTxMemPool& mempool = EnsureAnyMemPool(request.context);

    if (!mempool.IsLoaded()) {
        throw JSONRPCError(RPC_MISC_ERROR, "The mempool was not loaded yet");
    }

    if (!DumpMempool(mempool)) {
        throw JSONRPCError(RPC_MISC_ERROR, "Unable to dump mempool to disk");
    }

    return NullUniValue;
}

namespace {
//! Search for a given set of pubkey scripts
bool FindScriptPubKey(std::atomic<int>& scan_progress, const std::atomic<bool>& should_abort, int64_t& count, CCoinsViewCursor* cursor, const std::set<CScript>& needles, std::map<COutPoint, Coin>& out_results, std::function<void()>& interruption_point)
{
    scan_progress = 0;
    count = 0;
    while (cursor->Valid()) {
        COutPoint key;
        Coin coin;
        if (!cursor->GetKey(key) || !cursor->GetValue(coin)) return false;
        if (++count % 8192 == 0) {
            interruption_point();
            if (should_abort) {
                // allow to abort the scan via the abort reference
                return false;
            }
        }
        if (count % 256 == 0) {
            // update progress reference every 256 item
            uint32_t high = 0x100 * *key.hash.begin() + *(key.hash.begin() + 1);
            scan_progress = (int)(high * 100.0 / 65536.0 + 0.5);
        }
        if (needles.count(coin.out.scriptPubKey)) {
            out_results.emplace(key, coin);
        }
        cursor->Next();
    }
    scan_progress = 100;
    return true;
}
} // namespace

/** RAII object to prevent concurrency issue when scanning the txout set */
static std::mutex g_utxosetscan;
static std::atomic<int> g_scan_progress;
static std::atomic<bool> g_scan_in_progress;
static std::atomic<bool> g_should_abort_scan;
class CoinsViewScanReserver
{
private:
    bool m_could_reserve;
public:
    explicit CoinsViewScanReserver() : m_could_reserve(false) {}

    bool reserve() {
        CHECK_NONFATAL(!m_could_reserve);
        std::lock_guard<std::mutex> lock(g_utxosetscan);
        if (g_scan_in_progress) {
            return false;
        }
        g_scan_in_progress = true;
        m_could_reserve = true;
        return true;
    }

    ~CoinsViewScanReserver() {
        if (m_could_reserve) {
            std::lock_guard<std::mutex> lock(g_utxosetscan);
            g_scan_in_progress = false;
        }
    }
};

UniValue scantxoutset(const JSONRPCRequest& request)
{
    RPCHelpMan{"scantxoutset",
        "\nEXPERIMENTAL warning: this call may be removed or changed in future releases.\n"
        "\nScans the unspent transaction output set for entries that match certain output descriptors.\n"
        "Examples of output descriptors are:\n"
        "    addr(<address>)                      Outputs whose scriptPubKey corresponds to the specified address (does not include P2PK)\n"
        "    raw(<hex script>)                    Outputs whose scriptPubKey equals the specified hex scripts\n"
        "    combo(<pubkey>)                      P2PK and P2PKH outputs for the given pubkey\n"
        "    pkh(<pubkey>)                        P2PKH outputs for the given pubkey\n"
        "    sh(multi(<n>,<pubkey>,<pubkey>,...)) P2SH-multisig outputs for the given threshold and pubkeys\n"
        "\nIn the above, <pubkey> either refers to a fixed public key in hexadecimal notation, or to an xpub/xprv optionally followed by one\n"
        "or more path elements separated by \"/\", and optionally ending in \"/*\" (unhardened), or \"/*'\" or \"/*h\" (hardened) to specify all\n"
        "unhardened or hardened child keys.\n"
        "In the latter case, a range needs to be specified by below if different from 1000.\n"
        "For more information on output descriptors, see the documentation in the doc/descriptors.md file.\n",
        {
            {"action", RPCArg::Type::STR, RPCArg::Optional::NO, "The action to execute\n"
    "                                      \"start\" for starting a scan\n"
    "                                      \"abort\" for aborting the current scan (returns true when abort was successful)\n"
    "                                      \"status\" for progress report (in %) of the current scan"},
            {"scanobjects", RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "Array of scan objects. Required for \"start\" action\n"
    "                                  Every scan object is either a string descriptor or an object:",
                {
                    {"descriptor", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "An output descriptor"},
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "An object with output descriptor and metadata",
                        {
                            {"desc", RPCArg::Type::STR, RPCArg::Optional::NO, "An output descriptor"},
                            {"range", RPCArg::Type::RANGE, /* default */ "1000", "The range of HD chain indexes to explore (either end or [begin,end])"},
                        },
                    },
                },
                "[scanobjects,...]"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "success", "Whether the scan was completed"},
                {RPCResult::Type::NUM, "txouts", "The number of unspent transaction outputs scanned"},
                {RPCResult::Type::NUM, "height", "The current block height (index)"},
                {RPCResult::Type::STR_HEX, "bestblock", "The hash of the block at the tip of the chain"},
                {RPCResult::Type::ARR, "unspents", "",
                    {
                        {RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
                                {RPCResult::Type::NUM, "vout", "The vout value"},
                                {RPCResult::Type::STR_HEX, "scriptPubKey", "The script key"},
                                {RPCResult::Type::STR, "desc", "A specialized descriptor for the matched scriptPubKey"},
                                {RPCResult::Type::STR_AMOUNT, "amount", "The total amount in " + CURRENCY_UNIT + " of the unspent output"},
                                {RPCResult::Type::NUM, "height", "Height of the unspent transaction output"},
                            }},
                    }},
                {RPCResult::Type::STR_AMOUNT, "total_amount", "The total amount of all found unspent outputs in " + CURRENCY_UNIT},
            }},
        RPCExamples{""},
    }.Check(request);

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VARR});

    UniValue result(UniValue::VOBJ);
    if (request.params[0].get_str() == "status") {
        CoinsViewScanReserver reserver;
        if (reserver.reserve()) {
            // no scan in progress
            return NullUniValue;
        }
        result.pushKV("progress", g_scan_progress);
        return result;
    } else if (request.params[0].get_str() == "abort") {
        CoinsViewScanReserver reserver;
        if (reserver.reserve()) {
            // reserve was possible which means no scan was running
            return false;
        }
        // set the abort flag
        g_should_abort_scan = true;
        return true;
    } else if (request.params[0].get_str() == "start") {
        CoinsViewScanReserver reserver;
        if (!reserver.reserve()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Scan already in progress, use action \"abort\" or \"status\"");
        }

        if (request.params.size() < 2) {
            throw JSONRPCError(RPC_MISC_ERROR, "scanobjects argument is required for the start action");
        }

        std::set<CScript> needles;
        std::map<CScript, std::string> descriptors;
        CAmount total_in = 0;

        // loop through the scan objects
        for (const UniValue& scanobject : request.params[1].get_array().getValues()) {
            FlatSigningProvider provider;
            auto scripts = EvalDescriptorStringOrObject(scanobject, provider);
            for (const auto& script : scripts) {
                std::string inferred = InferDescriptor(script, provider)->ToString();
                needles.emplace(script);
                descriptors.emplace(std::move(script), std::move(inferred));
            }
        }

        // Scan the unspent transaction output set for inputs
        UniValue unspents(UniValue::VARR);
        std::vector<CTxOut> input_txos;
        std::map<COutPoint, Coin> coins;
        g_should_abort_scan = false;
        g_scan_progress = 0;
        int64_t count = 0;
        std::unique_ptr<CCoinsViewCursor> pcursor;
        CBlockIndex* tip;
        NodeContext& node = EnsureAnyNodeContext(request.context);
        {
            ChainstateManager& chainman = EnsureChainman(node);
            LOCK(cs_main);
            CChainState& active_chainstate = chainman.ActiveChainstate();
            active_chainstate.ForceFlushStateToDisk();
            pcursor = std::unique_ptr<CCoinsViewCursor>(active_chainstate.CoinsDB().Cursor());
            CHECK_NONFATAL(pcursor);
            tip = active_chainstate.m_chain.Tip();
            CHECK_NONFATAL(tip);
        }
        bool res = FindScriptPubKey(g_scan_progress, g_should_abort_scan, count, pcursor.get(), needles, coins, node.rpc_interruption_point);
        result.pushKV("success", res);
        result.pushKV("txouts", count);
        result.pushKV("height", tip->nHeight);
        result.pushKV("bestblock", tip->GetBlockHash().GetHex());

        for (const auto& it : coins) {
            const COutPoint& outpoint = it.first;
            const Coin& coin = it.second;
            const CTxOut& txo = coin.out;
            input_txos.push_back(txo);
            total_in += txo.nValue;

            UniValue unspent(UniValue::VOBJ);
            unspent.pushKV("txid", outpoint.hash.GetHex());
            unspent.pushKV("vout", (int32_t)outpoint.n);
            unspent.pushKV("scriptPubKey", HexStr(txo.scriptPubKey));
            unspent.pushKV("desc", descriptors[txo.scriptPubKey]);
            unspent.pushKV("amount", ValueFromAmount(txo.nValue));
            unspent.pushKV("height", (int32_t)coin.nHeight);

            unspents.push_back(unspent);
        }
        result.pushKV("unspents", unspents);
        result.pushKV("total_amount", ValueFromAmount(total_in));
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid command");
    }
    return result;
}

static UniValue getblockfilter(const JSONRPCRequest& request)
{
    RPCHelpMan{"getblockfilter",
        "\nRetrieve a BIP 157 content filter for a particular block.\n",
        {
            {"blockhash", RPCArg::Type::STR, RPCArg::Optional::NO, "The hash of the block"},
            {"filtertype", RPCArg::Type::STR, /* default */ "basic", "The type name of the filter"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "filter", "the hex-encoded filter data"},
                {RPCResult::Type::STR_HEX, "header", "the hex-encoded filter header"},
            }},
        RPCExamples{
            HelpExampleCli("getblockfilter", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\" \"basic\"")+
            HelpExampleRpc("getblockfilter", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\", \"basic\"")
        },
    }.Check(request);

    uint256 block_hash(ParseHashV(request.params[0], "blockhash"));
    std::string filtertype_name = "basic";
    if (!request.params[1].isNull()) {
        filtertype_name = request.params[1].get_str();
    }

    BlockFilterType filtertype;
    if (!BlockFilterTypeByName(filtertype_name, filtertype)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown filtertype");
    }

    BlockFilterIndex* index = GetBlockFilterIndex(filtertype);
    if (!index) {
        throw JSONRPCError(RPC_MISC_ERROR, "Index is not enabled for filtertype " + filtertype_name);
    }

    const CBlockIndex* block_index;
    bool block_was_connected;
    {
        ChainstateManager& chainman = EnsureAnyChainman(request.context);
        LOCK(cs_main);
        block_index = chainman.m_blockman.LookupBlockIndex(block_hash);
        if (!block_index) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }
        block_was_connected = block_index->IsValid(BLOCK_VALID_SCRIPTS);
    }

    bool index_ready = index->BlockUntilSyncedToCurrentChain();

    BlockFilter filter;
    uint256 filter_header;
    if (!index->LookupFilter(block_index, filter) ||
        !index->LookupFilterHeader(block_index, filter_header)) {
        int err_code;
        std::string errmsg = "Filter not found.";

        if (!block_was_connected) {
            err_code = RPC_INVALID_ADDRESS_OR_KEY;
            errmsg += " Block was not connected to active chain.";
        } else if (!index_ready) {
            err_code = RPC_MISC_ERROR;
            errmsg += " Block filters are still in the process of being indexed.";
        } else {
            err_code = RPC_INTERNAL_ERROR;
            errmsg += " This error is unexpected and indicates index corruption.";
        }

        throw JSONRPCError(err_code, errmsg);
    }

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("filter", HexStr(filter.GetEncodedFilter()));
    ret.pushKV("header", filter_header.GetHex());
    return ret;
}

/**
 * Serialize the UTXO set to a file for loading elsewhere.
 *
 * @see SnapshotMetadata
 */
UniValue dumptxoutset(const JSONRPCRequest& request)
{
    RPCHelpMan{
        "dumptxoutset",
        "Write the serialized UTXO set to disk.",
        {
            {"path", RPCArg::Type::STR, RPCArg::Optional::NO, "Path to the output file. If relative, will be prefixed by datadir."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::NUM, "coins_written", "the number of coins written in the snapshot"},
                    {RPCResult::Type::STR_HEX, "base_hash", "the hash of the base of the snapshot"},
                    {RPCResult::Type::NUM, "base_height", "the height of the base of the snapshot"},
                    {RPCResult::Type::STR, "path", "the absolute path that the snapshot was written to"},
                }
        },
        RPCExamples{
            HelpExampleCli("dumptxoutset", "utxo.dat")
        }
    }.Check(request);

    fs::path path = fs::absolute(request.params[0].get_str(), GetDataDir());
    // Write to a temporary path and then move into `path` on completion
    // to avoid confusion due to an interruption.
    fs::path temppath = fs::absolute(request.params[0].get_str() + ".incomplete", GetDataDir());

    if (fs::exists(path)) {
        throw JSONRPCError(
            RPC_INVALID_PARAMETER,
            path.string() + " already exists. If you are sure this is what you want, "
            "move it out of the way first");
    }

    FILE* file{fsbridge::fopen(temppath, "wb")};
    CAutoFile afile{file, SER_DISK, CLIENT_VERSION};
    NodeContext& node = EnsureAnyNodeContext(request.context);
    UniValue result = CreateUTXOSnapshot(node, node.chainman->ActiveChainstate(), afile);
    fs::rename(temppath, path);

    result.pushKV("path", path.string());
    return result;
}

UniValue CreateUTXOSnapshot(NodeContext& node, CChainState& chainstate, CAutoFile& afile)
{
    std::unique_ptr<CCoinsViewCursor> pcursor;
    CCoinsStats stats{CoinStatsHashType::NONE};
    CBlockIndex* tip;

    {
        // We need to lock cs_main to ensure that the coinsdb isn't written to
        // between (i) flushing coins cache to disk (coinsdb), (ii) getting stats
        // based upon the coinsdb, and (iii) constructing a cursor to the
        // coinsdb for use below this block.
        //
        // Cursors returned by leveldb iterate over snapshots, so the contents
        // of the pcursor will not be affected by simultaneous writes during
        // use below this block.
        //
        // See discussion here:
        //   https://github.com/bitcoin/bitcoin/pull/15606#discussion_r274479369
        //
        LOCK(::cs_main);

        chainstate.ForceFlushStateToDisk();

        if (!GetUTXOStats(&chainstate.CoinsDB(), chainstate.m_blockman, stats, node.rpc_interruption_point)) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Unable to read UTXO set");
        }

        pcursor = std::unique_ptr<CCoinsViewCursor>(chainstate.CoinsDB().Cursor());
        tip = chainstate.m_blockman.LookupBlockIndex(stats.hashBlock);
        CHECK_NONFATAL(tip);
    }

    SnapshotMetadata metadata{tip->GetBlockHash(), stats.coins_count, tip->nChainTx};

    afile << metadata;

    COutPoint key;
    Coin coin;
    unsigned int iter{0};

    while (pcursor->Valid()) {
        if (iter % 5000 == 0) node.rpc_interruption_point();
        ++iter;
        if (pcursor->GetKey(key) && pcursor->GetValue(coin)) {
            afile << key;
            afile << coin;
        }

        pcursor->Next();
    }

    afile.fclose();

    UniValue result(UniValue::VOBJ);
    result.pushKV("coins_written", stats.coins_count);
    result.pushKV("base_hash", tip->GetBlockHash().ToString());
    result.pushKV("base_height", tip->nHeight);

    return result;
}

void RegisterBlockchainRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
  //  --------------------- ------------------------  -----------------------  ----------
    { "blockchain",         "getblockchaininfo",      &getblockchaininfo,      {} },
    { "blockchain",         "getchaintxstats",        &getchaintxstats,        {"nblocks", "blockhash"} },
    { "blockchain",         "getblockstats",          &getblockstats,          {"hash_or_height", "stats"} },
    { "blockchain",         "getbestblockhash",       &getbestblockhash,       {} },
    { "blockchain",         "getbestchainlock",       &getbestchainlock,       {} },
    { "blockchain",         "getblockcount",          &getblockcount,          {} },
    { "blockchain",         "getblock",               &getblock,               {"blockhash","verbosity|verbose"} },
    { "blockchain",         "getblockhashes",         &getblockhashes,         {"high","low"} },
    { "blockchain",         "getblockhash",           &getblockhash,           {"height"} },
    { "blockchain",         "getblockheader",         &getblockheader,         {"blockhash","verbose"} },
    { "blockchain",         "getblockheaders",        &getblockheaders,        {"blockhash","count","verbose"} },
    { "blockchain",         "getmerkleblocks",        &getmerkleblocks,        {"filter","blockhash","count"} },
    { "blockchain",         "getchaintips",           &getchaintips,           {"count","branchlen"} },
    { "blockchain",         "getdifficulty",          &getdifficulty,          {} },
    { "blockchain",         "getmempoolancestors",    &getmempoolancestors,    {"txid","verbose"} },
    { "blockchain",         "getmempooldescendants",  &getmempooldescendants,  {"txid","verbose"} },
    { "blockchain",         "getmempoolentry",        &getmempoolentry,        {"txid"} },
    { "blockchain",         "getmempoolinfo",         &getmempoolinfo,         {} },
    { "blockchain",         "getrawmempool",          &getrawmempool,          {"verbose"} },
    { "blockchain",         "getspecialtxes",         &getspecialtxes,         {"blockhash", "type", "count", "skip", "verbosity"} },
    { "blockchain",         "gettxout",               &gettxout,               {"txid","n","include_mempool"} },
    { "blockchain",         "gettxoutsetinfo",        &gettxoutsetinfo,        {"hash_type", "hash_or_height", "use_index"} },
    { "blockchain",         "pruneblockchain",        &pruneblockchain,        {"height"} },
    { "blockchain",         "savemempool",            &savemempool,            {} },
    { "blockchain",         "verifychain",            &verifychain,            {"checklevel","nblocks"} },

    { "blockchain",         "preciousblock",          &preciousblock,          {"blockhash"} },
    { "blockchain",         "scantxoutset",           &scantxoutset,           {"action", "scanobjects"} },
    { "blockchain",         "getblockfilter",         &getblockfilter,         {"blockhash", "filtertype"} },

    /* Not shown in help */
    { "hidden",             "invalidateblock",        &invalidateblock,        {"blockhash"} },
    { "hidden",             "reconsiderblock",        &reconsiderblock,        {"blockhash"} },
    { "hidden",             "waitfornewblock",        &waitfornewblock,        {"timeout"} },
    { "hidden",             "waitforblock",           &waitforblock,           {"blockhash","timeout"} },
    { "hidden",             "waitforblockheight",     &waitforblockheight,     {"height","timeout"} },
    { "hidden",             "syncwithvalidationinterfacequeue", &syncwithvalidationinterfacequeue, {} },
    { "hidden",             "dumptxoutset",           &dumptxoutset,           {"path"} },
};
// clang-format on

    for (const auto& command : commands) {
        t.appendCommand(command.name, &command);
    }
}
