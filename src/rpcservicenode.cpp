// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "db.h"
#include "init.h"
#include "servicenodeconfig.h"
#include "rpcserver.h"
#include "utilmoneystr.h"
#include "key.h"
#include "base58.h"

#include <fstream>
#include <string>
using namespace std;
using namespace json_spirit;

Value servicenode(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "start" && strCommand != "start-alias" && strCommand != "start-many" && strCommand != "start-all" && 
         strCommand != "start-missing" && strCommand != "start-disabled" && strCommand != "list" && strCommand != "list-conf" && 
         strCommand != "count"  && strCommand != "enforce" && strCommand != "debug" && strCommand != "current" && 
         strCommand != "winners" && strCommand != "genkey" && strCommand != "connect" && strCommand != "outputs" && 
         strCommand != "status" && strCommand != "calcscore"))
        throw runtime_error(
                "servicenode \"command\"... ( \"passphrase\" )\n"
                "Set of commands to execute servicenode related actions\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "2. \"passphrase\"     (string, optional) The wallet passphrase\n"
                "\nAvailable commands:\n"
                "  count        - Print number of all known servicenodes (optional: 'ds', 'enabled', 'all', 'qualify')\n"
                "  current      - Print info on current servicenode winner\n"
                "  debug        - Print servicenode status\n"
                "  genkey       - Generate new servicenodeprivkey\n"
                "  enforce      - Enforce servicenode payments\n"
                "  outputs      - Print servicenode compatible outputs\n"
                "  start        - Start servicenode configured in crown.conf\n"
                "  start-alias  - Start single servicenode by assigned alias configured in servicenode.conf\n"
                "  start-<mode> - Start servicenodes configured in servicenode.conf (<mode>: 'all', 'missing', 'disabled')\n"
                "  status       - Print servicenode status information\n"
                "  list         - Print list of all known servicenodes (see servicenodelist for more info)\n"
                "  list-conf    - Print servicenode.conf in JSON format\n"
                "  winners      - Print list of servicenode winners\n"
                );

    if (strCommand == "list")
    {
        Array newParams(params.size() - 1);
        std::copy(params.begin() + 1, params.end(), newParams.begin());
        return servicenodelist(newParams, fHelp);
    }

    if (strCommand == "budget")
    {
        return "Show budgets";
    }

    if(strCommand == "connect")
    {
        std::string strAddress = "";
        if (params.size() == 2) {
            strAddress = params[1].get_str();
        } else {
            throw runtime_error("Servicenode address required\n");
        }

        CService addr = CService(strAddress);

        CNode *pnode = ConnectNode((CAddress)addr, NULL, false);
        if(pnode){
            pnode->Release();
            return "successfully connected";
        } else {
            throw runtime_error("error connecting\n");
        }
    }

    if (strCommand == "count")
    {
        // TODO to be implemented
        return "Option is not implemented yet";
    }

    if (strCommand == "current")
    {
        // TODO to be implemented
        return "Option is not implemented yet";
    }

    if (strCommand == "genkey")
    {
        CKey secret;
        secret.MakeNewKey(false);

        return CBitcoinSecret(secret).ToString();
    }


    return Value::null;
}

Value servicenodelist(const Array& params, bool fHelp)
{
    return Value::null;
}

Value servicenodebroadcast(const Array& params, bool fHelp)
{
    return Value::null;
}
