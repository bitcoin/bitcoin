// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <validation.h>
#include <node/blockstorage.h>
#include <boost/algorithm/string.hpp>
#include <rpc/util.h>
#include <services/assetconsensus.h>
#include <services/asset.h>
#include <services/rpc/assetrpc.h>
#include <chainparams.h>
#include <rpc/server.h>
#include <thread>
#include <policy/rbf.h>
#include <policy/policy.h>
#include <index/txindex.h>
#include <core_io.h>
#include <util/system.h>
#include <rpc/blockchain.h>
#include <node/context.h>
extern RecursiveMutex cs_setethstatus;
extern std::string EncodeDestination(const CTxDestination& dest);
extern CTxDestination DecodeDestination(const std::string& str);


bool BuildAssetJson(const CAsset& asset, const uint32_t& nBaseAsset, UniValue& oAsset) {
    oAsset.__pushKV("asset_guid", UniValue(nBaseAsset).write());
    oAsset.__pushKV("symbol", DecodeBase64(asset.strSymbol));
	oAsset.__pushKV("public_value", AssetPublicDataToJson(asset.strPubData));
    oAsset.__pushKV("contract", asset.vchContract.empty()? "" : "0x"+HexStr(asset.vchContract));
    oAsset.__pushKV("notary_address", asset.vchNotaryKeyID.empty()? "" : EncodeDestination(WitnessV0KeyHash(uint160{asset.vchNotaryKeyID})));
    if (!asset.notaryDetails.IsNull()) {
        UniValue value(UniValue::VOBJ);
        asset.notaryDetails.ToJson(value);
        oAsset.__pushKV("notary_details", value);
    }
    if (!asset.auxFeeDetails.IsNull()) {
        UniValue value(UniValue::VOBJ);
        asset.auxFeeDetails.ToJson(value, nBaseAsset);
		oAsset.__pushKV("auxfee", value);
    }
	oAsset.__pushKV("total_supply", ValueFromAmount(asset.nTotalSupply, nBaseAsset));
	oAsset.__pushKV("max_supply", ValueFromAmount(asset.nMaxSupply, nBaseAsset));
	oAsset.__pushKV("updatecapability_flags", asset.nUpdateCapabilityFlags);
	oAsset.__pushKV("precision", asset.nPrecision);
	return true;
}
bool ScanAssets(CAssetDB& passetdb, const uint32_t count, const uint32_t from, const UniValue& oOptions, UniValue& oRes) {
	std::string strTxid = "";
    uint32_t nBaseAsset = 0;
	if (!oOptions.isNull()) {
		const UniValue &txid = find_value(oOptions, "txid");
		if (txid.isStr()) {
			strTxid = txid.get_str();
		}
		const UniValue &assetObj = find_value(oOptions, "asset_guid");
		if (assetObj.isStr()) {
            uint64_t nAsset;
            if(!ParseUInt64(assetObj.get_str(), &nAsset))
                throw JSONRPCError(RPC_INVALID_PARAMS, "Could not parse asset_guid");
            nBaseAsset = GetBaseAssetID(nAsset);
		}
	}
	std::unique_ptr<CDBIterator> pcursor(passetdb.NewIterator());
	pcursor->SeekToFirst();
	CAsset txPos;
	uint32_t key = 0;
	uint32_t index = 0;
	while (pcursor->Valid()) {
		try {
            key = 0;
            txPos.SetNull();
			if (pcursor->GetKey(key) && key != 0 && pcursor->GetValue(txPos) && (nBaseAsset == 0 || nBaseAsset != key)) {
                if(txPos.IsNull()){
                    pcursor->Next();
                    continue;
                }
				UniValue oAsset(UniValue::VOBJ);
				if (!BuildAssetJson(txPos, key, oAsset))
				{
					pcursor->Next();
					continue;
				}
				index += 1;
				if (index <= from) {
					pcursor->Next();
					continue;
				}
				oRes.push_back(oAsset);
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

bool UpdateNotarySignature(CMutableTransaction& mtx, const uint64_t& nBaseAsset, const std::vector<unsigned char> &vchSig) {
    std::vector<unsigned char> data;
    bool bFilledNotarySig = false;
     // call API endpoint or notary signatures and fill them in for every asset
    if(mtx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM || mtx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN) {
        CBurnSyscoin burnSyscoin(mtx);
        if(FillNotarySig(burnSyscoin.voutAssets, nBaseAsset, vchSig)) {
            bFilledNotarySig = true;
            burnSyscoin.SerializeData(data);
        }
    } else if(mtx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_SEND || mtx.nVersion == SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION) {
        CAssetAllocation allocation(mtx);
        if(FillNotarySig(allocation.voutAssets, nBaseAsset, vchSig)) {
            bFilledNotarySig = true;
            allocation.SerializeData(data);
        }
    }
    if(bFilledNotarySig) {
        // find previous commitment (OP_RETURN) and replace script
        CScript scriptDataNew;
        scriptDataNew << OP_RETURN << data;
        for(auto& vout: mtx.vout) {
            if(vout.scriptPubKey.IsUnspendable()) {
                vout.scriptPubKey = scriptDataNew;
                return true;
            }
        }
    }
    return false;
}	
static RPCHelpMan assettransactionnotarize()
{
    return RPCHelpMan{"assettransactionnotarize",	
        "\nUpdate notary signature on an asset transaction. Will require re-signing transaction before submitting to network.\n",	
        {	
            {"hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Transaction to notarize."},
            {"asset_guid", RPCArg::Type::STR, RPCArg::Optional::NO, "The guid of the asset to notarize."},
            {"signature", RPCArg::Type::STR, RPCArg::Optional::NO, "Base64 encoded notary signature to add to transaction."}	
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "hex", "The notarized transaction hex, which you must sign prior to submitting."},
            }},	
        RPCExamples{	
            HelpExampleCli("assettransactionnotarize", "\"hex\" 12121 \"signature\"")	
            + HelpExampleRpc("assettransactionnotarize", "\"hex\",12121,\"signature\"")	
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    const std::string &hexstring = request.params[0].get_str();
    uint64_t nAsset;
    if(!ParseUInt64(request.params[1].get_str(), &nAsset))
         throw JSONRPCError(RPC_INVALID_PARAMS, "Could not parse asset_guid");
    std::vector<unsigned char> vchSig = DecodeBase64(request.params[2].get_str().c_str());
    CMutableTransaction mtx;
    if(!DecodeHexTx(mtx, hexstring, false, true)) {
        if(!DecodeHexTx(mtx, hexstring, true, true)) {
             throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Could not decode transaction");
        }
    }
    const uint64_t nBaseAsset = GetBaseAssetID(nAsset);
    UpdateNotarySignature(mtx, nBaseAsset, vchSig);
    UniValue ret(UniValue::VOBJ);	
    ret.pushKV("hex", EncodeHexTx(CTransaction(mtx)));
    return ret;
},
    };
}

static RPCHelpMan getnotarysighash()
{
    return RPCHelpMan{"getnotarysighash",	
        "\nGet sighash for notary to sign off on, use assettransactionnotarize to update the transaction after re-singing once sighash is used to create a notarized signature.\n",	
        {	
            {"hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Transaction to get sighash for."},
            {"asset_guid", RPCArg::Type::STR, RPCArg::Optional::NO, "The guid of the asset to sighash for."}
        },
        RPCResult{RPCResult::Type::STR_HEX, "sighash", "Notary sighash (uint256)"},	
        RPCExamples{	
            HelpExampleCli("getnotarysighash", "\"hex\" 12121")
            + HelpExampleRpc("getnotarysighash", "\"hex\",12121")	
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{	
    const std::string &hexstring = request.params[0].get_str();
    uint64_t nAsset;
    if(!ParseUInt64(request.params[1].get_str(), &nAsset))
         throw JSONRPCError(RPC_INVALID_PARAMS, "Could not parse asset_guid");
    CMutableTransaction mtx;
    if(!DecodeHexTx(mtx, hexstring, false, true)) {
        if(!DecodeHexTx(mtx, hexstring, true, true)) {
             throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Could not decode transaction");
        }
    }
    uint256 sigHash;
    // get asset
    CAsset theAsset;
    const uint64_t nBaseAsset = GetBaseAssetID(nAsset);
    // if asset has notary signature requirement set
    if(GetAsset(nBaseAsset, theAsset) && !theAsset.vchNotaryKeyID.empty()) {
        CTransaction tx(mtx);
        auto itVout = std::find_if( tx.voutAssets.begin(), tx.voutAssets.end(), [&nBaseAsset](const CAssetOut& element){ return GetBaseAssetID(element.key) == nBaseAsset;} );
        if(itVout != tx.voutAssets.end()) {
            sigHash = GetNotarySigHash(tx, *itVout);
        }
    }
    return sigHash.GetHex();
},
    };
}

static RPCHelpMan convertaddress()
{
    return RPCHelpMan{"convertaddress",	
        "\nConvert between Syscoin 3 and Syscoin 4 formats. This should only be used with addressed based on compressed private keys only. P2WPKH can be shown as P2PKH in Syscoin 3.\n",	
        {	
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The syscoin address to get the information of."}	
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "v3address", "The syscoin 3 address validated"},
                {RPCResult::Type::STR, "v4address", "The syscoin 4 address validated"},
            }},	
        RPCExamples{	
            HelpExampleCli("convertaddress", "\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"")	
            + HelpExampleRpc("convertaddress", "\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"")	
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{	

    UniValue ret(UniValue::VOBJ);	
    CTxDestination dest = DecodeDestination(request.params[0].get_str());	
    // Make sure the destination is valid	
    if (!IsValidDestination(dest)) {	
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");	
    }	
    std::string currentV4Address = "";	
    std::string currentV3Address = "";	
    CTxDestination v4Dest;	
    if (auto witness_id = std::get_if<WitnessV0KeyHash>(&dest)) {	
        v4Dest = dest;	
        currentV4Address =  EncodeDestination(v4Dest);	
        currentV3Address =  EncodeDestination(*witness_id);	
    }	
    else if (auto key_id = std::get_if<PKHash>(&dest)) {	
        v4Dest = WitnessV0KeyHash(*key_id);	
        currentV4Address =  EncodeDestination(v4Dest);	
        currentV3Address =  EncodeDestination(*key_id);	
    }	
    else if (auto script_id = std::get_if<ScriptHash>(&dest)) {	
        v4Dest = *script_id;	
        currentV4Address =  EncodeDestination(v4Dest);	
        currentV3Address =  currentV4Address;	
    }	
    else if (std::get_if<WitnessV0ScriptHash>(&dest)) {	
        v4Dest = dest;	
        currentV4Address =  EncodeDestination(v4Dest);	
        currentV3Address =  currentV4Address;	
    } 	


    ret.pushKV("v3address", currentV3Address);	
    ret.pushKV("v4address", currentV4Address); 	
    return ret;	
},
    };
}

int CheckActorsInTransactionGraph(const CTxMemPool& mempool, const uint256& lookForTxHash) {
    LOCK(cs_main);
    LOCK(mempool.cs);
    {
        CTxMemPool::setEntries setAncestors;
        const CTransactionRef &txRef = mempool.get(lookForTxHash);
        if (!txRef)
            return ZDAG_NOT_FOUND;
        if(!IsZdagTx(txRef->nVersion))
            return ZDAG_WARNING_NOT_ZDAG_TX;
        // the zdag tx should be under MTU of IP packet
        if(txRef->GetTotalSize() > MAX_STANDARD_ZDAG_TX_SIZE)
            return ZDAG_WARNING_SIZE_OVER_POLICY;
        // check if any inputs are dbl spent, reject if so
        if(mempool.existsConflicts(*txRef))
            return ZDAG_MAJOR_CONFLICT;        

        // check this transaction isn't RBF enabled
        RBFTransactionState rbfState = IsRBFOptIn(*txRef, mempool, setAncestors);
        if (rbfState == RBFTransactionState::UNKNOWN)
            return ZDAG_NOT_FOUND;
        else if (rbfState == RBFTransactionState::REPLACEABLE_BIP125)
            return ZDAG_WARNING_RBF;
        for (CTxMemPool::txiter it : setAncestors) {
            const CTransactionRef& ancestorTxRef = it->GetSharedTx();
            // should be under MTU of IP packet
            if(ancestorTxRef->GetTotalSize() > MAX_STANDARD_ZDAG_TX_SIZE)
                return ZDAG_WARNING_SIZE_OVER_POLICY;
            // check if any ancestor inputs are dbl spent, reject if so
            if(mempool.existsConflicts(*ancestorTxRef))
                return ZDAG_MAJOR_CONFLICT;
            if(!IsZdagTx(ancestorTxRef->nVersion))
                return ZDAG_WARNING_NOT_ZDAG_TX;
        }  
    }
    return ZDAG_STATUS_OK;
}

int VerifyTransactionGraph(const CTxMemPool& mempool, const uint256& lookForTxHash) {  
    int status = CheckActorsInTransactionGraph(mempool, lookForTxHash);
    if(status != ZDAG_STATUS_OK){
        return status;
    }
	return ZDAG_STATUS_OK;
}

static RPCHelpMan assetallocationverifyzdag()
{
    return RPCHelpMan{"assetallocationverifyzdag",
        "\nShow status as it pertains to any current Z-DAG conflicts or warnings related to a ZDAG transaction.\n"
        "Return value is in the status field and can represent 3 levels(0, 1 or 2)\n"
        "Level -1 means not found, not a ZDAG transaction, perhaps it is already confirmed.\n"
        "Level 0 means OK.\n"
        "Level 1 means warning (checked that in the mempool there are more spending balances than current POW sender balance). An active stance should be taken and perhaps a deeper analysis as to potential conflicts related to the sender.\n"
        "Level 2 means an active double spend was found and any depending asset allocation sends are also flagged as dangerous and should wait for POW confirmation before proceeding.\n",
        {
            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id of the ZDAG transaction."}
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "status", "The status level of the transaction"},
            }}, 
        RPCExamples{
            HelpExampleCli("assetallocationverifyzdag", "\"txid\"")
            + HelpExampleRpc("assetallocationverifyzdag", "\"txid\"")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
	const UniValue &params = request.params;
    const CTxMemPool& mempool = EnsureAnyMemPool(request.context);
	uint256 txid;
	txid.SetHex(params[0].get_str());
	UniValue oAssetAllocationStatus(UniValue::VOBJ);
    oAssetAllocationStatus.__pushKV("status", VerifyTransactionGraph(mempool, txid));
	return oAssetAllocationStatus;
},
    };
}

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
            {RPCResult::Type::STR, "txtype", "The syscoin transaction type"},
            {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            {RPCResult::Type::STR_HEX, "blockhash", "Block confirming the transaction, if any"},
            {RPCResult::Type::STR, "asset_guid", "The guid of the asset"},
            {RPCResult::Type::STR, "symbol", "The asset symbol"},
            {RPCResult::Type::ARR, "allocations", "(array of json receiver objects)",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                            {RPCResult::Type::STR, "address", "The address of the receiver"},
                            {RPCResult::Type::NUM, "amount", "The amount of the transaction"},
                    }},
                }},
            {RPCResult::Type::NUM, "total", "The total amount in this transaction"},
        }}, 
    RPCExamples{
        HelpExampleCli("syscoindecoderawtransaction", "\"hexstring\"")
        + HelpExampleRpc("syscoindecoderawtransaction", "\"hexstring\"")
    },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const UniValue &params = request.params;
    const NodeContext& node = EnsureAnyNodeContext(request.context);

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
    uint256 hashBlock = uint256();
    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }
    // block may not be found
    rawTx = GetTransaction(blockindex, node.mempool.get(), rawTx->GetHash(), Params().GetConsensus(), hashBlock);

    UniValue output(UniValue::VOBJ);
    if(rawTx && !DecodeSyscoinRawtransaction(*rawTx, hashBlock, output))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Not a Syscoin transaction");
    return output;
},
    };
}

