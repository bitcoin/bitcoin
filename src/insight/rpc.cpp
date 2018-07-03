// Copyright (c) 2018 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <rpc/util.h>

#include <utilstrencodings.h>
#include <insight/insight.h>
#include <insight/csindex.h>
#include <index/txindex.h>
#include <validation.h>
#include <txmempool.h>
#include <key_io.h>
#include <core_io.h>

#include <univalue.h>

bool getAddressFromIndex(const int &type, const uint256 &hash, std::string &address)
{
    if (type == ADDR_INDT_SCRIPT_ADDRESS) {
        address = CBitcoinAddress(CScriptID(uint160(hash.begin(), 20))).ToString();
    } else if (type == ADDR_INDT_PUBKEY_ADDRESS) {
        address = CBitcoinAddress(CKeyID(uint160(hash.begin(), 20))).ToString();
    } else if (type == ADDR_INDT_SCRIPT_ADDRESS_256) {
        address = CBitcoinAddress(CScriptID256(hash)).ToString();
    } else if (type == ADDR_INDT_PUBKEY_ADDRESS_256) {
        address = CBitcoinAddress(CKeyID256(hash)).ToString();
    } else {
        return false;
    }
    return true;
}

bool getAddressesFromParams(const UniValue& params, std::vector<std::pair<uint256, int> > &addresses)
{
    if (params[0].isStr()) {
        CBitcoinAddress address(params[0].get_str());
        uint256 hashBytes;
        int type = 0;
        if (!address.GetIndexKey(hashBytes, type)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
        }
        addresses.push_back(std::make_pair(hashBytes, type));
    } else
    if (params[0].isObject()) {

        UniValue addressValues = find_value(params[0].get_obj(), "addresses");
        if (!addressValues.isArray()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Addresses is expected to be an array");
        }

        std::vector<UniValue> values = addressValues.getValues();

        for (std::vector<UniValue>::iterator it = values.begin(); it != values.end(); ++it) {
            CBitcoinAddress address(it->get_str());
            uint256 hashBytes;
            int type = 0;
            if (!address.GetIndexKey(hashBytes, type)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }
            addresses.push_back(std::make_pair(hashBytes, type));
        }
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    return true;
}

bool heightSort(std::pair<CAddressUnspentKey, CAddressUnspentValue> a,
                std::pair<CAddressUnspentKey, CAddressUnspentValue> b)
{
    return a.second.blockHeight < b.second.blockHeight;
}

bool timestampSort(std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> a,
                   std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> b)
{
    return a.second.time < b.second.time;
}

UniValue getaddressmempool(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getaddressmempool addresses\n"
            "\nReturns all mempool deltas for an address (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ]\n"
            "}\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"address\"  (string) The base58check encoded address\n"
            "    \"txid\"  (string) The related txid\n"
            "    \"index\"  (number) The related input or output index\n"
            "    \"satoshis\"  (number) The difference of satoshis\n"
            "    \"timestamp\"  (number) The time the transaction entered the mempool (seconds)\n"
            "    \"prevtxid\"  (string) The previous txid (if spending)\n"
            "    \"prevout\"  (string) The previous transaction output index (if spending)\n"
            "  }\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddressmempool", "'{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}'")
            + HelpExampleRpc("getaddressmempool", "{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}")
        );

    if (!fAddressIndex) {
        throw JSONRPCError(RPC_MISC_ERROR, "Address index is not enabled.");
    }

    std::vector<std::pair<uint256, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> > indexes;

    if (!mempool.getAddressIndex(addresses, indexes)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
    }

    std::sort(indexes.begin(), indexes.end(), timestampSort);

    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> >::iterator it = indexes.begin();
         it != indexes.end(); it++) {

        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.addressBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        UniValue delta(UniValue::VOBJ);
        delta.push_back(Pair("address", address));
        delta.push_back(Pair("txid", it->first.txhash.GetHex()));
        delta.push_back(Pair("index", (int)it->first.index));
        delta.push_back(Pair("satoshis", it->second.amount));
        delta.push_back(Pair("timestamp", it->second.time));
        if (it->second.amount < 0) {
            delta.push_back(Pair("prevtxid", it->second.prevhash.GetHex()));
            delta.push_back(Pair("prevout", (int)it->second.prevout));
        }
        result.push_back(delta);
    }

    return result;
}

