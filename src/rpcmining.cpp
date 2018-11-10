// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcserver.h"
#include "base58.h"
#include "amount.h"
#include "chainparams.h"
#include "core_io.h"
#include "init.h"
#include "net.h"
#include "main.h"
#include "miner.h"
#include "pow.h"
#include "rpcserver.h"
#include "util.h"
#include "auxpow.h"
#include "spork.h"
#ifdef ENABLE_WALLET
#include "db.h"
#include "wallet.h"
#endif

#include <stdint.h>

#include <boost/assign/list_of.hpp>

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace json_spirit;
using namespace std;

#ifdef ENABLE_WALLET
// Key used by getwork miners.
// Allocated in InitRPCMining, free'd in ShutdownRPCMining
static CReserveKey* pminingKey = NULL;

void InitRPCMining()
{
    if (!pwalletMain)
        return;

    // getwork/getblocktemplate mining rewards paid here:
    pminingKey = new CReserveKey(pwalletMain);
}

void ShutdownRPCMining()
{
    if (!pminingKey)
        return;

    delete pminingKey;
    pminingKey = NULL;
}
#endif // ENABLE_WALLET

/**
 * Return average network hashes per second based on the last 'lookup' blocks,
 * or from the last difficulty change if 'lookup' is nonpositive.
 * If 'height' is nonnegative, compute the estimate at the time when a given block was found.
 */
Value GetNetworkHashPS(int lookup, int height) {
    CBlockIndex *pb = chainActive.Tip();

    if (height >= 0 && height < chainActive.Height())
        pb = chainActive[height];

    if (pb == NULL || !pb->nHeight)
        return 0;

    // If lookup is -1, then use blocks since last difficulty change.
    if (lookup <= 0)
        lookup = pb->nHeight % 2016 + 1;

    // If lookup is larger than chain, then set it to chain length.
    if (lookup > pb->nHeight)
        lookup = pb->nHeight;

    CBlockIndex *pb0 = pb;
    int64_t minTime = pb0->GetBlockTime();
    int64_t maxTime = minTime;
    for (int i = 0; i < lookup; i++) {
        pb0 = pb0->pprev;
        int64_t time = pb0->GetBlockTime();
        minTime = std::min(time, minTime);
        maxTime = std::max(time, maxTime);
    }

    // In case there's a situation where minTime == maxTime, we don't want a divide by zero exception.
    if (minTime == maxTime)
        return 0;

    arith_uint256 workDiff = pb->nChainWork - pb0->nChainWork;
    int64_t timeDiff = maxTime - minTime;

    return (int64_t)(workDiff.getdouble() / timeDiff);
}

Value getnetworkhashps(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "getnetworkhashps ( blocks height )\n"
            "\nReturns the estimated network hashes per second based on the last n blocks.\n"
            "Pass in [blocks] to override # of blocks, -1 specifies since last difficulty change.\n"
            "Pass in [height] to estimate the network speed at the time when a certain block was found.\n"
            "\nArguments:\n"
            "1. blocks     (numeric, optional, default=120) The number of blocks, or -1 for blocks since last difficulty change.\n"
            "2. height     (numeric, optional, default=-1) To estimate at the time of the given height.\n"
            "\nResult:\n"
            "x             (numeric) Hashes per second estimated\n"
            "\nExamples:\n"
            + HelpExampleCli("getnetworkhashps", "")
            + HelpExampleRpc("getnetworkhashps", "")
       );

    return GetNetworkHashPS(params.size() > 0 ? params[0].get_int() : 120, params.size() > 1 ? params[1].get_int() : -1);
}

#ifdef ENABLE_WALLET
Value getgenerate(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getgenerate\n"
            "\nReturn if the server is set to generate coins or not. The default is false.\n"
            "It is set with the command line argument -gen (or crown.conf setting gen)\n"
            "It can also be set with the setgenerate call.\n"
            "\nResult\n"
            "true|false      (boolean) If the server is set to generate coins or not\n"
            "\nExamples:\n"
            + HelpExampleCli("getgenerate", "")
            + HelpExampleRpc("getgenerate", "")
        );

    return GetBoolArg("-gen", false);
}


