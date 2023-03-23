// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/client.h>
#include <tinyformat.h>
#include <util/system.h>

#include <set>
#include <stdint.h>
#include <string>
#include <string_view>

class CRPCConvertParam
{
public:
    std::string methodName; //!< method whose params want conversion
    int paramIdx;           //!< 0-based idx of param to convert
    std::string paramName;  //!< parameter name
};

// clang-format off
/**
 * Specify a (method, idx, name) here if the argument is a non-string RPC
 * argument and needs to be converted from JSON.
 *
 * @note Parameter indexes start from 0.
 */
static const CRPCConvertParam vRPCConvertParams[] =
{
    { "setmocktime", 0, "timestamp" },
    { "mockscheduler", 0, "delta_time" },
    { "utxoupdatepsbt", 1, "descriptors" },
    { "generatetoaddress", 0, "nblocks" },
    { "generatetoaddress", 2, "maxtries" },
    { "generatetodescriptor", 0, "num_blocks" },
    { "generatetodescriptor", 2, "maxtries" },
    { "generateblock", 1, "transactions" },
    { "generateblock", 2, "submit" },
    { "getnetworkhashps", 0, "nblocks" },
    { "getnetworkhashps", 1, "height" },
    { "sendtoaddress", 1, "amount" },
    { "sendtoaddress", 4, "subtractfeefromamount" },
    { "sendtoaddress", 5 , "replaceable" },
    { "sendtoaddress", 6 , "conf_target" },
    { "sendtoaddress", 8, "avoid_reuse" },
    { "sendtoaddress", 9, "fee_rate"},
    { "sendtoaddress", 10, "verbose"},
    { "settxfee", 0, "amount" },
    { "sethdseed", 0, "newkeypool" },
    { "getreceivedbyaddress", 1, "minconf" },
    { "getreceivedbyaddress", 2, "include_immature_coinbase" },
    { "getreceivedbylabel", 1, "minconf" },
    { "getreceivedbylabel", 2, "include_immature_coinbase" },
    { "listreceivedbyaddress", 0, "minconf" },
    { "listreceivedbyaddress", 1, "include_empty" },
    { "listreceivedbyaddress", 2, "include_watchonly" },
    { "listreceivedbyaddress", 4, "include_immature_coinbase" },
    { "listreceivedbylabel", 0, "minconf" },
    { "listreceivedbylabel", 1, "include_empty" },
    { "listreceivedbylabel", 2, "include_watchonly" },
    { "listreceivedbylabel", 3, "include_immature_coinbase" },
    { "getbalance", 1, "minconf" },
    { "getbalance", 2, "include_watchonly" },
    { "getbalance", 3, "avoid_reuse" },
    { "getblockfrompeer", 1, "peer_id" },
    { "getblockhash", 0, "height" },
    { "waitforblockheight", 0, "height" },
    { "waitforblockheight", 1, "timeout" },
    { "waitforblock", 1, "timeout" },
    { "waitfornewblock", 0, "timeout" },
    { "listtransactions", 1, "count" },
    { "listtransactions", 2, "skip" },
    { "listtransactions", 3, "include_watchonly" },
    { "walletpassphrase", 1, "timeout" },
    { "getblocktemplate", 0, "template_request" },
    { "listsinceblock", 1, "target_confirmations" },
    { "listsinceblock", 2, "include_watchonly" },
    { "listsinceblock", 3, "include_removed" },
    { "listsinceblock", 4, "include_change" },
    { "sendmany", 1, "amounts" },
    { "sendmany", 2, "minconf" },
    { "sendmany", 4, "subtractfeefrom" },
    { "sendmany", 5 , "replaceable" },
    { "sendmany", 6 , "conf_target" },
    { "sendmany", 8, "fee_rate"},
    { "sendmany", 9, "verbose" },
    { "deriveaddresses", 1, "range" },
    { "scanblocks", 1, "scanobjects" },
    { "scanblocks", 2, "start_height" },
    { "scanblocks", 3, "stop_height" },
    { "scanblocks", 5, "options" },
    { "scantxoutset", 1, "scanobjects" },
    { "addmultisigaddress", 0, "nrequired" },
    { "addmultisigaddress", 1, "keys" },
    { "createmultisig", 0, "nrequired" },
    { "createmultisig", 1, "keys" },
    { "listunspent", 0, "minconf" },
    { "listunspent", 1, "maxconf" },
    { "listunspent", 2, "addresses" },
    { "listunspent", 3, "include_unsafe" },
    { "listunspent", 4, "query_options" },
    { "getblock", 1, "verbosity" },
    { "getblock", 1, "verbose" },
    { "getblockheader", 1, "verbose" },
    { "getchaintxstats", 0, "nblocks" },
    { "gettransaction", 1, "include_watchonly" },
    { "gettransaction", 2, "verbose" },
    { "getrawtransaction", 1, "verbosity" },
    { "getrawtransaction", 1, "verbose" },
    { "createrawtransaction", 0, "inputs" },
    { "createrawtransaction", 1, "outputs" },
    { "createrawtransaction", 2, "locktime" },
    { "createrawtransaction", 3, "replaceable" },
    { "decoderawtransaction", 1, "iswitness" },
    { "signrawtransactionwithkey", 1, "privkeys" },
    { "signrawtransactionwithkey", 2, "prevtxs" },
    { "signrawtransactionwithwallet", 1, "prevtxs" },
    { "sendrawtransaction", 1, "maxfeerate" },
    { "sendrawtransaction", 2, "maxburnamount" },
    { "testmempoolaccept", 0, "rawtxs" },
    { "testmempoolaccept", 1, "maxfeerate" },
    { "submitpackage", 0, "package" },
    { "combinerawtransaction", 0, "txs" },
    { "fundrawtransaction", 1, "options" },
    { "fundrawtransaction", 2, "iswitness" },
    { "walletcreatefundedpsbt", 0, "inputs" },
    { "walletcreatefundedpsbt", 1, "outputs" },
    { "walletcreatefundedpsbt", 2, "locktime" },
    { "walletcreatefundedpsbt", 3, "options" },
    { "walletcreatefundedpsbt", 4, "bip32derivs" },
    { "walletprocesspsbt", 1, "sign" },
    { "walletprocesspsbt", 3, "bip32derivs" },
    { "walletprocesspsbt", 4, "finalize" },
    { "createpsbt", 0, "inputs" },
    { "createpsbt", 1, "outputs" },
    { "createpsbt", 2, "locktime" },
    { "createpsbt", 3, "replaceable" },
    { "combinepsbt", 0, "txs"},
    { "joinpsbts", 0, "txs"},
    { "finalizepsbt", 1, "extract"},
    { "converttopsbt", 1, "permitsigdata"},
    { "converttopsbt", 2, "iswitness"},
    { "gettxout", 1, "n" },
    { "gettxout", 2, "include_mempool" },
    { "gettxoutproof", 0, "txids" },
    { "gettxoutsetinfo", 1, "hash_or_height" },
    { "gettxoutsetinfo", 2, "use_index"},
    { "lockunspent", 0, "unlock" },
    { "lockunspent", 1, "transactions" },
    { "lockunspent", 2, "persistent" },
    { "send", 0, "outputs" },
    { "send", 1, "conf_target" },
    { "send", 3, "fee_rate"},
    { "send", 4, "options" },
    { "sendall", 0, "recipients" },
    { "sendall", 1, "conf_target" },
    { "sendall", 3, "fee_rate"},
    { "sendall", 4, "options" },
    { "simulaterawtransaction", 0, "rawtxs" },
    { "simulaterawtransaction", 1, "options" },
    { "importprivkey", 2, "rescan" },
    { "importaddress", 2, "rescan" },
    { "importaddress", 3, "p2sh" },
    { "importpubkey", 2, "rescan" },
    { "importmulti", 0, "requests" },
    { "importmulti", 1, "options" },
    { "importdescriptors", 0, "requests" },
    { "listdescriptors", 0, "private" },
    { "verifychain", 0, "checklevel" },
    { "verifychain", 1, "nblocks" },
    { "getblockstats", 0, "hash_or_height" },
    { "getblockstats", 1, "stats" },
    { "pruneblockchain", 0, "height" },
    { "keypoolrefill", 0, "newsize" },
    { "getrawmempool", 0, "verbose" },
    { "getrawmempool", 1, "mempool_sequence" },
    { "estimatesmartfee", 0, "conf_target" },
    { "estimaterawfee", 0, "conf_target" },
    { "estimaterawfee", 1, "threshold" },
    { "prioritisetransaction", 1, "dummy" },
    { "prioritisetransaction", 2, "fee_delta" },
    { "setban", 2, "bantime" },
    { "setban", 3, "absolute" },
    { "setnetworkactive", 0, "state" },
    { "setwalletflag", 1, "value" },
    { "getmempoolancestors", 1, "verbose" },
    { "getmempooldescendants", 1, "verbose" },
    { "gettxspendingprevout", 0, "outputs" },
    { "bumpfee", 1, "options" },
    { "psbtbumpfee", 1, "options" },
    { "logging", 0, "include" },
    { "logging", 1, "exclude" },
    { "disconnectnode", 1, "nodeid" },
    { "upgradewallet", 0, "version" },
    // Echo with conversion (For testing only)
    { "echojson", 0, "arg0" },
    { "echojson", 1, "arg1" },
    { "echojson", 2, "arg2" },
    { "echojson", 3, "arg3" },
    { "echojson", 4, "arg4" },
    { "echojson", 5, "arg5" },
    { "echojson", 6, "arg6" },
    { "echojson", 7, "arg7" },
    { "echojson", 8, "arg8" },
    { "echojson", 9, "arg9" },
    { "rescanblockchain", 0, "start_height"},
    { "rescanblockchain", 1, "stop_height"},
    { "createwallet", 1, "disable_private_keys"},
    { "createwallet", 2, "blank"},
    { "createwallet", 4, "avoid_reuse"},
    { "createwallet", 5, "descriptors"},
    { "createwallet", 6, "load_on_startup"},
    { "createwallet", 7, "external_signer"},
    { "restorewallet", 2, "load_on_startup"},
    { "loadwallet", 1, "load_on_startup"},
    { "unloadwallet", 1, "load_on_startup"},
    { "getnodeaddresses", 0, "count"},
    { "addpeeraddress", 1, "port"},
    { "addpeeraddress", 2, "tried"},
    { "stop", 0, "wait" },
};
// clang-format on

