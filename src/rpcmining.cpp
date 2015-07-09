// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcserver.h"
#include "chainparams.h"
#include "init.h"
#include "net.h"
#include "main.h"
#include "miner.h"
#ifdef ENABLE_WALLET
#include "db.h"
#include "wallet.h"
#endif
#include <stdint.h>

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace json_spirit;
using namespace std;

static const unsigned int sha256DigestChunkByteSize = 64;

#ifdef ENABLE_WALLET
// Key used by getwork miners.
// Allocated in InitRPCMining, free'd in ShutdownRPCMining
static Credits_CReserveKey* pMiningKey = NULL;
static Credits_CReserveKey* pDepositMiningKey = NULL;
static Credits_CReserveKey* pDepositChangeMiningKey = NULL;
static Credits_CReserveKey* pDepositSigningMiningKey = NULL;
static bool coinbaseDepositDisabled = false;

void InitRPCMining(bool coinbaseDepositDisabledIn)
{
	coinbaseDepositDisabled = coinbaseDepositDisabledIn;

    if (!bitcredit_pwalletMain)
        return;

    // getwork/getblocktemplate mining rewards paid here
    //in the case of using coinbase as deposit the coinbase is spent in the same block and the
    //pDepositMiningKey becomes the holder of the mining reward
    if(bitcredit_pwalletMain->IsLocked() && !coinbaseDepositDisabled) {
    	pMiningKey = new Credits_CReserveKey(deposit_pwalletMain);
    } else {
    	pMiningKey = new Credits_CReserveKey(bitcredit_pwalletMain);
    }
    pDepositMiningKey = new Credits_CReserveKey(bitcredit_pwalletMain);
    pDepositChangeMiningKey = new Credits_CReserveKey(bitcredit_pwalletMain);
    pDepositSigningMiningKey = new Credits_CReserveKey(deposit_pwalletMain);
}

void ShutdownRPCMining()
{
    if (!pMiningKey)
        return;
    if (!pDepositMiningKey)
        return;
    if (!pDepositChangeMiningKey)
        return;
    if (!pDepositSigningMiningKey)
        return;

    delete pMiningKey; pMiningKey = NULL;
    delete pDepositMiningKey; pDepositMiningKey = NULL;
    delete pDepositChangeMiningKey; pDepositChangeMiningKey = NULL;
    delete pDepositSigningMiningKey; pDepositSigningMiningKey = NULL;
    coinbaseDepositDisabled = false;
}
#else
void InitRPCMining(bool coinbaseDepositDisabled)
{
}
void ShutdownRPCMining()
{
}
#endif

// Return average network hashes per second based on the last 'lookup' blocks,
// or from the last difficulty change if 'lookup' is nonpositive.
// If 'height' is nonnegative, compute the estimate at the time when a given block was found.
Value GetNetworkHashPS(int lookup, int height) {
    CBlockIndexBase *pb = credits_chainActive.Tip();

    if (height >= 0 && height < credits_chainActive.Height())
        pb = credits_chainActive[height];

    if (pb == NULL || !pb->nHeight)
        return 0;

    // If lookup is -1, then use blocks since last difficulty change.
    if (lookup <= 0)
        lookup = pb->nHeight % 2016 + 1;

    // If lookup is larger than chain, then set it to chain length.
    if (lookup > pb->nHeight)
        lookup = pb->nHeight;

    CBlockIndexBase *pb0 = pb;
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

    uint256 workDiff = pb->nChainWork - pb0->nChainWork;
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
            "It is set with the command line argument -gen (or credits.conf setting gen)\n"
            "It can also be set with the setgenerate call.\n"
            "\nResult\n"
            "true|false      (boolean) If the server is set to generate coins or not\n"
            "true|false      (boolean) If mining with coinbase included is disabled or not\n"
            "\nExamples:\n"
            + HelpExampleCli("getgenerate", "")
            + HelpExampleRpc("getgenerate", "")
        );

    if (!pMiningKey)
        return false;
    if (!pDepositMiningKey)
        return false;
    if (!pDepositChangeMiningKey)
        return false;
    if (!pDepositSigningMiningKey)
        return false;

    return GetBoolArg("-gen", false);
}


