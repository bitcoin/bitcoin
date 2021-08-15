// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <chain.h>
#include <chainparams.h>
#include <consensus/consensus.h>
#include <consensus/params.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <key_io.h>
#include <miner.h>
#include <net.h>
#include <poc/poc.h>
#include <policy/fees.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <script/script.h>
#include <shutdown.h>
#include <txdb.h>
#include <txmempool.h>
#include <univalue.h>
#include <util/fees.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/validation.h>
#include <validation.h>
#include <validationinterface.h>
#include <versionbitsinfo.h>
#include <warnings.h>

#ifdef ENABLE_WALLET
// TODO try remove
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>
#endif

#include <memory>
#include <stdint.h>

// generate nGenerate blocks to coinbase_script and signing with private_key
static UniValue generateBlocks(const CScript& coinbase_script, const std::shared_ptr<CKey> private_key, int nGenerate)
{
    int nHeightEnd = 0;
    int nHeight = 0;

    {   // Don't keep cs_main locked
        LOCK(cs_main);
        nHeight = ::ChainActive().Height();
        nHeightEnd = nHeight+nGenerate;
    }
    const uint64_t nPlotterId = 18378006326320094226ULL; // from "root minute ancient won check dove second spot book thump retreat add"
    UniValue blockHashes(UniValue::VARR);
    while (nHeight < nHeightEnd && !ShutdownRequested())
    {
        uint64_t nDeadline = static_cast<uint64_t>(Params().GetConsensus().nPowTargetSpacing);
        if (nHeight <= 1)
            nDeadline = 0;
        std::unique_ptr<CBlockTemplate> pblocktemplate(BlockAssembler(Params()).CreateNewBlock(coinbase_script, nPlotterId, nDeadline, nDeadline, private_key));
        if (!pblocktemplate.get())
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Couldn't create new block");
        CBlock *pblock = &pblocktemplate->block;
        std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(*pblock);
        if (!ProcessNewBlock(Params(), shared_pblock, true, nullptr))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "ProcessNewBlock, block not accepted");
        ++nHeight;
        blockHashes.push_back(pblock->GetHash().GetHex());
    }
    return blockHashes;
}

static UniValue getmininginfo(const JSONRPCRequest& request)
{
            RPCHelpMan{"getmininginfo",
                "\nReturns a json object containing mining-related information.",
                {},
                RPCResult{
                    "{\n"
                    "  \"blocks\": nnn,             (numeric) The current block\n"
                    "  \"currentblockweight\": nnn, (numeric, optional) The block weight of the last assembled block (only present if a block was ever assembled)\n"
                    "  \"currentblocktx\": nnn,     (numeric, optional) The number of block transactions of the last assembled block (only present if a block was ever assembled)\n"
                    "  \"difficulty\": xxx.xxxxx    (numeric) The current difficulty\n"
                    "  \"pooledtx\": n              (numeric) The size of the mempool\n"
                    "  \"basetarget\" : xxx,        (numeric) The current basetarget\n"
                    "  \"netcapacity\": nnn         (string) The net capacity\n"
                    "  \"smoothbeginheight\": nnn   (numeric) The smooth adjust ratio begin height\n"
                    "  \"smoothendheight\": nnn     (numeric) The smooth adjust ratio end height\n"
                    "  \"stagebeginheight\": nnn    (numeric) The stage adjust ratio begin height\n"
                    "  \"stagecapacity\": nnn       (numeric) The capacity of stage\n"
                    "  \"chain\": \"xxxx\",           (string) current network name as defined in BIP70 (main, test, regtest)\n"
                    "  \"warnings\": \"...\"          (string) any network and blockchain warnings\n"
                    "}\n"
                },
                RPCExamples{
                    HelpExampleCli("getmininginfo", "")
            + HelpExampleRpc("getmininginfo", "")
                },
            }.Check(request);

    LOCK(cs_main);

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("blocks",           (int)::ChainActive().Height());
    if (BlockAssembler::m_last_block_weight) obj.pushKV("currentblockweight", *BlockAssembler::m_last_block_weight);
    if (BlockAssembler::m_last_block_num_txs) obj.pushKV("currentblocktx", *BlockAssembler::m_last_block_num_txs);
    obj.pushKV("difficulty",       (double)GetDifficulty(::ChainActive().Tip()));
    obj.pushKV("pooledtx",         (uint64_t)mempool.size());
    obj.pushKV("basetarget",       (uint64_t)::ChainActive().Tip()->nBaseTarget);
    obj.pushKV("netcapacity",      ValueFromCapacity(std::max(poc::INITIAL_BASE_TARGET / ::ChainActive().Tip()->nBaseTarget, (uint64_t) 1)));
    obj.pushKV("chain",            Params().NetworkIDString());
    obj.pushKV("warnings",         GetWarnings("statusbar"));
    return obj;
}


