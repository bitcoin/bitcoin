// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <validation.h>
#include <node/blockstorage.h>
#include <rpc/util.h>
#include <services/nevmconsensus.h>
#include <services/rpc/nevmrpc.h>
#include <chainparams.h>
#include <rpc/server.h>
#include <thread>
#include <policy/rbf.h>
#include <policy/policy.h>
#include <index/txindex.h>
#include <core_io.h>
#include <rpc/blockchain.h>
#include <node/context.h>
#include <node/transaction.h>
#include <rpc/server_util.h>
#include <interfaces/node.h>
#include <llmq/quorums_chainlocks.h>
#include <key_io.h>
#include <common/args.h>
#include <logging.h>
using node::GetTransaction;
extern RecursiveMutex cs_setethstatus;

static RPCHelpMan syscoindecoderawtransaction()
{
    return RPCHelpMan{"syscoindecoderawtransaction",
    "\nDecode raw syscoin transaction (serialized, hex-encoded) and display information pertaining to the service that is included in the transactiion data output(OP_RETURN)\n",
    {
        {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction hex string."}
    },
    RPCResult{
        RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            {RPCResult::Type::STR_HEX, "blockhash", "Block confirming the transaction, if any"},
            {RPCResult::Type::NUM, "value", "The total amount in this transaction"},
        }},
    RPCExamples{
        HelpExampleCli("syscoindecoderawtransaction", "\"hexstring\"")
        + HelpExampleRpc("syscoindecoderawtransaction", "\"hexstring\"")
    },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    const UniValue &params = request.params;
    const node::NodeContext& node = EnsureAnyNodeContext(request.context);

    std::string hexstring = params[0].get_str();
    CMutableTransaction tx;
    if(!DecodeHexTx(tx, hexstring, false, true)) {
        if(!DecodeHexTx(tx, hexstring, true, true)) {
             throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Could not decode transaction");
        }
    }
    CTransactionRef rawTx(MakeTransactionRef(std::move(tx)));
    if (rawTx->IsNull())
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Could not decode transaction");

    CBlockIndex* blockindex = nullptr;
    uint256 hashBlock;
    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }
    // block may not be found
    rawTx = GetTransaction(blockindex, node.mempool.get(), rawTx->GetHash(), hashBlock, node.chainman->m_blockman);

    UniValue output(UniValue::VOBJ);
    if(rawTx && !DecodeSyscoinRawtransaction(*rawTx, hashBlock, output))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Not a Syscoin transaction");
    return output;
},
    };
}