Value setgenerate(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "setgenerate generate ( genproclimit )\n"
            "\nSet 'generate' true or false to turn generation on or off.\n"
            "Generation is limited to 'genproclimit' processors, -1 is unlimited.\n"
            "See the getgenerate call for the current setting.\n"
            "\nArguments:\n"
            "1. generate         (boolean, required) Set to true to turn on generation, false to turn off.\n"
            "2. genproclimit     (numeric, optional) Set the processor limit for when generation is on. Can be -1 for unlimited.\n"
            "                    Note: in -regtest mode, genproclimit controls how many blocks are generated immediately.\n"
            "\nResult\n"
            "[ blockhashes ]     (array, -regtest only) hashes of blocks generated\n"
            "\nExamples:\n"
            "\nSet the generation on with a limit of one processor\n"
            + HelpExampleCli("setgenerate", "true 1") +
            "\nCheck the setting\n"
            + HelpExampleCli("getgenerate", "") +
            "\nTurn off generation\n"
            + HelpExampleCli("setgenerate", "false") +
            "\nUsing json rpc\n"
            + HelpExampleRpc("setgenerate", "true, 1")
        );

    if (pwalletMain == NULL)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Method not found (disabled)");

    bool fGenerate = true;
    if (params.size() > 0)
        fGenerate = params[0].get_bool();

    int nGenProcLimit = -1;
    if (params.size() > 1)
    {
        nGenProcLimit = params[1].get_int();
        if (nGenProcLimit == 0)
            fGenerate = false;
    }

    // -regtest mode: don't return until nGenProcLimit blocks are generated
    if (fGenerate && Params().MineBlocksOnDemand())
    {
        int nHeightStart = 0;
        int nHeightEnd = 0;
        int nHeight = 0;
        int nGenerate = (nGenProcLimit > 0 ? nGenProcLimit : 1);
        CReserveKey reservekey(pwalletMain);

        {   // Don't keep cs_main locked
            LOCK(cs_main);
            nHeightStart = chainActive.Height();
            nHeight = nHeightStart;
            nHeightEnd = nHeightStart+nGenerate;
        }
        unsigned int nExtraNonce = 0;
        Array blockHashes;
        while (nHeight < nHeightEnd)
        {
            auto_ptr<CBlockTemplate> pblocktemplate(CreateNewBlockWithKey(reservekey));
            if (!pblocktemplate.get())
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Wallet keypool empty");
            CBlock *pblock = &pblocktemplate->block;
            {
                LOCK(cs_main);
                IncrementExtraNonce(pblock, chainActive.Tip(), nExtraNonce);
            }
            while (!CheckProofOfWork(pblock->GetHash(), pblock->nBits)) {
                // Yes, there is a chance every nonce could fail to satisfy the -regtest
                // target -- 1 in 2^(2^32). That ain't gonna happen.
                ++pblock->nNonce;
            }
            CValidationState state;
            if (!ProcessNewBlock(state, NULL, pblock))
                throw JSONRPCError(RPC_INTERNAL_ERROR, "ProcessNewBlock, block not accepted");
            ++nHeight;
            blockHashes.push_back(pblock->GetHash().GetHex());
        }
        return blockHashes;
    }
    else // Not -regtest: start generate thread, return immediately
    {
        mapArgs["-gen"] = (fGenerate ? "1" : "0");
        mapArgs ["-genproclimit"] = itostr(nGenProcLimit);
        GenerateBitcoins(fGenerate, pwalletMain, nGenProcLimit);
    }

    return Value::null;
}

Value gethashespersec(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "gethashespersec\n"
            "\nReturns a recent hashes per second performance measurement while generating.\n"
            "See the getgenerate and setgenerate calls to turn generation on and off.\n"
            "\nResult:\n"
            "n            (numeric) The recent hashes per second when generation is on (will return 0 if generation is off)\n"
            "\nExamples:\n"
            + HelpExampleCli("gethashespersec", "")
            + HelpExampleRpc("gethashespersec", "")
        );

    if (GetTimeMillis() - nHPSTimerStart > 8000)
        return (int64_t)0;
    return (int64_t)dHashesPerSec;
}
#endif


Value getmininginfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getmininginfo\n"
            "\nReturns a json object containing mining-related information."
            "\nResult:\n"
            "{\n"
            "  \"blocks\": nnn,             (numeric) The current block\n"
            "  \"currentblocksize\": nnn,   (numeric) The last block size\n"
            "  \"currentblocktx\": nnn,     (numeric) The last block transaction\n"
            "  \"difficulty\": xxx.xxxxx    (numeric) The current difficulty\n"
            "  \"errors\": \"...\"          (string) Current errors\n"
            "  \"generate\": true|false     (boolean) If the generation is on or off (see getgenerate or setgenerate calls)\n"
            "  \"genproclimit\": n          (numeric) The processor limit for generation. -1 if no generation. (see getgenerate or setgenerate calls)\n"
            "  \"hashespersec\": n          (numeric) The hashes per second of the generation, or 0 if no generation.\n"
            "  \"pooledtx\": n              (numeric) The size of the mem pool\n"
            "  \"testnet\": true|false      (boolean) If using testnet or not\n"
            "  \"chain\": \"xxxx\",         (string) current network name as defined in BIP70 (main, test, regtest)\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getmininginfo", "")
            + HelpExampleRpc("getmininginfo", "")
        );

    Object obj;
    obj.push_back(Pair("blocks",           (int)chainActive.Height()));
    obj.push_back(Pair("currentblocksize", (uint64_t)nLastBlockSize));
    obj.push_back(Pair("currentblocktx",   (uint64_t)nLastBlockTx));
    obj.push_back(Pair("difficulty",       (double)GetDifficulty()));
    obj.push_back(Pair("errors",           GetWarnings("statusbar")));
    obj.push_back(Pair("genproclimit",     (int)GetArg("-genproclimit", -1)));
    obj.push_back(Pair("networkhashps",    getnetworkhashps(params, false)));
    obj.push_back(Pair("pooledtx",         (uint64_t)mempool.size()));
    obj.push_back(Pair("testnet",          Params().TestnetToBeDeprecatedFieldRPC()));
    obj.push_back(Pair("chain",            Params().NetworkIDString()));