// NOTE: Unlike wallet RPC (which use BTC values), mining RPCs follow GBT (BIP 22) in using satoshi amounts
static UniValue prioritisetransaction(const JSONRPCRequest& request)
{
            RPCHelpMan{"prioritisetransaction",
                "Accepts the transaction into mined blocks at a higher (or lower) priority\n",
                {
                    {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id."},
                    {"dummy", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "API-Compatibility for previous API. Must be zero or null.\n"
            "                  DEPRECATED. For forward compatibility use named arguments and omit this parameter."},
                    {"fee_delta", RPCArg::Type::NUM, RPCArg::Optional::NO, "The fee value (in satoshis) to add (or subtract, if negative).\n"
            "                  Note, that this value is not a fee rate. It is a value to modify absolute fee of the TX.\n"
            "                  The fee is not actually paid, only the algorithm for selecting transactions into a block\n"
            "                  considers the transaction as it would have paid a higher (or lower) fee."},
                },
                RPCResult{
            "true              (boolean) Returns true\n"
                },
                RPCExamples{
                    HelpExampleCli("prioritisetransaction", "\"txid\" 0.0 10000")
            + HelpExampleRpc("prioritisetransaction", "\"txid\", 0.0, 10000")
                },
            }.Check(request);

    LOCK(cs_main);

    uint256 hash(ParseHashV(request.params[0], "txid"));
    CAmount nAmount = request.params[2].get_int64();

    if (!(request.params[1].isNull() || request.params[1].get_real() == 0)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Priority is no longer supported, dummy argument to prioritisetransaction must be 0.");
    }

    mempool.PrioritiseTransaction(hash, nAmount);
    return true;
}


// NOTE: Assumes a conclusive result; if result is inconclusive, it must be handled by caller
static UniValue BIP22ValidationResult(const CValidationState& state)
{
    if (state.IsValid())
        return NullUniValue;

    if (state.IsError())
        throw JSONRPCError(RPC_VERIFY_ERROR, FormatStateMessage(state));
    if (state.IsInvalid())
    {
        std::string strRejectReason = state.GetRejectReason();
        if (strRejectReason.empty())
            return "rejected";
        return strRejectReason;
    }
    // Should be impossible
    return "valid?";
}

static std::string gbt_vb_name(const Consensus::DeploymentPos pos) {
    const struct VBDeploymentInfo& vbinfo = VersionBitsDeploymentInfo[pos];
    std::string s = vbinfo.name;
    if (!vbinfo.gbt_force) {
        s.insert(s.begin(), '!');
    }
    return s;
}

static UniValue getblocktemplate(const JSONRPCRequest& request)
{
            RPCHelpMan{"getblocktemplate",
                "\nIf the request parameters include a 'mode' key, that is used to explicitly select between the default 'template' request or a 'proposal'.\n"
                "It returns data needed to construct a block to work on.\n"
                "For full specification, see BIPs 22, 23, 9, and 145:\n"
                "    https://github.com/bitcoin/bips/blob/master/bip-0022.mediawiki\n"
                "    https://github.com/bitcoin/bips/blob/master/bip-0023.mediawiki\n"
                "    https://github.com/bitcoin/bips/blob/master/bip-0009.mediawiki#getblocktemplate_changes\n"
                "    https://github.com/bitcoin/bips/blob/master/bip-0145.mediawiki\n",
                {
                    {"template_request", RPCArg::Type::OBJ, "{}", "A json object in the following spec",
                        {
                            {"mode", RPCArg::Type::STR, /* treat as named arg */ RPCArg::Optional::OMITTED_NAMED_ARG, "This must be set to \"template\", \"proposal\" (see BIP 23), or omitted"},
                            {"capabilities", RPCArg::Type::ARR, /* treat as named arg */ RPCArg::Optional::OMITTED_NAMED_ARG, "A list of strings",
                                {
                                    {"support", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "client side supported feature, 'longpoll', 'coinbasetxn', 'coinbasevalue', 'proposal', 'serverlist', 'workid'"},
                                },
                                },
                            {"rules", RPCArg::Type::ARR, RPCArg::Optional::NO, "A list of strings",
                                {
                                    {"support", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "client side supported softfork deployment"},
                                },
                                },
                        },
                        "\"template_request\""},
                },
                RPCResult{
            "{\n"
            "  \"version\" : n,                    (numeric) The preferred block version\n"
            "  \"rules\" : [ \"rulename\", ... ],    (array of strings) specific block rules that are to be enforced\n"
            "  \"vbavailable\" : {                 (json object) set of pending, supported versionbit (BIP 9) softfork deployments\n"
            "      \"rulename\" : bitnumber          (numeric) identifies the bit number as indicating acceptance and readiness for the named softfork rule\n"
            "      ,...\n"
            "  },\n"
            "  \"vbrequired\" : n,                 (numeric) bit mask of versionbits the server requires set in submissions\n"
            "  \"previousblockhash\" : \"xxxx\",     (string) The hash of current highest block\n"
            "  \"transactions\" : [                (array) contents of non-coinbase transactions that should be included in the next block\n"
            "      {\n"
            "         \"data\" : \"xxxx\",             (string) transaction data encoded in hexadecimal (byte-for-byte)\n"
            "         \"txid\" : \"xxxx\",             (string) transaction id encoded in little-endian hexadecimal\n"
            "         \"hash\" : \"xxxx\",             (string) hash encoded in little-endian hexadecimal (including witness data)\n"
            "         \"depends\" : [                (array) array of numbers \n"
            "             n                          (numeric) transactions before this one (by 1-based index in 'transactions' list) that must be present in the final block if this one is\n"
            "             ,...\n"
            "         ],\n"
            "         \"fee\": n,                    (numeric) difference in value between transaction inputs and outputs (in satoshis); for coinbase transactions, this is a negative Number of the total collected block fees (ie, not including the block subsidy); if key is not present, fee is unknown and clients MUST NOT assume there isn't one\n"
            "         \"sigops\" : n,                (numeric) total SigOps cost, as counted for purposes of block limits; if key is not present, sigop cost is unknown and clients MUST NOT assume it is zero\n"
            "         \"weight\" : n,                (numeric) total transaction weight, as counted for purposes of block limits\n"
            "      }\n"
            "      ,...\n"
            "  ],\n"
            "  \"coinbaseaux\" : {                 (json object) data that should be included in the coinbase's scriptSig content\n"
            "      \"flags\" : \"xx\"                  (string) key name is to be ignored, and value included in scriptSig\n"
            "  },\n"
            "  \"coinbasevalue\" : n,              (numeric) maximum allowable input to coinbase transaction, including the generation award and transaction fees (in satoshis)\n"
            "  \"coinbasetxn\" : { ... },          (json object) information for coinbase transaction\n"
            "  \"target\" : \"xxxx\",                (string) The hash target\n"
            "  \"mintime\" : xxx,                  (numeric) The minimum timestamp appropriate for next block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"mutable\" : [                     (array of string) list of ways the block template may be changed \n"
            "     \"value\"                          (string) A way the block template may be changed, e.g. 'time', 'transactions', 'prevblock'\n"
            "     ,...\n"
            "  ],\n"
            "  \"sigoplimit\" : n,                 (numeric) limit of sigops in blocks\n"
            "  \"sizelimit\" : n,                  (numeric) limit of block size\n"
            "  \"weightlimit\" : n,                (numeric) limit of block weight\n"
            "  \"curtime\" : ttt,                  (numeric) current timestamp in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"basetarget\" : xxx,               (numeric) current basetarget\n"
            "  \"height\" : n                      (numeric) The height of the next block\n"
            "}\n"
                },
                RPCExamples{
                    HelpExampleCli("getblocktemplate", "'{\"rules\": [\"segwit\"]}'")
            + HelpExampleRpc("getblocktemplate", "{\"rules\": [\"segwit\"]}")
                },
            }.Check(request);

    LOCK(cs_main);

    std::string strMode = "template";
    UniValue lpval = NullUniValue;
    std::set<std::string> setClientRules;
    int64_t nMaxVersionPreVB = -1;
    if (!request.params[0].isNull())
    {
        const UniValue& oparam = request.params[0].get_obj();
        const UniValue& modeval = find_value(oparam, "mode");
        if (modeval.isStr())
            strMode = modeval.get_str();
        else if (modeval.isNull())
        {
            /* Do nothing */
        }
        else
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");
        lpval = find_value(oparam, "longpollid");

        if (strMode == "proposal")
        {
            const UniValue& dataval = find_value(oparam, "data");
            if (!dataval.isStr())
                throw JSONRPCError(RPC_TYPE_ERROR, "Missing data String key for proposal");

            CBlock block;
            if (!DecodeHexBlk(block, dataval.get_str()))
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block decode failed");

            uint256 hash = block.GetHash();
            const CBlockIndex* pindex = LookupBlockIndex(hash);
            if (pindex) {
                if (pindex->IsValid(BLOCK_VALID_SCRIPTS))
                    return "duplicate";
                if (pindex->nStatus & BLOCK_FAILED_MASK)
                    return "duplicate-invalid";
                return "duplicate-inconclusive";
            }

            CBlockIndex* const pindexPrev = ::ChainActive().Tip();
            // TestBlockValidity only supports blocks built on the current Tip
            if (block.hashPrevBlock != pindexPrev->GetBlockHash())
                return "inconclusive-not-best-prevblk";
            CValidationState state;
            TestBlockValidity(state, Params(), block, pindexPrev, false, true);
            return BIP22ValidationResult(state);
        }

        const UniValue& aClientRules = find_value(oparam, "rules");
        if (aClientRules.isArray()) {
            for (unsigned int i = 0; i < aClientRules.size(); ++i) {
                const UniValue& v = aClientRules[i];
                setClientRules.insert(v.get_str());
            }
        } else {
            // NOTE: It is important that this NOT be read if versionbits is supported
            const UniValue& uvMaxVersion = find_value(oparam, "maxversion");
            if (uvMaxVersion.isNum()) {
                nMaxVersionPreVB = uvMaxVersion.get_int64();
            }
        }
    }

    if (strMode != "template")
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");

    if(!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    if (g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL) == 0)
        throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, PACKAGE_NAME " is not connected!");

    if (::ChainstateActive().IsInitialBlockDownload())
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, PACKAGE_NAME " is in initial sync and waiting for blocks...");

    static unsigned int nTransactionsUpdatedLast;

    if (!lpval.isNull())
    {
        // Wait to respond until either the best block changes, OR a minute has passed and there are more transactions
        uint256 hashWatchedChain;
        std::chrono::steady_clock::time_point checktxtime;
        unsigned int nTransactionsUpdatedLastLP;

        if (lpval.isStr())
        {
            // Format: <hashBestChain><nTransactionsUpdatedLast>
            std::string lpstr = lpval.get_str();

            hashWatchedChain = ParseHashV(lpstr.substr(0, 64), "longpollid");
            nTransactionsUpdatedLastLP = atoi64(lpstr.substr(64));
        }
        else
        {
            // NOTE: Spec does not specify behaviour for non-string longpollid, but this makes testing easier
            hashWatchedChain = ::ChainActive().Tip()->GetBlockHash();
            nTransactionsUpdatedLastLP = nTransactionsUpdatedLast;
        }

        // Release lock while waiting
        LEAVE_CRITICAL_SECTION(cs_main);
        {
            checktxtime = std::chrono::steady_clock::now() + std::chrono::minutes(1);

            WAIT_LOCK(g_best_block_mutex, lock);
            while (g_best_block == hashWatchedChain && IsRPCRunning())
            {
                if (g_best_block_cv.wait_until(lock, checktxtime) == std::cv_status::timeout)
                {
                    // Timeout: Check transactions for update
                    // without holding ::mempool.cs to avoid deadlocks
                    if (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLastLP)
                        break;
                    checktxtime += std::chrono::seconds(10);
                }
            }
        }
        ENTER_CRITICAL_SECTION(cs_main);

        if (!IsRPCRunning())
            throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Shutting down");
        // TODO: Maybe recheck connections/IBD and (if something wrong) send an expires-immediately template to stop miners?
    }

    // GBT must be called with 'segwit' set in the rules
    if (setClientRules.count("segwit") != 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "getblocktemplate must be called with the segwit rule set (call with {\"rules\": [\"segwit\"]})");
    }

    // Update block
    static CBlockIndex* pindexPrev;
    static int64_t nStart;
    static std::unique_ptr<CBlockTemplate> pblocktemplate;
    if (pindexPrev != ::ChainActive().Tip() ||
        (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 5))
    {
        // Clear pindexPrev so future calls make a new block, despite any failures from here on
        pindexPrev = nullptr;

        // Store the pindexBest used before CreateNewBlock, to avoid races
        nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
        CBlockIndex* pindexPrevNew = ::ChainActive().Tip();
        nStart = GetTime();

        // Create new block
        CScript scriptDummy = CScript() << OP_TRUE;
        pblocktemplate = BlockAssembler(Params()).CreateNewBlock(scriptDummy);
        if (!pblocktemplate)
            throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");

        // Need to update only after we know CreateNewBlock succeeded
        pindexPrev = pindexPrevNew;
    }
    assert(pindexPrev);
    CBlock* pblock = &pblocktemplate->block; // pointer for convenience
    const Consensus::Params& consensusParams = Params().GetConsensus();

    // NOTE: If at some point we support pre-segwit miners post-segwit-activation, this needs to take segwit support into consideration
    const bool fPreSegWit = (pindexPrev->nHeight + 1 < consensusParams.SegwitHeight);

    UniValue aCaps(UniValue::VARR); aCaps.push_back("proposal");

    UniValue transactions(UniValue::VARR);
    std::map<uint256, int64_t> setTxIndex;
    int i = 0;
    for (const auto& it : pblock->vtx) {
        const CTransaction& tx = *it;
        uint256 txHash = tx.GetHash();
        setTxIndex[txHash] = i++;

        if (tx.IsCoinBase())
            continue;

        UniValue entry(UniValue::VOBJ);

        entry.pushKV("data", EncodeHexTx(tx));
        entry.pushKV("txid", txHash.GetHex());
        entry.pushKV("hash", tx.GetWitnessHash().GetHex());

        UniValue deps(UniValue::VARR);
        for (const CTxIn &in : tx.vin)
        {
            if (setTxIndex.count(in.prevout.hash))
                deps.push_back(setTxIndex[in.prevout.hash]);
        }
        entry.pushKV("depends", deps);

        int index_in_template = i - 1;
        entry.pushKV("fee", pblocktemplate->vTxFees[index_in_template]);
        int64_t nTxSigOps = pblocktemplate->vTxSigOpsCost[index_in_template];
        if (fPreSegWit) {
            assert(nTxSigOps % WITNESS_SCALE_FACTOR == 0);
            nTxSigOps /= WITNESS_SCALE_FACTOR;
        }
        entry.pushKV("sigops", nTxSigOps);
        entry.pushKV("weight", GetTransactionWeight(tx));

        transactions.push_back(entry);
    }

    UniValue aux(UniValue::VOBJ);
    aux.pushKV("flags", HexStr(COINBASE_FLAGS.begin(), COINBASE_FLAGS.end()));

    arith_uint256 hashTarget = arith_uint256().SetCompact(pblock->nBaseTarget);

    UniValue aMutable(UniValue::VARR);
    aMutable.push_back("time");
    aMutable.push_back("transactions");
    aMutable.push_back("prevblock");

    UniValue result(UniValue::VOBJ);
    result.pushKV("capabilities", aCaps);

    UniValue aRules(UniValue::VARR);
    UniValue vbavailable(UniValue::VOBJ);
    for (int j = 0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
        Consensus::DeploymentPos pos = Consensus::DeploymentPos(j);
        ThresholdState state = VersionBitsState(pindexPrev, consensusParams, pos, versionbitscache);
        switch (state) {
            case ThresholdState::DEFINED:
            case ThresholdState::FAILED:
                // Not exposed to GBT at all
                break;
            case ThresholdState::LOCKED_IN:
                // Ensure bit is set in block version
                pblock->nVersion |= VersionBitsMask(consensusParams, pos);
                // FALL THROUGH to get vbavailable set...
            case ThresholdState::STARTED:
            {
                const struct VBDeploymentInfo& vbinfo = VersionBitsDeploymentInfo[pos];
                vbavailable.pushKV(gbt_vb_name(pos), consensusParams.vDeployments[pos].bit);
                if (setClientRules.find(vbinfo.name) == setClientRules.end()) {
                    if (!vbinfo.gbt_force) {
                        // If the client doesn't support this, don't indicate it in the [default] version
                        pblock->nVersion &= ~VersionBitsMask(consensusParams, pos);
                    }
                }
                break;
            }
            case ThresholdState::ACTIVE:
            {
                // Add to rules only
                const struct VBDeploymentInfo& vbinfo = VersionBitsDeploymentInfo[pos];
                aRules.push_back(gbt_vb_name(pos));
                if (setClientRules.find(vbinfo.name) == setClientRules.end()) {
                    // Not supported by the client; make sure it's safe to proceed
                    if (!vbinfo.gbt_force) {
                        // If we do anything other than throw an exception here, be sure version/force isn't sent to old clients
                        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Support for '%s' rule requires explicit client support", vbinfo.name));
                    }
                }
                break;
            }
        }
    }
    result.pushKV("version", pblock->nVersion);
    result.pushKV("rules", aRules);
    result.pushKV("vbavailable", vbavailable);
    result.pushKV("vbrequired", int(0));

    if (nMaxVersionPreVB >= 2) {
        // If VB is supported by the client, nMaxVersionPreVB is -1, so we won't get here
        // Because BIP 34 changed how the generation transaction is serialized, we can only use version/force back to v2 blocks
        // This is safe to do [otherwise-]unconditionally only because we are throwing an exception above if a non-force deployment gets activated
        // Note that this can probably also be removed entirely after the first BIP9 non-force deployment (ie, probably segwit) gets activated
        aMutable.push_back("version/force");
    }

    result.pushKV("previousblockhash", pblock->hashPrevBlock.GetHex());
    result.pushKV("transactions", transactions);
    result.pushKV("coinbaseaux", aux);
    result.pushKV("coinbasevalue", (int64_t)pblock->vtx[0]->vout[0].nValue);
    result.pushKV("longpollid", ::ChainActive().Tip()->GetBlockHash().GetHex() + i64tostr(nTransactionsUpdatedLast));
    result.pushKV("target", hashTarget.GetHex());
    result.pushKV("mintime", (int64_t)pindexPrev->GetMedianTimePast()+1);
    result.pushKV("mutable", aMutable);
    int64_t nSigOpLimit = MAX_BLOCK_SIGOPS_COST;
    int64_t nSizeLimit = MAX_BLOCK_SERIALIZED_SIZE;
    if (fPreSegWit) {
        assert(nSigOpLimit % WITNESS_SCALE_FACTOR == 0);
        nSigOpLimit /= WITNESS_SCALE_FACTOR;
        assert(nSizeLimit % WITNESS_SCALE_FACTOR == 0);
        nSizeLimit /= WITNESS_SCALE_FACTOR;
    }
    result.pushKV("sigoplimit", nSigOpLimit);
    result.pushKV("sizelimit", nSizeLimit);
    if (!fPreSegWit) {
        result.pushKV("weightlimit", (int64_t)MAX_BLOCK_WEIGHT);
    }
    result.pushKV("curtime", pblock->GetBlockTime());
    result.pushKV("basetarget", pblock->nBaseTarget);
    result.pushKV("height", (int64_t)(pindexPrev->nHeight+1));

    if (!pblocktemplate->vchCoinbaseCommitment.empty()) {
        result.pushKV("default_witness_commitment", HexStr(pblocktemplate->vchCoinbaseCommitment.begin(), pblocktemplate->vchCoinbaseCommitment.end()));
    }

    return result;
}