static RPCHelpMan assetinfo()
{
    return RPCHelpMan{"assetinfo",
        "\nShow stored values of a single asset and its.\n",
        {
            {"asset_guid", RPCArg::Type::STR, RPCArg::Optional::NO, "The asset guid"}
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "asset_guid", "The guid of the asset"},
                {RPCResult::Type::STR, "symbol", "The asset symbol"},
                {RPCResult::Type::STR_HEX, "txid", "The transaction id that created this asset"},
                {RPCResult::Type::STR, "public_value", "The public value attached to this asset"},
                {RPCResult::Type::STR_HEX, "contract", "The nevm contract address"},
                {RPCResult::Type::STR_AMOUNT, "total_supply", "The total supply of this asset"},
                {RPCResult::Type::STR_AMOUNT, "max_supply", "The maximum supply of this asset"},
                {RPCResult::Type::NUM, "updatecapability_flags", "The capability flag in decimal"},
                {RPCResult::Type::NUM, "precision", "The precision of this asset"},
                {RPCResult::Type::STR, "NFTID", "The NFT ID of the asset if applicable"},
            }},
        RPCExamples{
            HelpExampleCli("assetinfo", "\"assetguid\"")
            + HelpExampleRpc("assetinfo", "\"assetguid\"")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const UniValue &params = request.params;
    UniValue oAsset(UniValue::VOBJ);
    uint64_t nAsset;
    if(!ParseUInt64(params[0].get_str(), &nAsset))
         throw JSONRPCError(RPC_INVALID_PARAMS, "Could not parse asset_guid");
    const uint32_t &nBaseAsset = GetBaseAssetID(nAsset);
    CAsset txPos;
    if (!GetAsset(nBaseAsset, txPos))
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to read from asset DB");
    
    if(!BuildAssetJson(txPos, nBaseAsset, oAsset))
        oAsset.clear();
    if(nAsset != nBaseAsset) {
        oAsset.__pushKV("NFTID", UniValue(GetNFTID(nAsset)).write());
    }
    return oAsset;
},
    };
}

