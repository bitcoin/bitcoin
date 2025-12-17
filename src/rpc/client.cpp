// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <rpc/client.h>
#include <tinyformat.h>

#include <cstdint>
#include <set>
#include <string>
#include <string_view>

//! Specify whether parameter should be parsed by bitcoin-cli as a JSON value,
//! or passed unchanged as a string, or a combination of both.
enum ParamFormat { JSON, STRING, JSON_OR_STRING };

class CRPCConvertParam
{
public:
    std::string methodName; //!< method whose params want conversion
    int paramIdx;           //!< 0-based idx of param to convert
    std::string paramName;  //!< parameter name
    ParamFormat format{ParamFormat::JSON}; //!< parameter format
};

// clang-format off
/**
 * Specify a (method, idx, name, format) here if the argument is a non-string RPC
 * argument and needs to be converted from JSON, or if it is a string argument
 * passed to a method that accepts '=' characters in any string arguments.
 *
 * JSON parameters need to be listed here to make bitcoin-cli parse command line
 * arguments as JSON, instead of passing them as raw strings. `JSON` and
 * `JSON_OR_STRING` formats both make `bitcoin-cli` attempt to parse the
 * argument as JSON. But if parsing fails, the former triggers an error while
 * the latter falls back to passing the argument as a raw string. This is
 * useful for arguments like hash_or_height, allowing invocations such as
 * `bitcoin-cli getblockstats <hash>` without needing to quote the hash string
 * as JSON (`'"<hash>"'`).
 *
 * String parameters that may contain an '=' character (e.g. base64 strings,
 * filenames, or labels) need to be listed here with format `ParamFormat::STRING`
 * to make bitcoin-cli treat them as positional parameters when `-named` is used.
 * This prevents `bitcoin-cli` from splitting strings like "my=wallet" into a named
 * argument "my" and value "wallet" when the whole string is intended to be a
 * single positional argument. And if one string parameter is listed for a method,
 * other string parameters for that method need to be listed as well so bitcoin-cli
 * does not make the opposite mistake and pass other arguments by position instead of
 * name because it does not recognize their names. See \ref RPCConvertNamedValues
 * for more information on how named and positional arguments are distinguished with
 * -named.
 *
 * @note Parameter indexes start from 0.
 */