class submitblock_StateCatcher : public CValidationInterface
{
public:
    uint256 hash;
    bool found;
    CValidationState state;

    explicit submitblock_StateCatcher(const uint256 &hashIn) : hash(hashIn), found(false), state() {}

protected:
    void BlockChecked(const CBlock& block, const CValidationState& stateIn) override {
        if (block.GetHash() != hash)
            return;
        found = true;
        state = stateIn;
    }
};

static UniValue submitblock(const JSONRPCRequest& request)
{
    // We allow 2 arguments for compliance with BIP22. Argument 2 is ignored.
            RPCHelpMan{"submitblock",
                "\nAttempts to submit new block to network.\n"
                "See https://en.bitcoin.it/wiki/BIP_0022 for full specification.\n",
                {
                    {"hexdata", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "the hex-encoded block data to submit"},
                    {"dummy", RPCArg::Type::STR, /* default */ "ignored", "dummy value, for compatibility with BIP22. This value is ignored."},
                },
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("submitblock", "\"mydata\"")
            + HelpExampleRpc("submitblock", "\"mydata\"")
                },
            }.Check(request);

    std::shared_ptr<CBlock> blockptr = std::make_shared<CBlock>();
    CBlock& block = *blockptr;
    if (!DecodeHexBlk(block, request.params[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block decode failed");
    }

    if (block.vtx.empty() || !block.vtx[0]->IsCoinBase()) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block does not start with a coinbase");
    }

    uint256 hash = block.GetHash();
    {
        LOCK(cs_main);
        const CBlockIndex* pindex = LookupBlockIndex(hash);
        if (pindex) {
            if (pindex->IsValid(BLOCK_VALID_SCRIPTS)) {
                return "duplicate";
            }
            if (pindex->nStatus & BLOCK_FAILED_MASK) {
                return "duplicate-invalid";
            }
        }
    }

    {
        LOCK(cs_main);
        const CBlockIndex* pindex = LookupBlockIndex(block.hashPrevBlock);
        if (pindex) {
            UpdateUncommittedBlockStructures(block, pindex, Params().GetConsensus());
        }
    }

    bool new_block;
    submitblock_StateCatcher sc(block.GetHash());
    RegisterValidationInterface(&sc);
    bool accepted = ProcessNewBlock(Params(), blockptr, /* fForceProcessing */ true, /* fNewBlock */ &new_block);
    UnregisterValidationInterface(&sc);
    if (!new_block && accepted) {
        return "duplicate";
    }
    if (!sc.found) {
        return "inconclusive";
    }
    return BIP22ValidationResult(sc.state);
}

