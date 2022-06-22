// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2014-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <index/txindex.h>
#include <init.h>
#include <key_io.h>
#include <keystore.h>
#include <merkleblock.h>
#include <node/coin.h>
#include <node/context.h>
#include <node/transaction.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <primitives/transaction.h>
#include <psbt.h>
#include <rpc/blockchain.h>
#include <rpc/rawtransaction_util.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/standard.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/bip32.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <util/validation.h>
#include <validation.h>
#include <validationinterface.h>

#include <evo/specialtx.h>

#include <llmq/chainlocks.h>
#include <llmq/instantsend.h>

#include <numeric>
#include <stdint.h>

#include <univalue.h>

/** High fee for sendrawtransaction and testmempoolaccept.
 * By default, transaction with a fee higher than this will be rejected by the
 * RPCs. This can be overridden with the maxfeerate argument.
 */
constexpr static CAmount DEFAULT_MAX_RAW_TX_FEE{COIN / 10};

void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry)
{
    // Call into TxToUniv() in bitcoin-common to decode the transaction hex.
    //
    // Blockchain contextual information (confirmations and blocktime) is not
    // available to code in bitcoin-common, so we query them here and push the
    // data into the returned UniValue.

    uint256 txid = tx.GetHash();

    // Add spent information if spentindex is enabled
    CSpentIndexTxInfo txSpentInfo;
    for (const auto& txin : tx.vin) {
        if (!tx.IsCoinBase()) {
            CSpentIndexValue spentInfo;
            CSpentIndexKey spentKey(txin.prevout.hash, txin.prevout.n);
            if (GetSpentIndex(spentKey, spentInfo)) {
                txSpentInfo.mSpentInfo.emplace(spentKey, spentInfo);
            }
        }
    }
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        CSpentIndexValue spentInfo;
        CSpentIndexKey spentKey(txid, i);
        if (GetSpentIndex(spentKey, spentInfo)) {
            txSpentInfo.mSpentInfo.emplace(spentKey, spentInfo);
        }
    }

    TxToUniv(tx, uint256(), entry, true, &txSpentInfo);

    bool chainLock = false;
    if (!hashBlock.IsNull()) {
        LOCK(cs_main);

        entry.pushKV("blockhash", hashBlock.GetHex());
        CBlockIndex* pindex = LookupBlockIndex(hashBlock);
        if (pindex) {
            if (::ChainActive().Contains(pindex)) {
                entry.pushKV("height", pindex->nHeight);
                entry.pushKV("confirmations", 1 + ::ChainActive().Height() - pindex->nHeight);
                entry.pushKV("time", pindex->GetBlockTime());
                entry.pushKV("blocktime", pindex->GetBlockTime());

                chainLock = llmq::chainLocksHandler->HasChainLock(pindex->nHeight, pindex->GetBlockHash());
            } else {
                entry.pushKV("height", -1);
                entry.pushKV("confirmations", 0);
            }
        }
    }

    bool fLocked = llmq::quorumInstantSendManager->IsLocked(txid);
    entry.pushKV("instantlock", fLocked || chainLock);
    entry.pushKV("instantlock_internal", fLocked);
    entry.pushKV("chainlock", chainLock);
}