#ifdef ENABLE_WALLET
    obj.push_back(Pair("generate",         getgenerate(params, false)));
    obj.push_back(Pair("hashespersec",     gethashespersec(params, false)));
#endif
    return obj;
}


// NOTE: Unlike wallet RPC (which use BTC values), mining RPCs follow GBT (BIP 22) in using satoshi amounts
Value prioritisetransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "prioritisetransaction <txid> <priority delta> <fee delta>\n"
            "Accepts the transaction into mined blocks at a higher (or lower) priority\n"
            "\nArguments:\n"
            "1. \"txid\"       (string, required) The transaction id.\n"
            "2. priority delta (numeric, required) The priority to add or subtract.\n"
            "                  The transaction selection algorithm considers the tx as it would have a higher priority.\n"
            "                  (priority of a transaction is calculated: coinage * value_in_crowns / txsize) \n"
            "3. fee delta      (numeric, required) The fee value (in crowns) to add (or subtract, if negative).\n"
            "                  The fee is not actually paid, only the algorithm for selecting transactions into a block\n"
            "                  considers the transaction as it would have paid a higher (or lower) fee.\n"
            "\nResult\n"
            "true              (boolean) Returns true\n"
            "\nExamples:\n"
            + HelpExampleCli("prioritisetransaction", "\"txid\" 0.0 10000")
            + HelpExampleRpc("prioritisetransaction", "\"txid\", 0.0, 10000")
        );

    uint256 hash = ParseHashStr(params[0].get_str(), "txid");

    CAmount nAmount = params[2].get_int64();

    mempool.PrioritiseTransaction(hash, params[0].get_str(), params[1].get_real(), nAmount);
    return true;
}


// NOTE: Assumes a conclusive result; if result is inconclusive, it must be handled by caller
static Value BIP22ValidationResult(const CValidationState& state)
{
    if (state.IsValid())
        return Value::null;

    std::string strRejectReason = state.GetRejectReason();
    if (state.IsError())
        throw JSONRPCError(RPC_VERIFY_ERROR, strRejectReason);
    if (state.IsInvalid())
    {
        if (strRejectReason.empty())
            return "rejected";
        return strRejectReason;
    }
    // Should be impossible
    return "valid?";
}