UniValue getaddressutxos(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getaddressutxos addresses\n"
            "\nReturns all unspent outputs for an address (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ],\n"
            "  \"chainInfo\"  (boolean) Include chain info with results\n"
            "}\n"
            "\nResult\n"
            "[\n"
            "  {\n"
            "    \"address\"  (string) The address base58check encoded\n"
            "    \"txid\"  (string) The output txid\n"
            "    \"height\"  (number) The block height\n"
            "    \"outputIndex\"  (number) The output index\n"
            "    \"script\"  (strin) The script hex encoded\n"
            "    \"satoshis\"  (number) The number of satoshis of the output\n"
            "  }\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddressutxos", "'{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}'")
            + HelpExampleRpc("getaddressutxos", "{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}")
        );

    if (!fAddressIndex) {
        throw JSONRPCError(RPC_MISC_ERROR, "Address index is not enabled.");
    }

    bool includeChainInfo = false;
    if (request.params[0].isObject()) {
        UniValue chainInfo = find_value(request.params[0].get_obj(), "chainInfo");
        if (chainInfo.isBool()) {
            includeChainInfo = chainInfo.get_bool();
        }
    }

    std::vector<std::pair<uint256, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;

    for (std::vector<std::pair<uint256, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (!GetAddressUnspent(it->first, it->second, unspentOutputs)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
        }
    }

    std::sort(unspentOutputs.begin(), unspentOutputs.end(), heightSort);

    UniValue utxos(UniValue::VARR);

    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++) {
        UniValue output(UniValue::VOBJ);
        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.hashBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        output.push_back(Pair("address", address));
        output.push_back(Pair("txid", it->first.txhash.GetHex()));
        output.push_back(Pair("outputIndex", (int)it->first.index));
        output.push_back(Pair("script", HexStr(it->second.script.begin(), it->second.script.end())));
        output.push_back(Pair("satoshis", it->second.satoshis));
        output.push_back(Pair("height", it->second.blockHeight));
        utxos.push_back(output);
    }

    if (includeChainInfo) {
        UniValue result(UniValue::VOBJ);
        result.push_back(Pair("utxos", utxos));

        LOCK(cs_main);
        result.push_back(Pair("hash", chainActive.Tip()->GetBlockHash().GetHex()));
        result.push_back(Pair("height", (int)chainActive.Height()));
        return result;
    } else {
        return utxos;
    }
}

