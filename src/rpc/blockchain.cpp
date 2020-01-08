// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/blockchain.h>

#include <amount.h>
#include <blockfilter.h>
#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <node/coinstats.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <hash.h>
#include <index/blockfilterindex.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <policy/rbf.h>
#include <primitives/transaction.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <script/descriptor.h>
#include <streams.h>
#include <sync.h>
#include <txdb.h>
#include <txmempool.h>
#include <undo.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/validation.h>
#include <validation.h>
#include <validationinterface.h>
#include <versionbitsinfo.h>
#include <warnings.h>

#include <assert.h>
#include <stdint.h>

#include <univalue.h>

#include <boost/thread/thread.hpp> // boost::thread::interrupt

#include <condition_variable>
#include <memory>
#include <mutex>

struct CUpdatedBlock
{
    uint256 hash;
    int height;
};

static Mutex cs_blockchange;
static std::condition_variable cond_blockchange;
static CUpdatedBlock latestblock;

/* Calculate the difficulty for a given block index.
 */
double GetDifficulty(const CBlockIndex* blockindex)
{
    assert(blockindex);

    int nShift = (blockindex->nBits >> 24) & 0xff;
    double dDiff =
        (double)0x0000ffff / (double)(blockindex->nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
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

UniValue blockheaderToJSON(const CBlockIndex* tip, const CBlockIndex* blockindex)
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
    return result;
}

UniValue blockToJSON(const CBlock& block, const CBlockIndex* tip, const CBlockIndex* blockindex, bool txDetails)
{
    // Serialize passed information without accessing chain state of the active chain!
    AssertLockNotHeld(cs_main); // For performance reasons

    UniValue result(UniValue::VOBJ);
    result.pushKV("hash", blockindex->GetBlockHash().GetHex());
    const CBlockIndex* pnext;
    int confirmations = ComputeNextBlockAndDepth(tip, blockindex, pnext);
    result.pushKV("confirmations", confirmations);
    result.pushKV("strippedsize", (int)::GetSerializeSize(block, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS));
    result.pushKV("size", (int)::GetSerializeSize(block, PROTOCOL_VERSION));
    result.pushKV("weight", (int)::GetBlockWeight(block));
    result.pushKV("height", blockindex->nHeight);
    result.pushKV("version", block.nVersion);
    result.pushKV("versionHex", strprintf("%08x", block.nVersion));
    result.pushKV("merkleroot", block.hashMerkleRoot.GetHex());
    UniValue txs(UniValue::VARR);
    for(const auto& tx : block.vtx)
    {
        if(txDetails)
        {
            UniValue objTx(UniValue::VOBJ);
            TxToUniv(*tx, uint256(), objTx, true, RPCSerializationFlags());
            txs.push_back(objTx);
        }
        else
            txs.push_back(tx->GetHash().GetHex());
    }
    result.pushKV("tx", txs);
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
    return result;
}

static UniValue getblockcount(const JSONRPCRequest& request)
{
            RPCHelpMan{"getblockcount",
                "\nReturns the height of the most-work fully-validated chain.\n"
                "The genesis block has height 0.\n",
                {},
                RPCResult{
            "n    (numeric) The current block count\n"
                },
                RPCExamples{
                    HelpExampleCli("getblockcount", "")
            + HelpExampleRpc("getblockcount", "")
                },
            }.Check(request);

    LOCK(cs_main);
    return ::ChainActive().Height();
}

static UniValue getbestblockhash(const JSONRPCRequest& request)
{
            RPCHelpMan{"getbestblockhash",
                "\nReturns the hash of the best (tip) block in the most-work fully-validated chain.\n",
                {},
                RPCResult{
            "\"hex\"      (string) the block hash, hex-encoded\n"
                },
                RPCExamples{
                    HelpExampleCli("getbestblockhash", "")
            + HelpExampleRpc("getbestblockhash", "")
                },
            }.Check(request);

    LOCK(cs_main);
    return ::ChainActive().Tip()->GetBlockHash().GetHex();
}

void RPCNotifyBlockChange(bool ibd, const CBlockIndex * pindex)
{
    if(pindex) {
        std::lock_guard<std::mutex> lock(cs_blockchange);
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
            "{                           (json object)\n"
            "  \"hash\" : {       (string) The blockhash\n"
            "  \"height\" : {     (int) Block height\n"
            "}\n"
                },
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
            cond_blockchange.wait_for(lock, std::chrono::milliseconds(timeout), [&block]{return latestblock.height != block.height || latestblock.hash != block.hash || !IsRPCRunning(); });
        else
            cond_blockchange.wait(lock, [&block]{return latestblock.height != block.height || latestblock.hash != block.hash || !IsRPCRunning(); });
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
            "{                           (json object)\n"
            "  \"hash\" : {       (string) The blockhash\n"
            "  \"height\" : {     (int) Block height\n"
            "}\n"
                },
                RPCExamples{
                    HelpExampleCli("waitforblock", "\"0000000000079f8ef3d2c688c244eb7a4570b24c9ed7b4a8c619eb02596f8862\", 1000")
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
            cond_blockchange.wait_for(lock, std::chrono::milliseconds(timeout), [&hash]{return latestblock.hash == hash || !IsRPCRunning();});
        else
            cond_blockchange.wait(lock, [&hash]{return latestblock.hash == hash || !IsRPCRunning(); });
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
            "{                           (json object)\n"
            "  \"hash\" : {       (string) The blockhash\n"
            "  \"height\" : {     (int) Block height\n"
            "}\n"
                },
                RPCExamples{
                    HelpExampleCli("waitforblockheight", "\"100\", 1000")
            + HelpExampleRpc("waitforblockheight", "\"100\", 1000")
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
            cond_blockchange.wait_for(lock, std::chrono::milliseconds(timeout), [&height]{return latestblock.height >= height || !IsRPCRunning();});
        else
            cond_blockchange.wait(lock, [&height]{return latestblock.height >= height || !IsRPCRunning(); });
        block = latestblock;
    }
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
                RPCResults{},
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
            "n.nnn       (numeric) the proof-of-work difficulty as a multiple of the minimum difficulty.\n"
                },
                RPCExamples{
                    HelpExampleCli("getdifficulty", "")
            + HelpExampleRpc("getdifficulty", "")
                },
            }.Check(request);

    LOCK(cs_main);
    return GetDifficulty(::ChainActive().Tip());
}