static UniValue submitheader(const JSONRPCRequest& request)
{
            RPCHelpMan{"submitheader",
                "\nDecode the given hexdata as a header and submit it as a candidate chain tip if valid."
                "\nThrows when the header is invalid.\n",
                {
                    {"hexdata", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "the hex-encoded block header data"},
                },
                RPCResult{
            "None"
                },
                RPCExamples{
                    HelpExampleCli("submitheader", "\"aabbcc\"") +
                    HelpExampleRpc("submitheader", "\"aabbcc\"")
                },
            }.Check(request);

    CBlockHeader h;
    if (!DecodeHexBlockHeader(h, request.params[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block header decode failed");
    }
    {
        LOCK(cs_main);
        if (!LookupBlockIndex(h.hashPrevBlock)) {
            throw JSONRPCError(RPC_VERIFY_ERROR, "Must submit previous header (" + h.hashPrevBlock.GetHex() + ") first");
        }
    }

    CValidationState state;
    ProcessNewBlockHeaders({h}, state, Params(), /* ppindex */ nullptr, /* first_invalid */ nullptr);
    if (state.IsValid()) return NullUniValue;
    if (state.IsError()) {
        throw JSONRPCError(RPC_VERIFY_ERROR, FormatStateMessage(state));
    }
    throw JSONRPCError(RPC_VERIFY_ERROR, state.GetRejectReason());
}

static UniValue estimatesmartfee(const JSONRPCRequest& request)
{
            RPCHelpMan{"estimatesmartfee",
                "\nEstimates the approximate fee per kilobyte needed for a transaction to begin\n"
                "confirmation within conf_target blocks if possible and return the number of blocks\n"
                "for which the estimate is valid. Uses virtual transaction size as defined\n"
                "in BIP 141 (witness data is discounted).\n",
                {
                    {"conf_target", RPCArg::Type::NUM, RPCArg::Optional::NO, "Confirmation target in blocks (1 - 1008)"},
                    {"estimate_mode", RPCArg::Type::STR, /* default */ "CONSERVATIVE", "The fee estimate mode.\n"
            "                   Whether to return a more conservative estimate which also satisfies\n"
            "                   a longer history. A conservative estimate potentially returns a\n"
            "                   higher feerate and is more likely to be sufficient for the desired\n"
            "                   target, but is not as responsive to short term drops in the\n"
            "                   prevailing fee market.  Must be one of:\n"
            "       \"UNSET\"\n"
            "       \"ECONOMICAL\"\n"
            "       \"CONSERVATIVE\""},
                },
                RPCResult{
            "{\n"
            "  \"feerate\" : x.x,     (numeric, optional) estimate fee rate in " + CURRENCY_UNIT + "/kB\n"
            "  \"errors\": [ str... ] (json array of strings, optional) Errors encountered during processing\n"
            "  \"blocks\" : n         (numeric) block number where estimate was found\n"
            "}\n"
            "\n"
            "The request target will be clamped between 2 and the highest target\n"
            "fee estimation is able to return based on how long it has been running.\n"
            "An error is returned if not enough transactions and blocks\n"
            "have been observed to make an estimate for any number of blocks.\n"
                },
                RPCExamples{
                    HelpExampleCli("estimatesmartfee", "6")
                },
            }.Check(request);

    RPCTypeCheck(request.params, {UniValue::VNUM, UniValue::VSTR});
    RPCTypeCheckArgument(request.params[0], UniValue::VNUM);
    unsigned int max_target = ::feeEstimator.HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE);
    unsigned int conf_target = ParseConfirmTarget(request.params[0], max_target);
    bool conservative = true;
    if (!request.params[1].isNull()) {
        FeeEstimateMode fee_mode;
        if (!FeeModeFromString(request.params[1].get_str(), fee_mode)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid estimate_mode parameter");
        }
        if (fee_mode == FeeEstimateMode::ECONOMICAL) conservative = false;
    }

    UniValue result(UniValue::VOBJ);
    UniValue errors(UniValue::VARR);
    FeeCalculation feeCalc;
    CFeeRate feeRate = ::feeEstimator.estimateSmartFee(conf_target, &feeCalc, conservative);
    if (feeRate != CFeeRate(0)) {
        result.pushKV("feerate", ValueFromAmount(feeRate.GetFeePerK()));
    } else {
        errors.push_back("Insufficient data or no feerate found");
        result.pushKV("errors", errors);
    }
    result.pushKV("blocks", feeCalc.returnedTarget);
    return result;
}

static UniValue estimaterawfee(const JSONRPCRequest& request)
{
            RPCHelpMan{"estimaterawfee",
                "\nWARNING: This interface is unstable and may disappear or change!\n"
                "\nWARNING: This is an advanced API call that is tightly coupled to the specific\n"
                "         implementation of fee estimation. The parameters it can be called with\n"
                "         and the results it returns will change if the internal implementation changes.\n"
                "\nEstimates the approximate fee per kilobyte needed for a transaction to begin\n"
                "confirmation within conf_target blocks if possible. Uses virtual transaction size as\n"
                "defined in BIP 141 (witness data is discounted).\n",
                {
                    {"conf_target", RPCArg::Type::NUM, RPCArg::Optional::NO, "Confirmation target in blocks (1 - 1008)"},
                    {"threshold", RPCArg::Type::NUM, /* default */ "0.95", "The proportion of transactions in a given feerate range that must have been\n"
            "               confirmed within conf_target in order to consider those feerates as high enough and proceed to check\n"
            "               lower buckets."},
                },
                RPCResult{
            "{\n"
            "  \"short\" : {            (json object, optional) estimate for short time horizon\n"
            "      \"feerate\" : x.x,        (numeric, optional) estimate fee rate in " + CURRENCY_UNIT + "/kB\n"
            "      \"decay\" : x.x,          (numeric) exponential decay (per block) for historical moving average of confirmation data\n"
            "      \"scale\" : x,            (numeric) The resolution of confirmation targets at this time horizon\n"
            "      \"pass\" : {              (json object, optional) information about the lowest range of feerates to succeed in meeting the threshold\n"
            "          \"startrange\" : x.x,     (numeric) start of feerate range\n"
            "          \"endrange\" : x.x,       (numeric) end of feerate range\n"
            "          \"withintarget\" : x.x,   (numeric) number of txs over history horizon in the feerate range that were confirmed within target\n"
            "          \"totalconfirmed\" : x.x, (numeric) number of txs over history horizon in the feerate range that were confirmed at any point\n"
            "          \"inmempool\" : x.x,      (numeric) current number of txs in mempool in the feerate range unconfirmed for at least target blocks\n"
            "          \"leftmempool\" : x.x,    (numeric) number of txs over history horizon in the feerate range that left mempool unconfirmed after target\n"
            "      },\n"
            "      \"fail\" : { ... },       (json object, optional) information about the highest range of feerates to fail to meet the threshold\n"
            "      \"errors\":  [ str... ]   (json array of strings, optional) Errors encountered during processing\n"
            "  },\n"
            "  \"medium\" : { ... },    (json object, optional) estimate for medium time horizon\n"
            "  \"long\" : { ... }       (json object) estimate for long time horizon\n"
            "}\n"
            "\n"
            "Results are returned for any horizon which tracks blocks up to the confirmation target.\n"
                },
                RPCExamples{
                    HelpExampleCli("estimaterawfee", "6 0.9")
                },
            }.Check(request);

    RPCTypeCheck(request.params, {UniValue::VNUM, UniValue::VNUM}, true);
    RPCTypeCheckArgument(request.params[0], UniValue::VNUM);
    unsigned int max_target = ::feeEstimator.HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE);
    unsigned int conf_target = ParseConfirmTarget(request.params[0], max_target);
    double threshold = 0.95;
    if (!request.params[1].isNull()) {
        threshold = request.params[1].get_real();
    }
    if (threshold < 0 || threshold > 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid threshold");
    }

    UniValue result(UniValue::VOBJ);

    for (const FeeEstimateHorizon horizon : {FeeEstimateHorizon::SHORT_HALFLIFE, FeeEstimateHorizon::MED_HALFLIFE, FeeEstimateHorizon::LONG_HALFLIFE}) {
        CFeeRate feeRate;
        EstimationResult buckets;

        // Only output results for horizons which track the target
        if (conf_target > ::feeEstimator.HighestTargetTracked(horizon)) continue;

        feeRate = ::feeEstimator.estimateRawFee(conf_target, threshold, horizon, &buckets);
        UniValue horizon_result(UniValue::VOBJ);
        UniValue errors(UniValue::VARR);
        UniValue passbucket(UniValue::VOBJ);
        passbucket.pushKV("startrange", round(buckets.pass.start));
        passbucket.pushKV("endrange", round(buckets.pass.end));
        passbucket.pushKV("withintarget", round(buckets.pass.withinTarget * 100.0) / 100.0);
        passbucket.pushKV("totalconfirmed", round(buckets.pass.totalConfirmed * 100.0) / 100.0);
        passbucket.pushKV("inmempool", round(buckets.pass.inMempool * 100.0) / 100.0);
        passbucket.pushKV("leftmempool", round(buckets.pass.leftMempool * 100.0) / 100.0);
        UniValue failbucket(UniValue::VOBJ);
        failbucket.pushKV("startrange", round(buckets.fail.start));
        failbucket.pushKV("endrange", round(buckets.fail.end));
        failbucket.pushKV("withintarget", round(buckets.fail.withinTarget * 100.0) / 100.0);
        failbucket.pushKV("totalconfirmed", round(buckets.fail.totalConfirmed * 100.0) / 100.0);
        failbucket.pushKV("inmempool", round(buckets.fail.inMempool * 100.0) / 100.0);
        failbucket.pushKV("leftmempool", round(buckets.fail.leftMempool * 100.0) / 100.0);

        // CFeeRate(0) is used to indicate error as a return value from estimateRawFee
        if (feeRate != CFeeRate(0)) {
            horizon_result.pushKV("feerate", ValueFromAmount(feeRate.GetFeePerK()));
            horizon_result.pushKV("decay", buckets.decay);
            horizon_result.pushKV("scale", (int)buckets.scale);
            horizon_result.pushKV("pass", passbucket);
            // buckets.fail.start == -1 indicates that all buckets passed, there is no fail bucket to output
            if (buckets.fail.start != -1) horizon_result.pushKV("fail", failbucket);
        } else {
            // Output only information that is still meaningful in the event of error
            horizon_result.pushKV("decay", buckets.decay);
            horizon_result.pushKV("scale", (int)buckets.scale);
            horizon_result.pushKV("fail", failbucket);
            errors.push_back("Insufficient data or no feerate found which meets threshold");
            horizon_result.pushKV("errors",errors);
        }
        result.pushKV(StringForFeeEstimateHorizon(horizon), horizon_result);
    }
    return result;
}

#ifdef ENABLE_WALLET
// TODO move to rpcwalet.cpp
static UniValue generatetoaddress(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();


    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

            RPCHelpMan{"generatetoaddress",
                "\nMine up to nblocks blocks immediately (before the RPC call returns) to an address in the wallet.\n",
                {
                    {"nblocks", RPCArg::Type::NUM, RPCArg::Optional::NO, "How many blocks are generated immediately."},
                    {"address", RPCArg::Type::STR, /* default */ "", "The address to send the newly generated QTC to. Default use wallet primary address. Require address private key from wallet. "},
                },
                RPCResult{
            "[ blockhashes ]     (array) hashes of blocks generated\n"
                },
                RPCExamples{
            "\nGenerate 11 blocks to address\n"
            + HelpExampleCli("generatetoaddress", "11 \"address\"")
                },
            }.Check(request);

    int nGenerate = request.params[0].get_int();

    // address
    CTxDestination dest;
    if (!request.params[1].isNull()) {
        dest = DecodeDestination(request.params[1].get_str());
    } else {
        dest = pwallet->GetPrimaryDestination();
    }
    if (!boost::get<ScriptHash>(&dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Qitcoin address");
    }

    // private ekey
    std::shared_ptr<CKey> privateKey;
    auto keyid = GetKeyForDestination(*pwallet, dest);
    if (!keyid.IsNull()) {
        CKey key;
        if (!pwallet->GetKey(keyid, key)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + EncodeDestination(dest) + " is not known");
        }
        privateKey = std::make_shared<CKey>(key);
    }

    CScript coinbase_script = GetScriptForDestination(dest);
    return generateBlocks(coinbase_script, privateKey, nGenerate);
}
#endif

static UniValue generatetoprivkey(const JSONRPCRequest& request)
{
            RPCHelpMan{"generatetoprivkey",
                "\nMine blocks immediately to a specified private key P2WPKH address (before the RPC call returns)\n",
                {
                    {"nblocks", RPCArg::Type::NUM, RPCArg::Optional::NO, "How many blocks are generated immediately."},
                    {"privkey", RPCArg::Type::STR, RPCArg::Optional::NO, "The address (private key P2WPKH) to send the newly generated QTC to."},
                },
                RPCResult{
            "[ blockhashes ]     (array) hashes of blocks generated\n"
                },
                RPCExamples{
            "\nGenerate 11 blocks to myprivatekey\n"
            + HelpExampleCli("generatetoprivkey", "11 \"myprivatekey\"")
                },
            }.Check(request);

    int nGenerate = request.params[0].get_int();

    // privkey
    CKey key = DecodeSecret(request.params[1].get_str());
    if (!key.IsValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Qitcoin private key");
    }
    CKeyID keyid = key.GetPubKey().GetID();
    CTxDestination segwit = WitnessV0KeyHash(keyid);
    CTxDestination dest = ScriptHash(GetScriptForDestination(segwit));
    if (!boost::get<ScriptHash>(&dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Qitcoin address");
    }

    CScript coinbase_script = GetScriptForDestination(dest);
    return generateBlocks(coinbase_script, std::make_shared<CKey>(key), nGenerate);
}

static UniValue getactivebindplotteraddress(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getactivebindplotteraddress plotterId\n"
            "\nReturn active binded address of plotter ID.\n"
            "\nArguments:\n"
            "1. plotterId           (string, required) The plotter ID\n"
            "\nResult:\n"
            "\"address\"    (string) The active binded Qitcoin address\n"
            "\nExamples:\n"
            + HelpExampleCli("getactivebindplotteraddress", "\"12345678900000000000\"")
            + HelpExampleRpc("getactivebindplotteraddress", "\"12345678900000000000\"")
        );

    uint64_t plotterId = 0;
    if (!request.params[0].isStr() || !IsValidPlotterID(request.params[0].get_str(), &plotterId))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid plotter ID");

    LOCK(cs_main);
    const Coin &coin = ::ChainstateActive().CoinsTip().GetLastBindPlotterCoin(plotterId);
    if (!coin.IsSpent()) {
        return EncodeDestination(ExtractDestination(coin.out.scriptPubKey));
    }

    return UniValue();
}