static const CRPCConvertParam vRPCConvertParams[] =
{
    { "setmocktime", 0, "timestamp" },
    { "mockscheduler", 0, "delta_time" },
    { "utxoupdatepsbt", 0, "psbt", ParamFormat::STRING },
    { "utxoupdatepsbt", 1, "descriptors" },
    { "generatetoaddress", 0, "nblocks" },
    { "generatetoaddress", 2, "maxtries" },
    { "generatetodescriptor", 0, "num_blocks" },
    { "generatetodescriptor", 2, "maxtries" },
    { "generateblock", 1, "transactions" },
    { "generateblock", 2, "submit" },
    { "getnetworkhashps", 0, "nblocks" },
    { "getnetworkhashps", 1, "height" },
    { "sendtoaddress", 0, "address", ParamFormat::STRING },
    { "sendtoaddress", 1, "amount" },
    { "sendtoaddress", 2, "comment", ParamFormat::STRING },
    { "sendtoaddress", 3, "comment_to", ParamFormat::STRING },
    { "sendtoaddress", 4, "subtractfeefromamount" },
    { "sendtoaddress", 5 , "replaceable" },
    { "sendtoaddress", 6 , "conf_target" },
    { "sendtoaddress", 7, "estimate_mode", ParamFormat::STRING },
    { "sendtoaddress", 8, "avoid_reuse" },
    { "sendtoaddress", 9, "fee_rate"},
    { "sendtoaddress", 10, "verbose"},
    { "settxfee", 0, "amount" },
    { "getreceivedbyaddress", 1, "minconf" },
    { "getreceivedbyaddress", 2, "include_immature_coinbase" },
    { "getreceivedbylabel", 0, "label", ParamFormat::STRING },
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
    { "listtransactions", 0, "label", ParamFormat::STRING },
    { "listtransactions", 1, "count" },
    { "listtransactions", 2, "skip" },
    { "listtransactions", 3, "include_watchonly" },
    { "walletpassphrase", 0, "passphrase", ParamFormat::STRING },
    { "walletpassphrase", 1, "timeout" },
    { "getblocktemplate", 0, "template_request" },
    { "listsinceblock", 0, "blockhash", ParamFormat::STRING },
    { "listsinceblock", 1, "target_confirmations" },
    { "listsinceblock", 2, "include_watchonly" },
    { "listsinceblock", 3, "include_removed" },
    { "listsinceblock", 4, "include_change" },
    { "listsinceblock", 5, "label", ParamFormat::STRING },
    { "sendmany", 0, "dummy", ParamFormat::STRING },
    { "sendmany", 1, "amounts" },
    { "sendmany", 2, "minconf" },
    { "sendmany", 3, "comment", ParamFormat::STRING },
    { "sendmany", 4, "subtractfeefrom" },
    { "sendmany", 5 , "replaceable" },
    { "sendmany", 6 , "conf_target" },
    { "sendmany", 7, "estimate_mode", ParamFormat::STRING },
    { "sendmany", 8, "fee_rate"},
    { "sendmany", 9, "verbose" },
    { "deriveaddresses", 1, "range" },
    { "scanblocks", 1, "scanobjects" },
    { "scanblocks", 2, "start_height" },
    { "scanblocks", 3, "stop_height" },
    { "scanblocks", 5, "options" },
    { "scanblocks", 5, "filter_false_positives" },
    { "getdescriptoractivity", 0, "blockhashes" },
    { "getdescriptoractivity", 1, "scanobjects" },
    { "getdescriptoractivity", 2, "include_mempool" },
    { "scantxoutset", 1, "scanobjects" },
    { "createmultisig", 0, "nrequired" },
    { "createmultisig", 1, "keys" },
    { "listunspent", 0, "minconf" },
    { "listunspent", 1, "maxconf" },
    { "listunspent", 2, "addresses" },
    { "listunspent", 3, "include_unsafe" },
    { "listunspent", 4, "query_options" },
    { "listunspent", 4, "minimumAmount" },
    { "listunspent", 4, "maximumAmount" },
    { "listunspent", 4, "maximumCount" },
    { "listunspent", 4, "minimumSumAmount" },
    { "listunspent", 4, "include_immature_coinbase" },
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
    { "createrawtransaction", 4, "version" },
    { "decoderawtransaction", 1, "iswitness" },
    { "signrawtransactionwithkey", 1, "privkeys" },
    { "signrawtransactionwithkey", 2, "prevtxs" },
    { "signrawtransactionwithwallet", 1, "prevtxs" },
    { "sendrawtransaction", 1, "maxfeerate" },
    { "sendrawtransaction", 2, "maxburnamount" },
    { "testmempoolaccept", 0, "rawtxs" },
    { "testmempoolaccept", 1, "maxfeerate" },
    { "submitpackage", 0, "package" },
    { "submitpackage", 1, "maxfeerate" },
    { "submitpackage", 2, "maxburnamount" },
    { "combinerawtransaction", 0, "txs" },
    { "fundrawtransaction", 1, "options" },
    { "fundrawtransaction", 1, "add_inputs"},
    { "fundrawtransaction", 1, "include_unsafe"},
    { "fundrawtransaction", 1, "minconf"},
    { "fundrawtransaction", 1, "maxconf"},
    { "fundrawtransaction", 1, "changePosition"},
    { "fundrawtransaction", 1, "includeWatching"},
    { "fundrawtransaction", 1, "lockUnspents"},
    { "fundrawtransaction", 1, "fee_rate"},
    { "fundrawtransaction", 1, "feeRate"},
    { "fundrawtransaction", 1, "subtractFeeFromOutputs"},
    { "fundrawtransaction", 1, "input_weights"},
    { "fundrawtransaction", 1, "conf_target"},
    { "fundrawtransaction", 1, "replaceable"},
    { "fundrawtransaction", 1, "solving_data"},
    { "fundrawtransaction", 1, "max_tx_weight"},
    { "fundrawtransaction", 2, "iswitness" },
    { "walletcreatefundedpsbt", 0, "inputs" },
    { "walletcreatefundedpsbt", 1, "outputs" },
    { "walletcreatefundedpsbt", 2, "locktime" },
    { "walletcreatefundedpsbt", 3, "options" },
    { "walletcreatefundedpsbt", 3, "add_inputs"},
    { "walletcreatefundedpsbt", 3, "include_unsafe"},
    { "walletcreatefundedpsbt", 3, "minconf"},
    { "walletcreatefundedpsbt", 3, "maxconf"},
    { "walletcreatefundedpsbt", 3, "changePosition"},
    { "walletcreatefundedpsbt", 3, "includeWatching"},
    { "walletcreatefundedpsbt", 3, "lockUnspents"},
    { "walletcreatefundedpsbt", 3, "fee_rate"},
    { "walletcreatefundedpsbt", 3, "feeRate"},
    { "walletcreatefundedpsbt", 3, "subtractFeeFromOutputs"},
    { "walletcreatefundedpsbt", 3, "conf_target"},
    { "walletcreatefundedpsbt", 3, "replaceable"},
    { "walletcreatefundedpsbt", 3, "solving_data"},
    { "walletcreatefundedpsbt", 3, "max_tx_weight"},
    { "walletcreatefundedpsbt", 4, "bip32derivs" },
    { "walletcreatefundedpsbt", 5, "version" },
    { "walletprocesspsbt", 0, "psbt", ParamFormat::STRING },
    { "walletprocesspsbt", 1, "sign" },
    { "walletprocesspsbt", 2, "sighashtype", ParamFormat::STRING },
    { "walletprocesspsbt", 3, "bip32derivs" },
    { "walletprocesspsbt", 4, "finalize" },
    { "descriptorprocesspsbt", 0, "psbt", ParamFormat::STRING },
    { "descriptorprocesspsbt", 1, "descriptors"},
    { "descriptorprocesspsbt", 2, "sighashtype", ParamFormat::STRING },
    { "descriptorprocesspsbt", 3, "bip32derivs" },
    { "descriptorprocesspsbt", 4, "finalize" },
    { "createpsbt", 0, "inputs" },
    { "createpsbt", 1, "outputs" },
    { "createpsbt", 2, "locktime" },
    { "createpsbt", 3, "replaceable" },
    { "createpsbt", 4, "version" },
    { "combinepsbt", 0, "txs"},
    { "joinpsbts", 0, "txs"},
    { "finalizepsbt", 0, "psbt", ParamFormat::STRING },
    { "finalizepsbt", 1, "extract"},
    { "converttopsbt", 1, "permitsigdata"},
    { "converttopsbt", 2, "iswitness"},
    { "gettxout", 1, "n" },
    { "gettxout", 2, "include_mempool" },
    { "gettxoutproof", 0, "txids" },
    { "gettxoutsetinfo", 1, "hash_or_height", ParamFormat::JSON_OR_STRING },
    { "gettxoutsetinfo", 2, "use_index"},
    { "dumptxoutset", 0, "path", ParamFormat::STRING },
    { "dumptxoutset", 1, "type", ParamFormat::STRING },
    { "dumptxoutset", 2, "options" },
    { "dumptxoutset", 2, "rollback", ParamFormat::JSON_OR_STRING },
    { "lockunspent", 0, "unlock" },
    { "lockunspent", 1, "transactions" },
    { "lockunspent", 2, "persistent" },
    { "send", 0, "outputs" },
    { "send", 1, "conf_target" },
    { "send", 3, "fee_rate"},
    { "send", 4, "options" },
    { "send", 4, "add_inputs"},
    { "send", 4, "include_unsafe"},
    { "send", 4, "minconf"},
    { "send", 4, "maxconf"},
    { "send", 4, "add_to_wallet"},
    { "send", 4, "change_position"},
    { "send", 4, "fee_rate"},
    { "send", 4, "include_watching"},
    { "send", 4, "inputs"},
    { "send", 4, "locktime"},
    { "send", 4, "lock_unspents"},
    { "send", 4, "psbt"},
    { "send", 4, "subtract_fee_from_outputs"},
    { "send", 4, "conf_target"},
    { "send", 4, "replaceable"},
    { "send", 4, "solving_data"},
    { "send", 4, "max_tx_weight"},
    { "send", 5, "version"},
    { "sendall", 0, "recipients" },
    { "sendall", 1, "conf_target" },
    { "sendall", 3, "fee_rate"},
    { "sendall", 4, "options" },
    { "sendall", 4, "add_to_wallet"},
    { "sendall", 4, "fee_rate"},
    { "sendall", 4, "include_watching"},
    { "sendall", 4, "inputs"},
    { "sendall", 4, "locktime"},
    { "sendall", 4, "lock_unspents"},
    { "sendall", 4, "psbt"},
    { "sendall", 4, "send_max"},
    { "sendall", 4, "minconf"},
    { "sendall", 4, "maxconf"},
    { "sendall", 4, "conf_target"},
    { "sendall", 4, "replaceable"},
    { "sendall", 4, "solving_data"},
    { "sendall", 4, "version"},
    { "simulaterawtransaction", 0, "rawtxs" },
    { "simulaterawtransaction", 1, "options" },
    { "simulaterawtransaction", 1, "include_watchonly"},
    { "importmempool", 0, "filepath", ParamFormat::STRING },
    { "importmempool", 1, "options" },
    { "importmempool", 1, "apply_fee_delta_priority" },
    { "importmempool", 1, "use_current_time" },
    { "importmempool", 1, "apply_unbroadcast_set" },
    { "importdescriptors", 0, "requests" },
    { "listdescriptors", 0, "private" },
    { "verifychain", 0, "checklevel" },
    { "verifychain", 1, "nblocks" },
    { "getblockstats", 0, "hash_or_height", ParamFormat::JSON_OR_STRING },
    { "getblockstats", 1, "stats" },
    { "pruneblockchain", 0, "height" },
    { "keypoolrefill", 0, "newsize" },
    { "getrawmempool", 0, "verbose" },
    { "getrawmempool", 1, "mempool_sequence" },
    { "getorphantxs", 0, "verbosity" },
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
    { "bumpfee", 1, "conf_target"},
    { "bumpfee", 1, "fee_rate"},
    { "bumpfee", 1, "replaceable"},
    { "bumpfee", 1, "outputs"},
    { "bumpfee", 1, "original_change_index"},
    { "psbtbumpfee", 1, "options" },
    { "psbtbumpfee", 1, "conf_target"},
    { "psbtbumpfee", 1, "fee_rate"},
    { "psbtbumpfee", 1, "replaceable"},
    { "psbtbumpfee", 1, "outputs"},
    { "psbtbumpfee", 1, "original_change_index"},
    { "logging", 0, "include" },
    { "logging", 1, "exclude" },
    { "disconnectnode", 1, "nodeid" },
    { "gethdkeys", 0, "active_only" },
    { "gethdkeys", 0, "options" },
    { "gethdkeys", 0, "private" },
    { "createwalletdescriptor", 1, "options" },
    { "createwalletdescriptor", 1, "internal" },
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
    { "createwallet", 0, "wallet_name", ParamFormat::STRING },
    { "createwallet", 1, "disable_private_keys"},
    { "createwallet", 2, "blank"},
    { "createwallet", 3, "passphrase", ParamFormat::STRING },
    { "createwallet", 4, "avoid_reuse"},
    { "createwallet", 5, "descriptors"},
    { "createwallet", 6, "load_on_startup"},
    { "createwallet", 7, "external_signer"},
    { "restorewallet", 0, "wallet_name", ParamFormat::STRING },
    { "restorewallet", 1, "backup_file", ParamFormat::STRING },
    { "restorewallet", 2, "load_on_startup"},
    { "loadwallet", 0, "filename", ParamFormat::STRING },
    { "loadwallet", 1, "load_on_startup"},
    { "unloadwallet", 0, "wallet_name", ParamFormat::STRING },
    { "unloadwallet", 1, "load_on_startup"},
    { "getnodeaddresses", 0, "count"},
    { "addpeeraddress", 1, "port"},
    { "addpeeraddress", 2, "tried"},
    { "sendmsgtopeer", 0, "peer_id" },
    { "stop", 0, "wait" },
    { "addnode", 2, "v2transport" },
    { "addconnection", 2, "v2transport" },
    { "decodepsbt", 0, "psbt", ParamFormat::STRING },
    { "analyzepsbt", 0, "psbt", ParamFormat::STRING},
    { "verifymessage", 1, "signature", ParamFormat::STRING },
    { "verifymessage", 2, "message", ParamFormat::STRING },
    { "getnewaddress", 0, "label", ParamFormat::STRING },
    { "getnewaddress", 1, "address_type", ParamFormat::STRING },
    { "backupwallet", 0, "destination", ParamFormat::STRING },
    { "echoipc", 0, "arg", ParamFormat::STRING },
    { "encryptwallet", 0, "passphrase", ParamFormat::STRING },
    { "getaddressesbylabel", 0, "label", ParamFormat::STRING },
    { "loadtxoutset", 0, "path", ParamFormat::STRING },
    { "migratewallet", 0, "wallet_name", ParamFormat::STRING },
    { "migratewallet", 1, "passphrase", ParamFormat::STRING },
    { "setlabel", 1, "label", ParamFormat::STRING },
    { "signmessage", 1, "message", ParamFormat::STRING },
    { "signmessagewithprivkey", 1, "message", ParamFormat::STRING },
    { "walletpassphrasechange", 0, "oldpassphrase", ParamFormat::STRING },
    { "walletpassphrasechange", 1, "newpassphrase", ParamFormat::STRING },
};
// clang-format on