static std::string EntryDescriptionString()
{
    return "    \"vsize\" : n,            (numeric) virtual transaction size as defined in BIP 141. This is different from actual serialized size for witness transactions as witness data is discounted.\n"
           "    \"size\" : n,             (numeric) (DEPRECATED) same as vsize. Only returned if bitcoind is started with -deprecatedrpc=size\n"
           "                              size will be completely removed in v0.20.\n"
           "    \"weight\" : n,           (numeric) transaction weight as defined in BIP 141.\n"
           "    \"fee\" : n,              (numeric) transaction fee in " + CURRENCY_UNIT + " (DEPRECATED)\n"
           "    \"modifiedfee\" : n,      (numeric) transaction fee with fee deltas used for mining priority (DEPRECATED)\n"
           "    \"time\" : n,             (numeric) local time transaction entered pool in seconds since 1 Jan 1970 GMT\n"
           "    \"height\" : n,           (numeric) block height when transaction entered pool\n"
           "    \"descendantcount\" : n,  (numeric) number of in-mempool descendant transactions (including this one)\n"
           "    \"descendantsize\" : n,   (numeric) virtual transaction size of in-mempool descendants (including this one)\n"
           "    \"descendantfees\" : n,   (numeric) modified fees (see above) of in-mempool descendants (including this one) (DEPRECATED)\n"
           "    \"ancestorcount\" : n,    (numeric) number of in-mempool ancestor transactions (including this one)\n"
           "    \"ancestorsize\" : n,     (numeric) virtual transaction size of in-mempool ancestors (including this one)\n"
           "    \"ancestorfees\" : n,     (numeric) modified fees (see above) of in-mempool ancestors (including this one) (DEPRECATED)\n"
           "    \"wtxid\" : hash,         (string) hash of serialized transaction, including witness data\n"
           "    \"fees\" : {\n"
           "        \"base\" : n,         (numeric) transaction fee in " + CURRENCY_UNIT + "\n"
           "        \"modified\" : n,     (numeric) transaction fee with fee deltas used for mining priority in " + CURRENCY_UNIT + "\n"
           "        \"ancestor\" : n,     (numeric) modified fees (see above) of in-mempool ancestors (including this one) in " + CURRENCY_UNIT + "\n"
           "        \"descendant\" : n,   (numeric) modified fees (see above) of in-mempool descendants (including this one) in " + CURRENCY_UNIT + "\n"
           "    }\n"
           "    \"depends\" : [           (array) unconfirmed transactions used as inputs for this transaction\n"
           "        \"transactionid\",    (string) parent transaction id\n"
           "       ... ]\n"
           "    \"spentby\" : [           (array) unconfirmed transactions spending outputs from this transaction\n"
           "        \"transactionid\",    (string) child transaction id\n"
           "       ... ]\n"
           "    \"bip125-replaceable\" : true|false,  (boolean) Whether this transaction could be replaced due to BIP125 (replace-by-fee)\n";
}

static void entryToJSON(const CTxMemPool& pool, UniValue& info, const CTxMemPoolEntry& e) EXCLUSIVE_LOCKS_REQUIRED(pool.cs)
{
    AssertLockHeld(pool.cs);

    UniValue fees(UniValue::VOBJ);
    fees.pushKV("base", ValueFromAmount(e.GetFee()));
    fees.pushKV("modified", ValueFromAmount(e.GetModifiedFee()));
    fees.pushKV("ancestor", ValueFromAmount(e.GetModFeesWithAncestors()));
    fees.pushKV("descendant", ValueFromAmount(e.GetModFeesWithDescendants()));
    info.pushKV("fees", fees);

    info.pushKV("vsize", (int)e.GetTxSize());
    if (IsDeprecatedRPCEnabled("size")) info.pushKV("size", (int)e.GetTxSize());
    info.pushKV("weight", (int)e.GetTxWeight());
    info.pushKV("fee", ValueFromAmount(e.GetFee()));
    info.pushKV("modifiedfee", ValueFromAmount(e.GetModifiedFee()));
    info.pushKV("time", e.GetTime());
    info.pushKV("height", (int)e.GetHeight());
    info.pushKV("descendantcount", e.GetCountWithDescendants());
    info.pushKV("descendantsize", e.GetSizeWithDescendants());
    info.pushKV("descendantfees", e.GetModFeesWithDescendants());
    info.pushKV("ancestorcount", e.GetCountWithAncestors());
    info.pushKV("ancestorsize", e.GetSizeWithAncestors());
    info.pushKV("ancestorfees", e.GetModFeesWithAncestors());
    info.pushKV("wtxid", pool.vTxHashes[e.vTxHashesIdx].first.ToString());
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

    // Add opt-in RBF status
    bool rbfStatus = false;
    RBFTransactionState rbfState = IsRBFOptIn(tx, pool);
    if (rbfState == RBFTransactionState::UNKNOWN) {
        throw JSONRPCError(RPC_MISC_ERROR, "Transaction is not in mempool");
    } else if (rbfState == RBFTransactionState::REPLACEABLE_BIP125) {
        rbfStatus = true;
    }

    info.pushKV("bip125-replaceable", rbfStatus);
}

UniValue MempoolToJSON(const CTxMemPool& pool, bool verbose)
{
    if (verbose) {
        LOCK(pool.cs);
        UniValue o(UniValue::VOBJ);
        for (const CTxMemPoolEntry& e : pool.mapTx) {
            const uint256& hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(pool, info, e);
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
                RPCResult{"for verbose = false",
            "[                     (json array of string)\n"
            "  \"transactionid\"     (string) The transaction id\n"
            "  ,...\n"
            "]\n"
            "\nResult: (for verbose = true):\n"
            "{                           (json object)\n"
            "  \"transactionid\" : {       (json object)\n"
            + EntryDescriptionString()
            + "  }, ...\n"
            "}\n"
                },
                RPCExamples{
                    HelpExampleCli("getrawmempool", "true")
            + HelpExampleRpc("getrawmempool", "true")
                },
            }.Check(request);

    bool fVerbose = false;
    if (!request.params[0].isNull())
        fVerbose = request.params[0].get_bool();

    return MempoolToJSON(::mempool, fVerbose);
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
            "[                       (json array of strings)\n"
            "  \"transactionid\"           (string) The transaction id of an in-mempool ancestor transaction\n"
            "  ,...\n"
            "]\n"
                    },
                    RPCResult{"for verbose = true",
            "{                           (json object)\n"
            "  \"transactionid\" : {       (json object)\n"
            + EntryDescriptionString()
            + "  }, ...\n"
            "}\n"
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

    uint256 hash = ParseHashV(request.params[0], "parameter 1");

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
        for (CTxMemPool::txiter ancestorIt : setAncestors) {
            const CTxMemPoolEntry &e = *ancestorIt;
            const uint256& _hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(::mempool, info, e);
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
            "[                       (json array of strings)\n"
            "  \"transactionid\"           (string) The transaction id of an in-mempool descendant transaction\n"
            "  ,...\n"
            "]\n"
                    },
                    RPCResult{"for verbose = true",
            "{                           (json object)\n"
            "  \"transactionid\" : {       (json object)\n"
            + EntryDescriptionString()
            + "  }, ...\n"
            "}\n"
                    },
                },
                RPCExamples{
                    HelpExampleCli("getmempooldescendants", "\"mytxid\"")
            + HelpExampleRpc("getmempooldescendants", "\"mytxid\"")
                },
            }.Check(request);

    bool fVerbose = false;
    if (!request.params[1].isNull())
        fVerbose = request.params[1].get_bool();

    uint256 hash = ParseHashV(request.params[0], "parameter 1");

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
        for (CTxMemPool::txiter descendantIt : setDescendants) {
            const CTxMemPoolEntry &e = *descendantIt;
            const uint256& _hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(::mempool, info, e);
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
            "{                           (json object)\n"
            + EntryDescriptionString()
            + "}\n"
                },
                RPCExamples{
                    HelpExampleCli("getmempoolentry", "\"mytxid\"")
            + HelpExampleRpc("getmempoolentry", "\"mytxid\"")
                },
            }.Check(request);

    uint256 hash = ParseHashV(request.params[0], "parameter 1");

    LOCK(mempool.cs);

    CTxMemPool::txiter it = mempool.mapTx.find(hash);
    if (it == mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    const CTxMemPoolEntry &e = *it;
    UniValue info(UniValue::VOBJ);
    entryToJSON(::mempool, info, e);
    return info;
}