Value setgenerate(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "setgenerate generate ( genproclimit ) (coinbasedepositdisabled) \n"
            "\nSet 'generate' true or false to turn generation on or off.\n"
            "Generation is limited to 'genproclimit' processors, -1 is unlimited.\n"
            "See the getgenerate call for the current setting.\n"
            "\nArguments:\n"
            "1. generate         (boolean, required) Set to true to turn on generation, off to turn off.\n"
            "2. genproclimit     (numeric, optional) Set the processor limit for when generation is on. Can be -1 for unlimited.\n"
            "                    Note: in -regtest mode, genproclimit controls how many blocks are generated immediately.\n"
            "3. coinbasedepositdisabled     (boolean, optional) If set to true, using coinbase as part of deposit will be disabled. Default false. \n"
            "\nExamples:\n"
            "\nSet the generation on with a limit of one processor with coinabse as deposit disabled\n"
            + HelpExampleCli("setgenerate", "true 1 true") +
            "\nCheck the setting\n"
            + HelpExampleCli("getgenerate", "") +
            "\nTurn off generation\n"
            + HelpExampleCli("setgenerate", "false") +
            "\nUsing json rpc\n"
            + HelpExampleRpc("setgenerate", "true, 1")
        );

    if (bitcredit_pwalletMain == NULL)
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

    bool coinbaseDepositDisabled = false;
    if (params.size() > 2)
    {
    	coinbaseDepositDisabled = params[2].get_bool();
    }

    // -regtest mode: don't return until nGenProcLimit blocks are generated
    if (fGenerate && Credits_Params().NetworkID() == CChainParams::REGTEST)
    {
        int nHeightStart = 0;
        int nHeightEnd = 0;
        int nHeight = 0;
        int nGenerate = (nGenProcLimit > 0 ? nGenProcLimit : 1);
        {   // Don't keep cs_main locked
            LOCK(credits_mainState.cs_main);
            nHeightStart = credits_chainActive.Height();
            nHeight = nHeightStart;
            nHeightEnd = nHeightStart+nGenerate;
        }
        int nHeightLast = -1;
        while (nHeight < nHeightEnd)
        {
            if (nHeightLast != nHeight)
            {
                nHeightLast = nHeight;
                GenerateBitcredits(fGenerate, bitcredit_pwalletMain, deposit_pwalletMain, 1, coinbaseDepositDisabled);
            }
            MilliSleep(1);
            {   // Don't keep cs_main locked
                LOCK(credits_mainState.cs_main);
                nHeight = credits_chainActive.Height();
            }
        }
    }
    else // Not -regtest: start generate thread, return immediately
    {
        mapArgs["-gen"] = (fGenerate ? "1" : "0");
        mapArgs ["-genproclimit"] = itostr(nGenProcLimit);
        mapArgs ["-coinbasedepositdisabled"] = coinbaseDepositDisabled ? "1" : "0";
        GenerateBitcredits(fGenerate, bitcredit_pwalletMain, deposit_pwalletMain, nGenProcLimit, coinbaseDepositDisabled);
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
            "  \"requireddeposit\": xxxxxx    (numeric) The current required deposit, in smallest possible unit\n"
            "  \"errors\": \"...\"          (string) Current errors\n"
            "  \"generate\": true|false     (boolean) If the generation is on or off (see getgenerate or setgenerate calls)\n"
            "  \"genproclimit\": n          (numeric) The processor limit for generation. -1 if no generation. (see getgenerate or setgenerate calls)\n"
            "  \"coinbasedepositdisabled\": n          (boolean) If set to true, coinbase will NOT be used as deposit. (see getgenerate or setgenerate calls)\n"
            "  \"hashespersec\": n          (numeric) The hashes per second of the generation, or 0 if no generation.\n"
            "  \"pooledtx\": n              (numeric) The size of the mem pool\n"
            "  \"testnet\": true|false      (boolean) If using testnet or not\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getmininginfo", "")
            + HelpExampleRpc("getmininginfo", "")
        );

    const Credits_CBlockIndex* ptip = (Credits_CBlockIndex*)credits_chainActive.Tip();
    const uint64_t nTotalDepositBase = ptip->nTotalDepositBase;

    Object obj;
    obj.push_back(Pair("blocks",           (int)credits_chainActive.Height()));
    obj.push_back(Pair("currentblocksize", (uint64_t)bitcredit_nLastBlockSize));
    obj.push_back(Pair("currentblocktx",   (uint64_t)bitcredit_nLastBlockTx));
    obj.push_back(Pair("difficulty",       (double)Bitcredit_GetDifficulty()));
    obj.push_back(Pair("requireddeposit",       (double)Bitcredit_GetRequiredDeposit(nTotalDepositBase)));
    obj.push_back(Pair("errors",           Bitcredit_GetWarnings("statusbar")));
    obj.push_back(Pair("genproclimit",     (int)GetArg("-genproclimit", -1)));
    obj.push_back(Pair("coinbasedepositdisabled",     (bool)GetBoolArg("-coinbasedepositdisabled", false)));
    obj.push_back(Pair("networkhashps",    getnetworkhashps(params, false)));
    obj.push_back(Pair("pooledtx",         (uint64_t)credits_mempool.size()));
    obj.push_back(Pair("testnet",          Bitcredit_TestNet()));
