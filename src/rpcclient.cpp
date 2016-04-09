// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcclient.h"

#include "rpcprotocol.h"
#include "util.h"
#include "ui_interface.h"

#include <set>
#include <stdint.h>

using namespace std;
using namespace json_spirit;

class CRPCConvertParam
{
public:
    std::string methodName;            //! method whose params want conversion
    int paramIdx;                      //! 0-based idx of param to convert
};

static const CRPCConvertParam vRPCConvertParams[] =
{
    { "stop", 0 },
    { "setmocktime", 0 },
    { "getaddednodeinfo", 0 },
    { "setgenerate", 0 },
    { "setgenerate", 1 },
    { "getnetworkhashps", 0 },
    { "getnetworkhashps", 1 },
    { "sendtoaddress", 1 },
    { "settxfee", 0 },
    { "getreceivedbyaddress", 1 },
    { "getreceivedbyaccount", 1 },
    { "listreceivedbyaddress", 0 },
    { "listreceivedbyaddress", 1 },
    { "listreceivedbyaddress", 2 },
    { "listreceivedbyaccount", 0 },
    { "listreceivedbyaccount", 1 },
    { "listreceivedbyaccount", 2 },
    { "getbalance", 1 },
    { "getbalance", 2 },
    { "getblockhash", 0 },
    { "move", 2 },
    { "move", 3 },
    { "sendfrom", 2 },
    { "sendfrom", 3 },
    { "listtransactions", 1 },
    { "listtransactions", 2 },
    { "listtransactions", 3 },
    { "listaccounts", 0 },
    { "listaccounts", 1 },
    { "walletpassphrase", 1 },
    { "getblocktemplate", 0 },
    { "listsinceblock", 1 },
    { "listsinceblock", 2 },
    { "sendmany", 1 },
    { "sendmany", 2 },
    { "addmultisigaddress", 0 },
    { "addmultisigaddress", 1 },
    { "createmultisig", 0 },
    { "createmultisig", 1 },
    { "listunspent", 0 },
    { "listunspent", 1 },
    { "listunspent", 2 },
    { "getblock", 1 },
    { "gettransaction", 1 },
    { "getrawtransaction", 1 },
    { "createrawtransaction", 0 },
    { "createrawtransaction", 1 },
    { "signrawtransaction", 1 },
    { "signrawtransaction", 2 },
    { "sendrawtransaction", 1 },
    { "gettxout", 1 },
    { "gettxout", 2 },
    { "lockunspent", 0 },
    { "lockunspent", 1 },
    { "importprivkey", 2 },
    { "importaddress", 2 },
    { "verifychain", 0 },
    { "verifychain", 1 },
    { "keypoolrefill", 0 },
    { "getrawmempool", 0 },
    { "estimatefee", 0 },
    { "estimatepriority", 0 },
    { "prioritisetransaction", 1 },
    { "prioritisetransaction", 2 },

    /* Omni Core - backwards compatibility */
    { "getcrowdsale_MP", 0 },
    { "getcrowdsale_MP", 1 },
    { "getgrants_MP", 0 },
    { "send_MP", 2 },
    { "getbalance_MP", 1 },
    { "sendtoowners_MP", 1 },
    { "getproperty_MP", 0 },
    { "listtransactions_MP", 1 },
    { "listtransactions_MP", 2 },
    { "listtransactions_MP", 3 },
    { "listtransactions_MP", 4 },
    { "getallbalancesforid_MP", 0 },
    { "listblocktransactions_MP", 0 },
    { "getorderbook_MP", 0 },
    { "getorderbook_MP", 1 },
    { "trade_MP", 1 }, // depreciated
    { "trade_MP", 3 }, // depreciated
    { "trade_MP", 5 }, // depreciated

    /* Omni Core - data retrieval calls */
    { "omni_gettradehistoryforaddress", 1 },
    { "omni_gettradehistoryforaddress", 2 },
    { "omni_gettradehistoryforpair", 0 },
    { "omni_gettradehistoryforpair", 1 },
    { "omni_gettradehistoryforpair", 2 },
    { "omni_setautocommit", 0 },
    { "omni_getcrowdsale", 0 },
    { "omni_getcrowdsale", 1 },
    { "omni_getgrants", 0 },
    { "omni_getbalance", 1 },
    { "omni_getproperty", 0 },
    { "omni_listtransactions", 1 },
    { "omni_listtransactions", 2 },
    { "omni_listtransactions", 3 },
    { "omni_listtransactions", 4 },
    { "omni_getallbalancesforid", 0 },
    { "omni_listblocktransactions", 0 },
    { "omni_getorderbook", 0 },
    { "omni_getorderbook", 1 },
    { "omni_getseedblocks", 0 },
    { "omni_getseedblocks", 1 },
    { "omni_getmetadexhash", 0 },

    /* Omni Core - transaction calls */
    { "omni_send", 2 },
    { "omni_sendsto", 1 },
    { "omni_sendall", 2 },
    { "omni_sendtrade", 1 },
    { "omni_sendtrade", 3 },
    { "omni_sendcanceltradesbyprice", 1 },
    { "omni_sendcanceltradesbyprice", 3 },
    { "omni_sendcanceltradesbypair", 1 },
    { "omni_sendcanceltradesbypair", 2 },
    { "omni_sendcancelalltrades", 1 },
    { "omni_sendissuancefixed", 1 },
    { "omni_sendissuancefixed", 2 },
    { "omni_sendissuancefixed", 3 },
    { "omni_sendissuancemanaged", 1 },
    { "omni_sendissuancemanaged", 2 },
    { "omni_sendissuancemanaged", 3 },
    { "omni_sendissuancecrowdsale", 1 },
    { "omni_sendissuancecrowdsale", 2 },
    { "omni_sendissuancecrowdsale", 3 },
    { "omni_sendissuancecrowdsale", 9 },
    { "omni_sendissuancecrowdsale", 11 },
    { "omni_sendissuancecrowdsale", 12 },
    { "omni_sendissuancecrowdsale", 13 },
    { "omni_senddexsell", 1 },
    { "omni_senddexsell", 4 },
    { "omni_senddexsell", 6 },
    { "omni_senddexaccept", 2 },
    { "omni_senddexaccept", 4 },
    { "omni_sendclosecrowdsale", 1 },
    { "omni_sendgrant", 2 },
    { "omni_sendrevoke", 1 },
    { "omni_sendchangeissuer", 2 },
    { "omni_sendactivation", 1 },
    { "omni_sendactivation", 2 },
    { "omni_sendactivation", 3 },
    { "omni_sendalert", 1 },
    { "omni_sendalert", 2 },

    /* Omni Core - raw transaction calls */
    { "omni_decodetransaction", 1 },
    { "omni_decodetransaction", 2 },
    { "omni_createrawtx_reference", 2 },
    { "omni_createrawtx_input", 2 },
    { "omni_createrawtx_change", 1 },
    { "omni_createrawtx_change", 3 },
    { "omni_createrawtx_change", 4 },

    /* Omni Core - payload creation */
    { "omni_createpayload_simplesend", 0 },
    { "omni_createpayload_sendall", 0 },
    { "omni_createpayload_dexsell", 0 },
    { "omni_createpayload_dexsell", 3 },
    { "omni_createpayload_dexsell", 5 },
    { "omni_createpayload_dexaccept", 0 },
    { "omni_createpayload_sto", 0 },
    { "omni_createpayload_issuancefixed", 0 },
    { "omni_createpayload_issuancefixed", 1 },
    { "omni_createpayload_issuancefixed", 2 },
    { "omni_createpayload_issuancemanaged", 0 },
    { "omni_createpayload_issuancemanaged", 1 },
    { "omni_createpayload_issuancemanaged", 2 },
    { "omni_createpayload_issuancecrowdsale", 0 },
    { "omni_createpayload_issuancecrowdsale", 1 },
    { "omni_createpayload_issuancecrowdsale", 2 },
    { "omni_createpayload_issuancecrowdsale", 8 },
    { "omni_createpayload_issuancecrowdsale", 10 },
    { "omni_createpayload_issuancecrowdsale", 11 },
    { "omni_createpayload_issuancecrowdsale", 12 },
    { "omni_createpayload_closecrowdsale", 0 },
    { "omni_createpayload_grant", 0 },
    { "omni_createpayload_revoke", 0 },
    { "omni_createpayload_changeissuer", 0 },
    { "omni_createpayload_trade", 0 },
    { "omni_createpayload_trade", 2 },
    { "omni_createpayload_canceltradesbyprice", 0 },
    { "omni_createpayload_canceltradesbyprice", 2 },
    { "omni_createpayload_canceltradesbypair", 0 },
    { "omni_createpayload_canceltradesbypair", 1 },
    { "omni_createpayload_cancelalltrades", 0 },
};

class CRPCConvertTable
{
private:
    std::set<std::pair<std::string, int> > members;

public:
    CRPCConvertTable();

    bool convert(const std::string& method, int idx) {
        return (members.count(std::make_pair(method, idx)) > 0);
    }
};

CRPCConvertTable::CRPCConvertTable()
{
    const unsigned int n_elem =
        (sizeof(vRPCConvertParams) / sizeof(vRPCConvertParams[0]));

    for (unsigned int i = 0; i < n_elem; i++) {
        members.insert(std::make_pair(vRPCConvertParams[i].methodName,
                                      vRPCConvertParams[i].paramIdx));
    }
}

static CRPCConvertTable rpcCvtTable;

/** Convert strings to command-specific RPC representation */
Array RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    Array params;

    for (unsigned int idx = 0; idx < strParams.size(); idx++) {
        const std::string& strVal = strParams[idx];

        // insert string value directly
        if (!rpcCvtTable.convert(strMethod, idx)) {
            params.push_back(strVal);
        }

        // parse string as JSON, insert bool/number/object/etc. value
        else {
            Value jVal;
            if (!read_string(strVal, jVal))
                throw runtime_error(string("Error parsing JSON:")+strVal);
            params.push_back(jVal);
        }
    }

    return params;
}