UniValue getaddressdeltas(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1 || !request.params[0].isObject())
        throw std::runtime_error(
            "getaddressdeltas addresses\n"
            "\nReturns all changes for an address (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ]\n"
            "  \"start\" (number) The start block height\n"
            "  \"end\" (number) The end block height\n"
            "  \"chainInfo\" (boolean) Include chain info in results, only applies if start and end specified\n"
            "}\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"satoshis\"  (number) The difference of satoshis\n"
            "    \"txid\"  (string) The related txid\n"
            "    \"index\"  (number) The related input or output index\n"
            "    \"height\"  (number) The block height\n"
            "    \"address\"  (string) The base58check encoded address\n"
            "  }\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddressdeltas", "'{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}'")
            + HelpExampleRpc("getaddressdeltas", "{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}")
        );

    if (!fAddressIndex) {
        throw JSONRPCError(RPC_MISC_ERROR, "Address index is not enabled.");
    }

    UniValue startValue = find_value(request.params[0].get_obj(), "start");
    UniValue endValue = find_value(request.params[0].get_obj(), "end");

    UniValue chainInfo = find_value(request.params[0].get_obj(), "chainInfo");
    bool includeChainInfo = false;
    if (chainInfo.isBool()) {
        includeChainInfo = chainInfo.get_bool();
    }

    int start = 0;
    int end = 0;

    if (startValue.isNum() && endValue.isNum()) {
        start = startValue.get_int();
        end = endValue.get_int();
        if (start <= 0 || end <= 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Start and end is expected to be greater than zero");
        }
        if (end < start) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "End value is expected to be greater than start");
        }
    }

    std::vector<std::pair<uint256, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

    for (std::vector<std::pair<uint256, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (start > 0 && end > 0) {
            if (!GetAddressIndex(it->first, it->second, addressIndex, start, end)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        } else {
            if (!GetAddressIndex(it->first, it->second, addressIndex)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        }
    }

    UniValue deltas(UniValue::VARR);

    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.hashBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        UniValue delta(UniValue::VOBJ);
        delta.push_back(Pair("satoshis", it->second));
        delta.push_back(Pair("txid", it->first.txhash.GetHex()));
        delta.push_back(Pair("index", (int)it->first.index));
        delta.push_back(Pair("blockindex", (int)it->first.txindex));
        delta.push_back(Pair("height", it->first.blockHeight));
        delta.push_back(Pair("address", address));
        deltas.push_back(delta);
    }

    UniValue result(UniValue::VOBJ);

    if (includeChainInfo && start > 0 && end > 0) {
        LOCK(cs_main);

        if (start > chainActive.Height() || end > chainActive.Height()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Start or end is outside chain range");
        }

        CBlockIndex* startIndex = chainActive[start];
        CBlockIndex* endIndex = chainActive[end];

        UniValue startInfo(UniValue::VOBJ);
        UniValue endInfo(UniValue::VOBJ);

        startInfo.push_back(Pair("hash", startIndex->GetBlockHash().GetHex()));
        startInfo.push_back(Pair("height", start));

        endInfo.push_back(Pair("hash", endIndex->GetBlockHash().GetHex()));
        endInfo.push_back(Pair("height", end));

        result.push_back(Pair("deltas", deltas));
        result.push_back(Pair("start", startInfo));
        result.push_back(Pair("end", endInfo));

        return result;
    } else {
        return deltas;
    }
}

UniValue getaddressbalance(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getaddressbalance addresses\n"
            "\nReturns the balance for an address(es) (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ]\n"
            "}\n"
            "\nResult:\n"
            "{\n"
            "  \"balance\"  (string) The current balance in satoshis\n"
            "  \"received\"  (string) The total number of satoshis received (including change)\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddressbalance", "'{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}'")
            + HelpExampleRpc("getaddressbalance", "{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}")
        );

    if (!fAddressIndex) {
        throw JSONRPCError(RPC_MISC_ERROR, "Address index is not enabled.");
    }

    std::vector<std::pair<uint256, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

    for (std::vector<std::pair<uint256, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (!GetAddressIndex(it->first, it->second, addressIndex)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
        }
    }

    CAmount balance = 0;
    CAmount received = 0;

    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        if (it->second > 0) {
            received += it->second;
        }
        balance += it->second;
    }

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("balance", balance));
    result.push_back(Pair("received", received));

    return result;
}

UniValue getaddresstxids(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getaddresstxids addresses\n"
            "\nReturns the txids for an address(es) (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ]\n"
            "  \"start\" (number) The start block height\n"
            "  \"end\" (number) The end block height\n"
            "}\n"
            "\nResult:\n"
            "[\n"
            "  \"transactionid\"  (string) The transaction id\n"
            "  ,...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddresstxids", "'{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}'")
            + HelpExampleRpc("getaddresstxids", "{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}")
        );

    if (!fAddressIndex) {
        throw JSONRPCError(RPC_MISC_ERROR, "Address index is not enabled.");
    }

    std::vector<std::pair<uint256, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    int start = 0;
    int end = 0;
    if (request.params[0].isObject()) {
        UniValue startValue = find_value(request.params[0].get_obj(), "start");
        UniValue endValue = find_value(request.params[0].get_obj(), "end");
        if (startValue.isNum() && endValue.isNum()) {
            start = startValue.get_int();
            end = endValue.get_int();
        }
    }

    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

    for (std::vector<std::pair<uint256, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (start > 0 && end > 0) {
            if (!GetAddressIndex(it->first, it->second, addressIndex, start, end)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        } else {
            if (!GetAddressIndex(it->first, it->second, addressIndex)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        }
    }

    std::set<std::pair<int, std::string> > txids;
    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        int height = it->first.blockHeight;
        std::string txid = it->first.txhash.GetHex();

        if (addresses.size() > 1) {
            txids.insert(std::make_pair(height, txid));
        } else {
            if (txids.insert(std::make_pair(height, txid)).second) {
                result.push_back(txid);
            }
        }
    }

    if (addresses.size() > 1) {
        for (std::set<std::pair<int, std::string> >::const_iterator it=txids.begin(); it!=txids.end(); it++) {
            result.push_back(it->second);
        }
    }

    return result;
}

