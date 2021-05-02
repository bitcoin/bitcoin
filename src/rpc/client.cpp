// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/client.h>
#include <util/system.h>

#include <set>
#include <stdint.h>

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
    { "getnetworkhashps", 0, "nblocks" },
    { "getnetworkhashps", 1, "height" },
    { "sendtoaddress", 1, "amount" },
    { "sendtoaddress", 4, "subtractfeefromamount" },
    { "sendtoaddress", 5 , "replaceable" },
    { "sendtoaddress", 6 , "conf_target" },
    { "sendtoaddress", 8, "avoid_reuse" },
    { "settxfee", 0, "amount" },
    { "sethdseed", 0, "newkeypool" },
    { "getreceivedbyaddress", 1, "minconf" },
    { "getreceivedbylabel", 1, "minconf" },
    { "listreceivedbyaddress", 0, "minconf" },
    { "listreceivedbyaddress", 1, "include_empty" },
    { "listreceivedbyaddress", 2, "include_watchonly" },
    { "listreceivedbylabel", 0, "minconf" },
    { "listreceivedbylabel", 1, "include_empty" },
    { "listreceivedbylabel", 2, "include_watchonly" },
    { "getbalance", 1, "minconf" },
    { "getbalance", 2, "include_watchonly" },
    { "getbalance", 3, "avoid_reuse" },
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
    { "sendmany", 1, "amounts" },
    { "sendmany", 2, "minconf" },
    { "sendmany", 4, "subtractfeefrom" },
    { "sendmany", 5 , "replaceable" },
    { "sendmany", 6 , "conf_target" },
    { "deriveaddresses", 1, "range" },
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
    { "getrawtransaction", 1, "verbose" },
    { "createrawtransaction", 0, "inputs" },
    { "createrawtransaction", 1, "outputs" },
    { "createrawtransaction", 2, "locktime" },
    { "createrawtransaction", 3, "replaceable" },
    { "decoderawtransaction", 1, "iswitness" },
    { "signrawtransactionwithkey", 1, "privkeys" },
    { "signrawtransactionwithkey", 2, "prevtxs" },
    { "signrawtransactionwithwallet", 1, "prevtxs" },
    { "sendrawtransaction", 1, "allowhighfees" },
    { "sendrawtransaction", 1, "maxfeerate" },
    { "testmempoolaccept", 0, "rawtxs" },
    { "testmempoolaccept", 1, "allowhighfees" },
    { "testmempoolaccept", 1, "maxfeerate" },
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
    { "lockunspent", 0, "unlock" },
    { "lockunspent", 1, "transactions" },
    { "importprivkey", 2, "rescan" },
    { "importaddress", 2, "rescan" },
    { "importaddress", 3, "p2sh" },
    { "importpubkey", 2, "rescan" },
    { "importmulti", 0, "requests" },
    { "importmulti", 1, "options" },
    { "verifychain", 0, "checklevel" },
    { "verifychain", 1, "nblocks" },
    { "getblockstats", 0, "hash_or_height" },
    { "getblockstats", 1, "stats" },
    { "pruneblockchain", 0, "height" },
    { "keypoolrefill", 0, "newsize" },
    { "getrawmempool", 0, "verbose" },
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
    { "bumpfee", 1, "options" },
    { "logging", 0, "include" },
    { "logging", 1, "exclude" },
    { "disconnectnode", 1, "nodeid" },
    { "getaddresstxids", 0, "addresses"},
    { "getaddressmempool", 0, "addresses"},
    { "getaddressdeltas", 0, "addresses"},
    { "getaddressbalance", 0, "addresses"},
    { "getaddressutxos", 0, "addresses"},
    { "getblockhashes", 0, "high"},
    { "getblockhashes", 1, "low"},
    { "getblockhashes", 2, "options"},
    { "getspentinfo", 0, "argument"},
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
    { "getnodeaddresses", 0, "count"},
    { "stop", 0, "wait" },


    /* Omni Core - data retrieval calls */
    { "omni_gettradehistoryforaddress", 1 , "count"},
    { "omni_gettradehistoryforaddress", 2, "propertyid" },
    { "omni_gettradehistoryforpair", 0, "propertyid" },
    { "omni_gettradehistoryforpair", 1, "propertyidsecond" },
    { "omni_gettradehistoryforpair", 2, "count" },
    { "omni_setautocommit", 0, "flag" },
    { "omni_getcrowdsale", 0, "propertyid" },
    { "omni_getcrowdsale", 1, "verbose" },
    { "omni_getgrants", 0, "propertyid" },
    { "omni_getbalance", 1, "propertyid" },
    { "omni_getproperty", 0, "propertyid" },
    { "omni_listtransactions", 1, "count" },
    { "omni_listtransactions", 2, "skip" },
    { "omni_listtransactions", 3, "startblock" },
    { "omni_listtransactions", 4, "endblock" },
    { "omni_getallbalancesforid", 0, "propertyid" },
    { "omni_listblocktransactions", 0, "index" },
    { "omni_listblockstransactions", 0, "firstblock" },
    { "omni_listblockstransactions", 1, "lastblock" },
    { "omni_getorderbook", 0, "propertyid" },
    { "omni_getorderbook", 1, "propertyid" },
    { "omni_getseedblocks", 0, "startblock" },
    { "omni_getseedblocks", 1, "endblock" },
    { "omni_getmetadexhash", 0, "propertyid" },
    { "omni_getfeecache", 0, "propertyid" },
    { "omni_getfeeshare", 1, "ecosystem" },
    { "omni_getfeetrigger", 0, "propertyid" },
    { "omni_getfeedistribution", 0, "distributionid" },
    { "omni_getfeedistributions", 0, "propertyid" },
    { "omni_getbalanceshash", 0, "propertyid" },
    { "omni_getwalletbalances", 0, "includewatchonly" },
    { "omni_getwalletaddressbalances", 0, "includewatchonly" },
    { "omni_getnonfungibletokens", 1, "propertyid"},
    { "omni_getnonfungibletokendata", 0, "propertyid"},
    { "omni_getnonfungibletokendata", 2, "tokenidend"},
    { "omni_getnonfungibletokenranges", 0, "propertyid"},
    { "omni_getnonfungibletokenranges", 0, "propertyid"},

    /* Omni Core - transaction calls */
    { "omni_send", 2, "propertyid" },
    { "omni_sendsto", 1, "propertyid" },
    { "omni_sendsto", 4, "distributionproperty" },
    { "omni_sendall", 2, "ecosystem" },
    { "omni_sendtrade", 1, "propertyidforsale" },
    { "omni_sendtrade", 3, "propertiddesired" },
    { "omni_sendcanceltradesbyprice", 1, "propertyidforsale" },
    { "omni_sendcanceltradesbyprice", 3, "propertiddesired" },
    { "omni_sendcanceltradesbypair", 1, "propertyidforsale" },
    { "omni_sendcanceltradesbypair", 2, "propertiddesired" },
    { "omni_sendcancelalltrades", 1, "ecosystem" },
    { "omni_sendissuancefixed", 1, "ecosystem" },
    { "omni_sendissuancefixed", 2, "type" },
    { "omni_sendissuancefixed", 3, "previousid" },
    { "omni_sendissuancemanaged", 1, "ecosystem" },
    { "omni_sendissuancemanaged", 2, "type" },
    { "omni_sendissuancemanaged", 3, "previousid" },
    { "omni_sendissuancecrowdsale", 1, "ecosystem" },
    { "omni_sendissuancecrowdsale", 2, "type" },
    { "omni_sendissuancecrowdsale", 3, "previousid" },
    { "omni_sendissuancecrowdsale", 9, "propertyiddesired" },
    { "omni_sendissuancecrowdsale", 11, "deadline" },
    { "omni_sendissuancecrowdsale", 12, "earlybonus" },
    { "omni_sendissuancecrowdsale", 13, "issuerpercentage" },
    { "omni_senddexsell", 1, "propertyidforsale" },
    { "omni_senddexsell", 4, "paymentwindow" },
    { "omni_senddexsell", 6, "action" },
    { "omni_sendnewdexorder", 1, "propertyidforsale" },
    { "omni_sendnewdexorder", 4, "paymentwindow" },
    { "omni_sendupdatedexorder", 1, "propertyidforsale" },
    { "omni_sendupdatedexorder", 4, "paymentwindow" },
    { "omni_sendcanceldexorder", 1, "propertyidforsale" },
    { "omni_senddexaccept", 2, "propertyid" },
    { "omni_senddexaccept", 4, "override" },
    { "omni_senddexpay", 2, "propertyid" },
    { "omni_sendclosecrowdsale", 1, "propertyid" },
    { "omni_sendgrant", 2, "propertyid" },
    { "omni_sendrevoke", 1, "propertyid" },
    { "omni_sendchangeissuer", 2, "propertyid" },
    { "omni_sendenablefreezing", 1, "propertyid" },
    { "omni_senddisablefreezing", 1, "propertyid" },
    { "omni_sendfreeze", 2, "propertyid" },
    { "omni_sendunfreeze", 2, "propertyid" },
    { "omni_senddeactivation", 1, "featureid" },
    { "omni_sendactivation", 1, "featureid" },
    { "omni_sendactivation", 2, "block" },
    { "omni_sendactivation", 3, "minclientversion" },
    { "omni_sendalert", 1, "alerttype" },
    { "omni_sendalert", 2, "expiryvalue" },
    { "omni_funded_send", 2, "propertyid" },
    { "omni_funded_sendall", 2, "ecosystem" },
    { "omni_sendnonfungible", 2, "propertyid"},
    { "omni_sendnonfungible", 3, "tokenstart"},
    { "omni_sendnonfungible", 4, "tokenend"},
    { "omni_setnonfungibledata", 0, "propertyid"},
    { "omni_setnonfungibledata", 1, "tokenstart"},
    { "omni_setnonfungibledata", 2, "tokenend"},
    { "omni_setnonfungibledata", 3, "issuer"},

    /* Omni Core - raw transaction calls */
    { "omni_decodetransaction", 1, "prevtxs" },
    { "omni_decodetransaction", 2, "height" },
    { "omni_createrawtx_input", 2, "n" },
    { "omni_createrawtx_change", 1, "prevtxs" },
    { "omni_createrawtx_change", 3, "fee" },
    { "omni_createrawtx_change", 4, "position" },

    /* Omni Core - payload creation */
    { "omni_createpayload_simplesend", 0, "propertyid" },
    { "omni_createpayload_sendall", 0, "ecosystem" },
    { "omni_createpayload_dexsell", 0, "propertyidforsale" },
    { "omni_createpayload_dexsell", 3, "paymentwindow" },
    { "omni_createpayload_dexsell", 5, "action" },
    { "omni_createpayload_dexaccept", 0, "propertyid" },
    { "omni_createpayload_sto", 0, "propertyid" },
    { "omni_createpayload_sto", 2, "distributionproperty" },
    { "omni_createpayload_issuancefixed", 0, "ecosystem" },
    { "omni_createpayload_issuancefixed", 1, "type" },
    { "omni_createpayload_issuancefixed", 2, "previousid" },
    { "omni_createpayload_issuancemanaged", 0, "ecosystem" },
    { "omni_createpayload_issuancemanaged", 1, "type" },
    { "omni_createpayload_issuancemanaged", 2, "previousid" },
    { "omni_createpayload_issuancecrowdsale", 0, "ecosystem" },
    { "omni_createpayload_issuancecrowdsale", 1, "type" },
    { "omni_createpayload_issuancecrowdsale", 2, "previousid" },
    { "omni_createpayload_issuancecrowdsale", 8, "propertyiddesired" },
    { "omni_createpayload_issuancecrowdsale", 10, "deadline" },
    { "omni_createpayload_issuancecrowdsale", 11, "earlybonus" },
    { "omni_createpayload_issuancecrowdsale", 12, "issuerpercentage" },
    { "omni_createpayload_closecrowdsale", 0, "propertyid" },
    { "omni_createpayload_grant", 0, "propertyid" },
    { "omni_createpayload_revoke", 0, "propertyid" },
    { "omni_createpayload_changeissuer", 0, "propertyid" },
    { "omni_createpayload_trade", 0, "propertyidforsale" },
    { "omni_createpayload_trade", 2, "propertiddesired" },
    { "omni_createpayload_canceltradesbyprice", 0, "propertyidforsale" },
    { "omni_createpayload_canceltradesbyprice", 2, "propertiddesired" },
    { "omni_createpayload_canceltradesbypair", 0, "propertyidforsale" },
    { "omni_createpayload_canceltradesbypair", 1, "propertiddesired" },
    { "omni_createpayload_cancelalltrades", 0, "ecosystem" },
    { "omni_createpayload_enablefreezing", 0, "propertyid" },
    { "omni_createpayload_disablefreezing", 0, "propertyid" },
    { "omni_createpayload_freeze", 1, "propertyid" },
    { "omni_createpayload_unfreeze", 1, "propertyid" },
    { "omni_createpayload_sendnonfungible", 0, "propertyid"},
    { "omni_createpayload_sendnonfungible", 1, "tokenstart"},
    { "omni_createpayload_sendnonfungible", 2, "tokenend"},
    { "omni_createpayload_setnonfungibledata", 0, "propertyid"},
    { "omni_createpayload_setnonfungibledata", 1, "tokenid"},
    { "omni_createpayload_setnonfungibledata", 2, "issuer"},

    /* Omni Core - backwards compatibility */
    { "getcrowdsale_MP", 0, "propertyid" },
    { "getcrowdsale_MP", 1, "verbose" },
    { "getgrants_MP", 0, "propertyid" },
    { "send_MP", 2, "propertyid" },
    { "getbalance_MP", 1, "propertyid" },
    { "sendtoowners_MP", 1, "propertyid" },
    { "sendtoowners_MP", 4, "distributionproperty" },
    { "getproperty_MP", 0, "propertyid" },
    { "listtransactions_MP", 1, "count" },
    { "listtransactions_MP", 2, "skip" },
    { "listtransactions_MP", 3, "startblock" },
    { "listtransactions_MP", 4, "endblock" },
    { "getallbalancesforid_MP", 0, "propertyid" },
    { "listblocktransactions_MP", 0, "index" },
    { "getorderbook_MP", 0, "propertyid" },
    { "getorderbook_MP", 1, "propertyiddesired" },
    { "trade_MP", 1, "propertyidforsale" }, // deprecated
    { "trade_MP", 3, "propertiddesired" }, // deprecated
    { "trade_MP", 5, "action" }, // deprecated
};
// clang-format on