/** Parse string to UniValue or throw runtime_error if string contains invalid JSON */
static UniValue Parse(std::string_view raw, ParamFormat format = ParamFormat::JSON)
{
    UniValue parsed;
    if (!parsed.read(raw)) {
        if (format != ParamFormat::JSON_OR_STRING) throw std::runtime_error(tfm::format("Error parsing JSON: %s", raw));
        return UniValue(std::string(raw));
    }
    return parsed;
}

namespace rpc_convert
{
const CRPCConvertParam* FromPosition(std::string_view method, size_t pos)
{
    auto it = std::ranges::find_if(vRPCConvertParams, [&](const auto& p) {
        return p.methodName == method && p.paramIdx == static_cast<int>(pos);
    });

    return it == std::end(vRPCConvertParams) ? nullptr : &*it;
}

const CRPCConvertParam* FromName(std::string_view method, std::string_view name)
{
    auto it = std::ranges::find_if(vRPCConvertParams, [&](const auto& p) {
        return p.methodName == method && p.paramName == name;
    });

    return it == std::end(vRPCConvertParams) ? nullptr : &*it;
}
} // namespace rpc_convert

static UniValue ParseParam(const CRPCConvertParam* param, std::string_view raw)
{
    // Only parse parameters which have the JSON or JSON_OR_STRING format; otherwise, treat them as strings.
    return (param && (param->format == ParamFormat::JSON || param->format == ParamFormat::JSON_OR_STRING)) ? Parse(raw, param->format) : UniValue(std::string(raw));
}