UniValue getspentinfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1 || !request.params[0].isObject())
        throw std::runtime_error(
            "getspentinfo inputs\n"
            "\nReturns the txid and index where an output is spent.\n"
            "\nArguments:\n"
            "{\n"
            "  \"txid\" (string) The hex string of the txid\n"
            "  \"index\" (number) The start block height\n"
            "}\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\"  (string) The transaction id\n"
            "  \"index\"  (number) The spending input index\n"
            "  ,...\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getspentinfo", "'{\"txid\": \"0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9\", \"index\": 0}'")
            + HelpExampleRpc("getspentinfo", "{\"txid\": \"0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9\", \"index\": 0}")
        );

    UniValue txidValue = find_value(request.params[0].get_obj(), "txid");
    UniValue indexValue = find_value(request.params[0].get_obj(), "index");

    if (!txidValue.isStr() || !indexValue.isNum()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid txid or index");
    }

    uint256 txid = ParseHashV(txidValue, "txid");
    int outputIndex = indexValue.get_int();

    CSpentIndexKey key(txid, outputIndex);
    CSpentIndexValue value;

    if (!GetSpentIndex(key, value)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unable to get spent info");
    }

    UniValue obj(UniValue::VOBJ);
    obj.push_back(Pair("txid", value.txid.GetHex()));
    obj.push_back(Pair("index", (int)value.inputIndex));
    obj.push_back(Pair("height", value.blockHeight));

    return obj;
}