#ifdef ENABLE_WALLET
    obj.push_back(Pair("generate",         getgenerate(params, false)));
    obj.push_back(Pair("hashespersec",     gethashespersec(params, false)));
#endif
    return obj;
}


#ifdef ENABLE_WALLET
Value getwork(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getwork ( \"data\" )\n"
            "\nIf 'data' is not specified, it returns the formatted hash data to work on.\n"
            "If 'data' is specified, tries to solve the block and returns true if it was successful.\n"
            "\nArguments:\n"
            "1. \"data\"       (string, optional) The hex encoded data to solve\n"
            "\nResult (when 'data' is not specified):\n"
            "{\n"
            "  \"midstate\" : \"xxxx\",   (string) The precomputed hash state after hashing the first half of the data (DEPRECATED)\n" // deprecated
            "  \"data\" : \"xxxxx\",      (string) The block data\n"
            "  \"hash1\" : \"xxxxx\",     (string) The formatted hash buffer for second hash (DEPRECATED)\n" // deprecated
            "  \"target\" : \"xxxx\"      (string) The little endian hash target\n"
            "}\n"
            "\nResult (when 'data' is specified):\n"
            "true|false       (boolean) If solving the block specified in the 'data' was successfull\n"
            "\nExamples:\n"
            + HelpExampleCli("getwork", "")
            + HelpExampleRpc("getwork", "")
        );

    CNetParams * netParams = Credits_NetParams();

    if (netParams->vNodes.empty())
        throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Credits is not connected!");

    if (Bitcredit_IsInitialBlockDownload() || Bitcoin_IsInitialBlockDownload())
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Credits is downloading blocks...");

    typedef map<pair<uint256, uint256>, pair<Credits_CBlock*, pair<Credits_CTransaction, pair<unsigned int, pair<Credits_CTransaction, std::vector<CCompactSignature> > > > > > mapNewBlock_t;
    static mapNewBlock_t mapNewBlock;    // FIXME: thread safety
    static vector<Credits_CBlockTemplate*> vNewBlockTemplate;

    if (params.size() == 0)
    {
        // Update block
        static unsigned int nTransactionsUpdatedLast;
        static Credits_CBlockIndex* pindexPrev;
        static int64_t nStart;
        static Credits_CBlockTemplate* pblocktemplate;
        if (pindexPrev != credits_chainActive.Tip() ||
            (credits_mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 60))
        {
            if (pindexPrev != credits_chainActive.Tip())
            {
                // Deallocate old blocks since they're obsolete now
                mapNewBlock.clear();
                BOOST_FOREACH(Credits_CBlockTemplate* pblocktemplate, vNewBlockTemplate)
                    delete pblocktemplate;
                vNewBlockTemplate.clear();
            }

            // Clear pindexPrev so future getworks make a new block, despite any failures from here on
            pindexPrev = NULL;

            // Store the pindexBest used before CreateNewBlock, to avoid races
            nTransactionsUpdatedLast = credits_mempool.GetTransactionsUpdated();
            Credits_CBlockIndex* pindexPrevNew = (Credits_CBlockIndex*)credits_chainActive.Tip();
            nStart = GetTime();

            // Create new block
            pblocktemplate = CreateNewBlockWithKey(*pMiningKey, *pDepositMiningKey, *pDepositChangeMiningKey, *pDepositSigningMiningKey, bitcredit_pwalletMain, deposit_pwalletMain, coinbaseDepositDisabled);
            if (!pblocktemplate)
                throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");
            vNewBlockTemplate.push_back(pblocktemplate);

            // Need to update only after we know CreateNewBlock succeeded
            pindexPrev = pindexPrevNew;
        }
        Credits_CBlock* pblock = &pblocktemplate->block; // pointer for convenience

        // Update nTime
        Bitcredit_UpdateTime(*pblock, pindexPrev);
        pblock->nNonce = 0;

        // Update nExtraNonce
        static unsigned int nExtraNonce = 0;
        IncrementExtraNonce(pblock, pindexPrev, nExtraNonce, bitcredit_pwalletMain, deposit_pwalletMain, coinbaseDepositDisabled);

        //Arbitrary size limit to prevent running out of memory
    	if(mapNewBlock.size() > 5000) {
    		mapNewBlock.clear();
    	}

        // Save
    	const Credits_CTransaction coinbase = pblock->vtx[0];
    	//Find the position of the coinbase referencing deposit (if any)
    	const uint256 coinbaseHash = coinbase.GetHash();
    	unsigned int coinbaseSpendingDepositPos = 1;
    	for (unsigned int i = coinbaseSpendingDepositPos; i < pblock->vtx.size(); i++) {
    		if (pblock->vtx[i].IsDeposit()) {
    			Credits_CTransaction &txDeposit = pblock->vtx[i];
    			Credits_CTxIn & txDepositIn = txDeposit.vin[0];

    			if(txDepositIn.prevout.hash == coinbaseHash) {
    				coinbaseSpendingDepositPos = i;
    			}
    		} else {
    			break;
    		}
    	}
    	const Credits_CTransaction coinbaseSpendingDeposit = pblock->vtx[coinbaseSpendingDepositPos];
    	const std::vector<CCompactSignature> vSigs = pblock->vsig;
        mapNewBlock[make_pair(pblock->hashMerkleRoot, pblock->hashSigMerkleRoot)] = make_pair(pblock, make_pair(coinbase, make_pair(coinbaseSpendingDepositPos, make_pair(coinbaseSpendingDeposit, vSigs))));

        // Pre-build hash buffers
        char pmidstate[32];
        char pmidstate2[32];
        char pdata[168];
        char phash1[64];
        FormatHashBuffers(pblock, pmidstate, pdata, phash1);

        //Precalc the second hashing of three 64 byte chunks that are required to hash the header
    	SHA256Transform(pmidstate2, pdata + sha256DigestChunkByteSize, pmidstate);

        uint256 hashTarget = uint256().SetCompact(pblock->nBits);

        if (bitcredit_fBenchmark) {
			//// debug print
			LogPrintf("getwork:\n\n\n");
			LogPrintf("block sent as data: %s\n", pblock->GetHash().GetHex());
			pblock->print();
			LogPrintf("\n\n\n");
        }

        Object result;
        result.push_back(Pair("midstate", HexStr(BEGIN(pmidstate2), END(pmidstate2)))); // deprecated
        result.push_back(Pair("data",     HexStr(BEGIN(pdata), END(pdata))));
        result.push_back(Pair("hash1",    HexStr(BEGIN(phash1), END(phash1)))); // deprecated
        result.push_back(Pair("target",   HexStr(BEGIN(hashTarget), END(hashTarget))));
        return result;
    }
    else
    {
        // Parse parameters
        vector<unsigned char> vchData = ParseHex(params[0].get_str());
        if (vchData.size() != 168)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter");
        Credits_CBlock* pdata = (Credits_CBlock*)&vchData[0];

        // Byte reverse
        for (int i = 0; i < 168/4; i++)
            ((unsigned int*)pdata)[i] = ByteReverse(((unsigned int*)pdata)[i]);

        pair<uint256, uint256> blockKey = make_pair(pdata->hashMerkleRoot, pdata->hashSigMerkleRoot);

        // Get saved block
        if (!mapNewBlock.count(blockKey)) {
        	LogPrintf("could not find block from key, merkle: %s, sigmerkle: %s, time: %d\n", pdata->hashMerkleRoot.GetHex(), pdata->hashSigMerkleRoot.GetHex(), pdata->nTime);
        	return false;
        }

        Credits_CBlock block = *mapNewBlock[blockKey].first;
        block.nTime = pdata->nTime;
        block.nNonce = pdata->nNonce;
        block.vtx[0] = mapNewBlock[blockKey].second.first;
    	unsigned int coinbaseSpendingDepositPos = mapNewBlock[blockKey].second.second.first;
        block.vtx[coinbaseSpendingDepositPos] = mapNewBlock[blockKey].second.second.second.first;
        block.vsig = mapNewBlock[blockKey].second.second.second.second;
        block.hashMerkleRoot = block.BuildMerkleTree();
        block.hashSigMerkleRoot = block.BuildSigMerkleTree();

        if (bitcredit_fBenchmark) {
			//// debug print
			LogPrintf("\n\ngetwork:\n");
			LogPrintf("block recreated from data: %s\n", block.GetHash().GetHex());
			block.print();
			LogPrintf("\n\n\n");
        }

        if(block.hashMerkleRoot != pdata->hashMerkleRoot) {
        	LogPrintf("recreated block merkle hash does not match data merkle hash, recreated: %s, data: %s\n", block.hashMerkleRoot.GetHex(), pdata->hashMerkleRoot.GetHex());
        	return false;
        }
        if(block.hashSigMerkleRoot != pdata->hashSigMerkleRoot) {
        	LogPrintf("recreated block sig merkle hash does not match data sig merkle hash, recreated: %s, data: %s\n", block.hashSigMerkleRoot.GetHex(), pdata->hashSigMerkleRoot.GetHex());
        	return false;
        }

        assert(bitcredit_pwalletMain != NULL);
        Credits_CBlock * pblock = &block;
        return CheckWork(pblock, *bitcredit_pwalletMain, *deposit_pwalletMain, *pMiningKey, *pDepositMiningKey, *pDepositChangeMiningKey, *pDepositSigningMiningKey);
    }
}
#endif