class CRPCConvertTable
{
private:
    std::set<std::pair<std::string, int>> members;
    std::set<std::pair<std::string, std::string>> membersByName;

public:
    CRPCConvertTable();

    /** Return arg_value as UniValue, and first parse it if it is a non-string parameter */
    UniValue ArgToUniValue(std::string_view arg_value, const std::string& method, int param_idx)
    {
        return members.count({method, param_idx}) > 0 ? ParseNonRFCJSONValue(arg_value) : arg_value;
    }

    /** Return arg_value as UniValue, and first parse it if it is a non-string parameter */
    UniValue ArgToUniValue(std::string_view arg_value, const std::string& method, const std::string& param_name)
    {
        return membersByName.count({method, param_name}) > 0 ? ParseNonRFCJSONValue(arg_value) : arg_value;
    }
};

CRPCConvertTable::CRPCConvertTable()
{
    for (const auto& cp : vRPCConvertParams) {
        members.emplace(cp.methodName, cp.paramIdx);
        membersByName.emplace(cp.methodName, cp.paramName);
    }
}

static CRPCConvertTable rpcCvtTable;

/** Non-RFC4627 JSON parser, accepts internal values (such as numbers, true, false, null)
 * as well as objects and arrays.
 */
UniValue ParseNonRFCJSONValue(std::string_view raw)
{
    UniValue parsed;
    if (!parsed.read(raw)) throw std::runtime_error(tfm::format("Error parsing JSON: %s", raw));
    return parsed;
}

UniValue RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    UniValue params(UniValue::VARR);

    for (unsigned int idx = 0; idx < strParams.size(); idx++) {
        std::string_view value{strParams[idx]};
        params.push_back(rpcCvtTable.ArgToUniValue(value, strMethod, idx));
    }

    return params;
}

UniValue RPCConvertNamedValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    UniValue params(UniValue::VOBJ);
    UniValue positional_args{UniValue::VARR};

    for (std::string_view s: strParams) {
        size_t pos = s.find('=');
        if (pos == std::string::npos) {
            positional_args.push_back(rpcCvtTable.ArgToUniValue(s, strMethod, positional_args.size()));
            continue;
        }

        std::string name{s.substr(0, pos)};
        std::string_view value{s.substr(pos+1)};

        // Intentionally overwrite earlier named values with later ones as a
        // convenience for scripts and command line users that want to merge
        // options.
        params.pushKV(name, rpcCvtTable.ArgToUniValue(value, strMethod, name));
    }

    if (!positional_args.empty()) {
        // Use __pushKV instead of pushKV to avoid overwriting an explicit
        // "args" value with an implicit one. Let the RPC server handle the
        // request as given.
        params.__pushKV("args", positional_args);
    }

    return params;
}