static UniValue getactivebindplotter(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getactivebindplotter plotterId\n"
            "\nReturn active binded information of plotter ID.\n"
            "\nArguments:\n"
            "1. plotterId           (string, required) The plotter ID\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"address\":\"address\",           (string) The Qitcoin address of the binded.\n"
            "    \"txid\":\"txid\",                 (string) The last binded transaction id.\n"
            "    \"blockhash\":\"blockhash\",       (string) The binded transaction included block hash.\n"
            "    \"blocktime\": xxx,              (numeric) The block time in seconds since epoch (1 Jan 1970 GMT).\n"
            "    \"blockheight\":height,          (numeric) The binded transaction included block height.\n"
            "    \"bindheightlimit\":height,      (numeric) The plotter bind small fee limit height. Other require high fee.\n"
            "    \"unbindheightlimit\":height,    (numeric) The plotter unbind limit height.\n"
            "    \"lastBlock\": {                   (object) The plotter last generated block. Maybe not exist.\n"
            "        \"blockhash\":\"blockhash\",   (string) The plotter last generated block hash.\n"
            "        \"blocktime\": xxx,            (numeric) The block time in seconds since epoch (1 Jan 1970 GMT).\n"
            "        \"blockheight\":blockheight    (numeric) The plotter last generated block height.\n"
            "     }\n"
            "  }\n"
            "]\n"

            "\nExamples:\n"
            + HelpExampleCli("getactivebindplotter", "\"12345678900000000000\"")
            + HelpExampleRpc("getactivebindplotter", "\"12345678900000000000\"")
        );

    uint64_t plotterId = 0;
    if (!request.params[0].isStr() || !IsValidPlotterID(request.params[0].get_str(), &plotterId))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid plotter ID");

    LOCK(cs_main);

    const CBindPlotterInfo lastBindInfo = ::ChainstateActive().CoinsTip().GetLastBindPlotterInfo(plotterId);
    if (!lastBindInfo.outpoint.IsNull()) {
        const Coin &coin = ::ChainstateActive().CoinsTip().AccessCoin(lastBindInfo.outpoint);
        UniValue item(UniValue::VOBJ);
        item.pushKV("address", EncodeDestination(ExtractDestination(coin.out.scriptPubKey)));
        item.pushKV("txid", lastBindInfo.outpoint.hash.GetHex());
        item.pushKV("blockhash", ::ChainActive()[coin.nHeight]->GetBlockHash().GetHex());
        item.pushKV("blocktime", ::ChainActive()[coin.nHeight]->GetBlockTime());
        item.pushKV("blockheight", static_cast<int>(coin.nHeight));
        item.pushKV("bindheightlimit", Consensus::GetBindPlotterLimitHeight(::ChainActive().Height() + 1, lastBindInfo, Params().GetConsensus()));
        item.pushKV("unbindheightlimit", Consensus::GetUnbindPlotterLimitHeight(lastBindInfo, ::ChainstateActive().CoinsTip(), Params().GetConsensus()));

        // Last generate block
        for (const CBlockIndex& block : poc::GetEvalBlocks(::ChainActive().Height(), false, Params().GetConsensus())) {
            if (block.nPlotterId == plotterId) {
                UniValue lastBlock(UniValue::VOBJ);
                lastBlock.pushKV("blockhash", block.GetBlockHash().GetHex());
                lastBlock.pushKV("blocktime", block.GetBlockTime());
                lastBlock.pushKV("blockheight", block.nHeight);
                item.pushKV("lastBlock", lastBlock);
                break;
            }
        }
        return item;
    } else {
        return UniValue();
    }
}

static UniValue listbindplotterofaddress(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 4)
        throw std::runtime_error(
            "listbindplotterofaddress \"address\" (plotterId count verbose)\n"
            "\nReturns up to binded plotter of address.\n"
            "\nArguments:\n"
            "1. address             (string, required) The Qitcoin address\n"
            "2. plotterId           (string, optional) The filter plotter ID. If 0 or not set then output all binded plotter ID\n"
            "3. count               (numeric, optional) The result of count binded to list. If not set then output all binded plotter ID\n"
            "4. verbose             (bool, optional, default=false) If true, return bindheightlimit, unbindheightlimit and active\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"address\":\"address\",               (string) The Qitcoin address of the binded.\n"
            "    \"plotterId\": \"plotterId\",          (string) The binded plotter ID.\n"
            "    \"txid\": \"transactionid\",           (string) The transaction id.\n"
            "    \"blockhash\": \"hashvalue\",          (string) The block hash containing the transaction.\n"
            "    \"blocktime\": xxx,                  (numeric) The block time in seconds since epoch (1 Jan 1970 GMT).\n"
            "    \"blockheight\": xxx,                (numeric) The block height.\n"
            "    \"capacity\": \"xxx TB/PB\",           (string) The plotter capacity.\n"
            "    \"bindheightlimit\": xxx             (numeric) The plotter bind small fee limit height. Other require high fee. Only for verbose mode.\n"
            "    \"unbindheightlimit\": xxx,          (numeric) The plotter unbind limit height.Only for verbose mode.\n"
            "  }\n"
            "]\n"

            "\nExamples:\n"
            "\nList binded plotter of address\n"
            + HelpExampleCli("listbindplotterofaddress", std::string("\"") + Params().GetConsensus().FundAddress + "\" \"0\" 10")
            + HelpExampleRpc("listbindplotterofaddress", std::string("\"") + Params().GetConsensus().FundAddress + "\", \"0\" 10")
        );

    if (!request.params[0].isStr())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");
    const CAccountID accountID = ExtractAccountID(DecodeDestination(request.params[0].get_str()));
    if (accountID.IsNull())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid address");
    uint64_t plotterId = 0;
    if (request.params.size() >= 2) {
        if (!request.params[1].isStr() || (!request.params[1].get_str().empty() && !IsValidPlotterID(request.params[1].get_str(), &plotterId)))
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid plotter ID");
    }
    int count = std::numeric_limits<int>::max();
    if (request.params.size() >= 3)
        count = request.params[2].get_int();
    if (count < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid count");

    bool fVerbose = false;
    if (!request.params[3].isNull()) {
        fVerbose = request.params[3].isNum() ? (request.params[3].get_int() != 0) : request.params[3].get_bool();
    }

    UniValue ret(UniValue::VARR);
    if (count == 0)
        return ret;

    LOCK(cs_main);

    // Load all relation coins
    typedef std::map<uint32_t, CBindPlotterCoinsMap, std::greater<uint32_t> > CCoinsOrderByHeightMap;
    CCoinsOrderByHeightMap mapOrderedCoins;
    {
        for (auto pair : ::ChainstateActive().CoinsTip().GetAccountBindPlotterEntries(accountID, plotterId)) {
            CBindPlotterCoinsMap &mapCoins = mapOrderedCoins[pair.second.nHeight];
            mapCoins[pair.first] = std::move(pair.second);
        }
    }

    // Capacity
    uint64_t nNetCapacityTB = 0;
    int nBlockCount = 0;
    std::map<uint64_t, int> mapPlotterMiningCount;
    if (!mapOrderedCoins.empty()) {
        nNetCapacityTB = poc::GetNetCapacity(::ChainActive().Height(), Params().GetConsensus(),
            [&mapPlotterMiningCount, &nBlockCount](const CBlockIndex &block) {
                nBlockCount++;
                mapPlotterMiningCount[block.nPlotterId]++;
            }
        );
    }

    bool fContinue = true;
    for (CCoinsOrderByHeightMap::const_iterator itMapCoins = mapOrderedCoins.cbegin(); fContinue && itMapCoins != mapOrderedCoins.cend(); ++itMapCoins) {
        const CBindPlotterCoinsMap &mapCoins = itMapCoins->second;
        for (CBindPlotterCoinsMap::const_reverse_iterator it = mapCoins.rbegin(); fContinue && it != mapCoins.rend(); ++it) {
            UniValue item(UniValue::VOBJ);
            item.pushKV("address", EncodeDestination(ExtractDestination(::ChainstateActive().CoinsTip().AccessCoin(it->first).out.scriptPubKey)));
            item.pushKV("plotterId", std::to_string(it->second.plotterId));
            item.pushKV("txid", it->first.hash.GetHex());
            item.pushKV("blockhash", ::ChainActive()[static_cast<int>(it->second.nHeight)]->GetBlockHash().GetHex());
            item.pushKV("blocktime", ::ChainActive()[static_cast<int>(it->second.nHeight)]->GetBlockTime());
            item.pushKV("blockheight", it->second.nHeight);
            if (nBlockCount > 0) {
                item.pushKV("capacity", ValueFromCapacity((nNetCapacityTB * mapPlotterMiningCount[it->second.plotterId]) / nBlockCount));
            } else {
                item.pushKV("capacity", ValueFromCapacity(0));
            }
            if (fVerbose) {
                item.pushKV("bindheightlimit", GetBindPlotterLimitHeight(::ChainActive().Height() + 1, CBindPlotterInfo(*it), Params().GetConsensus()));
                item.pushKV("unbindheightlimit", GetUnbindPlotterLimitHeight(CBindPlotterInfo(*it), ::ChainstateActive().CoinsTip(), Params().GetConsensus()));
            }

            ret.push_back(item);

            if (--count <= 0)
                fContinue = false;
        }
    }

    return ret;
}