static RPCHelpMan getnevmblockchaininfo()
{
    return RPCHelpMan{"getnevmblockchaininfo",
        "\nReturn NEVM blockchain information and status.\n",
        {
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "bestblockhash", "The tip of the NEVM blockchain"},
                {RPCResult::Type::STR_HEX, "previousblockhash", "The hash of the previous block"},
                {RPCResult::Type::STR_HEX, "txroot", "Transaction root of the current tip"},
                {RPCResult::Type::STR_HEX, "receiptroot", "Receipt root of the current tip"},
                {RPCResult::Type::NUM, "blocksize", "Serialized NEVM block size. 0 means it has been pruned."},
                {RPCResult::Type::NUM, "height", "The current NEVM blockchain height"},
                {RPCResult::Type::STR, "commandline", "The NEVM command line parameters used to pass through to sysgeth"},
                {RPCResult::Type::STR, "status", "The NEVM status, online or offline"},
            }},
        RPCExamples{
            HelpExampleCli("getnevmblockchaininfo", "")
            + HelpExampleRpc("getnevmblockchaininfo", "")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    UniValue oNEVM(UniValue::VOBJ);
    CNEVMHeader evmBlock;
    BlockValidationState state;
    int nHeight;
    CBlock block;
    {
        LOCK(cs_main);
        auto *tip = chainman.ActiveChain().Tip();
        nHeight = tip->nHeight;
        auto *pblockindex = chainman.m_blockman.LookupBlockIndex(tip->GetBlockHash());
        if (!pblockindex) {
            throw JSONRPCError(RPC_MISC_ERROR, tip->GetBlockHash().ToString() + " not found");
        }
        if (chainman.m_blockman.IsBlockPruned(pblockindex)) {
            throw JSONRPCError(RPC_MISC_ERROR, tip->GetBlockHash().ToString() + " not available (pruned data)");
        }
        if (!chainman.m_blockman.ReadBlockFromDisk(block, *pblockindex)) {
            throw JSONRPCError(RPC_MISC_ERROR, tip->GetBlockHash().ToString() + " not found");
        }
        if(!GetNEVMData(state, block, evmBlock)) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, state.ToString());
        }
    }
    const std::vector<std::string> &cmdLine = chainman.GethCommandLine();
    std::vector<UniValue> vec;
    for(auto& cmd: cmdLine) {
        UniValue v;
        v.setStr(cmd);
        vec.push_back(v);
    }
    std::reverse (evmBlock.nBlockHash.begin (), evmBlock.nBlockHash.end ()); // correct endian
    oNEVM.pushKVEnd("bestblockhash", "0x" + evmBlock.nBlockHash.ToString());
    oNEVM.pushKVEnd("txroot", "0x" + evmBlock.nTxRoot.GetHex());
    oNEVM.pushKVEnd("receiptroot", "0x" + evmBlock.nReceiptRoot.GetHex());
    oNEVM.pushKVEnd("height", (nHeight - Params().GetConsensus().nNEVMStartBlock) + 1);
    oNEVM.pushKVEnd("blocksize", (int)block.vchNEVMBlockData.size());
    UniValue arrVec(UniValue::VARR);
    arrVec.push_backV(vec);
    oNEVM.pushKVEnd("commandline", arrVec);
    bool bResponse = false;
    GetMainSignals().NotifyNEVMComms("status", bResponse);
    oNEVM.pushKVEnd("status", bResponse? "online": "offline");
    return oNEVM;
},
    };
}

static RPCHelpMan getnevmblobdata()
{
    return RPCHelpMan{"getnevmblobdata",
        "\nReturn NEVM blob information and status from a version hash.\n",
        {
            {"versionhash_or_txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The version hash or txid of the NEVM blob"},
            {"getdata", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Optional, retrieve the blob data"}
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
            HelpExampleCli("getnevmblobdata", "")
            + HelpExampleRpc("getnevmblobdata", "")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    bool bGetData = false;
    if(request.params.size() > 1) {
        bGetData = request.params[1].get_bool();
    }
    node::NodeContext& node = EnsureAnyNodeContext(request.context);
    CBlockIndex* pblockindex = nullptr;
    uint256 hashBlock;
    uint256 txhash = ParseHashV(request.params[0], "parameter 1");
    std::vector<uint8_t> vchVH;
    uint32_t nSize = 0;
    CNEVMData nevmData;
    CTransactionRef tx;
    {
        LOCK(cs_main);
        const Coin& coin = AccessByTxid(node.chainman->ActiveChainstate().CoinsTip(), txhash);
        if (!coin.IsSpent()) {
            pblockindex = node.chainman->ActiveChain()[coin.nHeight];
        }
        if (pblockindex == nullptr) {
            uint32_t nBlockHeight;
            if(pblockindexdb != nullptr && pblockindexdb->ReadBlockHeight(txhash, nBlockHeight)) {
                pblockindex = node.chainman->ActiveChain()[nBlockHeight];
            }
        }
        hashBlock.SetNull();
        if(pblockindex != nullptr) {
            tx = GetTransaction(pblockindex, nullptr, txhash, hashBlock, node.chainman->m_blockman);
        }
        if(!tx || hashBlock.IsNull()) {
            vchVH = ParseHex(request.params[0].get_str());
        }
        else {
            nevmData = CNEVMData(*tx);
            if(nevmData.IsNull()){
                throw JSONRPCError(RPC_INVALID_PARAMS, "Could not parse blob data from transaction");
            }
            vchVH = nevmData.vchVersionHash;
        }
    }

    UniValue oNEVM(UniValue::VOBJ);
    BlockValidationState state;
    std::vector<uint8_t> vchData;
    int64_t mpt = -1;
    if(!pnevmdatadb->ReadMTP(vchVH, mpt)) {
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("Could not find MTP for versionhash %s", HexStr(vchVH)));
    }
    if(!pnevmdatadb->ReadDataSize(vchVH, nSize)) {
        throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("Could not find data size for versionhash %s", HexStr(vchVH)));
    }
    oNEVM.pushKVEnd("versionhash", HexStr(vchVH));
    oNEVM.pushKVEnd("mpt", mpt);
    oNEVM.pushKVEnd("datasize", nSize);
    if(pblockindex != nullptr) {
        oNEVM.pushKVEnd("blockhash", pblockindex->GetBlockHash().GetHex());
        oNEVM.pushKVEnd("height", pblockindex->nHeight);
    }
    if(bGetData) {
        if(!pnevmdatadb->ReadData(vchVH, vchData)) {
            throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("Could not find payload for versionhash %s", HexStr(vchVH)));
        }
        oNEVM.pushKVEnd("data", HexStr(vchData));
    }
    return oNEVM;
},
    };
}

