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

    /* Omni Core related conversions */
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
    { "setautocommit_OMNI", 0 },
    { "getorderbook_MP", 0 },
    { "getorderbook_MP", 1 },
    { "gettradessince_MP", 0 },
    { "gettradessince_MP", 1 },
    { "gettradessince_MP", 2 },
    { "gettradehistoryforaddress_OMNI", 1 },
    { "gettradehistoryforaddress_OMNI", 2 },
    { "gettradehistoryformarket_OMNI", 0 },
    { "gettradehistoryformarket_OMNI", 1 },
    // omni_rpctx
    { "send_OMNI", 2 },
    { "sendsto_OMNI", 1 },
    { "trade_MP", 1 }, // depreciated
    { "trade_MP", 3 }, // depreciated
    { "trade_MP", 5 }, // depreciated
    { "sendtrade_OMNI", 1 },
    { "sendtrade_OMNI", 3 },
    { "sendcanceltradesbyprice_OMNI", 1 },
    { "sendcanceltradesbyprice_OMNI", 3 },
    { "sendcanceltradesbypair_OMNI", 1 },
    { "sendcanceltradesbypair_OMNI", 2 },
    { "sendcancelalltrades_OMNI", 1 },
    { "sendissuancefixed_OMNI", 1 },
    { "sendissuancefixed_OMNI", 2 },
    { "sendissuancefixed_OMNI", 3 },
    { "sendissuancemanaged_OMNI", 1 },
    { "sendissuancemanaged_OMNI", 2 },
    { "sendissuancemanaged_OMNI", 3 },
    { "sendissuancecrowdsale_OMNI", 1 },
    { "sendissuancecrowdsale_OMNI", 2 },
    { "sendissuancecrowdsale_OMNI", 3 },
    { "sendissuancecrowdsale_OMNI", 9 },
    { "sendissuancecrowdsale_OMNI", 11 },
    { "sendissuancecrowdsale_OMNI", 12 },
    { "sendissuancecrowdsale_OMNI", 13 },
    { "senddexsell_OMNI", 1 },
    { "senddexsell_OMNI", 4 },
    { "senddexsell_OMNI", 6 },
    { "senddexaccept_OMNI", 2 },
    { "senddexaccept_OMNI", 4 },
    { "sendclosecrowdsale_OMNI", 1 },
    { "sendgrant_OMNI", 2 },
    { "sendrevoke_OMNI", 1 },
    { "sendchangeissuer_OMNI", 2 },
    { "sendalert_OMNI", 1 },
    { "sendalert_OMNI", 2 },
    { "sendalert_OMNI", 3 },
    { "sendalert_OMNI", 4 },
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