UniValue listcoldstakeunspent(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 3)
        throw std::runtime_error(
            "listcoldstakeunspent \"stakeaddress\" (height, options)\n"
            "\nReturns the unspent outputs of \"stakeaddress\" at height.\n"
            "\nArguments:\n"
            "1. \"stakeaddress\"        (string, required) The stakeaddress to filter outputs by.\n"
            "2. height                (numeric, optional) The block height to return outputs for.\n"
            "3. options               (object, optional)\n"
            "   {\n"
            "     \"mature_only\"         (boolean, optional, default false) Return only outputs stakeable at height.\n"\
            "     \"all_staked\"          (boolean, optional, default false) Ignore maturity check for outputs of coinstake transactions.\n"
            "   }\n"
            "\nResult:\n"
            "[\n"
            " {\n"
            "  \"height\" : n,           (numeric) The height the output was staked into the chain.\n"
            "  \"value\" : n,            (numeric) The value of the output.\n"
            "  \"addrspend\" : \"addr\",   (string) The spending address of the output\n"
            " } ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("listcoldstakeunspent", "\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\" 1000")
            + HelpExampleRpc("listcoldstakeunspent", "\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\", 1000")
        );

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VNUM}, true);

    if (!g_txindex) {
        throw JSONRPCError(RPC_MISC_ERROR, "Requires -txindex enabled");
    }
    if (!g_txindex->m_cs_index) {
        throw JSONRPCError(RPC_MISC_ERROR, "Requires -csindex enabled");
    }

    ColdStakeIndexLinkKey seek_key;
    CTxDestination stake_dest = DecodeDestination(request.params[0].get_str());
    if (stake_dest.type() == typeid(CKeyID)) {
        seek_key.m_stake_type = TX_PUBKEYHASH;
        CKeyID id = boost::get<CKeyID>(stake_dest);
        memcpy(seek_key.m_stake_id.begin(), id.begin(), 20);
    } else
    if (stake_dest.type() == typeid(CKeyID256)) {
        seek_key.m_stake_type = TX_PUBKEYHASH256;
        seek_key.m_stake_id = boost::get<CKeyID256>(stake_dest);
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unrecognised stake address type.");
    }

    CDBWrapper &db = g_txindex->GetDB();

    LOCK(cs_main);

    int height = !request.params[1].isNull() ? request.params[1].get_int() : chainActive.Tip()->nHeight;

    bool mature_only = false;
    bool all_staked = false;
    if (request.params[2].isObject()) {
        const UniValue &options = request.params[2];
        RPCTypeCheck(options, {UniValue::VBOOL, UniValue::VBOOL}, true);
        if (options["mature_only"].isBool()) {
            mature_only = options["mature_only"].get_bool();
        }
        if (options["all_staked"].isBool()) {
            all_staked = options["all_staked"].get_bool();
        }
    }

    UniValue rv(UniValue::VARR);

    std::unique_ptr<CDBIterator> it(db.NewIterator());
    it->Seek(std::make_pair(DB_TXINDEX_CSLINK, seek_key));

    int min_kernel_depth = Params().GetStakeMinConfirmations();
    std::pair<char, ColdStakeIndexLinkKey> key;
    while (it->Valid() && it->GetKey(key)) {
        ColdStakeIndexLinkKey &lk = key.second;

        if (key.first != DB_TXINDEX_CSLINK
            || lk.m_stake_id != seek_key.m_stake_id
            || (int)lk.m_height > height)
            break;

        std::vector <ColdStakeIndexOutputKey> oks;
        ColdStakeIndexOutputValue ov;
        UniValue output(UniValue::VOBJ);

        if (it->GetValue(oks)) {
            for (const auto &ok : oks) {
                if (db.Read(std::make_pair(DB_TXINDEX_CSOUTPUT, ok), ov)
                    && (ov.m_spend_height == -1 || ov.m_spend_height > height)) {

                    if (mature_only
                        && (!all_staked || !(ov.m_flags & CSI_FROM_STAKE))) {
                        int depth = height - lk.m_height;
                        int depth_required = std::min(min_kernel_depth-1, (int)(height / 2));
                        if (depth < depth_required) {
                            it->Next();
                            continue;
                        }
                    }

                    UniValue output(UniValue::VOBJ);
                    output.pushKV("height", (int)lk.m_height);
                    output.pushKV("value", ov.m_value);

                    switch (lk.m_spend_type) {
                        case TX_PUBKEYHASH: {
                            CKeyID idk;
                            memcpy(idk.begin(), lk.m_spend_id.begin(), 20);
                            output.pushKV("addrspend", EncodeDestination(idk));
                            }
                            break;
                        case TX_PUBKEYHASH256:
                            output.pushKV("addrspend", EncodeDestination(lk.m_spend_id));
                            break;
                        case TX_SCRIPTHASH: {
                            CScriptID ids;
                            memcpy(ids.begin(), lk.m_spend_id.begin(), 20);
                            output.pushKV("addrspend", EncodeDestination(ids));
                            }
                            break;
                        case TX_SCRIPTHASH256: {
                            CScriptID256 ids;
                            memcpy(ids.begin(), lk.m_spend_id.begin(), 32);
                            output.pushKV("addrspend", EncodeDestination(ids));
                            }
                            break;
                        default:
                            output.pushKV("addrspend", "unknown_type");
                            break;
                    }

                    rv.push_back(output);
                }
            }
        }
        it->Next();
    }

    return rv;
}