static UniValue createbindplotterdata(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 3)
        throw std::runtime_error(
            "createbindplotterdata \"address\" \"passphrase\" (lastActiveHeight)\n"
            "\nCreate bind plotter hex data.\n"
            "\nArguments:\n"
            "1. address             (string, required) The Qitcoin address\n"
            "2. passphrase          (string, required) The passphrase for bind\n"
            "3. lastActiveHeight    (numeric, optional) The last active height for bind data. Max large then tip 12 blocks\n"
            ""
            "\nResult: bind plotter hex data. See \"bindplotter\"\n"

            "\nExamples:\n"
            "\nReturn bind plotter hex data\n"
            + HelpExampleCli("createbindplotterdata", std::string("\"") + Params().GetConsensus().FundAddress + "\" \"root minute ancient won check dove second spot book thump retreat add\"")
            + HelpExampleRpc("createbindplotterdata", std::string("\"") + Params().GetConsensus().FundAddress + "\", \"root minute ancient won check dove second spot book thump retreat add\"")
        );

    if (!request.params[0].isStr())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");
    if (!request.params[1].isStr())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid passphrase");
    int lastActiveHeight = 0;
    if (request.params.size() >= 3)
        lastActiveHeight = request.params[2].get_int();

    int activeHeight;
    {
        LOCK(cs_main);
        activeHeight = std::max(::ChainActive().Height(), 2);
    }
    if (lastActiveHeight == 0)
        lastActiveHeight = activeHeight + PROTOCOL_BINDPLOTTER_DEFAULTMAXALIVE;
    if (lastActiveHeight > activeHeight + PROTOCOL_BINDPLOTTER_DEFAULTMAXALIVE)
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Last active height too large and unsafe (limit %d)", activeHeight + PROTOCOL_BINDPLOTTER_DEFAULTMAXALIVE));

    CScript script = GetBindPlotterScriptForDestination(DecodeDestination(request.params[0].get_str()), request.params[1].get_str(), lastActiveHeight);
    if (script.empty())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot generate bind script");

    return HexStr(script.begin(), script.end());
}

static UniValue decodebindplotterdata(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "decodebindplotterdata \"address\" \"hexdata\"\n"
            "\nDecode bind plotter hex data.\n"
            "\nArguments:\n"
            "1. hexdata             (string, required) The bind hex data\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"plotterId\":\"plotterId\",               (string) The binded plotter ID.\n"
            "    \"lastActiveHeight\":lastActiveHeight,   (numeric) The bind last active height for tx package.\n"
            "    \"pubkey\":\"publickeyhex\",               (string) The public key.\n"
            "    \"signature\":\"signaturehex\"             (string) The signature.\n"
            "  }\n"
            "]\n"

            "\nExamples:\n"
            "\nDecode bind plotter hex data\n"
            + HelpExampleCli("decodebindplotterdata", "\"6a041000000004670100002039dc2e813bb45ff063a376e316b10cd0addd7306555ca0dd2890194d3796015240a101125217d82d81779e3c047d8ca1c5ed92860d693ef216a384572d254cd20ff19945a60a7f3f0cdb935dc174d9acaaa93ce1b2b131d319ee7f43ff341bba9f\"")
            + HelpExampleRpc("decodebindplotterdata", "\"6a041000000004670100002039dc2e813bb45ff063a376e316b10cd0addd7306555ca0dd2890194d3796015240a101125217d82d81779e3c047d8ca1c5ed92860d693ef216a384572d254cd20ff19945a60a7f3f0cdb935dc174d9acaaa93ce1b2b131d319ee7f43ff341bba9f\"")
        );

    std::vector<unsigned char> bindData = ParseHex(request.params[0].get_str());

    uint64_t plotterId = 0;
    std::string pubkeyHex, signatureHex;
    int lastActiveHeight = 0;
    if (!DecodeBindPlotterScript(CScript(bindData.cbegin(), bindData.cend()), plotterId, pubkeyHex, signatureHex, lastActiveHeight))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid data");

    UniValue result(UniValue::VOBJ);
    result.pushKV("plotterId", std::to_string(plotterId));
    result.pushKV("lastActiveHeight", lastActiveHeight);
    result.pushKV("pubkey", pubkeyHex);
    result.pushKV("signature", signatureHex);
    return result;
}

static UniValue verifybindplotterdata(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        throw std::runtime_error(
            "verifybindplotterdata \"address\" \"hexdata\"\n"
            "\nVerify plotter hex data.\n"
            "\nArguments:\n"
            "1. address             (string, required) The Qitcoin address\n"
            "2. hexdata             (string, required) The bind hex data\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"result\":\"result\",                     (string) Verify result. 1.success; 2.reject: can't verify signature; 3.invalid: The data not bind plotter hex data\n"
            "    \"plotterId\":\"plotterId\",               (string) The binded plotter ID.\n"
            "    \"lastActiveHeight\":lastActiveHeight,   (numeric) The bind last active height for tx package.\n"
            "    \"address\":\"address\",                   (string) The Qitcoin address of the binded.\n"
            "  }\n"
            "]\n"

            "\nExamples:\n"
            "\nVerify bind plotter hex data\n"
            + HelpExampleCli("verifybindplotterdata", std::string("\"") + Params().GetConsensus().FundAddress + "\" \"6a041000000004670100002039dc2e813bb45ff063a376e316b10cd0addd7306555ca0dd2890194d3796015240a101125217d82d81779e3c047d8ca1c5ed92860d693ef216a384572d254cd20ff19945a60a7f3f0cdb935dc174d9acaaa93ce1b2b131d319ee7f43ff341bba9f\"")
            + HelpExampleRpc("verifybindplotterdata", std::string("\"") + Params().GetConsensus().FundAddress + "\", \"6a041000000004670100002039dc2e813bb45ff063a376e316b10cd0addd7306555ca0dd2890194d3796015240a101125217d82d81779e3c047d8ca1c5ed92860d693ef216a384572d254cd20ff19945a60a7f3f0cdb935dc174d9acaaa93ce1b2b131d319ee7f43ff341bba9f\"")
        );

    CTxDestination bindToDest = DecodeDestination(request.params[0].get_str());
    if (!IsValidDestination(bindToDest))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");

    std::vector<unsigned char> bindData = ParseHex(request.params[1].get_str());

    CMutableTransaction dummyTx;
    dummyTx.vin.push_back(CTxIn());
    dummyTx.vout.push_back(CTxOut(PROTOCOL_BINDPLOTTER_LOCKAMOUNT, GetScriptForDestination(bindToDest)));
    dummyTx.vout.push_back(CTxOut(0, CScript(bindData.cbegin(), bindData.cend())));

    UniValue result(UniValue::VOBJ);
    int nHeight = 0;
    {
        LOCK(cs_main);
        nHeight = std::max(::ChainActive().Height(), 2);
    }
    auto payload = ExtractTxoutPayload(dummyTx.vout[0], nHeight, {TXOUT_TYPE_BINDPLOTTER}, true);
    if (payload && payload->type == TXOUT_TYPE_BINDPLOTTER) {
        // Verify pass
        result.pushKV("result", "success");
        result.pushKV("plotterId", std::to_string(BindPlotterPayload::As(payload)->GetId()));
        result.pushKV("address", EncodeDestination(bindToDest));
    } else {
        // Not bind plotter hex data
        result.pushKV("result", "invalid");
    }

    return result;
}

static UniValue getbindplotterlimit(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getbindplotterlimit \"plotterId\"\n"
            "\nGet bind plotter limit height for plotter ID.\n"
            "\nArguments:\n"
            "1. plotterId           (string, required) The plotter ID.\n"
            "\nResult:\n"
            "Bind limit height\n"

            "\nExamples:\n"
            "\nGet bind plotter limit height for plotter ID\n"
            + HelpExampleCli("getbindplotterlimit", "\"1234567890\"")
            + HelpExampleRpc("getbindplotterlimit", "\"1234567890\"")
        );

    uint64_t plotterId = 0;
    if (!request.params[0].isStr() || !IsValidPlotterID(request.params[0].get_str(), &plotterId))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid plotter ID");

    LOCK(cs_main);
    const CBindPlotterInfo lastBindInfo = ::ChainstateActive().CoinsTip().GetLastBindPlotterInfo(plotterId);
    if (!lastBindInfo.outpoint.IsNull())
        return Consensus::GetBindPlotterLimitHeight(GetSpendHeight(::ChainstateActive().CoinsTip()), lastBindInfo, Params().GetConsensus());

    return UniValue();
}

static UniValue getunbindplotterlimit(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getunbindplotterlimit \"txid\"\n"
            "\nGet unbind plotter limit height from bind transaction.\n"
            "\nArguments:\n"
            "1. txid           (string, required) The bind plotter transaction ID.\n"
            "\nResult:\n"
            "Unbind limit height\n"

            "\nExamples:\n"
            "\nGet unbind plotter limit height from bind transaction\n"
            + HelpExampleCli("getunbindplotterlimit", "\"0000000000000000000000000000000000000000000000000000000000000000\"")
            + HelpExampleRpc("getunbindplotterlimit", "\"0000000000000000000000000000000000000000000000000000000000000000\"")
        );

    const uint256 txid = ParseHashV(request.params[0], "parameter 1");

    LOCK(cs_main);
    const COutPoint coinEntry(txid, 0);
    Coin coin;
    if (!::ChainstateActive().CoinsTip().GetCoin(coinEntry, coin))
        throw JSONRPCError(RPC_INVALID_PARAMS, "Not found valid bind transaction");

    if (!coin.IsBindPlotter())
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid bind transaction");

    return Consensus::GetUnbindPlotterLimitHeight(CBindPlotterInfo(coinEntry, coin), ::ChainstateActive().CoinsTip(), Params().GetConsensus());
}