static RPCHelpMan listassets()
{
    return RPCHelpMan{"listassets",
        "\nScan through all assets.\n",
        {
            {"count", RPCArg::Type::NUM, RPCArg::Default{10}, "The number of results to return."},
            {"from", RPCArg::Type::NUM, RPCArg::Default{0}, "The number of results to skip."},
            {"options", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "A json object with options to filter results.",
                {
                    {"asset_guid", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Asset GUID to filter"},
                }
                }
            },
            RPCResult{
                RPCResult::Type::ARR, "", "",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "asset_guid", "The guid of the asset"},
                        {RPCResult::Type::STR, "symbol", "The asset symbol"},
                        {RPCResult::Type::STR, "public_value", "The public value attached to this asset"},
                        {RPCResult::Type::STR_HEX, "contract", "The nevm contract address"},
                        {RPCResult::Type::STR_AMOUNT, "total_supply", "The total supply of this asset"},
                        {RPCResult::Type::STR_AMOUNT, "max_supply", "The maximum supply of this asset"},
                        {RPCResult::Type::NUM, "updatecapability_flags", "The capability flag in decimal"},
                        {RPCResult::Type::NUM, "precision", "The precision of this asset"},
                    }},
                }
            },
            RPCExamples{
            HelpExampleCli("listassets", "0")
            + HelpExampleCli("listassets", "10 10")
            + HelpExampleCli("listassets", "0 0 '{\"asset_guid\":\"3473733\"}'")
            + HelpExampleRpc("listassets", "0, 0, '{\"asset_guid\":\"3473733\"}'")
            },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const UniValue &params = request.params;
    UniValue options;
    uint32_t count = 10;
    uint32_t from = 0;
    if (params.size() > 0) {
        count = params[0].get_uint();
        if (count == 0) {
            count = 10;
        }
    }
    if (params.size() > 1) {
        from = params[1].get_uint();
    }
    if (params.size() > 2) {
        options = params[2];
    }
    UniValue oRes(UniValue::VARR);
    if (!ScanAssets(*passetdb, count, from, options, oRes))
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
        {"txid", RPCArg::Type::STR, RPCArg::Optional::NO, "A transaction that is in the block"},
        {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED_NAMED_ARG, "If specified, looks for txid in the block with this hash"}
    },
    RPCResult{
        RPCResult::Type::ANY, "proof", "JSON representation of merkle proof (transaction index, siblings and block header and some other information useful for moving coins/assets to another chain)"},
    RPCExamples{""},
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
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
    CTransactionRef tx;
    {
        LOCK(cs_main);
        hashBlock = pblockindex->GetBlockHash();
        tx = GetTransaction(pblockindex, nullptr, txhash, Params().GetConsensus(), hashBlock);
        if(!tx || hashBlock.IsNull())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not yet in block");
    }

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
    CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
    ssBlock << pblockindex->GetBlockHeader(Params().GetConsensus());
    const std::string &rawTx = EncodeHexTx(*tx, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS);
    res.__pushKV("transaction",rawTx);
    res.__pushKV("blockhash", hashBlock.GetHex());
    // get first 80 bytes of header (non auxpow part)
    res.__pushKV("header", HexStr(std::vector<unsigned char>(ssBlock.begin(), ssBlock.begin()+80)));
    UniValue siblings(UniValue::VARR);
    // store the index of the transaction we are looking for within the block
    int nIndex = 0;
    for (unsigned int i = 0;i < block.vtx.size();i++) {
        const uint256 &txHashFromBlock = block.vtx[i]->GetHash();
        if(txhash == txHashFromBlock)
            nIndex = i;
        siblings.push_back(txHashFromBlock.GetHex());
    }
    res.__pushKV("siblings", siblings);
    res.__pushKV("index", nIndex);  
    CNEVMBlock evmBlock;
    BlockValidationState state;
    if(!GetNEVMData(state, block, evmBlock)) {
        throw JSONRPCError(RPC_MISC_ERROR, "NEVM data not found");
    }  
    std::reverse (evmBlock.nBlockHash.begin (), evmBlock.nBlockHash.end ()); // correct endian
    res.__pushKV("nevm_blockhash", evmBlock.nBlockHash.GetHex());
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
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if(!StopGethNode(gethPID))
        throw JSONRPCError(RPC_MISC_ERROR, "Could not stop Geth");
    UniValue ret(UniValue::VOBJ);
    ret.__pushKV("status", "success");
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
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    StopGethNode(gethPID);
    const std::string gethDescriptorURL = gArgs.GetArg("-gethDescriptorURL", fTestNet || fRegTest? "https://raw.githubusercontent.com/syscoin/descriptors/testnet/gethdescriptor.json": "https://raw.githubusercontent.com/syscoin/descriptors/master/gethdescriptor.json");
    if(!StartGethNode(gethDescriptorURL, gethPID))
        throw JSONRPCError(RPC_MISC_ERROR, "Could not start Geth");
    UniValue ret(UniValue::VOBJ);
    ret.__pushKV("status", "success");
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
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    LOCK(cs_setethstatus);
    std::string blockHashStr = request.params[0].get_str();
    boost::erase_all(blockHashStr, "0x");  // strip 0x
    uint256 nBlockHash;
    if(!ParseHashStr(blockHashStr, nBlockHash)) {
         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Could not read block hash");
    }
    std::pair<std::vector<unsigned char>,std::vector<unsigned char>> vchTxRoots;
    NEVMTxRoot txRootDB;
    if(!pnevmtxrootsdb || !pnevmtxrootsdb->ReadTxRoots(nBlockHash, txRootDB)){
       throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Could not read transaction roots");
    }
      
    UniValue ret(UniValue::VOBJ);  
    ret.pushKV("blockhash", nBlockHash.GetHex());
    ret.pushKV("txroot", HexStr(txRootDB.vchTxRoot));
    ret.pushKV("receiptroot", HexStr(txRootDB.vchReceiptRoot));
    
    return ret;
},
    };
}