static RPCHelpMan settestparams()
{
    return RPCHelpMan{"settestparams",
        "\nSet test setting. Used in testing only.\n",
        {
            {"setting", RPCArg::Type::STR, RPCArg::Optional::NO, "DIP3 setting"}
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
            HelpExampleCli("settestparams", "\"1\"")
            + HelpExampleRpc("settestparams", "\"1\"")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
        bool fTestSetting = false;
        if(request.params[0].get_str() == "1") {
            fTestSetting = true;
        }
        return "success";
},
    };
}
bool ScanBlobs(CNEVMDataDB& pnevmdatadb, const uint32_t count, const uint32_t from, const UniValue& oOptions, UniValue& oRes) {
	bool getdata = false;
	if (!oOptions.isNull()) {
		const UniValue &getdataObj = oOptions.find_value("getdata");
		if (getdataObj.isBool()) {
			getdata = getdataObj.get_bool();
		}
	}
    uint32_t index = 0;
    const auto & mapCache = pnevmdatadb.GetMapCache();
    for (auto const& [key, val] :mapCache) {
        UniValue oBlob(UniValue::VOBJ);
        oBlob.pushKV("versionhash",  HexStr(key));
        oBlob.pushKV("mpt", val.second);
        oBlob.pushKV("datasize", (uint32_t)val.first.size());
        if(getdata) {
            oBlob.pushKV("data", HexStr(val.first));
        }
        index += 1;
        if (index <= from) {
            continue;
        }
        oRes.push_back(oBlob);
        if (index >= count + from)
            return true;
    }
	std::unique_ptr<CDBIterator> pcursor(pnevmdatadb.NewIterator());
	pcursor->SeekToFirst();
	std::pair<std::vector<uint8_t>, bool> key;
	while (pcursor->Valid()) {
		try {
            key.first.clear();
			if (pcursor->GetKey(key)) {
                // MTP
                if(key.second == false && mapCache.find(key.first) == mapCache.end()) {
                    UniValue oBlob(UniValue::VOBJ);
                    uint64_t nMedianTime;
                    if(pcursor->GetValue(nMedianTime)) {
                        oBlob.pushKV("versionhash",  HexStr(key.first));
                        oBlob.pushKV("mpt", nMedianTime);
                        uint32_t nSize = 0;
                        std::vector<uint8_t> vchData;
                        if(!pnevmdatadb.ReadDataSize(key.first, nSize)) {
                           pnevmdatadb.ReadData(key.first, vchData);
                        }
                        oBlob.pushKV("datasize", nSize);
                        if(getdata) {
                            if(vchData.empty()) {
                                pnevmdatadb.ReadData(key.first, vchData);
                            }
                            oBlob.pushKV("data", HexStr(vchData));
                        }
                        oRes.push_back(oBlob);
                    }
                } else {
           			pcursor->Next();
					continue;
                }
				index += 1;
				if (index <= from) {
					pcursor->Next();
					continue;
				}
				if (index >= count + from)
					break;
			}
			pcursor->Next();
		}
		catch (std::exception &e) {
			return error("%s() : deserialize error", __PRETTY_FUNCTION__);
		}
	}
	return true;
}
static RPCHelpMan listnevmblobdata()
{
    return RPCHelpMan{"listnevmblobdata",
        "\nScan through all blobs.\n",
        {
            {"count", RPCArg::Type::NUM, RPCArg::Default{10}, "The number of results to return."},
            {"from", RPCArg::Type::NUM, RPCArg::Default{0}, "The number of results to skip."},
            {"options", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "A json object with options to filter results.",
                {
                    {"getdata", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Return data from blob"},
                }
                }
            },
            RPCResult{
                RPCResult::Type::ARR, "", "",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR_HEX, "versionhash", "The version hash of the NEVM blob"},
                        {RPCResult::Type::NUM, "datasize", "Size of data blob in bytes"},
                        {RPCResult::Type::STR_HEX, "data", "Blob data if getdata is true"},
                    }},
                },
            },
            RPCExamples{
            HelpExampleCli("listnevmblobdata", "0")
            + HelpExampleCli("listnevmblobdata", "10 10")
            + HelpExampleCli("listnevmblobdata", "0 0 '{\"getdata\":true}'")
            + HelpExampleRpc("listnevmblobdata", "0, 0, '{\"getdata\":false}'")
            },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    const UniValue &params = request.params;
    UniValue options;
    uint32_t count = 10;
    uint32_t from = 0;
    if (params.size() > 0) {
        count = params[0].getInt<int>();
        if (count == 0) {
            count = 10;
        }
    }
    if (params.size() > 1) {
        from = params[1].getInt<int>();
    }
    if (params.size() > 2) {
        options = params[2];
    }
    UniValue oRes(UniValue::VARR);
    if (!ScanBlobs(*pnevmdatadb, count, from, options, oRes))
        throw JSONRPCError(RPC_MISC_ERROR, "Scan failed");
    return oRes;
},
    };
}