/**
 * Convert command lines arguments to params object when -named is disabled.
 */
UniValue RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    UniValue params(UniValue::VARR);

    for (std::string_view s : strParams) {
        params.push_back(ParseParam(rpc_convert::FromPosition(strMethod, params.size()), s));
    }

    return params;
}

/**
 * Convert command line arguments to params object when -named is enabled.
 *
 * The -named syntax accepts named arguments in NAME=VALUE format, as well as
 * positional arguments without names. The syntax is inherently ambiguous if
 * names are omitted and values contain '=', so a heuristic is used to
 * disambiguate:
 *
 * - Arguments that do not contain '=' are treated as positional parameters.
 *
 * - Arguments that do contain '=' are assumed to be named parameters in
 *   NAME=VALUE format except for two special cases:
 *
 *   1. The case where NAME is not a known parameter name, and the next
 *      positional parameter requires a JSON value, and the argument parses as
 *      JSON. E.g. ["list", "with", "="].
 *
 *   2. The case where NAME is not a known parameter name and the next
 *      positional parameter requires a string value. E.g. "my=wallet".
 *
 * For example, the command `bitcoin-cli -named createwallet "my=wallet"`,
 * the parser initially sees "my=wallet" and attempts to process it as a
 * parameter named "my". When it finds that "my" is not a valid named parameter
 * parameter for this method, it falls back to checking the rule for the
 * next available positional parameter (index 0). Because it finds the rule
 * that this parameter is a ParamFormat::STRING, it correctly treats the entire
 * "my=wallet" as a single positional string, successfully creating a
 * wallet with that literal name.
 */