Value getblocktemplate(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getblocktemplate ( \"jsonrequestobject\" )\n"
            "\nIf the request parameters include a 'mode' key, that is used to explicitly select between the default 'template' request or a 'proposal'.\n"
            "It returns data needed to construct a block to work on.\n"
            "See https://en.bitcoin.it/wiki/BIP_0022 for full specification.\n"

            "\nArguments:\n"
            "1. \"jsonrequestobject\"       (string, optional) A json object in the following spec\n"
            "     {\n"
            "       \"mode\":\"template\"    (string, optional) This must be set to \"template\" or omitted\n"
            "       \"capabilities\":[       (array, optional) A list of strings\n"
            "           \"support\"           (string) client side supported feature, 'longpoll', 'coinbasetxn', 'coinbasevalue', 'proposal', 'serverlist', 'workid'\n"
            "           ,...\n"
            "         ]\n"
            "     }\n"
            "\n"

            "\nResult:\n"
            "{\n"
            "  \"version\" : n,                    (numeric) The block version\n"
            "  \"previousblockhash\" : \"xxxx\",    (string) The hash of current highest block\n"
            "  \"transactions\" : [                (array) contents of non-coinbase transactions that should be included in the next block\n"
            "      {\n"
            "         \"data\" : \"xxxx\",          (string) transaction data encoded in hexadecimal (byte-for-byte)\n"
            "         \"hash\" : \"xxxx\",          (string) hash/id encoded in little-endian hexadecimal\n"
            "         \"depends\" : [              (array) array of numbers \n"
            "             n                        (numeric) transactions before this one (by 1-based index in 'transactions' list) that must be present in the final block if this one is\n"
            "             ,...\n"
            "         ],\n"
            "         \"fee\": n,                   (numeric) difference in value between transaction inputs and outputs (in crowns); for coinbase transactions, this is a negative Number of the total collected block fees (ie, not including the block subsidy); if key is not present, fee is unknown and clients MUST NOT assume there isn't one\n"
            "         \"sigops\" : n,               (numeric) total number of SigOps, as counted for purposes of block limits; if key is not present, sigop count is unknown and clients MUST NOT assume there aren't any\n"
            "         \"required\" : true|false     (boolean) if provided and true, this transaction must be in the final block\n"
            "      }\n"
            "      ,...\n"
            "  ],\n"
            "  \"coinbaseaux\" : {                  (json object) data that should be included in the coinbase's scriptSig content\n"
            "      \"flags\" : \"flags\"            (string) \n"
            "  },\n"
            "  \"coinbasevalue\" : n,               (numeric) maximum allowable input to coinbase transaction, including the generation award and transaction fees (in crowns)\n"
            "  \"coinbasetxn\" : { ... },           (json object) information for coinbase transaction\n"
            "  \"target\" : \"xxxx\",               (string) The hash target\n"
            "  \"mintime\" : xxx,                   (numeric) The minimum timestamp appropriate for next block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"mutable\" : [                      (array of string) list of ways the block template may be changed \n"
            "     \"value\"                         (string) A way the block template may be changed, e.g. 'time', 'transactions', 'prevblock'\n"
            "     ,...\n"
            "  ],\n"
            "  \"noncerange\" : \"00000000ffffffff\",   (string) A range of valid nonces\n"
            "  \"sigoplimit\" : n,                 (numeric) limit of sigops in blocks\n"
            "  \"sizelimit\" : n,                  (numeric) limit of block size\n"
            "  \"curtime\" : ttt,                  (numeric) current timestamp in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"bits\" : \"xxx\",                 (string) compressed target of next block\n"
            "  \"height\" : n                      (numeric) The height of the next block\n"
            "  \"payee\" : \"xxx\",                (string) required masternode payee for the next block\n"
            "  \"payee_amount\" : n,               (numeric) required amount to pay masternode\n"
            "  \"votes\" : [\n                     (array) show vote candidates\n"
            "        { ... }                       (json object) vote candidate\n"
            "        ,...\n"
            "  ],\n"
            "  \"masternode_payments\" : true|false,         (boolean) true, if masternode payments are enabled\n"
            "  \"enforce_masternode_payments\" : true|false  (boolean) true, if masternode payments are enforced\n"
            "  \"systemnode_payments\" : true|false,         (boolean) true, if systemnode payments are enabled\n"
            "  \"enforce_systemnode_payments\" : true|false  (boolean) true, if systemnode payments are enforced\n"
            "  \"payeeSN\" : \"xxx\",                        (string) required systemnode payee for the next block\n"
            "  \"payeeSN_amount\" : n,                       (numeric) required amount to pay systemnode \n"
            "  \"superblock\" : [                  (array) required superblock payees that must be included in the next block\n"
            "      {\n"
            "         \"payee\" : \"xxxx\",        (string) payee address\n"
            "         \"script\" : \"xxxx\",       (string) payee scriptPubKey\n"
            "         \"amount\": n                (numeric) required amount to pay\n"
            "      }\n"
            "      ,...\n"
            "  ],\n"
            "  \"superblocks_enabled\" : true|false  (boolean) true, if superblock payments are enabled\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("getblocktemplate", "")
            + HelpExampleRpc("getblocktemplate", "")
         );

    std::string strMode = "template";
    Value lpval = Value::null;
    if (params.size() > 0)
    {
        const Object& oparam = params[0].get_obj();
        const Value& modeval = find_value(oparam, "mode");
        if (modeval.type() == str_type)
            strMode = modeval.get_str();
        else if (modeval.type() == null_type)
        {
            /* Do nothing */
        }
        else
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");
        lpval = find_value(oparam, "longpollid");

        if (strMode == "proposal")
        {
            const Value& dataval = find_value(oparam, "data");
            if (dataval.type() != str_type)
                throw JSONRPCError(RPC_TYPE_ERROR, "Missing data String key for proposal");

            CBlock block;
            if (!DecodeHexBlk(block, dataval.get_str()))
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block decode failed");

            uint256 hash = block.GetHash();
            BlockMap::iterator mi = mapBlockIndex.find(hash);
            if (mi != mapBlockIndex.end()) {
                CBlockIndex *pindex = mi->second;
                if (pindex->IsValid(BLOCK_VALID_SCRIPTS))
                    return "duplicate";
                if (pindex->nStatus & BLOCK_FAILED_MASK)
                    return "duplicate-invalid";
                return "duplicate-inconclusive";
            }

            CBlockIndex* const pindexPrev = chainActive.Tip();
            // TestBlockValidity only supports blocks built on the current Tip
            if (block.hashPrevBlock != pindexPrev->GetBlockHash())
                return "inconclusive-not-best-prevblk";
            CValidationState state;
            TestBlockValidity(state, block, pindexPrev, false, true);
            return BIP22ValidationResult(state);
        }
    }

    if (strMode != "template")
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");

    if (vNodes.empty())
        throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Crown is not connected!");

    if (IsInitialBlockDownload())
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Crown is downloading blocks...");

    static unsigned int nTransactionsUpdatedLast;

    if (lpval.type() != null_type)
    {
        // Wait to respond until either the best block changes, OR a minute has passed and there are more transactions
        uint256 hashWatchedChain;
        boost::system_time checktxtime;
        unsigned int nTransactionsUpdatedLastLP;

        if (lpval.type() == str_type)
        {
            // Format: <hashBestChain><nTransactionsUpdatedLast>
            std::string lpstr = lpval.get_str();

            hashWatchedChain.SetHex(lpstr.substr(0, 64));
            nTransactionsUpdatedLastLP = atoi64(lpstr.substr(64));
        }
        else
        {
            // NOTE: Spec does not specify behaviour for non-string longpollid, but this makes testing easier
            hashWatchedChain = chainActive.Tip()->GetBlockHash();
            nTransactionsUpdatedLastLP = nTransactionsUpdatedLast;
        }

        // Release the wallet and main lock while waiting
#ifdef ENABLE_WALLET
        if(pwalletMain)
            LEAVE_CRITICAL_SECTION(pwalletMain->cs_wallet);
#endif
        LEAVE_CRITICAL_SECTION(cs_main);
        {
            checktxtime = boost::get_system_time() + boost::posix_time::minutes(1);

            boost::unique_lock<boost::mutex> lock(csBestBlock);
            while (chainActive.Tip()->GetBlockHash() == hashWatchedChain && IsRPCRunning())
            {
                if (!cvBlockChange.timed_wait(lock, checktxtime))
                {
                    // Timeout: Check transactions for update
                    if (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLastLP)
                        break;
                    checktxtime += boost::posix_time::seconds(10);
                }
            }
        }
        ENTER_CRITICAL_SECTION(cs_main);
#ifdef ENABLE_WALLET
        if(pwalletMain)
            ENTER_CRITICAL_SECTION(pwalletMain->cs_wallet);
#endif

        if (!IsRPCRunning())
            throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Shutting down");
        // TODO: Maybe recheck connections/IBD and (if something wrong) send an expires-immediately template to stop miners?
    }

    // Update block
    static CBlockIndex* pindexPrev;
    static int64_t nStart;
    static CBlockTemplate* pblocktemplate;
    if (pindexPrev != chainActive.Tip() ||
        (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 5))
    {
        // Clear pindexPrev so future calls make a new block, despite any failures from here on
        pindexPrev = NULL;

        // Store the chainActive.Tip() used before CreateNewBlock, to avoid races
        nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
        CBlockIndex* pindexPrevNew = chainActive.Tip();
        nStart = GetTime();

        // Create new block
        if(pblocktemplate)
        {
            delete pblocktemplate;
            pblocktemplate = NULL;
        }
        CScript scriptDummy = CScript() << OP_TRUE;
        pblocktemplate = CreateNewBlock(scriptDummy);
        if (!pblocktemplate)
            throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");

        // Need to update only after we know CreateNewBlock succeeded
        pindexPrev = pindexPrevNew;
    }
    CBlock* pblock = &pblocktemplate->block; // pointer for convenience

    // Update nTime
    UpdateTime(pblock, pindexPrev);
    pblock->nNonce = 0;

    static const Array aCaps = boost::assign::list_of("proposal");

    Array transactions;
    map<uint256, int64_t> setTxIndex;
    int i = 0;
    BOOST_FOREACH (CTransaction& tx, pblock->vtx)
    {
        uint256 txHash = tx.GetHash();
        setTxIndex[txHash] = i++;

        if (tx.IsCoinBase())
            continue;

        Object entry;

        entry.push_back(Pair("data", EncodeHexTx(tx)));

        entry.push_back(Pair("hash", txHash.GetHex()));

        Array deps;
        BOOST_FOREACH (const CTxIn &in, tx.vin)
        {
            if (setTxIndex.count(in.prevout.hash))
                deps.push_back(setTxIndex[in.prevout.hash]);
        }
        entry.push_back(Pair("depends", deps));

        int index_in_template = i - 1;
        entry.push_back(Pair("fee", pblocktemplate->vTxFees[index_in_template]));
        entry.push_back(Pair("sigops", pblocktemplate->vTxSigOps[index_in_template]));

        transactions.push_back(entry);
    }

    Object aux;
    aux.push_back(Pair("flags", HexStr(COINBASE_FLAGS.begin(), COINBASE_FLAGS.end())));

    arith_uint256 hashTarget = arith_uint256().SetCompact(pblock->nBits);

    static Array aMutable;
    if (aMutable.empty())
    {
        aMutable.push_back("time");
        aMutable.push_back("transactions");
        aMutable.push_back("prevblock");
    }

    Array aVotes;

    Object result;
    result.push_back(Pair("capabilities", aCaps));
    result.push_back(Pair("version", pblock->nVersion.GetFullVersion()));
    result.push_back(Pair("previousblockhash", pblock->hashPrevBlock.GetHex()));
    result.push_back(Pair("transactions", transactions));
    result.push_back(Pair("coinbaseaux", aux));
    result.push_back(Pair("coinbasevalue", (int64_t)pblock->vtx[0].GetValueOut()));
    result.push_back(Pair("longpollid", chainActive.Tip()->GetBlockHash().GetHex() + i64tostr(nTransactionsUpdatedLast)));
    result.push_back(Pair("target", hashTarget.GetHex()));
    result.push_back(Pair("mintime", (int64_t)pindexPrev->GetMedianTimePast()+1));
    result.push_back(Pair("mutable", aMutable));
    result.push_back(Pair("noncerange", "00000000ffffffff"));
    result.push_back(Pair("sigoplimit", (int64_t)MAX_BLOCK_SIGOPS));
    result.push_back(Pair("sizelimit", (int64_t)MAX_BLOCK_SIZE));
    result.push_back(Pair("curtime", pblock->GetBlockTime()));
    result.push_back(Pair("bits", strprintf("%08x", pblock->nBits)));
    result.push_back(Pair("height", (int64_t)(pindexPrev->nHeight+1)));
    result.push_back(Pair("votes", aVotes));


    if(pblock->payee != CScript()){
        CTxDestination address1;
        ExtractDestination(pblock->payee, address1);
        CBitcoinAddress address2(address1);
        result.push_back(Pair("payee", address2.ToString().c_str()));
        result.push_back(Pair("payee_amount", (int64_t)pblock->vtx[0].vout[1].nValue));
    } else {
        result.push_back(Pair("payee", ""));
        result.push_back(Pair("payee_amount", ""));
    }

    result.push_back(Pair("masternode_payments", pblock->nTime > Params().StartMasternodePayments()));
    result.push_back(Pair("enforce_masternode_payments", true));

    result.push_back(Pair("systemnode_payments", pblock->nTime > Params().StartMasternodePayments()));
    result.push_back(Pair("enforce_systemnode_payments", true));

    if(pblock->payeeSN != CScript()){
        CTxDestination address1;
        ExtractDestination(pblock->payeeSN, address1);
        CBitcoinAddress address2(address1);
        result.push_back(Pair("payeeSN", address2.ToString().c_str()));
        result.push_back(Pair("payeeSN_amount", (int64_t)pblock->vtx[0].vout[2].nValue));
    } else {
        result.push_back(Pair("payeeSN", ""));
        result.push_back(Pair("payeeSN_amount", ""));
    }

    Array superblockObjArray;
    if(pblock->voutSuperblock.size())
    {
        BOOST_FOREACH(const CTxOut& txout, pblock->voutSuperblock)
        {
            Object entry;
            CTxDestination address;
            ExtractDestination(txout.scriptPubKey, address);
            CBitcoinAddress address2(address);
            entry.push_back(Pair("payee", address2.ToString().c_str()));
            entry.push_back(Pair("script", HexStr(txout.scriptPubKey)));
            entry.push_back(Pair("amount", txout.nValue));
            superblockObjArray.push_back(entry);
        }
    }
    result.push_back(Pair("superblock", superblockObjArray));
    result.push_back(Pair("superblocks_enabled", IsSporkActive(SPORK_13_ENABLE_SUPERBLOCKS)));

    return result;
}