static UniValue getblockhash(const JSONRPCRequest& request)
{
            RPCHelpMan{"getblockhash",
                "\nReturns hash of block in best-block-chain at height provided.\n",
                {
                    {"height", RPCArg::Type::NUM, RPCArg::Optional::NO, "The height index"},
                },
                RPCResult{
            "\"hash\"         (string) The block hash\n"
                },
                RPCExamples{
                    HelpExampleCli("getblockhash", "1000")
            + HelpExampleRpc("getblockhash", "1000")
                },
            }.Check(request);

    LOCK(cs_main);

    int nHeight = request.params[0].get_int();
    if (nHeight < 0 || nHeight > ::ChainActive().Height())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");

    CBlockIndex* pblockindex = ::ChainActive()[nHeight];
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
            "{\n"
            "  \"hash\" : \"hash\",     (string) the block hash (same as provided)\n"
            "  \"confirmations\" : n,   (numeric) The number of confirmations, or -1 if the block is not on the main chain\n"
            "  \"height\" : n,          (numeric) The block height or index\n"
            "  \"version\" : n,         (numeric) The block version\n"
            "  \"versionHex\" : \"00000000\", (string) The block version formatted in hexadecimal\n"
            "  \"merkleroot\" : \"xxxx\", (string) The merkle root\n"
            "  \"time\" : ttt,          (numeric) The block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"mediantime\" : ttt,    (numeric) The median block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"nonce\" : n,           (numeric) The nonce\n"
            "  \"bits\" : \"1d00ffff\", (string) The bits\n"
            "  \"difficulty\" : x.xxx,  (numeric) The difficulty\n"
            "  \"chainwork\" : \"0000...1f3\"     (string) Expected number of hashes required to produce the current chain (in hex)\n"
            "  \"nTx\" : n,             (numeric) The number of transactions in the block.\n"
            "  \"previousblockhash\" : \"hash\",  (string) The hash of the previous block\n"
            "  \"nextblockhash\" : \"hash\",      (string) The hash of the next block\n"
            "}\n"
                    },
                    RPCResult{"for verbose=false",
            "\"data\"             (string) A string that is serialized, hex-encoded data for block 'hash'.\n"
                    },
                },
                RPCExamples{
                    HelpExampleCli("getblockheader", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
            + HelpExampleRpc("getblockheader", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
                },
            }.Check(request);

    uint256 hash(ParseHashV(request.params[0], "hash"));

    bool fVerbose = true;
    if (!request.params[1].isNull())
        fVerbose = request.params[1].get_bool();

    const CBlockIndex* pblockindex;
    const CBlockIndex* tip;
    {
        LOCK(cs_main);
        pblockindex = LookupBlockIndex(hash);
        tip = ::ChainActive().Tip();
    }

    if (!pblockindex) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
    }

    if (!fVerbose)
    {
        CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
        ssBlock << pblockindex->GetBlockHeader();
        std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
        return strHex;
    }

    return blockheaderToJSON(tip, pblockindex);
}