static RPCHelpMan syscoingetspvproof()
{
    return RPCHelpMan{"syscoingetspvproof",
    "\nReturns SPV proof for use with inter-chain transfers.\n",
    {
        {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "A transaction that is in the block"},
        {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "If specified, looks for txid in the block with this hash"}
    },
    RPCResult{
        RPCResult::Type::ANY, "proof", "JSON representation of merkle proof (transaction index, siblings and block header and some other information useful for moving coins to another chain)"},
    RPCExamples{""},
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    node::NodeContext& node = EnsureAnyNodeContext(request.context);
    CBlockIndex* pblockindex = nullptr;
    uint256 hashBlock;
    uint256 txhash = ParseHashV(request.params[0], "parameter 1");
    UniValue res(UniValue::VOBJ);
    {
        LOCK(cs_main);
        if (!request.params[1].isNull()) {
            hashBlock = ParseHashV(request.params[1], "blockhash");
            pblockindex = node.chainman->m_blockman.LookupBlockIndex(hashBlock);
            if (!pblockindex) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
            }
        } else {
            const Coin& coin = AccessByTxid(node.chainman->ActiveChainstate().CoinsTip(), txhash);
            if (!coin.IsSpent()) {
                pblockindex = node.chainman->ActiveChain()[coin.nHeight];
            }
        }
        if (pblockindex == nullptr) {
            uint32_t nBlockHeight;
            if(pblockindexdb != nullptr && pblockindexdb->ReadBlockHeight(txhash, nBlockHeight)) {
                pblockindex = node.chainman->ActiveChain()[nBlockHeight];
            }
        }
    }

    // Allow txindex to catch up if we need to query it and before we acquire cs_main.
    if (g_txindex && !pblockindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }
    if(!pblockindex) {
         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not yet in block");
    }
    CTransactionRef tx;
    hashBlock.SetNull();
    tx = GetTransaction(pblockindex, nullptr, txhash, hashBlock, node.chainman->m_blockman);
    if(!tx || hashBlock.IsNull())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not yet in block");

    CBlock block;
    {
        LOCK(cs_main);
        if (node.chainman->m_blockman.IsBlockPruned(pblockindex)) {
            throw JSONRPCError(RPC_MISC_ERROR, "Block not available (pruned data)");
        }
    }

    if (!node.chainman->m_blockman.ReadBlockFromDisk(block, *pblockindex)) {
        // Block not found on disk. This could be because we have the block
        // header in our index but don't have the block (for example if a
        // non-whitelisted node sends us an unrequested long chain of valid
        // blocks, we add the headers to our index, but don't accept the
        // block).
        throw JSONRPCError(RPC_MISC_ERROR, "Block not found on disk");
    }
    CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
    ssBlock << pblockindex->GetBlockHeader(*node.chainman);
    const std::string &rawTx = EncodeHexTx(*tx, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS);
    res.pushKVEnd("transaction",rawTx);
    res.pushKVEnd("blockhash", hashBlock.GetHex());
    const auto bytesVec = MakeUCharSpan(ssBlock);
    // get first 80 bytes of header (non auxpow part)
    res.pushKVEnd("header", HexStr(std::vector<unsigned char>(bytesVec.begin(), bytesVec.begin()+80)));
    UniValue siblings(UniValue::VARR);
    // store the index of the transaction we are looking for within the block
    int nIndex = 0;
    for (unsigned int i = 0;i < block.vtx.size();i++) {
        const uint256 &txHashFromBlock = block.vtx[i]->GetHash();
        if(txhash == txHashFromBlock)
            nIndex = i;
        siblings.push_back(txHashFromBlock.GetHex());
    }
    res.pushKVEnd("siblings", siblings);
    res.pushKVEnd("index", nIndex);
    CNEVMHeader evmBlock;
    BlockValidationState state;
    if(!GetNEVMData(state, block, evmBlock)) {
        throw JSONRPCError(RPC_MISC_ERROR, state.ToString());
    }
    std::reverse (evmBlock.nBlockHash.begin (), evmBlock.nBlockHash.end ()); // correct endian
    res.pushKVEnd("nevm_blockhash", evmBlock.nBlockHash.GetHex());
    // SYSCOIN
    if(llmq::chainLocksHandler)
        res.pushKVEnd("chainlock", llmq::chainLocksHandler->HasChainLock(pblockindex->nHeight, hashBlock));
    return res;
},
    };
}