class submitblock_StateCatcher : public CValidationInterface
{
public:
    uint256 hash;
    bool found;
    CValidationState state;

    submitblock_StateCatcher(const uint256 &hashIn) : hash(hashIn), found(false), state() {};

protected:
    virtual void BlockChecked(const CBlock& block, const CValidationState& stateIn) {
        if (block.GetHash() != hash)
            return;
        found = true;
        state = stateIn;
    };
};

Value submitblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "submitblock \"hexdata\" ( \"jsonparametersobject\" )\n"
            "\nAttempts to submit new block to network.\n"
            "The 'jsonparametersobject' parameter is currently ignored.\n"
            "See https://en.bitcoin.it/wiki/BIP_0022 for full specification.\n"

            "\nArguments\n"
            "1. \"hexdata\"    (string, required) the hex-encoded block data to submit\n"
            "2. \"jsonparametersobject\"     (string, optional) object of optional parameters\n"
            "    {\n"
            "      \"workid\" : \"id\"    (string, optional) if the server provided a workid, it MUST be included with submissions\n"
            "    }\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("submitblock", "\"mydata\"")
            + HelpExampleRpc("submitblock", "\"mydata\"")
        );

    CBlock block;
    if (!DecodeHexBlk(block, params[0].get_str()))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block decode failed");

    uint256 hash = block.GetHash();
    bool fBlockPresent = false;
    {
        LOCK(cs_main);
        BlockMap::iterator mi = mapBlockIndex.find(hash);
        if (mi != mapBlockIndex.end()) {
            CBlockIndex *pindex = mi->second;
            if (pindex->IsValid(BLOCK_VALID_SCRIPTS))
                return "duplicate";
            if (pindex->nStatus & BLOCK_FAILED_MASK)
                return "duplicate-invalid";
            // Otherwise, we might only have the header - process the block before returning
            fBlockPresent = true;
        }
    }

    CValidationState state;
    submitblock_StateCatcher sc(block.GetHash());
    RegisterValidationInterface(&sc);
    bool fAccepted = ProcessNewBlock(state, NULL, &block);
    UnregisterValidationInterface(&sc);
    if (fBlockPresent)
    {
        if (fAccepted && !sc.found)
            return "duplicate-inconclusive";
        return "duplicate";
    }
    if (fAccepted)
    {
        if (!sc.found)
            return "inconclusive";
        state = sc.state;
    }
    return BIP22ValidationResult(state);
}