static CBlock GetBlockChecked(const CBlockIndex* pblockindex)
{
    CBlock block;
    if (IsBlockPruned(pblockindex)) {
        throw JSONRPCError(RPC_MISC_ERROR, "Block not available (pruned data)");
    }

    if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) {
        // Block not found on disk. This could be because we have the block
        // header in our index but don't have the block (for example if a
        // non-whitelisted node sends us an unrequested long chain of valid
        // blocks, we add the headers to our index, but don't accept the
        // block).
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

static UniValue getblock(const JSONRPCRequest& request)
{
    RPCHelpMan{"getblock",
                "\nIf verbosity is 0, returns a string that is serialized, hex-encoded data for block 'hash'.\n"
                "If verbosity is 1, returns an Object with information about block <hash>.\n"
                "If verbosity is 2, returns an Object with information about block <hash> and information about each transaction. \n",
                {
                    {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The block hash"},
                    {"verbosity", RPCArg::Type::NUM, /* default */ "1", "0 for hex-encoded data, 1 for a json object, and 2 for json object with transaction data"},
                },
                {
                    RPCResult{"for verbosity = 0",
            "\"data\"             (string) A string that is serialized, hex-encoded data for block 'hash'.\n"
                    },
                    RPCResult{"for verbosity = 1",
            "{\n"
            "  \"hash\" : \"hash\",     (string) the block hash (same as provided)\n"
            "  \"confirmations\" : n,   (numeric) The number of confirmations, or -1 if the block is not on the main chain\n"
            "  \"size\" : n,            (numeric) The block size\n"
            "  \"strippedsize\" : n,    (numeric) The block size excluding witness data\n"
            "  \"weight\" : n           (numeric) The block weight as defined in BIP 141\n"
            "  \"height\" : n,          (numeric) The block height or index\n"
            "  \"version\" : n,         (numeric) The block version\n"
            "  \"versionHex\" : \"00000000\", (string) The block version formatted in hexadecimal\n"
            "  \"merkleroot\" : \"xxxx\", (string) The merkle root\n"
            "  \"tx\" : [               (array of string) The transaction ids\n"
            "     \"transactionid\"     (string) The transaction id\n"
            "     ,...\n"
            "  ],\n"
            "  \"time\" : ttt,          (numeric) The block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"mediantime\" : ttt,    (numeric) The median block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"nonce\" : n,           (numeric) The nonce\n"
            "  \"bits\" : \"1d00ffff\", (string) The bits\n"
            "  \"difficulty\" : x.xxx,  (numeric) The difficulty\n"
            "  \"chainwork\" : \"xxxx\",  (string) Expected number of hashes required to produce the chain up to this block (in hex)\n"
            "  \"nTx\" : n,             (numeric) The number of transactions in the block.\n"
            "  \"previousblockhash\" : \"hash\",  (string) The hash of the previous block\n"
            "  \"nextblockhash\" : \"hash\"       (string) The hash of the next block\n"
            "}\n"
                    },
                    RPCResult{"for verbosity = 2",
            "{\n"
            "  ...,                     Same output as verbosity = 1.\n"
            "  \"tx\" : [               (array of Objects) The transactions in the format of the getrawtransaction RPC. Different from verbosity = 1 \"tx\" result.\n"
            "         ,...\n"
            "  ],\n"
            "  ,...                     Same output as verbosity = 1.\n"
            "}\n"
                    },
                },
                RPCExamples{
                    HelpExampleCli("getblock", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
            + HelpExampleRpc("getblock", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
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

    CBlock block;
    const CBlockIndex* pblockindex;
    const CBlockIndex* tip;
    {
        LOCK(cs_main);
        pblockindex = LookupBlockIndex(hash);
        tip = ::ChainActive().Tip();

        if (!pblockindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }

        block = GetBlockChecked(pblockindex);
    }

    if (verbosity <= 0)
    {
        CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags());
        ssBlock << block;
        std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
        return strHex;
    }

    return blockToJSON(block, tip, pblockindex, verbosity >= 2);
}

static UniValue pruneblockchain(const JSONRPCRequest& request)
{
            RPCHelpMan{"pruneblockchain", "",
                {
                    {"height", RPCArg::Type::NUM, RPCArg::Optional::NO, "The block height to prune up to. May be set to a discrete height, or a unix timestamp\n"
            "                  to prune blocks whose block time is at least 2 hours older than the provided timestamp."},
                },
                RPCResult{
            "n    (numeric) Height of the last block pruned.\n"
                },
                RPCExamples{
                    HelpExampleCli("pruneblockchain", "1000")
            + HelpExampleRpc("pruneblockchain", "1000")
                },
            }.Check(request);

    if (!fPruneMode)
        throw JSONRPCError(RPC_MISC_ERROR, "Cannot prune blocks because node is not in prune mode.");

    LOCK(cs_main);

    int heightParam = request.params[0].get_int();
    if (heightParam < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative block height.");

    // Height value more than a billion is too high to be a block height, and
    // too low to be a block time (corresponds to timestamp from Sep 2001).
    if (heightParam > 1000000000) {
        // Add a 2 hour buffer to include blocks which might have had old timestamps
        CBlockIndex* pindex = ::ChainActive().FindEarliestAtLeast(heightParam - TIMESTAMP_WINDOW, 0);
        if (!pindex) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Could not find block with at least the specified timestamp.");
        }
        heightParam = pindex->nHeight;
    }

    unsigned int height = (unsigned int) heightParam;
    unsigned int chainHeight = (unsigned int) ::ChainActive().Height();
    if (chainHeight < Params().PruneAfterHeight())
        throw JSONRPCError(RPC_MISC_ERROR, "Blockchain is too short for pruning.");
    else if (height > chainHeight)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Blockchain is shorter than the attempted prune height.");
    else if (height > chainHeight - MIN_BLOCKS_TO_KEEP) {
        LogPrint(BCLog::RPC, "Attempt to prune blocks close to the tip.  Retaining the minimum number of blocks.\n");
        height = chainHeight - MIN_BLOCKS_TO_KEEP;
    }

    PruneBlockFilesManual(height);
    const CBlockIndex* block = ::ChainActive().Tip();
    assert(block);
    while (block->pprev && (block->pprev->nStatus & BLOCK_HAVE_DATA)) {
        block = block->pprev;
    }
    return uint64_t(block->nHeight);
}

static UniValue gettxoutsetinfo(const JSONRPCRequest& request)
{
            RPCHelpMan{"gettxoutsetinfo",
                "\nReturns statistics about the unspent transaction output set.\n"
                "Note this call may take some time.\n",
                {},
                RPCResult{
            "{\n"
            "  \"height\":n,     (numeric) The current block height (index)\n"
            "  \"bestblock\": \"hex\",   (string) The hash of the block at the tip of the chain\n"
            "  \"transactions\": n,      (numeric) The number of transactions with unspent outputs\n"
            "  \"txouts\": n,            (numeric) The number of unspent transaction outputs\n"
            "  \"bogosize\": n,          (numeric) A meaningless metric for UTXO set size\n"
            "  \"hash_serialized_2\": \"hash\", (string) The serialized hash\n"
            "  \"disk_size\": n,         (numeric) The estimated size of the chainstate on disk\n"
            "  \"total_amount\": x.xxx          (numeric) The total amount\n"
            "}\n"
                },
                RPCExamples{
                    HelpExampleCli("gettxoutsetinfo", "")
            + HelpExampleRpc("gettxoutsetinfo", "")
                },
            }.Check(request);

    UniValue ret(UniValue::VOBJ);

    CCoinsStats stats;
    ::ChainstateActive().ForceFlushStateToDisk();

    CCoinsView* coins_view = WITH_LOCK(cs_main, return &ChainstateActive().CoinsDB());
    if (GetUTXOStats(coins_view, stats)) {
        ret.pushKV("height", (int64_t)stats.nHeight);
        ret.pushKV("bestblock", stats.hashBlock.GetHex());
        ret.pushKV("transactions", (int64_t)stats.nTransactions);
        ret.pushKV("txouts", (int64_t)stats.nTransactionOutputs);
        ret.pushKV("bogosize", (int64_t)stats.nBogoSize);
        ret.pushKV("hash_serialized_2", stats.hashSerialized.GetHex());
        ret.pushKV("disk_size", stats.nDiskSize);
        ret.pushKV("total_amount", ValueFromAmount(stats.nTotalAmount));
    } else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Unable to read UTXO set");
    }
    return ret;
}

UniValue gettxout(const JSONRPCRequest& request)
{
            RPCHelpMan{"gettxout",
                "\nReturns details about an unspent transaction output.\n",
                {
                    {"txid", RPCArg::Type::STR, RPCArg::Optional::NO, "The transaction id"},
                    {"n", RPCArg::Type::NUM, RPCArg::Optional::NO, "vout number"},
                    {"include_mempool", RPCArg::Type::BOOL, /* default */ "true", "Whether to include the mempool. Note that an unspent output that is spent in the mempool won't appear."},
                },
                RPCResult{
            "{\n"
            "  \"bestblock\":  \"hash\",    (string) The hash of the block at the tip of the chain\n"
            "  \"confirmations\" : n,       (numeric) The number of confirmations\n"
            "  \"value\" : x.xxx,           (numeric) The transaction value in " + CURRENCY_UNIT + "\n"
            "  \"scriptPubKey\" : {         (json object)\n"
            "     \"asm\" : \"code\",       (string) \n"
            "     \"hex\" : \"hex\",        (string) \n"
            "     \"reqSigs\" : n,          (numeric) Number of required signatures\n"
            "     \"type\" : \"pubkeyhash\", (string) The type, eg pubkeyhash\n"
            "     \"addresses\" : [          (array of string) array of bitcoin addresses\n"
            "        \"address\"     (string) bitcoin address\n"
            "        ,...\n"
            "     ]\n"
            "  },\n"
            "  \"coinbase\" : true|false   (boolean) Coinbase or not\n"
            "}\n"
                },
                RPCExamples{
            "\nGet unspent transactions\n"
            + HelpExampleCli("listunspent", "") +
            "\nView the details\n"
            + HelpExampleCli("gettxout", "\"txid\" 1") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("gettxout", "\"txid\", 1")
                },
            }.Check(request);

    LOCK(cs_main);

    UniValue ret(UniValue::VOBJ);

    uint256 hash(ParseHashV(request.params[0], "txid"));
    int n = request.params[1].get_int();
    COutPoint out(hash, n);
    bool fMempool = true;
    if (!request.params[2].isNull())
        fMempool = request.params[2].get_bool();

    Coin coin;
    CCoinsViewCache* coins_view = &::ChainstateActive().CoinsTip();

    if (fMempool) {
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

    const CBlockIndex* pindex = LookupBlockIndex(coins_view->GetBestBlock());
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
    int nCheckLevel = gArgs.GetArg("-checklevel", DEFAULT_CHECKLEVEL);
    int nCheckDepth = gArgs.GetArg("-checkblocks", DEFAULT_CHECKBLOCKS);
            RPCHelpMan{"verifychain",
                "\nVerifies blockchain database.\n",
                {
                    {"checklevel", RPCArg::Type::NUM, /* default */ strprintf("%d, range=0-4", nCheckLevel), "How thorough the block verification is."},
                    {"nblocks", RPCArg::Type::NUM, /* default */ strprintf("%d, 0=all", nCheckDepth), "The number of blocks to check."},
                },
                RPCResult{
            "true|false       (boolean) Verified or not\n"
                },
                RPCExamples{
                    HelpExampleCli("verifychain", "")
            + HelpExampleRpc("verifychain", "")
                },
            }.Check(request);

    LOCK(cs_main);

    if (!request.params[0].isNull())
        nCheckLevel = request.params[0].get_int();
    if (!request.params[1].isNull())
        nCheckDepth = request.params[1].get_int();

    return CVerifyDB().VerifyDB(
        Params(), &::ChainstateActive().CoinsTip(), nCheckLevel, nCheckDepth);
}

static void BuriedForkDescPushBack(UniValue& softforks, const std::string &name, int height) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    // For buried deployments.
    // A buried deployment is one where the height of the activation has been hardcoded into
    // the client implementation long after the consensus change has activated. See BIP 90.
    // Buried deployments with activation height value of
    // std::numeric_limits<int>::max() are disabled and thus hidden.
    if (height == std::numeric_limits<int>::max()) return;

    UniValue rv(UniValue::VOBJ);
    rv.pushKV("type", "buried");
    // getblockchaininfo reports the softfork as active from when the chain height is
    // one below the activation height
    rv.pushKV("active", ::ChainActive().Tip()->nHeight + 1 >= height);
    rv.pushKV("height", height);
    softforks.pushKV(name, rv);
}

static void BIP9SoftForkDescPushBack(UniValue& softforks, const std::string &name, const Consensus::Params& consensusParams, Consensus::DeploymentPos id) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    // For BIP9 deployments.
    // Deployments (e.g. testdummy) with timeout value before Jan 1, 2009 are hidden.
    // A timeout value of 0 guarantees a softfork will never be activated.
    // This is used when merging logic to implement a proposed softfork without a specified deployment schedule.
    if (consensusParams.vDeployments[id].nTimeout <= 1230768000) return;

    UniValue bip9(UniValue::VOBJ);
    const ThresholdState thresholdState = VersionBitsTipState(consensusParams, id);
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
    int64_t since_height = VersionBitsTipStateSinceHeight(consensusParams, id);
    bip9.pushKV("since", since_height);
    if (ThresholdState::STARTED == thresholdState)
    {
        UniValue statsUV(UniValue::VOBJ);
        BIP9Stats statsStruct = VersionBitsTipStatistics(consensusParams, id);
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
            "{\n"
            "  \"chain\": \"xxxx\",              (string) current network name as defined in BIP70 (main, test, regtest)\n"
            "  \"blocks\": xxxxxx,             (numeric) the height of the most-work fully-validated chain. The genesis block has height 0\n"
            "  \"headers\": xxxxxx,            (numeric) the current number of headers we have validated\n"
            "  \"bestblockhash\": \"...\",       (string) the hash of the currently best block\n"
            "  \"difficulty\": xxxxxx,         (numeric) the current difficulty\n"
            "  \"mediantime\": xxxxxx,         (numeric) median time for the current best block\n"
            "  \"verificationprogress\": xxxx, (numeric) estimate of verification progress [0..1]\n"
            "  \"initialblockdownload\": xxxx, (bool) (debug information) estimate of whether this node is in Initial Block Download mode.\n"
            "  \"chainwork\": \"xxxx\"           (string) total amount of work in active chain, in hexadecimal\n"
            "  \"size_on_disk\": xxxxxx,       (numeric) the estimated size of the block and undo files on disk\n"
            "  \"pruned\": xx,                 (boolean) if the blocks are subject to pruning\n"
            "  \"pruneheight\": xxxxxx,        (numeric) lowest-height complete block stored (only present if pruning is enabled)\n"
            "  \"automatic_pruning\": xx,      (boolean) whether automatic pruning is enabled (only present if pruning is enabled)\n"
            "  \"prune_target_size\": xxxxxx,  (numeric) the target size used by pruning (only present if automatic pruning is enabled)\n"
            "  \"softforks\": {                (object) status of softforks\n"
            "     \"xxxx\" : {                 (string) name of the softfork\n"
            "        \"type\": \"xxxx\",         (string) one of \"buried\", \"bip9\"\n"
            "        \"bip9\": {               (object) status of bip9 softforks (only for \"bip9\" type)\n"
            "           \"status\": \"xxxx\",    (string) one of \"defined\", \"started\", \"locked_in\", \"active\", \"failed\"\n"
            "           \"bit\": xx,           (numeric) the bit (0-28) in the block version field used to signal this softfork (only for \"started\" status)\n"
            "           \"start_time\": xx,     (numeric) the minimum median time past of a block at which the bit gains its meaning\n"
            "           \"timeout\": xx,       (numeric) the median time past of a block at which the deployment is considered failed if not yet locked in\n"
            "           \"since\": xx,         (numeric) height of the first block to which the status applies\n"
            "           \"statistics\": {      (object) numeric statistics about BIP9 signalling for a softfork\n"
            "              \"period\": xx,     (numeric) the length in blocks of the BIP9 signalling period \n"
            "              \"threshold\": xx,  (numeric) the number of blocks with the version bit set required to activate the feature \n"
            "              \"elapsed\": xx,    (numeric) the number of blocks elapsed since the beginning of the current period \n"
            "              \"count\": xx,      (numeric) the number of blocks with the version bit set in the current period \n"
            "              \"possible\": xx    (boolean) returns false if there are not enough blocks left in this period to pass activation threshold \n"
            "           }\n"
            "        },\n"
            "        \"height\": \"xxxxxx\",     (numeric) height of the first block which the rules are or will be enforced (only for \"buried\" type, or \"bip9\" type with \"active\" status)\n"
            "        \"active\": xx,           (boolean) true if the rules are enforced for the mempool and the next block\n"
            "     }\n"
            "  }\n"
            "  \"warnings\" : \"...\",           (string) any network and blockchain warnings.\n"
            "}\n"
                },
                RPCExamples{
                    HelpExampleCli("getblockchaininfo", "")
            + HelpExampleRpc("getblockchaininfo", "")
                },
            }.Check(request);

    LOCK(cs_main);

    const CBlockIndex* tip = ::ChainActive().Tip();
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("chain",                 Params().NetworkIDString());
    obj.pushKV("blocks",                (int)::ChainActive().Height());
    obj.pushKV("headers",               pindexBestHeader ? pindexBestHeader->nHeight : -1);
    obj.pushKV("bestblockhash",         tip->GetBlockHash().GetHex());
    obj.pushKV("difficulty",            (double)GetDifficulty(tip));
    obj.pushKV("mediantime",            (int64_t)tip->GetMedianTimePast());
    obj.pushKV("verificationprogress",  GuessVerificationProgress(Params().TxData(), tip));
    obj.pushKV("initialblockdownload",  ::ChainstateActive().IsInitialBlockDownload());
    obj.pushKV("chainwork",             tip->nChainWork.GetHex());
    obj.pushKV("size_on_disk",          CalculateCurrentUsage());
    obj.pushKV("pruned",                fPruneMode);
    if (fPruneMode) {
        const CBlockIndex* block = tip;
        assert(block);
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
    BuriedForkDescPushBack(softforks, "bip34", consensusParams.BIP34Height);
    BuriedForkDescPushBack(softforks, "bip66", consensusParams.BIP66Height);
    BuriedForkDescPushBack(softforks, "bip65", consensusParams.BIP65Height);
    BuriedForkDescPushBack(softforks, "csv", consensusParams.CSVHeight);
    BuriedForkDescPushBack(softforks, "segwit", consensusParams.SegwitHeight);
    BIP9SoftForkDescPushBack(softforks, "testdummy", consensusParams, Consensus::DEPLOYMENT_TESTDUMMY);
    obj.pushKV("softforks",             softforks);

    obj.pushKV("warnings", GetWarnings("statusbar"));
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
                {},
                RPCResult{
            "[\n"
            "  {\n"
            "    \"height\": xxxx,         (numeric) height of the chain tip\n"
            "    \"hash\": \"xxxx\",         (string) block hash of the tip\n"
            "    \"branchlen\": 0          (numeric) zero for main chain\n"
            "    \"status\": \"active\"      (string) \"active\" for the main chain\n"
            "  },\n"
            "  {\n"
            "    \"height\": xxxx,\n"
            "    \"hash\": \"xxxx\",\n"
            "    \"branchlen\": 1          (numeric) length of branch connecting the tip to the main chain\n"
            "    \"status\": \"xxxx\"        (string) status of the chain (active, valid-fork, valid-headers, headers-only, invalid)\n"
            "  }\n"
            "]\n"
            "Possible values for status:\n"
            "1.  \"invalid\"               This branch contains at least one invalid block\n"
            "2.  \"headers-only\"          Not all blocks for this branch are available, but the headers are valid\n"
            "3.  \"valid-headers\"         All blocks are available for this branch, but they were never fully validated\n"
            "4.  \"valid-fork\"            This branch is not part of the active chain, but is fully validated\n"
            "5.  \"active\"                This is the tip of the active main chain, which is certainly valid\n"
                },
                RPCExamples{
                    HelpExampleCli("getchaintips", "")
            + HelpExampleRpc("getchaintips", "")
                },
            }.Check(request);

    LOCK(cs_main);

    /*
     * Idea:  the set of chain tips is ::ChainActive().tip, plus orphan blocks which do not have another orphan building off of them.
     * Algorithm:
     *  - Make one pass through g_blockman.m_block_index, picking out the orphan blocks, and also storing a set of the orphan block's pprev pointers.
     *  - Iterate through the orphan blocks. If the block isn't pointed to by another orphan, it is a chain tip.
     *  - add ::ChainActive().Tip()
     */
    std::set<const CBlockIndex*, CompareBlocksByHeight> setTips;
    std::set<const CBlockIndex*> setOrphans;
    std::set<const CBlockIndex*> setPrevs;

    for (const std::pair<const uint256, CBlockIndex*>& item : ::BlockIndex())
    {
        if (!::ChainActive().Contains(item.second)) {
            setOrphans.insert(item.second);
            setPrevs.insert(item.second->pprev);
        }
    }

    for (std::set<const CBlockIndex*>::iterator it = setOrphans.begin(); it != setOrphans.end(); ++it)
    {
        if (setPrevs.erase(*it) == 0) {
            setTips.insert(*it);
        }
    }

    // Always report the currently active tip.
    setTips.insert(::ChainActive().Tip());

    /* Construct the output array.  */
    UniValue res(UniValue::VARR);
    for (const CBlockIndex* block : setTips)
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("height", block->nHeight);
        obj.pushKV("hash", block->phashBlock->GetHex());

        const int branchLen = block->nHeight - ::ChainActive().FindFork(block)->nHeight;
        obj.pushKV("branchlen", branchLen);

        std::string status;
        if (::ChainActive().Contains(block)) {
            // This block is part of the currently active chain.
            status = "active";
        } else if (block->nStatus & BLOCK_FAILED_MASK) {
            // This block or one of its ancestors is invalid.
            status = "invalid";
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

UniValue MempoolInfoToJSON(const CTxMemPool& pool)
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

    return ret;
}

static UniValue getmempoolinfo(const JSONRPCRequest& request)
{
            RPCHelpMan{"getmempoolinfo",
                "\nReturns details on the active state of the TX memory pool.\n",
                {},
                RPCResult{
            "{\n"
            "  \"loaded\": true|false         (boolean) True if the mempool is fully loaded\n"
            "  \"size\": xxxxx,               (numeric) Current tx count\n"
            "  \"bytes\": xxxxx,              (numeric) Sum of all virtual transaction sizes as defined in BIP 141. Differs from actual serialized size because witness data is discounted\n"
            "  \"usage\": xxxxx,              (numeric) Total memory usage for the mempool\n"
            "  \"maxmempool\": xxxxx,         (numeric) Maximum memory usage for the mempool\n"
            "  \"mempoolminfee\": xxxxx       (numeric) Minimum fee rate in " + CURRENCY_UNIT + "/kB for tx to be accepted. Is the maximum of minrelaytxfee and minimum mempool fee\n"
            "  \"minrelaytxfee\": xxxxx       (numeric) Current minimum relay fee for transactions\n"
            "}\n"
                },
                RPCExamples{
                    HelpExampleCli("getmempoolinfo", "")
            + HelpExampleRpc("getmempoolinfo", "")
                },
            }.Check(request);

    return MempoolInfoToJSON(::mempool);
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
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("preciousblock", "\"blockhash\"")
            + HelpExampleRpc("preciousblock", "\"blockhash\"")
                },
            }.Check(request);

    uint256 hash(ParseHashV(request.params[0], "blockhash"));
    CBlockIndex* pblockindex;

    {
        LOCK(cs_main);
        pblockindex = LookupBlockIndex(hash);
        if (!pblockindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }
    }

    CValidationState state;
    PreciousBlock(state, Params(), pblockindex);

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, FormatStateMessage(state));
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
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("invalidateblock", "\"blockhash\"")
            + HelpExampleRpc("invalidateblock", "\"blockhash\"")
                },
            }.Check(request);

    uint256 hash(ParseHashV(request.params[0], "blockhash"));
    CValidationState state;

    CBlockIndex* pblockindex;
    {
        LOCK(cs_main);
        pblockindex = LookupBlockIndex(hash);
        if (!pblockindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }
    }
    InvalidateBlock(state, Params(), pblockindex);

    if (state.IsValid()) {
        ActivateBestChain(state, Params());
    }

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, FormatStateMessage(state));
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
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("reconsiderblock", "\"blockhash\"")
            + HelpExampleRpc("reconsiderblock", "\"blockhash\"")
                },
            }.Check(request);

    uint256 hash(ParseHashV(request.params[0], "blockhash"));

    {
        LOCK(cs_main);
        CBlockIndex* pblockindex = LookupBlockIndex(hash);
        if (!pblockindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }

        ResetBlockFailureFlags(pblockindex);
    }

    CValidationState state;
    ActivateBestChain(state, Params());

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, FormatStateMessage(state));
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
            "{\n"
            "  \"time\": xxxxx,                         (numeric) The timestamp for the final block in the window in UNIX format.\n"
            "  \"txcount\": xxxxx,                      (numeric) The total number of transactions in the chain up to that point.\n"
            "  \"window_final_block_hash\": \"...\",      (string) The hash of the final block in the window.\n"
            "  \"window_final_block_height\": xxxxx,    (numeric) The height of the final block in the window.\n"
            "  \"window_block_count\": xxxxx,           (numeric) Size of the window in number of blocks.\n"
            "  \"window_tx_count\": xxxxx,              (numeric) The number of transactions in the window. Only returned if \"window_block_count\" is > 0.\n"
            "  \"window_interval\": xxxxx,              (numeric) The elapsed time in the window in seconds. Only returned if \"window_block_count\" is > 0.\n"
            "  \"txrate\": x.xx,                        (numeric) The average rate of transactions per second in the window. Only returned if \"window_interval\" is > 0.\n"
            "}\n"
                },
                RPCExamples{
                    HelpExampleCli("getchaintxstats", "")
            + HelpExampleRpc("getchaintxstats", "2016")
                },
            }.Check(request);

    const CBlockIndex* pindex;
    int blockcount = 30 * 24 * 60 * 60 / Params().GetConsensus().nPowTargetSpacing; // By default: 1 month

    if (request.params[1].isNull()) {
        LOCK(cs_main);
        pindex = ::ChainActive().Tip();
    } else {
        uint256 hash(ParseHashV(request.params[1], "blockhash"));
        LOCK(cs_main);
        pindex = LookupBlockIndex(hash);
        if (!pindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }
        if (!::ChainActive().Contains(pindex)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block is not in main chain");
        }
    }

    assert(pindex != nullptr);

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