static RPCHelpMan syscoinstopgeth()
{
    return RPCHelpMan{"syscoinstopgeth",
    "\nStops Geth from running.\n",
    {},
    RPCResult{
        RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR, "status", "Result"},
        }},
    RPCExamples{
        HelpExampleCli("syscoinstopgeth", "")
        + HelpExampleRpc("syscoinstopgeth", "")
    },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    if(!chainman.ActiveChainstate().StopGethNode()) {
        throw JSONRPCError(RPC_MISC_ERROR, "Could not stop Geth");
    }
    UniValue ret(UniValue::VOBJ);
    ret.pushKVEnd("status", "success");
    return ret;
},
    };
}

static RPCHelpMan syscoinstartgeth()
{
    return RPCHelpMan{"syscoinstartgeth",
    "\nStarts Geth.\n",
    {},
    RPCResult{
        RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR, "status", "Result"},
        }},
    RPCExamples{
        HelpExampleCli("syscoinstartgeth", "")
        + HelpExampleRpc("syscoinstartgeth", "")
    },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    if(!chainman.ActiveChainstate().RestartGethNode()) {
        throw JSONRPCError(RPC_MISC_ERROR, "Could not restart geth, see debug.log for more information...");
    }
    BlockValidationState state;
    chainman.ActiveChainstate().ActivateBestChain(state);

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.ToString());
    }
    UniValue ret(UniValue::VOBJ);
    ret.pushKVEnd("status", "success");
    return ret;
},
    };
}