static RPCHelpMan syscoincheckmint()
{
    return RPCHelpMan{"syscoincheckmint",
    "\nGet the Syscoin mint transaction by looking up using NEVM TXID.\n",
    {
        {"nevm_txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "NEVM TXID used to burn funds to move to Syscoin."}
    },
    RPCResult{
        RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
        }}, 
    RPCExamples{
        HelpExampleCli("syscoincheckmint", "0xd8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10")
        + HelpExampleRpc("syscoincheckmint", "0xd8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10")
    },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string strTxHash = request.params[0].get_str();
    boost::erase_all(strTxHash, "0x");  // strip 0x
    uint256 sysTxid;
    if(!pnevmtxmintdb || !pnevmtxmintdb->Read(strTxHash, sysTxid)){
       throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Could not read Syscoin transaction using block hash");
    }
    UniValue output(UniValue::VOBJ);
    output.pushKV("txid", sysTxid.GetHex());
    return output;
},
    };
}

static RPCHelpMan syscoinsetethheaders()
{
    return RPCHelpMan{"syscoinsetethheaders",
        "\nSets NEVM headers in Syscoin to validate transactions through the SYSX bridge. Only useful for testing in regtest mode.\n",
        {
            {"headers", RPCArg::Type::ARR, RPCArg::Optional::NO, "An array of arrays (block hashes, tx root) from NEVM blockchain", 
                {
                    {"", RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "An array of [block hashes, tx root] ",
                        {
                            {"block_hash", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Hash of the block"},
                            {"tx_root", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The NEVM TX root of the block height"},
                            {"receipt_root", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The NEVM TX Receipt root of the block height"},
                        }
                    }
                },
                "[blockhash, txroot, txreceiptroot] ..."
            }
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "status", "Result"},
            }},
        RPCExamples{
            HelpExampleCli("syscoinsetethheaders", "\"[[\\\"0xd8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10\\\",\\\"0xd8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10\\\"],...]\"")
            + HelpExampleRpc("syscoinsetethheaders", "\"[[\\\"0xd8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10\\\",\\\"0xd8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10\\\"],...]\"")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if(!fRegTest) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "This function is only available in regtest mode for testing");
    }
    const UniValue &params = request.params;
    LOCK(cs_setethstatus);
    NEVMTxRootMap txRootMap;       
    const UniValue &headerArray = params[0].get_array();
    for(size_t i =0;i<headerArray.size();i++){
        NEVMTxRoot txRoot;
        const UniValue &tupleArray = headerArray[i].get_array();
        if(tupleArray.size() != 3)
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid size in a NEVM header input, should be size of 3");
        std::string blockHashStr = tupleArray[0].get_str();
        uint256 nBlockHash;
        boost::erase_all(blockHashStr, "0x");  // strip 0x
        if(!ParseHashStr(blockHashStr, nBlockHash)) {
             throw JSONRPCError(RPC_INVALID_PARAMS, "Could not parse block hash");
        }
        std::string txRootStr = tupleArray[1].get_str();
        boost::erase_all(txRootStr, "0x");  // strip 0x
        // add RLP header in case it doesn't already have it
        if(txRootStr.find("a0") != 0)
            txRootStr = "a0" + txRootStr;
        txRoot.vchTxRoot = ParseHex(txRootStr);
        std::string txReceiptRoot = tupleArray[2].get_str();
        boost::erase_all(txReceiptRoot, "0x");  // strip 0x
        if(txReceiptRoot.find("a0") != 0)
            txReceiptRoot = "a0" + txReceiptRoot;
        txRoot.vchReceiptRoot = ParseHex(txReceiptRoot);
        txRootMap.try_emplace(nBlockHash, txRoot);
    } 
    bool res = pnevmtxrootsdb->FlushWrite(txRootMap);
    UniValue ret(UniValue::VOBJ);
    ret.__pushKV("status", res? "success": "fail");
    return ret;
},
    };
}

// clang-format on
void RegisterAssetRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                                actor (function)        
    //  --------------------- ------------------------          -----------------------
    { "syscoin",            &syscoingettxroots,             },
    { "syscoin",            &syscoingetspvproof,            },
    { "syscoin",            &convertaddress,                },
    { "syscoin",            &syscoindecoderawtransaction,   },
    { "syscoin",            &assetinfo,                     },
    { "syscoin",            &listassets,                    },
    { "syscoin",            &assetallocationverifyzdag,     },
    { "syscoin",            &syscoinsetethheaders,          },
    { "syscoin",            &syscoinstopgeth,               },
    { "syscoin",            &syscoinstartgeth,              },
    { "syscoin",            &syscoincheckmint,              },
    { "syscoin",            &assettransactionnotarize,      },
    { "syscoin",            &getnotarysighash,              },
};
// clang-format on
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