void CalculatePercentilesByWeight(CAmount result[NUM_GETBLOCKSTATS_PERCENTILES], std::vector<std::pair<CAmount, int64_t>>& scores, int64_t total_weight)
{
    if (scores.empty()) {
        return;
    }

    std::sort(scores.begin(), scores.end());

    // 10th, 25th, 50th, 75th, and 90th percentile weight units.
    const double weights[NUM_GETBLOCKSTATS_PERCENTILES] = {
        total_weight / 10.0, total_weight / 4.0, total_weight / 2.0, (total_weight * 3.0) / 4.0, (total_weight * 9.0) / 10.0
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
                "\nCompute per block statistics for a given window. All amounts are in satoshis.\n"
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
            "{                           (json object)\n"
            "  \"avgfee\": xxxxx,          (numeric) Average fee in the block\n"
            "  \"avgfeerate\": xxxxx,      (numeric) Average feerate (in satoshis per virtual byte)\n"
            "  \"avgtxsize\": xxxxx,       (numeric) Average transaction size\n"
            "  \"blockhash\": xxxxx,       (string) The block hash (to check for potential reorgs)\n"
            "  \"feerate_percentiles\": [  (array of numeric) Feerates at the 10th, 25th, 50th, 75th, and 90th percentile weight unit (in satoshis per virtual byte)\n"
            "      \"10th_percentile_feerate\",      (numeric) The 10th percentile feerate\n"
            "      \"25th_percentile_feerate\",      (numeric) The 25th percentile feerate\n"
            "      \"50th_percentile_feerate\",      (numeric) The 50th percentile feerate\n"
            "      \"75th_percentile_feerate\",      (numeric) The 75th percentile feerate\n"
            "      \"90th_percentile_feerate\",      (numeric) The 90th percentile feerate\n"
            "  ],\n"
            "  \"height\": xxxxx,          (numeric) The height of the block\n"
            "  \"ins\": xxxxx,             (numeric) The number of inputs (excluding coinbase)\n"
            "  \"maxfee\": xxxxx,          (numeric) Maximum fee in the block\n"
            "  \"maxfeerate\": xxxxx,      (numeric) Maximum feerate (in satoshis per virtual byte)\n"
            "  \"maxtxsize\": xxxxx,       (numeric) Maximum transaction size\n"
            "  \"medianfee\": xxxxx,       (numeric) Truncated median fee in the block\n"
            "  \"mediantime\": xxxxx,      (numeric) The block median time past\n"
            "  \"mediantxsize\": xxxxx,    (numeric) Truncated median transaction size\n"
            "  \"minfee\": xxxxx,          (numeric) Minimum fee in the block\n"
            "  \"minfeerate\": xxxxx,      (numeric) Minimum feerate (in satoshis per virtual byte)\n"
            "  \"mintxsize\": xxxxx,       (numeric) Minimum transaction size\n"
            "  \"outs\": xxxxx,            (numeric) The number of outputs\n"
            "  \"subsidy\": xxxxx,         (numeric) The block subsidy\n"
            "  \"swtotal_size\": xxxxx,    (numeric) Total size of all segwit transactions\n"
            "  \"swtotal_weight\": xxxxx,  (numeric) Total weight of all segwit transactions divided by segwit scale factor (4)\n"
            "  \"swtxs\": xxxxx,           (numeric) The number of segwit transactions\n"
            "  \"time\": xxxxx,            (numeric) The block time\n"
            "  \"total_out\": xxxxx,       (numeric) Total amount in all outputs (excluding coinbase and thus reward [ie subsidy + totalfee])\n"
            "  \"total_size\": xxxxx,      (numeric) Total size of all non-coinbase transactions\n"
            "  \"total_weight\": xxxxx,    (numeric) Total weight of all non-coinbase transactions divided by segwit scale factor (4)\n"
            "  \"totalfee\": xxxxx,        (numeric) The fee total\n"
            "  \"txs\": xxxxx,             (numeric) The number of transactions (excluding coinbase)\n"
            "  \"utxo_increase\": xxxxx,   (numeric) The increase/decrease in the number of unspent outputs\n"
            "  \"utxo_size_inc\": xxxxx,   (numeric) The increase/decrease in size for the utxo index (not discounting op_return and similar)\n"
            "}\n"
                },
                RPCExamples{
                    HelpExampleCli("getblockstats", "1000 '[\"minfeerate\",\"avgfeerate\"]'")
            + HelpExampleRpc("getblockstats", "1000 '[\"minfeerate\",\"avgfeerate\"]'")
                },
    }.Check(request);

    LOCK(cs_main);

    CBlockIndex* pindex;
    if (request.params[0].isNum()) {
        const int height = request.params[0].get_int();
        const int current_tip = ::ChainActive().Height();
        if (height < 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Target block height %d is negative", height));
        }
        if (height > current_tip) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Target block height %d after current tip %d", height, current_tip));
        }

        pindex = ::ChainActive()[height];
    } else {
        const uint256 hash(ParseHashV(request.params[0], "hash_or_height"));
        pindex = LookupBlockIndex(hash);
        if (!pindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }
        if (!::ChainActive().Contains(pindex)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Block is not in chain %s", Params().NetworkIDString()));
        }
    }

    assert(pindex != nullptr);

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
    const bool do_calculate_size = do_mediantxsize ||
        SetHasKeys(stats, "total_size", "avgtxsize", "mintxsize", "maxtxsize", "swtotal_size");
    const bool do_calculate_weight = do_all || SetHasKeys(stats, "total_weight", "avgfeerate", "swtotal_weight", "avgfeerate", "feerate_percentiles", "minfeerate", "maxfeerate");
    const bool do_calculate_sw = do_all || SetHasKeys(stats, "swtxs", "swtotal_size", "swtotal_weight");

    CAmount maxfee = 0;
    CAmount maxfeerate = 0;
    CAmount minfee = MAX_MONEY;
    CAmount minfeerate = MAX_MONEY;
    CAmount total_out = 0;
    CAmount totalfee = 0;
    int64_t inputs = 0;
    int64_t maxtxsize = 0;
    int64_t mintxsize = MAX_BLOCK_SERIALIZED_SIZE;
    int64_t outputs = 0;
    int64_t swtotal_size = 0;
    int64_t swtotal_weight = 0;
    int64_t swtxs = 0;
    int64_t total_size = 0;
    int64_t total_weight = 0;
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

        int64_t weight = 0;
        if (do_calculate_weight) {
            weight = GetTransactionWeight(*tx);
            total_weight += weight;
        }

        if (do_calculate_sw && tx->HasWitness()) {
            ++swtxs;
            swtotal_size += tx_size;
            swtotal_weight += weight;
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
            assert(MoneyRange(txfee));
            if (do_medianfee) {
                fee_array.push_back(txfee);
            }
            maxfee = std::max(maxfee, txfee);
            minfee = std::min(minfee, txfee);
            totalfee += txfee;

            // New feerate uses satoshis per virtual byte instead of per serialized byte
            CAmount feerate = weight ? (txfee * WITNESS_SCALE_FACTOR) / weight : 0;
            if (do_feerate_percentiles) {
                feerate_array.emplace_back(std::make_pair(feerate, weight));
            }
            maxfeerate = std::max(maxfeerate, feerate);
            minfeerate = std::min(minfeerate, feerate);
        }
    }

    CAmount feerate_percentiles[NUM_GETBLOCKSTATS_PERCENTILES] = { 0 };
    CalculatePercentilesByWeight(feerate_percentiles, feerate_array, total_weight);

    UniValue feerates_res(UniValue::VARR);
    for (int64_t i = 0; i < NUM_GETBLOCKSTATS_PERCENTILES; i++) {
        feerates_res.push_back(feerate_percentiles[i]);
    }

    UniValue ret_all(UniValue::VOBJ);
    ret_all.pushKV("avgfee", (block.vtx.size() > 1) ? totalfee / (block.vtx.size() - 1) : 0);
    ret_all.pushKV("avgfeerate", total_weight ? (totalfee * WITNESS_SCALE_FACTOR) / total_weight : 0); // Unit: sat/vbyte
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
    ret_all.pushKV("mintxsize", mintxsize == MAX_BLOCK_SERIALIZED_SIZE ? 0 : mintxsize);
    ret_all.pushKV("outs", outputs);
    ret_all.pushKV("subsidy", GetBlockSubsidy(pindex->nHeight, Params().GetConsensus()));
    ret_all.pushKV("swtotal_size", swtotal_size);
    ret_all.pushKV("swtotal_weight", swtotal_weight);
    ret_all.pushKV("swtxs", swtxs);
    ret_all.pushKV("time", pindex->GetBlockTime());
    ret_all.pushKV("total_out", total_out);
    ret_all.pushKV("total_size", total_size);
    ret_all.pushKV("total_weight", total_weight);
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