UniValue getblockreward(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getblockreward height\n"
            "\nReturns the blockreward for block at height.\n"
            "\nArguments:\n"
            "1. height                (numeric, required) The height index.\n"
            "\nResult:\n"
            "{\n"
            "  \"blockhash\" : \"id\",     (id) The hash of the block.\n"
            "  \"stakereward\" : n,      (numeric) The stake reward portion, newly minted coin.\n"
            "  \"blockreward\" : n,      (numeric) The block reward, value paid to staker, including fees.\n"
            "  \"foundationreward\" : n, (numeric) The accumulated foundation reward payout, if any.\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getblockreward", "1000")
            + HelpExampleRpc("getblockreward", "1000")
        );

    RPCTypeCheck(request.params, {UniValue::VNUM});

    if (!g_txindex) {
        throw JSONRPCError(RPC_MISC_ERROR, "Requires -txindex enabled");
    }

    int nHeight = request.params[0].get_int();
    if (nHeight < 0 || nHeight > chainActive.Height()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
    }

    LOCK(cs_main);

    CBlockIndex *pblockindex = chainActive[nHeight];

    CAmount stake_reward = 0;
    if (pblockindex->pprev) {
        stake_reward = Params().GetProofOfStakeReward(pblockindex->pprev, 0);
    }

    CBlock block;
    if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) {
        throw JSONRPCError(RPC_MISC_ERROR, "Block not found on disk");
    }

    const DevFundSettings *devfundconf = Params().GetDevFundSettings(pblockindex->GetBlockTime());
    CScript devFundScriptPubKey;
    if (devfundconf) {
        CTxDestination dest = DecodeDestination(devfundconf->sDevFundAddresses);
        devFundScriptPubKey = GetScriptForDestination(dest);
    }

    const auto &tx = block.vtx[0];

    CAmount value_out = 0, value_in = 0, value_foundation = 0;
    for (const auto &txout : tx->vpout) {
        if (!txout->IsStandardOutput()) {
            continue;
        }

        if (devfundconf && *txout->GetPScriptPubKey() == devFundScriptPubKey) {
            value_foundation += txout->GetValue();
            continue;
        }

        value_out += txout->GetValue();
    }

    for (const auto& txin : tx->vin) {
        if (txin.IsAnonInput()) {
            continue;
        }

        CBlockIndex *blockindex = nullptr;
        CTransactionRef tx_prev;
        uint256 hashBlock;
        if (!GetTransaction(txin.prevout.hash, tx_prev, Params().GetConsensus(), hashBlock, true, blockindex)) {
            throw JSONRPCError(RPC_MISC_ERROR, "Transaction not found on disk");
        }
        if (txin.prevout.n > tx_prev->GetNumVOuts()) {
            throw JSONRPCError(RPC_MISC_ERROR, "prevout not found on disk");
        }
        value_in += tx_prev->vpout[txin.prevout.n]->GetValue();
    }

    CAmount block_reward = value_out - value_in;

    UniValue rv(UniValue::VOBJ);
    rv.pushKV("blockhash", pblockindex->GetBlockHash().ToString());
    rv.pushKV("stakereward", ValueFromAmount(stake_reward));
    rv.pushKV("blockreward", ValueFromAmount(block_reward));
    if (value_foundation > 0)
        rv.pushKV("foundationreward", ValueFromAmount(value_foundation));

    return rv;
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
  //  --------------------- ------------------------  -----------------------  ----------
    /* Address index */
    { "addressindex",       "getaddressmempool",      &getaddressmempool,      {"addresses"} },
    { "addressindex",       "getaddressutxos",        &getaddressutxos,        {"addresses"} },
    { "addressindex",       "getaddressdeltas",       &getaddressdeltas,       {"addresses"} },
    { "addressindex",       "getaddresstxids",        &getaddresstxids,        {"addresses"} },
    { "addressindex",       "getaddressbalance",      &getaddressbalance,      {"addresses"} },

    /* Blockchain */
    { "blockchain",         "getspentinfo",           &getspentinfo,           {"inputs"} },

    { "csindex",            "listcoldstakeunspent",   &listcoldstakeunspent,   {"stakeaddress","height","options"} },
    { "blockchain",         "getblockreward",         &getblockreward,         {"height"} },

};

void RegisterInsightRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