/* ************************************************************************** */
/* Merge mining.  */

#ifdef ENABLE_WALLET
Value getauxblock(const Array& params, bool fHelp)
{
    if (fHelp || (params.size() != 0 && params.size() != 2))
        throw std::runtime_error(
            "getauxblock (hash auxpow)\n"
            "\nCreate or submit a merge-mined block.\n"
            "\nWithout arguments, create a new block and return information\n"
            "required to merge-mine it.  With arguments, submit a solved\n"
            "auxpow for a previously returned block.\n"
            "\nArguments:\n"
            "1. \"hash\"    (string, optional) hash of the block to submit\n"
            "2. \"auxpow\"  (string, optional) serialised auxpow found\n"
            "\nResult (without arguments):\n"
            "{\n"
            "  \"hash\"               (string) hash of the created block\n"
            "  \"chainid\"            (numeric) chain ID for this block\n"
            "  \"previousblockhash\"  (string) hash of the previous block\n"
            "  \"coinbasevalue\"      (numeric) value of the block's coinbase\n"
            "  \"bits\"               (string) compressed target of the block\n"
            "  \"height\"             (numeric) height of the block\n"
            "  \"target\"            (string) target in reversed byte order, deprecated\n"
            "}\n"
            "\nResult (with arguments):\n"
            "xxxxx        (boolean) whether the submitted block was correct\n"
            "\nExamples:\n"
            + HelpExampleCli("getauxblock", "")
            + HelpExampleCli("getauxblock", "\"hash\" \"serialised auxpow\"")
            + HelpExampleRpc("getauxblock", "")
            );

    /*
    boost::shared_ptr<CReserveScript> coinbaseScript;
    GetMainSignals().ScriptForMining(coinbaseScript);

    // If the keypool is exhausted, no script is returned at all.  Catch this.
    if (!coinbaseScript)
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");

    //throw an error if no script was provided
    if (!coinbaseScript->reserveScript.size())
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No coinbase script available (mining requires a wallet)");
    */

    if (vNodes.empty())
        throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED,
                           "Crown is not connected!");

    if (IsInitialBlockDownload())
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD,
                           "Crown is downloading blocks...");

    /* This should never fail, since the chain is already
        past the point of merge-mining start.  Check nevertheless.  */
    {
        LOCK(cs_main);
        if (chainActive.Height() + 1 < Params().AuxpowStartHeight())
            throw std::runtime_error("getauxblock method is not yet available");
    }

    /* The variables below are used to keep track of created and not yet
       submitted auxpow blocks.  Lock them to be sure even for multiple
       RPC threads running in parallel.  */
    static CCriticalSection cs_auxblockCache;
    LOCK(cs_auxblockCache);
    static std::map<uint256, CBlock*> mapNewBlock;
    static std::vector<CBlockTemplate*> vNewBlockTemplate;

    /* Create a new block?  */
    if (params.size() == 0)
    {
        static unsigned nTransactionsUpdatedLast;
        static const CBlockIndex* pindexPrev = NULL;
        static uint64_t nStart;
        static CBlockTemplate* pblocktemplate;
        static unsigned nExtraNonce = 0;

        // Update block
        {
        LOCK(cs_main);
        if (pindexPrev != chainActive.Tip()
            || (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast
                && GetTime() - nStart > 60))
        {
            if (pindexPrev != chainActive.Tip())
            {
                // Deallocate old blocks since they're obsolete now
                mapNewBlock.clear();
                BOOST_FOREACH(CBlockTemplate* pbt, vNewBlockTemplate)
                    delete pbt;
                vNewBlockTemplate.clear();
            }

            // Create new block with nonce = 0 and extraNonce = 1
            CKeyID result;
            CBitcoinAddress auxminingaddr(GetArg("-auxminingaddr", ""));
            if (!auxminingaddr.GetKeyID(result)) {
                CReserveKey reservekey(pwalletMain);
                CPubKey pubkey;
                reservekey.GetReservedKey(pubkey);
                result = pubkey.GetID();
            }
            CScript scriptCoinbase = GetScriptForDestination(result);
            pblocktemplate = CreateNewBlock(scriptCoinbase);
            if (!pblocktemplate)
                throw JSONRPCError(RPC_OUT_OF_MEMORY, "out of memory");

            // Update state only when CreateNewBlock succeeded
            nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
            pindexPrev = chainActive.Tip();
            nStart = GetTime();

            // Finalise it by setting the version and building the merkle root
            CBlock* pblock = &pblocktemplate->block;
            IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);
            pblock->nVersion.SetAuxpow(true);

            // Save
            mapNewBlock[pblock->GetHash()] = pblock;
            vNewBlockTemplate.push_back(pblocktemplate);
        }
        }

        const CBlock& block = pblocktemplate->block;

        arith_uint256 target;
        bool fNegative, fOverflow;
        target.SetCompact(block.nBits, &fNegative, &fOverflow);
        if (fNegative || fOverflow || target == 0)
            throw std::runtime_error("invalid difficulty bits in block");

        json_spirit::Object result;
        result.push_back(Pair("hash", block.GetHash().GetHex()));
        result.push_back(Pair("chainid", block.nVersion.GetChainId()));
        result.push_back(Pair("previousblockhash", block.hashPrevBlock.GetHex()));
        result.push_back(Pair("coinbasevalue", (int64_t)block.vtx[0].vout[0].nValue));
        result.push_back(Pair("bits", strprintf("%08x", block.nBits)));
        result.push_back(Pair("height", static_cast<int64_t> (pindexPrev->nHeight + 1)));
        result.push_back(Pair("target", HexStr(BEGIN(target), END(target))));

        return result;
    }

    /* Submit a block instead.  */

    assert(params.size() == 2);
    uint256 hash;
    hash.SetHex(params[0].get_str());

    const std::map<uint256, CBlock*>::iterator mit = mapNewBlock.find(hash);
    if (mit == mapNewBlock.end())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "block hash unknown");
    CBlock& block = *mit->second;

    const std::vector<unsigned char> vchAuxPow = ParseHex(params[1].get_str());
    CDataStream ss(vchAuxPow, SER_GETHASH, PROTOCOL_VERSION);
    CAuxPow pow;
    ss >> pow;
    block.SetAuxpow(new CAuxPow(pow));
    assert(block.GetHash() == hash);

    return ProcessBlockFound(&block, *pwalletMain, *pminingKey);
}
#endif // ENABLE_WALLET