static UniValue savemempool(const JSONRPCRequest& request)
{
            RPCHelpMan{"savemempool",
                "\nDumps the mempool to disk. It will fail until the previous dump is fully loaded.\n",
                {},
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("savemempool", "")
            + HelpExampleRpc("savemempool", "")
                },
            }.Check(request);

    if (!::mempool.IsLoaded()) {
        throw JSONRPCError(RPC_MISC_ERROR, "The mempool was not loaded yet");
    }

    if (!DumpMempool(::mempool)) {
        throw JSONRPCError(RPC_MISC_ERROR, "Unable to dump mempool to disk");
    }

    return NullUniValue;
}

//! Search for a given set of pubkey scripts
bool FindScriptPubKey(std::atomic<int>& scan_progress, const std::atomic<bool>& should_abort, int64_t& count, CCoinsViewCursor* cursor, const std::set<CScript>& needles, std::map<COutPoint, Coin>& out_results) {
    scan_progress = 0;
    count = 0;
    while (cursor->Valid()) {
        COutPoint key;
        Coin coin;
        if (!cursor->GetKey(key) || !cursor->GetValue(coin)) return false;
        if (++count % 8192 == 0) {
            boost::this_thread::interruption_point();
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
        assert (!m_could_reserve);
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
                "    combo(<pubkey>)                      P2PK, P2PKH, P2WPKH, and P2SH-P2WPKH outputs for the given pubkey\n"
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
            "{\n"
            "  \"success\": true|false,         (boolean) Whether the scan was completed\n"
            "  \"txouts\": n,                   (numeric) The number of unspent transaction outputs scanned\n"
            "  \"height\": n,                   (numeric) The current block height (index)\n"
            "  \"bestblock\": \"hex\",            (string) The hash of the block at the tip of the chain\n"
            "  \"unspents\": [\n"
            "   {\n"
            "    \"txid\": \"hash\",              (string) The transaction id\n"
            "    \"vout\": n,                   (numeric) The vout value\n"
            "    \"scriptPubKey\": \"script\",    (string) The script key\n"
            "    \"desc\": \"descriptor\",        (string) A specialized descriptor for the matched scriptPubKey\n"
            "    \"amount\": x.xxx,             (numeric) The total amount in " + CURRENCY_UNIT + " of the unspent output\n"
            "    \"height\": n,                 (numeric) Height of the unspent transaction output\n"
            "   }\n"
            "   ,...],\n"
            "  \"total_amount\": x.xxx,          (numeric) The total amount of all found unspent outputs in " + CURRENCY_UNIT + "\n"
            "]\n"
                },
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
        {
            LOCK(cs_main);
            ::ChainstateActive().ForceFlushStateToDisk();
            pcursor = std::unique_ptr<CCoinsViewCursor>(::ChainstateActive().CoinsDB().Cursor());
            assert(pcursor);
            tip = ::ChainActive().Tip();
            assert(tip);
        }
        bool res = FindScriptPubKey(g_scan_progress, g_should_abort_scan, count, pcursor.get(), needles, coins);
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
            unspent.pushKV("scriptPubKey", HexStr(txo.scriptPubKey.begin(), txo.scriptPubKey.end()));
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
                    {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hash of the block"},
                    {"filtertype", RPCArg::Type::STR, /*default*/ "basic", "The type name of the filter"},
                },
                RPCResult{
                    "{\n"
                    "  \"filter\" : (string) the hex-encoded filter data\n"
                    "  \"header\" : (string) the hex-encoded filter header\n"
                    "}\n"
                },
                RPCExamples{
                    HelpExampleCli("getblockfilter", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\" \"basic\"")
                }
            }.Check(request);

    uint256 block_hash = ParseHashV(request.params[0], "blockhash");
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
        LOCK(cs_main);
        block_index = LookupBlockIndex(block_hash);
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

// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
  //  --------------------- ------------------------  -----------------------  ----------
    { "blockchain",         "getblockchaininfo",      &getblockchaininfo,      {} },
    { "blockchain",         "getchaintxstats",        &getchaintxstats,        {"nblocks", "blockhash"} },
    { "blockchain",         "getblockstats",          &getblockstats,          {"hash_or_height", "stats"} },
    { "blockchain",         "getbestblockhash",       &getbestblockhash,       {} },
    { "blockchain",         "getblockcount",          &getblockcount,          {} },
    { "blockchain",         "getblock",               &getblock,               {"blockhash","verbosity|verbose"} },
    { "blockchain",         "getblockhash",           &getblockhash,           {"height"} },
    { "blockchain",         "getblockheader",         &getblockheader,         {"blockhash","verbose"} },
    { "blockchain",         "getchaintips",           &getchaintips,           {} },
    { "blockchain",         "getdifficulty",          &getdifficulty,          {} },
    { "blockchain",         "getmempoolancestors",    &getmempoolancestors,    {"txid","verbose"} },
    { "blockchain",         "getmempooldescendants",  &getmempooldescendants,  {"txid","verbose"} },
    { "blockchain",         "getmempoolentry",        &getmempoolentry,        {"txid"} },
    { "blockchain",         "getmempoolinfo",         &getmempoolinfo,         {} },
    { "blockchain",         "getrawmempool",          &getrawmempool,          {"verbose"} },
    { "blockchain",         "gettxout",               &gettxout,               {"txid","n","include_mempool"} },
    { "blockchain",         "gettxoutsetinfo",        &gettxoutsetinfo,        {} },
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
};
// clang-format on

void RegisterBlockchainRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