Value getblocktemplate(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
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
            "         \"fee\": n,                   (numeric) difference in value between transaction inputs and outputs (in Satoshis); for coinbase transactions, this is a negative Number of the total collected block fees (ie, not including the block subsidy); if key is not present, fee is unknown and clients MUST NOT assume there isn't one\n"
            "         \"sigops\" : n,               (numeric) total number of SigOps, as counted for purposes of block limits; if key is not present, sigop count is unknown and clients MUST NOT assume there aren't any\n"
            "         \"required\" : true|false     (boolean) if provided and true, this transaction must be in the final block\n"
            "      }\n"
            "      ,...\n"
            "  ],\n"
            "  \"coinbaseaux\" : {                  (json object) data that should be included in the coinbase's scriptSig content\n"
            "      \"flags\" : \"flags\"            (string) \n"
            "  },\n"
            "  \"coinbasevalue\" : n,               (numeric) maximum allowable input to coinbase transaction, including the generation award and transaction fees (in Satoshis)\n"
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
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("getblocktemplate", "")
            + HelpExampleRpc("getblocktemplate", "")
         );

    CNetParams * netParams = Credits_NetParams();

    std::string strMode = "template";
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
    }
    bool coinbaseDepositDisabled = true;
    if (params.size() > 1)
    {
        coinbaseDepositDisabled = params[1].get_bool();
    }

    if (strMode != "template")
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");

    if (netParams->vNodes.empty())
        throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Credits is not connected!");

    if (Bitcredit_IsInitialBlockDownload() || Bitcoin_IsInitialBlockDownload())
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Credits is downloading blocks...");

    // Update block
    static unsigned int nTransactionsUpdatedLast;
    static Credits_CBlockIndex* pindexPrev;
    static int64_t nStart;
    static Credits_CBlockTemplate* pblocktemplate;
    if (pindexPrev != credits_chainActive.Tip() ||
        (credits_mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 5))
    {
        // Clear pindexPrev so future calls make a new block, despite any failures from here on
        pindexPrev = NULL;

        // Store the pindexBest used before CreateNewBlock, to avoid races
        nTransactionsUpdatedLast = credits_mempool.GetTransactionsUpdated();
        Credits_CBlockIndex* pindexPrevNew = (Credits_CBlockIndex*)credits_chainActive.Tip();
        nStart = GetTime();

        // Create new block
        if(pblocktemplate)
        {
            delete pblocktemplate;
            pblocktemplate = NULL;
        }
        //In bitcoin code, this section sets empty keys with CreateNewBlock instead of CreateNewBlockWithKey.
        //We don't have that freedom here since the coinbase and deposits are linked toghether IF coinbase is used as deposit.
        //The creation of the coinbase is also quite different from bitcoin coinbase since it has a block "locking" value in vin[0].hash.
        //See block.RecalcLockHashAndMerkleRoot() for more info
        //
        //Coinbase and all other transactions can not be freely updated by miners participating in pools and pMiningKey must be set in this rpc call
        //The dependencies are as follows:
        //  - IF coinbase is used as deposit coinbase -> deposit -> hashMerkleRoot -> hashSigMerkleRoot
        //  - ANY tx including coinbase and deposits -> hashMerkleRoot -> hashSigMerkleRoot
        //The hashSigMerkleRoot means that transactions can not be freely replaced in the block without asking the depositors for a new signature of the hashMerkleRoot
        //
        //Note that the keys are not taken out of circulation here so it may happen that they are reused. It also means that the wallet(s) for the server
        //which answers to the getblocktemplate calls are VERY important, since it holds keys to coin txouts
        pblocktemplate = CreateNewBlockWithKey(*pMiningKey, *pDepositMiningKey, *pDepositChangeMiningKey, *pDepositSigningMiningKey, bitcredit_pwalletMain, deposit_pwalletMain, coinbaseDepositDisabled);
        if (!pblocktemplate)
            throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");

        // Need to update only after we know CreateNewBlock succeeded
        pindexPrev = pindexPrevNew;
    }
    Credits_CBlock* pblock = &pblocktemplate->block; // pointer for convenience

    // Update nTime
    Bitcredit_UpdateTime(*pblock, pindexPrev);
    pblock->nNonce = 0;

    Array transactions;
    map<uint256, int64_t> setTxIndex;
    int i = 0;
    BOOST_FOREACH (Credits_CTransaction& tx, pblock->vtx)
    {
        uint256 txHash = tx.GetHash();
        setTxIndex[txHash] = i++;

        if (tx.IsCoinBase())
            continue;

        Object entry;

        CDataStream ssTx(SER_NETWORK, CREDITS_PROTOCOL_VERSION);
        ssTx << tx;
        entry.push_back(Pair("data", HexStr(ssTx.begin(), ssTx.end())));

        entry.push_back(Pair("hash", txHash.GetHex()));

        Array deps;
        BOOST_FOREACH (const Credits_CTxIn &in, tx.vin)
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
    aux.push_back(Pair("flags", HexStr(BITCREDIT_COINBASE_FLAGS.begin(), BITCREDIT_COINBASE_FLAGS.end())));

    uint256 hashTarget = uint256().SetCompact(pblock->nBits);

    static Array aMutable;
    if (aMutable.empty())
    {
        aMutable.push_back("time");
        aMutable.push_back("transactions");
        aMutable.push_back("prevblock");
    }

    Object result;
    result.push_back(Pair("version", pblock->nVersion));
    result.push_back(Pair("previousblockhash", pblock->hashPrevBlock.GetHex()));
    result.push_back(Pair("transactions", transactions));
    result.push_back(Pair("coinbaseaux", aux));
    result.push_back(Pair("coinbasevalue", (int64_t)pblock->vtx[0].vout[0].nValue));
    result.push_back(Pair("target", hashTarget.GetHex()));
    result.push_back(Pair("mintime", (int64_t)pindexPrev->GetMedianTimePast()+1));
    result.push_back(Pair("mutable", aMutable));
    result.push_back(Pair("noncerange", "00000000ffffffff"));
    result.push_back(Pair("totalmonetarybase", (pblock->nTotalMonetaryBase + pblock->vtx[0].vout[0].nValue)));
    result.push_back(Pair("totaldepositbase", (pblock->nTotalDepositBase + pblock->vtx[0].vout[0].nValue)));
    result.push_back(Pair("deposit", pblock->nDepositAmount));
    result.push_back(Pair("hashlinkedbitcoinblock", pblock->hashLinkedBitcoinBlock.GetHex()));
    result.push_back(Pair("hashsigmerkleroot", pblock->hashSigMerkleRoot.GetHex()));
    result.push_back(Pair("sigoplimit", (int64_t)BITCREDIT_MAX_BLOCK_SIGOPS));
    result.push_back(Pair("sizelimit", (int64_t)BITCREDIT_MAX_BLOCK_SIZE));
    result.push_back(Pair("curtime", (int64_t)pblock->nTime));
    result.push_back(Pair("bits", HexBits(pblock->nBits)));
    result.push_back(Pair("height", (int64_t)(pindexPrev->nHeight+1)));

    return result;
}

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

    vector<unsigned char> blockData(ParseHex(params[0].get_str()));
    CDataStream ssBlock(blockData, SER_NETWORK, CREDITS_PROTOCOL_VERSION);
    Credits_CBlock pblock;
    try {
        ssBlock >> pblock;
    }
    catch (std::exception &e) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block decode failed");
    }

    CValidationState state;
    bool fAccepted = Bitcredit_ProcessBlock(state, NULL, &pblock);
    if (!fAccepted)
        return "rejected"; // TODO: report validation state

    return Value::null;
}