class CRPCConvertTable
{
private:
    std::set<std::pair<std::string, int>> members;
    std::set<std::pair<std::string, std::string>> membersByName;

public:
    CRPCConvertTable();

    bool convert(const std::string& method, int idx) {
        return (members.count(std::make_pair(method, idx)) > 0);
    }
    bool convert(const std::string& method, const std::string& name) {
        return (membersByName.count(std::make_pair(method, name)) > 0);
    }
};

CRPCConvertTable::CRPCConvertTable()
{
    const unsigned int n_elem =
        (sizeof(vRPCConvertParams) / sizeof(vRPCConvertParams[0]));

    for (unsigned int i = 0; i < n_elem; i++) {
        members.insert(std::make_pair(vRPCConvertParams[i].methodName,
                                      vRPCConvertParams[i].paramIdx));
        membersByName.insert(std::make_pair(vRPCConvertParams[i].methodName,
                                            vRPCConvertParams[i].paramName));
    }
}

static CRPCConvertTable rpcCvtTable;

/** Non-RFC4627 JSON parser, accepts internal values (such as numbers, true, false, null)
 * as well as objects and arrays.
 */
UniValue ParseNonRFCJSONValue(const std::string& strVal)
{
    UniValue jVal;
    if (!jVal.read(std::string("[")+strVal+std::string("]")) ||
        !jVal.isArray() || jVal.size()!=1)
        throw std::runtime_error(std::string("Error parsing JSON:")+strVal);
    return jVal[0];
}

UniValue RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    UniValue params(UniValue::VARR);

    for (unsigned int idx = 0; idx < strParams.size(); idx++) {
        const std::string& strVal = strParams[idx];

        if (!rpcCvtTable.convert(strMethod, idx)) {
            // insert string value directly
            params.push_back(strVal);
        } else {
            // parse string as JSON, insert bool/number/object/etc. value
            params.push_back(ParseNonRFCJSONValue(strVal));
        }
    }

    return params;
}

UniValue RPCConvertNamedValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    UniValue params(UniValue::VOBJ);

    for (const std::string &s: strParams) {
        size_t pos = s.find('=');
        if (pos == std::string::npos) {
            throw(std::runtime_error("No '=' in named argument '"+s+"', this needs to be present for every argument (even if it is empty)"));
        }

        std::string name = s.substr(0, pos);
        std::string value = s.substr(pos+1);

        if (!rpcCvtTable.convert(strMethod, name)) {
            // insert string value directly
            params.pushKV(name, value);
        } else {
            // parse string as JSON, insert bool/number/object/etc. value
            params.pushKV(name, ParseNonRFCJSONValue(value));
        }
    }

    return params;
}
