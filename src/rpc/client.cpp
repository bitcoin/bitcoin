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
 * This prevents `bitcoin-cli` from splitting strings like an argument that
 * contains an equals sign into a named argument and value when the whole string
 * is intended to be a single positional argument. And if one string parameter
 * is listed for a method, other string parameters for that method need to be
 * listed as well so bitcoin-cli does not make the opposite mistake and pass
 * other arguments by position instead of name because it does not recognize
 * their names. See \ref RPCConvertNamedValues for more information on how named
 * and positional arguments are distinguished with -named.
 *
 * @note Parameter indexes start from 0.
 */
static const CRPCConvertParam vRPCConvertParams[] =
{
    { "setmocktime", 0, "timestamp" },
    { "mockscheduler", 0, "delta_time" },
    { "generatetoaddress", 0, "nblocks" },
    { "generatetoaddress", 2, "maxtries" },
    { "generatetodescriptor", 0, "num_blocks" },
    { "generatetodescriptor", 2, "maxtries" },
    { "generateblock", 1, "transactions" },
    { "generateblock", 2, "submit" },
    { "getnetworkhashps", 0, "nblocks" },
    { "getnetworkhashps", 1, "height" },
    { "getblockfrompeer", 1, "peer_id" },
    { "getblockhash", 0, "height" },
    { "waitforblockheight", 0, "height" },
    { "waitforblockheight", 1, "timeout" },
    { "waitforblock", 1, "timeout" },
    { "waitfornewblock", 0, "timeout" },
    { "getblocktemplate", 0, "template_request" },
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
    { "getblock", 1, "verbosity" },
    { "getblock", 1, "verbose" },
    { "getblockheader", 1, "verbose" },
    { "getchaintxstats", 0, "nblocks" },
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
    { "sendrawtransaction", 1, "maxfeerate" },
    { "sendrawtransaction", 2, "maxburnamount" },
    { "testmempoolaccept", 0, "rawtxs" },
    { "testmempoolaccept", 1, "maxfeerate" },
    { "submitpackage", 0, "package" },
    { "submitpackage", 1, "maxfeerate" },
    { "submitpackage", 2, "maxburnamount" },
    { "combinerawtransaction", 0, "txs" },
    { "gettxout", 1, "n" },
    { "gettxout", 2, "include_mempool" },
    { "gettxoutproof", 0, "txids" },
    { "gettxoutsetinfo", 1, "hash_or_height", ParamFormat::JSON_OR_STRING },
    { "gettxoutsetinfo", 2, "use_index"},
    { "dumptxoutset", 0, "path", ParamFormat::STRING },
    { "dumptxoutset", 1, "type", ParamFormat::STRING },
    { "dumptxoutset", 2, "options" },
    { "dumptxoutset", 2, "rollback", ParamFormat::JSON_OR_STRING },
    { "dumptxoutset", 2, "in_memory" },
    { "simulaterawtransaction", 0, "rawtxs" },
    { "simulaterawtransaction", 1, "options" },
    { "simulaterawtransaction", 1, "include_watchonly"},
    { "importmempool", 0, "filepath", ParamFormat::STRING },
    { "importmempool", 1, "options" },
    { "importmempool", 1, "apply_fee_delta_priority" },
    { "importmempool", 1, "use_current_time" },
    { "importmempool", 1, "apply_unbroadcast_set" },
    { "verifychain", 0, "checklevel" },
    { "verifychain", 1, "nblocks" },
    { "getblockstats", 0, "hash_or_height", ParamFormat::JSON_OR_STRING },
    { "getblockstats", 1, "stats" },
    { "pruneblockchain", 0, "height" },
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
    { "getmempoolancestors", 1, "verbose" },
    { "getmempooldescendants", 1, "verbose" },
    { "gettxspendingprevout", 0, "outputs" },
    { "gettxspendingprevout", 1, "options" },
    { "gettxspendingprevout", 1, "mempool_only" },
    { "gettxspendingprevout", 1, "return_spending_tx" },
    { "logging", 0, "include" },
    { "logging", 1, "exclude" },
    { "disconnectnode", 1, "nodeid" },
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
    { "getnodeaddresses", 0, "count"},
    { "addpeeraddress", 1, "port"},
    { "addpeeraddress", 2, "tried"},
    { "sendmsgtopeer", 0, "peer_id" },
    { "stop", 0, "wait" },
    { "addnode", 2, "v2transport" },
    { "addconnection", 2, "v2transport" },
    { "verifymessage", 1, "signature", ParamFormat::STRING },
    { "verifymessage", 2, "message", ParamFormat::STRING },
    { "echoipc", 0, "arg", ParamFormat::STRING },
    { "loadtxoutset", 0, "path", ParamFormat::STRING },
    { "signmessagewithprivkey", 1, "message", ParamFormat::STRING },
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
 *      positional parameter requires a string value.
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