static UniValue getpledgeofaddress(const std::string &address, uint64_t nPlotterId, bool fVerbose)
{
    LOCK(cs_main);
    const CAccountID accountID = ExtractAccountID(DecodeDestination(address));
    if (accountID.IsNull()) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address, must from Qitcoin wallet (P2SH address)");
    }

    CAmount balance = 0, balanceBindPlotter = 0, balancePoint[2] = {0, 0}, balanceStaking[2] = {0, 0};
    balance = ::ChainstateActive().CoinsTip().GetAccountBalance(accountID, &balanceBindPlotter, balancePoint, balanceStaking);

    UniValue result(UniValue::VOBJ);
    //! This balance belong to your
    result.pushKV("balance", ValueFromAmount(balance));
    //! This balance spendable
    result.pushKV("spendableBalance", ValueFromAmount(balance - balanceBindPlotter - balancePoint[0] - balanceStaking[0]));
    //! This balance locked in bind plotter and point
    result.pushKV("lockedBalance", ValueFromAmount(balanceBindPlotter + balancePoint[0] + balanceStaking[0]));
    //! This balance locked in point sent
    result.pushKV("pointSentBalance", ValueFromAmount(balancePoint[0]));
    //! This balance recevied from point received. For mining. YOUR CANNOT SPENT IT.
    result.pushKV("pointReceivedBalance", ValueFromAmount(balancePoint[1]));
    //! This balance locked in staking sent
    result.pushKV("stakingSentBalance", ValueFromAmount(balanceStaking[0]));
    //! This balance recevied from staking received. YOUR CANNOT SPENT IT.
    result.pushKV("stakingReceivedBalance", ValueFromAmount(balanceStaking[1]));

    const Consensus::Params &params = Params().GetConsensus();
    const int nHeight = ::ChainActive().Height();

    typedef struct {
        int minedCount;
        const CBlockIndex *pindexLast;
    } PlotterItem;
    std::map<uint64_t, PlotterItem> mapBindPlotter; // Plotter ID => PlotterItem

    int nBlockCount = 0, nMinedBlockCount = 0;
    int64_t nNetCapacityTB = 0, nCapacityTB = 0;
    std::set<uint64_t> plotters = ::ChainstateActive().CoinsTip().GetAccountBindPlotters(accountID);
    if (!plotters.empty()) {
        for (const uint64_t& plotterId : plotters) {
            mapBindPlotter[plotterId] = PlotterItem{0, nullptr};
        }
        nNetCapacityTB = poc::GetNetCapacity(::ChainActive().Height(), params,
            [&nBlockCount, &nMinedBlockCount, &plotters, &mapBindPlotter](const CBlockIndex &block) {
                nBlockCount++;
                if (plotters.count(block.nPlotterId)) {
                    nMinedBlockCount++;
                    
                    PlotterItem &item = mapBindPlotter[block.nPlotterId];
                    item.minedCount++;
                    item.pindexLast = &block;
                }
            }
        );
        if (nMinedBlockCount < nBlockCount)
            nMinedBlockCount++;
        if (nBlockCount > 0)
            nCapacityTB = std::max((int64_t) ((nNetCapacityTB * nMinedBlockCount) / nBlockCount), (int64_t) 1);
    }

    result.pushKV("capacity", ValueFromCapacity(nCapacityTB));
    result.pushKV("miningRequireBalance", ValueFromAmount(poc::GetCapacityRequireBalance(nCapacityTB, nHeight, params)));
    result.pushKV("height", nHeight);
    result.pushKV("address", address);

    // Bind plotter
    if (fVerbose) {
        UniValue objBindPlotters(UniValue::VOBJ);
        for (auto it = mapBindPlotter.cbegin(); it != mapBindPlotter.cend(); it++) {
            nCapacityTB = nBlockCount > 0 ? (int64_t) ((nNetCapacityTB * it->second.minedCount) / nBlockCount) : 0;

            UniValue item(UniValue::VOBJ);
            item.pushKV("capacity", ValueFromCapacity(nCapacityTB));
            item.pushKV("pledge", ValueFromAmount(poc::GetCapacityRequireBalance(nCapacityTB, nHeight, params)));
            if (it->second.pindexLast != nullptr) {
                UniValue lastBlock(UniValue::VOBJ);
                lastBlock.pushKV("blockhash", it->second.pindexLast->GetBlockHash().GetHex());
                lastBlock.pushKV("blockheight", it->second.pindexLast->nHeight);
                item.pushKV("lastBlock", lastBlock);
            }

            objBindPlotters.pushKV(std::to_string(it->first), item);
        }
        result.pushKV("plotters", objBindPlotters);
    }

    return result;
}

#ifdef ENABLE_WALLET
// TODO move to rpcwalet.cpp
static UniValue getpledge(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();

    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() > 2)
        throw std::runtime_error(
            "getpledge (plotterId)\n"
            "Get pledge amount of wallet.\n"
            "\nArguments:\n"
            "1. plotterId       (string, optional) Plotter ID\n"
            "2. verbose         (bool, optional, default=false) If true, return detail pledge\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"balance\": xxx,                     (numeric) All amounts belonging to this address\n"
            "    \"lockedBalance\": xxx,               (numeric) Unspendable amount. Freeze in bind plotter and point sent\n"
            "    \"spendableBalance\": xxx,            (numeric) Spendable amount. Include immarture and exclude locked amount\n"
            "    \"pointSentBalance\": xxx,            (numeric) Point sent amount\n"
            "    \"pointReceivedBalance\": xxx,        (numeric) Point received amount.For mining\n"
            "    \"stakingSentBalance\": xxx,          (numeric) Staking sent amount\n"
            "    \"stakingReceivedBalance\": xxx,      (numeric) Staking received amount\n"
            "    \"capacity\": \"xxx TB\",               (string) The address capacity. The unit of TB or PB\n"
            "    ...\n"
            "  }\n"
            "]\n"
            "\nExample:\n"
            + HelpExampleCli("getpledge", "\"0\" true")
            + HelpExampleRpc("getpledge", "\"0\", true")
        );

    CTxDestination dest = pwallet->GetPrimaryDestination();
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Inner error! Invalid primary address.");
    }

    uint64_t nPlotterId = 0;
    if (!request.params[0].isNull() && (!request.params[0].isStr() || !IsValidPlotterID(request.params[0].get_str(), &nPlotterId)))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid plotter ID");

    bool fVerbose = false;
    if (!request.params[1].isNull()) {
        fVerbose = request.params[1].isNum() ? (request.params[1].get_int() != 0) : request.params[1].get_bool();
    }

    return getpledgeofaddress(EncodeDestination(dest), nPlotterId, fVerbose);
}
#endif

static UniValue getpledgeofaddress(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 3)
        throw std::runtime_error(
            "getpledgeofaddress address (plotterId)\n"
            "Get pledge information of address.\n"
            "\nArguments:\n"
            "1. address         (string, required) The Qitcoin address.\n"
            "2. plotterId       (string, optional) DEPRECTED after QTCIP006. Plotter ID\n"
            "3. verbose         (bool, optional, default=false) If true, return detail pledge\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"balance\": xxx,                     (numeric) All amounts belonging to this address\n"
            "    \"lockedBalance\": xxx,               (numeric) Unspendable amount. Freeze in bind plotter and point sent\n"
            "    \"spendableBalance\": xxx,            (numeric) Spendable amount. Include immarture and exclude locked amount\n"
            "    \"pointSentBalance\": xxx,            (numeric) Point sent amount\n"
            "    \"pointReceivedBalance\": xxx,        (numeric) Point received amount.For mining\n"
            "    \"stakingSentBalance\": xxx,          (numeric) Staking sent amount\n"
            "    \"stakingReceivedBalance\": xxx,      (numeric) Staking received amount\n"
            "    \"capacity\": \"xxx TB\",               (string) The address capacity. The unit of TB or PB\n"
            "    ...\n"
            "  }\n"
            "]\n"
            "\nExample:\n"
            + HelpExampleCli("getpledgeofaddress", std::string("\"") + Params().GetConsensus().FundAddress + "\" \"0\" true")
            + HelpExampleRpc("getpledgeofaddress", std::string("\"") + Params().GetConsensus().FundAddress + "\", \"0\", true")
            );

    LOCK(cs_main);

    if (!request.params[0].isStr())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    uint64_t plotterId = 0;
    if (!request.params[1].isNull()) {
        if (!request.params[1].isStr() || (!request.params[1].get_str().empty() && !IsValidPlotterID(request.params[1].get_str(), &plotterId)))
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid plotter ID");
    }

    bool fVerbose = false;
    if (!request.params[2].isNull()) {
        fVerbose = request.params[2].isNum() ? (request.params[2].get_int() != 0) : request.params[2].get_bool();
    }

    return getpledgeofaddress(request.params[0].get_str(), plotterId, fVerbose);
}