Value estimatefee(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "estimatefee nblocks\n"
            "\nEstimates the approximate fee per kilobyte\n"
            "needed for a transaction to begin confirmation\n"
            "within nblocks blocks.\n"
            "\nArguments:\n"
            "1. nblocks     (numeric)\n"
            "\nResult:\n"
            "n :    (numeric) estimated fee-per-kilobyte\n"
            "\n"
            "-1.0 is returned if not enough transactions and\n"
            "blocks have been observed to make an estimate.\n"
            "\nExample:\n"
            + HelpExampleCli("estimatefee", "6")
            );

    RPCTypeCheck(params, boost::assign::list_of(int_type));

    int nBlocks = params[0].get_int();
    if (nBlocks < 1)
        nBlocks = 1;

    CFeeRate feeRate = mempool.estimateFee(nBlocks);
    if (feeRate == CFeeRate(0))
        return -1.0;

    return ValueFromAmount(feeRate.GetFeePerK());
}

Value estimatepriority(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "estimatepriority nblocks\n"
            "\nEstimates the approximate priority\n"
            "a zero-fee transaction needs to begin confirmation\n"
            "within nblocks blocks.\n"
            "\nArguments:\n"
            "1. nblocks     (numeric)\n"
            "\nResult:\n"
            "n :    (numeric) estimated priority\n"
            "\n"
            "-1.0 is returned if not enough transactions and\n"
            "blocks have been observed to make an estimate.\n"
            "\nExample:\n"
            + HelpExampleCli("estimatepriority", "6")
            );

    RPCTypeCheck(params, boost::assign::list_of(int_type));

    int nBlocks = params[0].get_int();
    if (nBlocks < 1)
        nBlocks = 1;

    return mempool.estimatePriority(nBlocks);
}