static UniValue getrawtransaction(const JSONRPCRequest& request)
{
    RPCHelpMan{
                "getrawtransaction",
                "\nReturn the raw transaction data.\n"
                "\nBy default this function only works for mempool transactions. When called with a blockhash\n"
                "argument, getrawtransaction will return the transaction if the specified block is available and\n"
                "the transaction is found in that block. When called without a blockhash argument, getrawtransaction\n"
                "will return the transaction if it is in the mempool, or if -txindex is enabled and the transaction\n"
                "is in a block in the blockchain.\n"
                "\nHint: Use gettransaction for wallet transactions.\n"

            "\nIf verbose is 'true', returns an Object with information about 'txid'.\n"
            "If verbose is 'false' or omitted, returns a string that is serialized, hex-encoded data for 'txid'.\n",
                {
                    {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                    {"verbose", RPCArg::Type::BOOL, /* default */ "false", "If false, return a string, otherwise return a json object"},
                    {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED_NAMED_ARG, "The block in which to look for the transaction"},
                },
                {
                    RPCResult{"if verbose is not set or set to false",
                         RPCResult::Type::STR, "data", "The serialized, hex-encoded data for 'txid'"
                    },
                    RPCResult{"if verbose is set to true",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::BOOL, "in_active_chain", "Whether specified block is in the active chain or not (only present with explicit \"blockhash\" argument)"},
                            {RPCResult::Type::STR_HEX, "txid", "The transaction id (same as provided)"},
                            {RPCResult::Type::NUM, "size", "The serialized transaction size"},
                            {RPCResult::Type::NUM, "version", "The version"},
                            {RPCResult::Type::NUM, "version", "The type"},
                            {RPCResult::Type::NUM_TIME, "locktime", "The lock time"},
                            {RPCResult::Type::ARR, "vin", "",
                            {
                                {RPCResult::Type::OBJ, "", "",
                                {
                                    {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
                                    {RPCResult::Type::STR, "vout", ""},
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
                                        {RPCResult::Type::STR, "hex", "the hex"},
                                        {RPCResult::Type::NUM, "reqSigs", "The required sigs"},
                                        {RPCResult::Type::STR, "type", "The type, eg 'pubkeyhash'"},
                                        {RPCResult::Type::ARR, "addresses", "",
                                        {
                                            {RPCResult::Type::STR, "address", "dash address"},
                                        }},
                                    }},
                                }},
                            }},
                            {RPCResult::Type::NUM, "extraPayloadSize", true /*optional*/, "Size of DIP2 extra payload. Only present if it's a special TX"},
                            {RPCResult::Type::STR_HEX, "extraPayload", true /*optional*/, "Hex-encoded DIP2 extra payload data. Only present if it's a special TX"},
                            {RPCResult::Type::STR_HEX, "hex", "The serialized, hex-encoded data for 'txid'"},
                            {RPCResult::Type::STR_HEX, "blockhash", "the block hash"},
                            {RPCResult::Type::NUM, "height", "The block height"},
                            {RPCResult::Type::NUM, "confirmations", "The confirmations"},
                            {RPCResult::Type::NUM_TIME, "blocktime", "The block time expressed in " + UNIX_EPOCH_TIME},
                            {RPCResult::Type::NUM, "time", "Same as \"blocktime\""},
                            {RPCResult::Type::BOOL, "instantlock", "Current transaction lock state"},
                            {RPCResult::Type::BOOL, "instantlock_internal", "Current internal transaction lock state"},
                            {RPCResult::Type::BOOL, "chainlock", "he state of the corresponding block chainlock"},
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
    }.Check(request);

    const NodeContext& node = EnsureNodeContext(request.context);

    bool in_active_chain = true;
    uint256 hash = ParseHashV(request.params[0], "parameter 1");
    CBlockIndex* blockindex = nullptr;

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
        blockindex = LookupBlockIndex(blockhash);
        if (!blockindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block hash not found");
        }
        in_active_chain = ::ChainActive().Contains(blockindex);
    }

    bool f_txindex_ready = false;
    if (g_txindex && !blockindex) {
        f_txindex_ready = g_txindex->BlockUntilSyncedToCurrentChain();
    }

    uint256 hash_block;
    const CTransactionRef tx = GetTransaction(blockindex, node.mempool, hash, Params().GetConsensus(), hash_block);
    if (!tx) {
        std::string errmsg;
        if (blockindex) {
            if (!(blockindex->nStatus & BLOCK_HAVE_DATA)) {
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

    UniValue result(UniValue::VOBJ);
    if (blockindex) result.pushKV("in_active_chain", in_active_chain);
    TxToJSON(*tx, hash_block, result);
    return result;
}

static UniValue gettxoutproof(const JSONRPCRequest& request)
{
    RPCHelpMan{"gettxoutproof",
        "\nReturns a hex-encoded proof that \"txid\" was included in a block.\n"
        "\nNOTE: By default this function only works sometimes. This is when there is an\n"
        "unspent output in the utxo for this transaction. To make it always work,\n"
        "you need to maintain a transaction index, using the -txindex command line option or\n"
        "specify the block in which the transaction is included manually (by blockhash).\n",
        {
            {"txids", RPCArg::Type::ARR, RPCArg::Optional::NO, "A json array of txids to filter",
                {
                    {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A transaction hash"},
                },
                },
            {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED_NAMED_ARG, "If specified, looks for txid in the block with this hash"},
        },
        RPCResult{
            RPCResult::Type::STR, "data", "A string that is a serialized, hex-encoded data for the proof."
        },
        RPCExamples{
            HelpExampleCli("gettxoutproof", "'[\"mytxid\",...]'")
    + HelpExampleCli("gettxoutproof", "'[\"mytxid\",...]' \"blockhash\"")
    + HelpExampleRpc("gettxoutproof", "[\"mytxid\",...], \"blockhash\"")
        },
    }.Check(request);

    std::set<uint256> setTxids;
    uint256 oneTxid;
    UniValue txids = request.params[0].get_array();
    for (unsigned int idx = 0; idx < txids.size(); idx++) {
        const UniValue& txid = txids[idx];
        if (txid.get_str().length() != 64 || !IsHex(txid.get_str())) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid txid ")+txid.get_str());
        }
        uint256 hash(uint256S(txid.get_str()));
        if (setTxids.count(hash)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, duplicated txid: ")+txid.get_str());
        }
       setTxids.insert(hash);
       oneTxid = hash;
    }

    CBlockIndex* pblockindex = nullptr;
    uint256 hashBlock;
    if (!request.params[1].isNull()) {
        LOCK(cs_main);
        hashBlock = uint256S(request.params[1].get_str());
        pblockindex = LookupBlockIndex(hashBlock);
        if (!pblockindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }
    } else {
        LOCK(cs_main);

        // Loop through txids and try to find which block they're in. Exit loop once a block is found.
        for (const auto& tx : setTxids) {
            const Coin& coin = AccessByTxid(::ChainstateActive().CoinsTip(), tx);
            if (!coin.IsSpent()) {
                pblockindex = ::ChainActive()[coin.nHeight];
                break;
            }
        }
    }


    // Allow txindex to catch up if we need to query it and before we acquire cs_main.
    if (g_txindex && !pblockindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    LOCK(cs_main);

    if (pblockindex == nullptr) {
        const CTransactionRef tx = GetTransaction(/* block_index */ nullptr, /* mempool */ nullptr, oneTxid, Params().GetConsensus(), hashBlock);
        if (!tx || hashBlock.IsNull()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not yet in block");
        }
        pblockindex = LookupBlockIndex(hashBlock);
        if (!pblockindex) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Transaction index corrupt");
        }
    }

    CBlock block;
    if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");
    }

    unsigned int ntxFound = 0;
    for (const auto& tx : block.vtx) {
        if (setTxids.count(tx->GetHash())) {
            ntxFound++;
        }
    }
    if (ntxFound != setTxids.size()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Not all transactions found in specified or retrieved block");
    }

    CDataStream ssMB(SER_NETWORK, PROTOCOL_VERSION);
    CMerkleBlock mb(block, setTxids);
    ssMB << mb;
    std::string strHex = HexStr(ssMB);
    return strHex;
}

static UniValue verifytxoutproof(const JSONRPCRequest& request)
{
    RPCHelpMan{"verifytxoutproof",
        "\nVerifies that a proof points to a transaction in a block, returning the transaction it commits to\n"
        "and throwing an RPC error if the block is not in our best chain\n",
        {
            {"proof", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex-encoded proof generated by gettxoutproof"},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "The txid(s) which the proof commits to, or empty array if the proof can not be validated."},
            }
        },
        RPCExamples{
            HelpExampleCli("verifytxoutproof", "\"proof\"")
    + HelpExampleRpc("gettxoutproof", "\"proof\"")
        },
    }.Check(request);

    CDataStream ssMB(ParseHexV(request.params[0], "proof"), SER_NETWORK, PROTOCOL_VERSION);
    CMerkleBlock merkleBlock;
    ssMB >> merkleBlock;

    UniValue res(UniValue::VARR);

    std::vector<uint256> vMatch;
    std::vector<unsigned int> vIndex;
    if (merkleBlock.txn.ExtractMatches(vMatch, vIndex) != merkleBlock.header.hashMerkleRoot)
        return res;

    LOCK(cs_main);

    const CBlockIndex* pindex = LookupBlockIndex(merkleBlock.header.GetHash());
    if (!pindex || !::ChainActive().Contains(pindex) || pindex->nTx == 0) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found in chain");
    }

    // Check if proof is valid, only add results if so
    if (pindex->nTx == merkleBlock.txn.GetNumTransactions()) {
        for (const uint256& hash : vMatch) {
            res.push_back(hash.GetHex());
        }
    }

    return res;
}

static UniValue createrawtransaction(const JSONRPCRequest& request)
{
    RPCHelpMan{"createrawtransaction",
        "\nCreate a transaction spending the given inputs and creating new outputs.\n"
        "Outputs can be addresses or data.\n"
        "Returns hex-encoded raw transaction.\n"
        "Note that the transaction's inputs are not signed, and\n"
        "it is not stored in the wallet or transmitted to the network.\n",
        {
            {"inputs", RPCArg::Type::ARR, RPCArg::Optional::NO, "A json array of json objects",
                {
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                        {
                            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                            {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                            {"sequence", RPCArg::Type::NUM, /* default */ "", "The sequence number"},
                        },
                        },
                },
                },
            {"outputs", RPCArg::Type::ARR, RPCArg::Optional::NO, "a json array with outputs (key-value pairs), where none of the keys are duplicated.\n"
                    "That is, each address can only appear once and there can only be one 'data' object.\n"
                    "For compatibility reasons, a dictionary, which holds the key-value pairs directly, is also\n"
                    "                             accepted as second parameter.",
                {
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                        {
                            {"address", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "A key-value pair. The key (string) is the dash address, the value (float or string) is the amount in " + CURRENCY_UNIT},
                        },
                        },
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                        {
                            {"data", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "A key-value pair. The key must be \"data\", the value is hex-encoded data"},
                        },
                        },
                },
                },
            {"locktime", RPCArg::Type::NUM, /* default */ "0", "Raw locktime. Non-0 value also locktime-activates inputs"},
        },
        RPCResult{
            RPCResult::Type::STR_HEX, "transaction", "hex string of the transaction"
        },
        RPCExamples{
            HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"[{\\\"address\\\":0.01}]\"")
    + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"[{\\\"data\\\":\\\"00010203\\\"}]\"")
    + HelpExampleRpc("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"[{\\\"address\\\":0.01}]\"")
    + HelpExampleRpc("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"[{\\\"data\\\":\\\"00010203\\\"}]\"")
        },
    }.Check(request);

    RPCTypeCheck(request.params, {
        UniValue::VARR,
        UniValueType(), // ARR or OBJ, checked later
        UniValue::VNUM,
        }, true
    );

    CMutableTransaction rawTx = ConstructTransaction(request.params[0], request.params[1], request.params[2]);

    return EncodeHexTx(CTransaction(rawTx));
}

static UniValue decoderawtransaction(const JSONRPCRequest& request)
{
    RPCHelpMan{"decoderawtransaction",
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
                        {RPCResult::Type::NUM, "version", "The type"},
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
                                    {RPCResult::Type::STR_HEX, "hex", "the hex"},
                                    {RPCResult::Type::NUM, "reqSigs", "The required sigs"},
                                    {RPCResult::Type::STR, "type", "The type, eg 'pubkeyhash'"},
                                    {RPCResult::Type::ARR, "addresses", "",
                                    {
                                        {RPCResult::Type::STR, "address", "dash address"},
                                    }},
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
    }.Check(request);

    RPCTypeCheck(request.params, {UniValue::VSTR});

    CMutableTransaction mtx;

    if (!DecodeHexTx(mtx, request.params[0].get_str()))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");

    UniValue result(UniValue::VOBJ);
    TxToUniv(CTransaction(std::move(mtx)), uint256(), result, false);

    return result;
}

static std::string GetAllOutputTypes()
{
    std::string ret;
    for (int i = TX_NONSTANDARD; i <= TX_NULL_DATA; ++i) {
        if (i != TX_NONSTANDARD) ret += ", ";
        ret += GetTxnOutputType(static_cast<txnouttype>(i));
    }
    return ret;
}

static UniValue decodescript(const JSONRPCRequest& request)
{
    RPCHelpMan{"decodescript",
                "\nDecode a hex-encoded script.\n",
                {
                    {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "the hex-encoded script"},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "asm", "Script public key"},
                        {RPCResult::Type::STR, "type", "The output type (e.g. "+GetAllOutputTypes()+")"},
                        {RPCResult::Type::NUM, "reqSigs", "The required signatures"},
                        {RPCResult::Type::ARR, "addresses", "",
                        {
                            {RPCResult::Type::STR, "address", "dash address"},
                        }},
                        {RPCResult::Type::STR, "p2sh", "address of P2SH script wrapping this redeem script (not returned if the script is already a P2SH)"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("decodescript", "\"hexstring\"")
            + HelpExampleRpc("decodescript", "\"hexstring\"")
                },
    }.Check(request);

    RPCTypeCheck(request.params, {UniValue::VSTR});

    UniValue r(UniValue::VOBJ);
    CScript script;
    if (request.params[0].get_str().size() > 0){
        std::vector<unsigned char> scriptData(ParseHexV(request.params[0], "argument"));
        script = CScript(scriptData.begin(), scriptData.end());
    } else {
        // Empty scripts are valid
    }
    ScriptPubKeyToUniv(script, r, /* fIncludeHex */ false);

    UniValue type;

    type = find_value(r, "type");

    if (type.isStr() && type.get_str() != "scripthash") {
        // P2SH cannot be wrapped in a P2SH. If this script is already a P2SH,
        // don't return the address for a P2SH of the P2SH.
        r.pushKV("p2sh", EncodeDestination(CScriptID(script)));
    }

    return r;
}

static UniValue combinerawtransaction(const JSONRPCRequest& request)
{
    RPCHelpMan{"combinerawtransaction",
        "\nCombine multiple partially signed transactions into one transaction.\n"
        "The combined transaction may be another partially signed transaction or a \n"
        "fully signed transaction.",
        {
            {"txs", RPCArg::Type::ARR, RPCArg::Optional::NO, "A json array of hex strings of partially signed transactions",
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
    }.Check(request);


    UniValue txs = request.params[0].get_array();
    std::vector<CMutableTransaction> txVariants(txs.size());

    for (unsigned int idx = 0; idx < txs.size(); idx++) {
        if (!DecodeHexTx(txVariants[idx], txs[idx].get_str())) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed for tx %d", idx));
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
        const CTxMemPool& mempool = EnsureMemPool(request.context);
        LOCK(cs_main);
        LOCK(mempool.cs);
        CCoinsViewCache &viewChain = ::ChainstateActive().CoinsTip();
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
}

static UniValue signrawtransactionwithkey(const JSONRPCRequest& request)
{
    RPCHelpMan{"signrawtransactionwithkey",
        "\nSign inputs for raw transaction (serialized, hex-encoded).\n"
        "The second argument is an array of base58-encoded private\n"
        "keys that will be the only keys used to sign the transaction.\n"
        "The third optional argument (may be null) is an array of previous transaction outputs that\n"
        "this transaction depends on but may not yet be in the block chain.\n",
        {
            {"hexstring", RPCArg::Type::STR, RPCArg::Optional::NO, "The transaction hex string"},
            {"privkeys", RPCArg::Type::ARR, RPCArg::Optional::NO, "A json array of base58-encoded private keys for signing",
                {
                    {"privatekey", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "private key in base58-encoding"},
                },
                },
            {"prevtxs", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "A json array of previous dependent transaction outputs",
                {
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                        {
                            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                            {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                            {"scriptPubKey", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "script key"},
                            {"redeemScript", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "(required for P2SH or P2WSH) redeem script"},
                            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The amount spent"},
                        },
                    },
                },
            },
            {"sighashtype", RPCArg::Type::STR, /* default */ "ALL", "The signature hash type. Must be one of:\n"
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
                {RPCResult::Type::ARR, "errors", "Script verification errors (if there are any)",
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
    }.Check(request);

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VARR, UniValue::VARR, UniValue::VSTR}, true);

    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    CBasicKeyStore keystore;
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
    NodeContext& node = EnsureNodeContext(request.context);
    FindCoins(node, coins);

    return SignTransaction(mtx, request.params[2], &keystore, coins, true, request.params[3]);
}

UniValue sendrawtransaction(const JSONRPCRequest& request)
{
    RPCHelpMan{"sendrawtransaction",                "\nSubmit a raw transaction (serialized, hex-encoded) to local node and network.\n"
                "\nNote that the transaction will be sent unconditionally to all peers, so using this\n"
                "for manual rebroadcast may degrade privacy by leaking the transaction's origin, as\n"
                "nodes will normally not rebroadcast non-wallet transactions already in their mempool.\n"
                "\nAlso see createrawtransaction and signrawtransactionwithkey calls.\n",
                {
                    {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex string of the raw transaction"},
                    {"maxfeerate", RPCArg::Type::AMOUNT, /* default */ FormatMoney(DEFAULT_MAX_RAW_TX_FEE), "Reject transactions whose fee rate is higher than the specified value, expressed in " + CURRENCY_UNIT + "/kB\n"},
                    {"instantsend", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Deprecated and ignored"},
                    {"bypasslimits", RPCArg::Type::BOOL, /* default_val */ "false", "Bypass transaction policy limits"},
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
    }.Check(request);

    RPCTypeCheck(request.params, {
        UniValue::VSTR,
        UniValueType(), // NUM or BOOL, checked later
        UniValue::VBOOL
    });

    // parse hex string from parameter
    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_str()))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    CTransactionRef tx(MakeTransactionRef(std::move(mtx)));
    CAmount max_raw_tx_fee = DEFAULT_MAX_RAW_TX_FEE;

    // TODO: temporary migration code for old clients. Remove in v0.20
    if (request.params[1].isBool()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Second argument must be numeric (maxfeerate) and no longer supports a boolean. To allow a transaction with high fees, set maxfeerate to 0.");
    } else if (!request.params[1].isNull()) {
        CFeeRate fr(AmountFromValue(request.params[1]));
        max_raw_tx_fee = fr.GetFee(GetVirtualTransactionSize(*tx));
    }

    bool bypass_limits = false;
    if (!request.params[3].isNull()) bypass_limits = request.params[3].get_bool();
    std::string err_string;
    AssertLockNotHeld(cs_main);
    NodeContext& node = EnsureNodeContext(request.context);
    const TransactionError err = BroadcastTransaction(node, tx, err_string, max_raw_tx_fee, /* relay */ true, /* wait_callback */ true, bypass_limits);
    if (TransactionError::OK != err) {
        throw JSONRPCTransactionError(err, err_string);
    }

    return tx->GetHash().GetHex();
}

static UniValue testmempoolaccept(const JSONRPCRequest& request)
{
    RPCHelpMan{"testmempoolaccept",
                "\nReturns result of mempool acceptance tests indicating if raw transaction (serialized, hex-encoded) would be accepted by mempool.\n"
                "\nThis checks if the transaction violates the consensus or policy rules.\n"
                "\nSee sendrawtransaction call.\n",
                {
                    {"rawtxs", RPCArg::Type::ARR, RPCArg::Optional::NO, "An array of hex strings of raw transactions.",
                        {
                            {"rawtx", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, ""},
                        },
                        },
                    {"maxfeerate", RPCArg::Type::AMOUNT, /* default */ FormatMoney(DEFAULT_MAX_RAW_TX_FEE), "Reject transactions whose fee rate is higher than the specified value, expressed in " + CURRENCY_UNIT + "/kB\n"},
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "The result of the mempool acceptance test for each raw transaction in the input array.\n"
                        "Length is exactly one for now.",
                    {
                        {RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR_HEX, "txid", "The transaction hash in hex"},
                            {RPCResult::Type::BOOL, "allowed", "If the mempool allows this tx to be inserted"},
                            {RPCResult::Type::STR, "reject-reason", "Rejection string (only present when 'allowed' is false)"},
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
    }.Check(request);

    RPCTypeCheck(request.params, {
        UniValue::VARR,
        UniValueType(), // NUM or BOOL, checked later
    });

    if (request.params[0].get_array().size() != 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Array must contain exactly one raw transaction for now");
    }

    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_array()[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }
    CTransactionRef tx(MakeTransactionRef(std::move(mtx)));
    const uint256& tx_hash = tx->GetHash();

    CAmount max_raw_tx_fee = DEFAULT_MAX_RAW_TX_FEE;
    // TODO: temporary migration code for old clients. Remove in v0.20
    if (request.params[1].isBool()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Second argument must be numeric (maxfeerate) and no longer supports a boolean. To allow a transaction with high fees, set maxfeerate to 0.");
    } else if (!request.params[1].isNull()) {
        CFeeRate fr(AmountFromValue(request.params[1]));
        max_raw_tx_fee = fr.GetFee(GetVirtualTransactionSize(*tx));
    }

    CTxMemPool& mempool = EnsureMemPool(request.context);

    UniValue result(UniValue::VARR);
    UniValue result_0(UniValue::VOBJ);
    result_0.pushKV("txid", tx_hash.GetHex());

    CValidationState state;
    bool missing_inputs;
    bool test_accept_res;
    {
        LOCK(cs_main);
        test_accept_res = AcceptToMemoryPool(mempool, state, std::move(tx), &missing_inputs,
            false /* bypass_limits */, max_raw_tx_fee, /* test_accept */ true);
    }
    result_0.pushKV("allowed", test_accept_res);
    if (!test_accept_res) {
        if (state.IsInvalid()) {
            result_0.pushKV("reject-reason", strprintf("%i: %s", state.GetRejectCode(), state.GetRejectReason()));
        } else if (missing_inputs) {
            result_0.pushKV("reject-reason", "missing-inputs");
        } else {
            result_0.pushKV("reject-reason", state.GetRejectReason());
        }
    }

    result.push_back(std::move(result_0));
    return result;
}

static std::string WriteHDKeypath(std::vector<uint32_t>& keypath)
{
    std::string keypath_str = "m";
    for (uint32_t num : keypath) {
        keypath_str += "/";
        bool hardened = false;
        if (num & 0x80000000) {
            hardened = true;
            num &= ~0x80000000;
        }

        keypath_str += std::to_string(num);
        if (hardened) {
            keypath_str += "'";
        }
    }
    return keypath_str;
}

UniValue decodepsbt(const JSONRPCRequest& request)
{
    RPCHelpMan{"decodepsbt",
        "\nReturn a JSON object representing the serialized, base64-encoded partially signed Dash transaction.\n",
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
                            {RPCResult::Type::OBJ, "pubkey", /* optional */ true, "The public key with the derivation path as the value.",
                            {
                                {RPCResult::Type::STR, "master_fingerprint", "The fingerprint of the master key"},
                                {RPCResult::Type::STR, "path", "The path"},
                            }},
                        }},
                        {RPCResult::Type::OBJ, "final_scriptsig", /* optional */ true, "",
                        {
                            {RPCResult::Type::STR, "asm", "The asm"},
                            {RPCResult::Type::STR, "hex", "The hex"},
                        }},
                        {RPCResult::Type::OBJ_DYN, "unknown", "The unknown global fields",
                        {
                            {RPCResult::Type::STR_HEX, "key", "(key-value pair) An unknown key-value pair"},
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
                        {RPCResult::Type::OBJ_DYN, "unknown", "The unknown global fields",
                        {
                            {RPCResult::Type::STR_HEX, "key", "(key-value pair) An unknown key-value pair"},
                        }},
                    }},
                }},
                {RPCResult::Type::STR_AMOUNT, "fee", /* optional */ true, "The transaction fee paid if all UTXOs slots in the PSBT have been filled."},
            }
        },
        RPCExamples{
            HelpExampleCli("decodepsbt", "\"psbt\"")
        },
    }.Check(request);

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
    TxToUniv(CTransaction(*psbtx.tx), uint256(), tx_univ, false);
    result.pushKV("tx", tx_univ);

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
        if (input.non_witness_utxo) {
            UniValue non_wit(UniValue::VOBJ);
            TxToUniv(*input.non_witness_utxo, uint256(), non_wit, false);
            in.pushKV("non_witness_utxo", non_wit);
            total_in += input.non_witness_utxo->vout[psbtx.tx->vin[i].prevout.n].nValue;
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
        if (input.sighash_type > 0) {
            in.pushKV("sighash", SighashToStr((unsigned char)input.sighash_type));
        }

        // Redeem script
        if (!input.redeem_script.empty()) {
            UniValue r(UniValue::VOBJ);
            ScriptToUniv(input.redeem_script, r, false);
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
            ScriptToUniv(output.redeem_script, r, false);
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
        output_value += psbtx.tx->vout[i].nValue;
    }
    result.pushKV("outputs", outputs);
    if (have_all_utxos) {
        result.pushKV("fee", ValueFromAmount(total_in - output_value));
    }

    return result;
}

UniValue combinepsbt(const JSONRPCRequest& request)
{
    RPCHelpMan{"combinepsbt",
        "\nCombine multiple partially signed Dash transactions into one transaction.\n"
        "Implements the Combiner role.\n",
        {
            {"txs", RPCArg::Type::ARR, RPCArg::Optional::NO, "A json array of base64 strings of partially signed transactions",
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
    }.Check(request);

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
    return EncodeBase64(ssTx.str());
}

UniValue finalizepsbt(const JSONRPCRequest& request)
{
            RPCHelpMan{"finalizepsbt",
                "Finalize the inputs of a PSBT. If the transaction is fully signed, it will produce a\n"
                "network serialized transaction which can be broadcast with sendrawtransaction. Otherwise a PSBT will be\n"
                "created which has the final_scriptSig field filled for inputs that are complete.\n"
                "Implements the Finalizer and Extractor roles.\n",
                {
                    {"psbt", RPCArg::Type::STR, RPCArg::Optional::NO, "A base64 string of a PSBT"},
                    {"extract", RPCArg::Type::BOOL, /* default */ "true", "If true and the transaction is complete,\n"
            "                             extract and return the complete transaction in normal network serialization instead of the PSBT."},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "psbt", "The base64-encoded partially signed transaction if not extracted"},
                        {RPCResult::Type::STR_HEX, "hex", "The hex-encoded network transaction if extracted"},
                        {RPCResult::Type::BOOL, "complete", "If the transaction has a complete set of signatures"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("finalizepsbt", "\"psbt\"")
                },
            }.Check(request);

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
        result_str = HexStr(ssTx.str());
        result.pushKV("hex", result_str);
    } else {
        ssTx << psbtx;
        result_str = EncodeBase64(ssTx.str());
        result.pushKV("psbt", result_str);
    }
    result.pushKV("complete", complete);

    return result;
}

UniValue createpsbt(const JSONRPCRequest& request)
{
    RPCHelpMan{"createpsbt",
        "\nCreates a transaction in the Partially Signed Transaction format.\n"
        "Implements the Creator role.\n",
        {
            {"inputs", RPCArg::Type::ARR, RPCArg::Optional::NO, "A json array of json objects",
                {
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                        {
                            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                            {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                            {"sequence", RPCArg::Type::NUM, /* default */ "depends on the value of 'locktime' argument", "The sequence number"},
                        },
                        },
                },
                },
            {"outputs", RPCArg::Type::ARR, RPCArg::Optional::NO, "a json array with outputs (key-value pairs), where none of the keys are duplicated.\n"
                    "That is, each address can only appear once and there can only be one 'data' object.\n"
                    "For compatibility reasons, a dictionary, which holds the key-value pairs directly, is also\n"
                    "                             accepted as second parameter.",
                {
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
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
            {"locktime", RPCArg::Type::NUM, /* default */ "0", "Raw locktime. Non-0 value also locktime-activates inputs"},
        },
        RPCResult{
            RPCResult::Type::STR, "", "The resulting raw transaction (base64-encoded string)"
        },
        RPCExamples{
            HelpExampleCli("createpsbt", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"[{\\\"data\\\":\\\"00010203\\\"}]\"")
        },
    }.Check(request);


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

    return EncodeBase64(ssTx.str());
}

UniValue converttopsbt(const JSONRPCRequest& request)
{
    RPCHelpMan{"converttopsbt",
                "\nConverts a network serialized transaction to a PSBT. This should be used only with createrawtransaction and fundrawtransaction\n"
                "createpsbt and walletcreatefundedpsbt should be used for new applications.\n",
                {
                    {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex string of a raw transaction"},
                    {"permitsigdata", RPCArg::Type::BOOL, /* default */ "false", "If true, any signatures in the input will be discarded and conversion.\n"
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
    }.Check(request);

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

    return EncodeBase64(ssTx.str());
}

UniValue utxoupdatepsbt(const JSONRPCRequest& request)
{

    RPCHelpMan{"utxoupdatepsbt",
    "\nUpdates a PSBT with data from output descriptors, UTXOs retrieved from the UTXO set or the mempool.\n",
    {
        {"psbt", RPCArg::Type::STR, RPCArg::Optional::NO, "A base64 string of a PSBT"},
        {"descriptors", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "An array of either strings or objects", {
            {"", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "An output descriptor"},
            {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "An object with an output descriptor and extra information", {
                 {"desc", RPCArg::Type::STR, RPCArg::Optional::NO, "An output descriptor"},
                 {"range", RPCArg::Type::RANGE, "1000", "Up to what index HD chains should be explored (either end or [begin,end])"},
            }},
        }},
    },
    RPCResult {
            RPCResult::Type::STR, "", "The base64-encoded partially signed transaction with inputs updated"
    },
    RPCExamples {
        HelpExampleCli("utxoupdatepsbt", "\"psbt\"")
    }}.Check(request);
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
    HidingSigningProvider public_provider(&provider, /* nosign */ true, /* nobip32derivs */ false);

    // Fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        const CTxMemPool& mempool = EnsureMemPool(request.context);
        LOCK2(cs_main, mempool.cs);
        CCoinsViewCache &viewChain = ::ChainstateActive().CoinsTip();
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        for (const CTxIn& txin : psbtx.tx->vin) {
            view.AccessCoin(txin.prevout); // Load entries from viewChain into view; can fail.
        }

        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }

    // Fill the inputs
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        PSBTInput& input = psbtx.inputs.at(i);

        if (input.non_witness_utxo) {
            continue;
        }

        // Update script/keypath information using descriptor data.
        // Note that SignPSBTInput does a lot more than just constructing ECDSA signatures
        // we don't actually care about those here, in fact.
        SignPSBTInput(public_provider, *psbtx.tx, input, i, /* sighash_type */ 1);
    }

    // Update script/keypath information using descriptor data.
    for (unsigned int i = 0; i < psbtx.tx->vout.size(); ++i) {
        UpdatePSBTOutput(public_provider, psbtx, i);
    }

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << psbtx;
    return EncodeBase64(ssTx.str());
}

UniValue joinpsbts(const JSONRPCRequest& request)
{
    RPCHelpMan{"joinpsbts",
    "\nJoins multiple distinct PSBTs with different inputs and outputs into one PSBT with inputs and outputs from all of the PSBTs\n"
    "No input in any of the PSBTs can be in more than one of the PSBTs.\n",
    {
        {"txs", RPCArg::Type::ARR, RPCArg::Optional::NO, "A json array of base64 strings of partially signed transactions",
            {
                {"psbt", RPCArg::Type::STR, RPCArg::Optional::NO, "A base64 string of a PSBT"}
            }}
    },
    RPCResult {
            RPCResult::Type::STR, "", "The base64-encoded partially signed transaction"
    },
    RPCExamples {
        HelpExampleCli("joinpsbts", "\"psbt\"")
    }}.Check(request);

    RPCTypeCheck(request.params, {UniValue::VARR}, true);

    // Unserialize the transactions
    std::vector<PartiallySignedTransaction> psbtxs;
    UniValue txs = request.params[0].get_array();

    if (txs.size() <= 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "At least two PSBTs are required to join PSBTs.");
    }

    int32_t best_version = 1;
    uint32_t best_locktime = 0xffffffff;
    for (unsigned int i = 0; i < txs.size(); ++i) {
        PartiallySignedTransaction psbtx;
        std::string error;
        if (!DecodeBase64PSBT(psbtx, txs[i].get_str(), error)) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed %s", error));
        }
        psbtxs.push_back(psbtx);
        // Choose the highest version number
        if (psbtx.tx->nVersion > best_version) {
            best_version = psbtx.tx->nVersion;
        }
        // Choose the lowest lock time
        if (psbtx.tx->nLockTime < best_locktime) {
            best_locktime = psbtx.tx->nLockTime;
        }
    }

    // Create a blank psbt where everything will be added
    PartiallySignedTransaction merged_psbt;
    merged_psbt.tx = CMutableTransaction();
    merged_psbt.tx->nVersion = best_version;
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
        merged_psbt.unknown.insert(psbt.unknown.begin(), psbt.unknown.end());
    }

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << merged_psbt;
    return EncodeBase64(ssTx.str());
}

// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                            actor (function)            argNames
  //  --------------------- ------------------------        -----------------------     ----------
    { "rawtransactions",    "getrawtransaction",            &getrawtransaction,         {"txid","verbose","blockhash"} },
    { "rawtransactions",    "createrawtransaction",         &createrawtransaction,      {"inputs","outputs","locktime"} },
    { "rawtransactions",    "decoderawtransaction",         &decoderawtransaction,      {"hexstring"} },
    { "rawtransactions",    "decodescript",                 &decodescript,              {"hexstring"} },
    { "rawtransactions",    "sendrawtransaction",           &sendrawtransaction,        {"hexstring","allowhighfees|maxfeerate","instantsend","bypasslimits"} },
    { "rawtransactions",    "combinerawtransaction",        &combinerawtransaction,     {"txs"} },
    { "rawtransactions",    "signrawtransactionwithkey",    &signrawtransactionwithkey, {"hexstring","privkeys","prevtxs","sighashtype"} },
    { "rawtransactions",    "testmempoolaccept",            &testmempoolaccept,         {"rawtxs","allowhighfees|maxfeerate"} },
    { "rawtransactions",    "decodepsbt",                   &decodepsbt,                {"psbt"} },
    { "rawtransactions",    "combinepsbt",                  &combinepsbt,               {"txs"} },
    { "rawtransactions",    "finalizepsbt",                 &finalizepsbt,              {"psbt", "extract"} },
    { "rawtransactions",    "createpsbt",                   &createpsbt,                {"inputs","outputs","locktime"} },
    { "rawtransactions",    "converttopsbt",                &converttopsbt,             {"hexstring","permitsigdata"} },
    { "rawtransactions",    "utxoupdatepsbt",               &utxoupdatepsbt,            {"psbt"} },
    { "rawtransactions",    "joinpsbts",                    &joinpsbts,                 {"txs"} },

    { "blockchain",         "gettxoutproof",                &gettxoutproof,             {"txids", "blockhash"} },
    { "blockchain",         "verifytxoutproof",             &verifytxoutproof,          {"proof"} },
};
// clang-format on

void RegisterRawTransactionRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