static UniValue getplottermininginfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 3)
        throw std::runtime_error(
            "getplottermininginfo plotterId height\n"
            "Get mining information of plotter.\n"
            "\nArguments:\n"
            "1. plotterId       (string, required) Plotter\n"
            "2. verbose         (bool, optional, default=true) If true, return detail plotter mining information\n"
            "\nResult:\n"
            "The mining information of plotter\n"
            "\n"
            "\nExample:\n"
            + HelpExampleCli("getplottermininginfo", "\"1234567890\" true")
            + HelpExampleRpc("getplottermininginfo", "\"1234567890\", true")
            );

    uint64_t nPlotterId = 0;
    if (!request.params[0].isStr() || !IsValidPlotterID(request.params[0].get_str(), &nPlotterId))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid plotter ID");

    bool fVerbose = true;
    if (!request.params[1].isNull()) {
        fVerbose = request.params[1].isNum() ? (request.params[1].get_int() != 0) : request.params[1].get_bool();
    }

    LOCK(cs_main);
    const Consensus::Params& params = Params().GetConsensus();
    const int nHeight = ::ChainActive().Height();
    const poc::CBlockList vBlocks = poc::GetEvalBlocks(::ChainActive().Height(), true, params);

    int64_t nNetCapacityTB = 0, nCapacityTB = 0;
    if (!vBlocks.empty()) {
        uint64_t nBaseTarget = 0;
        int nBlockCount = 0;
        int nMinedBlockCount = 0;
        for (const CBlockIndex& block : vBlocks) {
            if (block.nPlotterId == nPlotterId)
                nMinedBlockCount++;

            nBaseTarget += block.nBaseTarget;
            nBlockCount++;
        }
        if (nBlockCount > 0) {
            nBaseTarget = std::max(nBaseTarget / nBlockCount, uint64_t(1));
            nNetCapacityTB = std::max(static_cast<int64_t>(poc::INITIAL_BASE_TARGET / nBaseTarget), (int64_t) 1);
            if (nMinedBlockCount < (int) vBlocks.size())
                nMinedBlockCount++;
            nCapacityTB = std::max((int64_t) ((nNetCapacityTB * nMinedBlockCount) / vBlocks.size()), (int64_t) 1);
        }
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("plotterId", std::to_string(nPlotterId));
    result.pushKV("capacity", ValueFromCapacity(nCapacityTB));
    result.pushKV("pledge", ValueFromAmount(poc::GetCapacityRequireBalance(nCapacityTB, nHeight, params)));

    if (fVerbose) {
        COutPoint outpoint;
        const Coin &coin = ::ChainstateActive().CoinsTip().GetLastBindPlotterCoin(nPlotterId, &outpoint);
        if (!coin.IsSpent()) {
            UniValue item(UniValue::VOBJ);
            item.pushKV("capacity", ValueFromCapacity(nCapacityTB));
            item.pushKV("pledge", ValueFromAmount(poc::GetCapacityRequireBalance(nCapacityTB, nHeight, params)));
            item.pushKV("txid", outpoint.hash.GetHex());
            item.pushKV("vout", 0);
            item.pushKV("blockhash", ::ChainActive()[coin.nHeight]->GetBlockHash().GetHex());
            item.pushKV("blocktime", ::ChainActive()[coin.nHeight]->GetBlockTime());
            item.pushKV("blockheight", (int) coin.nHeight);
            UniValue objBindAddress(UniValue::VOBJ);
            objBindAddress.pushKV(EncodeDestination(ExtractDestination(coin.out.scriptPubKey)), item);
            result.pushKV("bindAddresses", objBindAddress);
        }
    }

    // Mined
    if (fVerbose) {
        UniValue vMinedBlocks(UniValue::VARR);
        for (auto it = vBlocks.rbegin(); it != vBlocks.rend(); it++) {
            const CBlockIndex& blockIndex = *it;
            if (blockIndex.nPlotterId == nPlotterId) {
                UniValue item(UniValue::VOBJ);
                item.pushKV("blockhash", blockIndex.GetBlockHash().GetHex());
                item.pushKV("blocktime", blockIndex.GetBlockTime());
                item.pushKV("blockheight", blockIndex.nHeight);
                if (blockIndex.nTx > 0) {
                    CBlock block;
                    if (ReadBlockFromDisk(block, &blockIndex, params))
                        item.pushKV("address", EncodeDestination(ExtractDestination(block.vtx[0]->vout[0].scriptPubKey)));
                }
                vMinedBlocks.push_back(item);
            }
        }
        result.pushKV("blocks", vMinedBlocks);
    }

    return result;
}

static UniValue ListPoint(CCoinsViewCursorRef pcursor) {
    assert(pcursor != nullptr);
    UniValue ret(UniValue::VARR);
    for (; pcursor->Valid(); pcursor->Next()) {
        COutPoint key;
        Coin coin;
        if (pcursor->GetKey(key) && pcursor->GetValue(coin)) {
            assert(!coin.IsSpent());
            assert(coin.IsPoint());

            UniValue item(UniValue::VOBJ);
            item.pushKV("from", EncodeDestination(ExtractDestination(coin.out.scriptPubKey)));
            item.pushKV("to", EncodeDestination(ScriptHash(PointPayload::As(coin.payload)->GetReceiverID())));
            item.pushKV("lock_amount", ValueFromAmount(coin.out.nValue));
            item.pushKV("effective_amount", ValueFromAmount(PointPayload::As(coin.payload)->GetAmount()));
            item.pushKV("lock_blocks", (int)PointPayload::As(coin.payload)->GetLockBlocks());
            item.pushKV("txid", key.hash.GetHex());
            item.pushKV("blockhash", ::ChainActive()[(int)coin.nHeight]->GetBlockHash().GetHex());
            item.pushKV("blocktime", ::ChainActive()[(int)coin.nHeight]->GetBlockTime());
            item.pushKV("blockheight", (int)coin.nHeight);
            
            ret.push_back(item);
        } else
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Unable to read UTXO set");
    }

    return ret;
}

static UniValue listpledgeloanofaddress(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "listpledgeloanofaddress \"address\"\n"
            "\nReturns up to point sent coins.\n"
            "\nArguments:\n"
            "1. address             (string, required) The Qitcoin address\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"from\":\"address\",                  (string) The Qitcoin address of the point sender.\n"
            "    \"to\":\"address\",                    (string) The Qitcoin address of the point receiver\n"
            "    \"amount\": x.xxx,                   (numeric) The amount in " + CURRENCY_UNIT + ".\n"
            "    \"txid\": \"transactionid\",           (string) The transaction id.\n"
            "    \"blockhash\": \"hashvalue\",          (string) The block hash containing the transaction.\n"
            "    \"blocktime\": xxx,                  (numeric) The block time in seconds since epoch (1 Jan 1970 GMT).\n"
            "    \"blockheight\": xxx,                (numeric) The block height.\n"
            "  }\n"
            "]\n"

            "\nExamples:\n"
            "\nList the point sent coins from UTXOs\n"
            + HelpExampleCli("listpledgeloanofaddress", std::string("\"") + Params().GetConsensus().FundAddress + "\"")
            + HelpExampleRpc("listpledgeloanofaddress", std::string("\"") + Params().GetConsensus().FundAddress + "\"")
        );

    if (!request.params[0].isStr())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");
    const CAccountID accountID = ExtractAccountID(DecodeDestination(request.params[0].get_str()));
    if (accountID.IsNull())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid address");

    LOCK(cs_main);

    CValidationState state;
    if (!::ChainstateActive().FlushStateToDisk(Params(), state, FlushStateMode::ALWAYS)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, strprintf("Unable to flush state to disk (%s)\n", FormatStateMessage(state)));
    }
    return ListPoint(::ChainstateActive().CoinsDB().PointSendCursor(accountID));
}

static UniValue listpledgedebitofaddress(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "listpledgedebitofaddress \"address\"\n"
            "\nReturns up to point receive coins.\n"
            "\nArguments:\n"
            "1. address             (string, required) The Qitcoin address\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"from\":\"address\",                  (string) The Qitcoin address of the point sender.\n"
            "    \"to\":\"address\",                    (string) The Qitcoin address of the point receiver\n"
            "    \"amount\": x.xxx,                   (numeric) The amount in " + CURRENCY_UNIT + ".\n"
            "    \"txid\": \"transactionid\",           (string) The transaction id.\n"
            "    \"blockhash\": \"hashvalue\",          (string) The block hash containing the transaction.\n"
            "    \"blocktime\": xxx,                  (numeric) The block time in seconds since epoch (1 Jan 1970 GMT).\n"
            "    \"blockheight\": xxx,                 (numeric) The block height.\n"
            "  }\n"
            "]\n"

            "\nExamples:\n"
            "\nList the point receive coins from UTXOs\n"
            + HelpExampleCli("listpledgedebitofaddress", std::string("\"") + Params().GetConsensus().FundAddress + "\"")
            + HelpExampleRpc("listpledgedebitofaddress", std::string("\"") + Params().GetConsensus().FundAddress + "\"")
        );

    if (!request.params[0].isStr())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");
    const CAccountID accountID = ExtractAccountID(DecodeDestination(request.params[0].get_str()));
    if (accountID.IsNull())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid address");

    LOCK(cs_main);

    CValidationState state;
    if (!::ChainstateActive().FlushStateToDisk(Params(), state, FlushStateMode::ALWAYS)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, strprintf("Unable to flush state to disk (%s)\n", FormatStateMessage(state)));
    }
    return ListPoint(::ChainstateActive().CoinsDB().PointReceiveCursor(accountID));
}
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                            actor (function)                argNames
  //  --------------------- ------------------------------  ------------------------------  ----------
    { "mining",             "getmininginfo",                &getmininginfo,                 {} },
    { "mining",             "prioritisetransaction",        &prioritisetransaction,         {"txid","dummy","fee_delta"} },
    { "mining",             "getblocktemplate",             &getblocktemplate,              {"template_request"} },
    { "mining",             "submitblock",                  &submitblock,                   {"hexdata","dummy"} },
    { "mining",             "submitheader",                 &submitheader,                  {"hexdata"} },

    { "util",               "estimatesmartfee",             &estimatesmartfee,              {"conf_target", "estimate_mode"} },

    { "hidden",             "estimaterawfee",               &estimaterawfee,                {"conf_target", "threshold"} },

    // for Qitcoin
#ifdef ENABLE_WALLET
    // TODO move to rpcwalet.cpp
    { "wallet",             "generatetoaddress",            &generatetoaddress,             {"nblocks","address"} },
#endif
    { "generating",         "generatetoprivkey",            &generatetoprivkey,             {"nblocks","privatekey"} },
    { "mining",             "getactivebindplotteraddress",  &getactivebindplotteraddress,   {"plotterId"} },
    { "mining",             "getactivebindplotter",         &getactivebindplotter,          {"plotterId"} },
    { "mining",             "listbindplotterofaddress",     &listbindplotterofaddress,      {"address", "plotterId", "count", "verbose"} },
    { "mining",             "createbindplotterdata",        &createbindplotterdata,         {"address", "passphrase", "lastActiveHeight"} },
    { "mining",             "decodebindplotterdata",        &decodebindplotterdata,         { "hexdata"} },
    { "mining",             "verifybindplotterdata",        &verifybindplotterdata,         {"address", "hexdata"} },
    { "mining",             "getbindplotterlimit",          &getbindplotterlimit,           {"plotterId"} },
    { "mining",             "getunbindplotterlimit",        &getunbindplotterlimit,         {"txid"} },
#ifdef ENABLE_WALLET
    // TODO move to rpcwalet.cpp
    { "wallet",             "getpledge",                    &getpledge,                     {"plotterId","verbose"} },
#endif
    { "mining",             "getpledgeofaddress",           &getpledgeofaddress,            {"address", "plotterId", "verbose"} },
    { "mining",             "getplottermininginfo",         &getplottermininginfo,          {"plotterId", "verbose"} },
    { "mining",             "listpledgeloanofaddress",      &listpledgeloanofaddress,       {"address"} },
    { "mining",             "listpledgedebitofaddress",     &listpledgedebitofaddress,      {"address"} },
};
// clang-format on

void RegisterMiningRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