UniValue RPCConvertNamedValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    UniValue params(UniValue::VOBJ);
    UniValue positional_args{UniValue::VARR};

    for (std::string_view s: strParams) {
        size_t pos = s.find('=');
        if (pos == std::string_view::npos) {
            positional_args.push_back(ParseParam(rpc_convert::FromPosition(strMethod, positional_args.size()), s));
            continue;
        }

        std::string name{s.substr(0, pos)};
        std::string_view value{s.substr(pos+1)};

        const CRPCConvertParam* named_param{rpc_convert::FromName(strMethod, name)};
        if (!named_param) {
            const CRPCConvertParam* positional_param = rpc_convert::FromPosition(strMethod, positional_args.size());
            UniValue parsed_value;
            if (positional_param && positional_param->format == ParamFormat::JSON && parsed_value.read(s)) {
                positional_args.push_back(std::move(parsed_value));
                continue;
            } else if (positional_param && positional_param->format == ParamFormat::STRING) {
                positional_args.push_back(s);
                continue;
            }
        }

        // Intentionally overwrite earlier named values with later ones as a
        // convenience for scripts and command line users that want to merge
        // options.
        params.pushKV(name, ParseParam(named_param, value));
    }

    if (!positional_args.empty()) {
        // Use pushKVEnd instead of pushKV to avoid overwriting an explicit
        // "args" value with an implicit one. Let the RPC server handle the
        // request as given.
        params.pushKVEnd("args", std::move(positional_args));
    }

    return params;
}