static RPCHelpMan syscoingettxroots()
{
    return RPCHelpMan{"syscoingettxroots",
    "\nGet NEVM transaction and receipt roots based on block hash.\n",
    {
        {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The block hash to lookup."}
    },
    RPCResult{
        RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR_HEX, "txroot", "The transaction merkle root"},
            {RPCResult::Type::STR_HEX, "receiptroot", "The receipt merkle root"},
        }},
    RPCExamples{
        HelpExampleCli("syscoingettxroots", "0xd8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10")
        + HelpExampleRpc("syscoingettxroots", "0xd8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10")
    },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    LOCK(cs_setethstatus);
    std::string blockHashStr = request.params[0].get_str();
    blockHashStr = RemovePrefix(blockHashStr, "0x");  // strip 0x
    uint256 nBlockHash;
    if(!ParseHashStr(blockHashStr, nBlockHash)) {
         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Could not read block hash");
    }
    NEVMTxRoot txRootDB;
    if(!pnevmtxrootsdb || !pnevmtxrootsdb->ReadTxRoots(nBlockHash, txRootDB)){
       throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Could not read transaction roots");
    }

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("blockhash", nBlockHash.GetHex());
    ret.pushKV("txroot", txRootDB.nTxRoot.GetHex());
    ret.pushKV("receiptroot", txRootDB.nReceiptRoot.GetHex());

    return ret;
},
    };
}

static RPCHelpMan syscoincheckmint()
{
    return RPCHelpMan{"syscoincheckmint",
    "\nGet the Syscoin mint transaction by looking up using NEVM tx hash (This is no the txid, it is the sha3 of the transaction bytes value).\n",
    {
        {"nevm_txhash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "NEVM Tx Hash used to burn funds to move to Syscoin."}
    },
    RPCResult{
        RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
        }},
    RPCExamples{
        HelpExampleCli("syscoincheckmint", "d8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10")
        + HelpExampleRpc("syscoincheckmint", "d8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10")
    },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    std::string strTxHash = request.params[0].get_str();
    strTxHash = RemovePrefix(strTxHash, "0x");  // strip 0x
    uint256 sysTxid;
    if(!pnevmtxmintdb || !pnevmtxmintdb->Read(uint256S(strTxHash), sysTxid)){
       throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Could not read Syscoin txid using mint transaction hash");
    }
    UniValue output(UniValue::VOBJ);
    output.pushKV("txid", sysTxid.GetHex());
    return output;
},
    };
}

// clang-format on
void RegisterNEVMRPCCommands(CRPCTable &t)
{
    static const CRPCCommand commands[]{
        {"syscoin", &syscoingettxroots},
        {"syscoin", &syscoingetspvproof},
        {"syscoin", &syscoindecoderawtransaction},
        {"syscoin", &listnevmblobdata},
        {"syscoin", &syscoinstopgeth},
        {"syscoin", &syscoinstartgeth},
        {"syscoin", &syscoincheckmint},
        {"syscoin", &getnevmblockchaininfo},
        {"syscoin", &getnevmblobdata},
        {"syscoin", &settestparams},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
