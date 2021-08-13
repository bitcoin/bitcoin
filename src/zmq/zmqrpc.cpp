// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqrpc.h>

#include <rpc/server.h>
#include <rpc/util.h>
#include <zmq/zmqabstractnotifier.h>
#include <zmq/zmqnotificationinterface.h>

#include <univalue.h>

namespace {

UniValue getzmqnotifications(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
            "getzmqnotifications\n"
            "\nReturns information about the active ZeroMQ notifications.\n"
            "\nResult:\n"
            "[\n"
            "  {                        (json object)\n"
            "    \"type\": \"pubhashtx\",   (string) Type of notification\n"
            "    \"address\": \"...\"       (string) Address of the publisher\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getzmqnotifications", "")
            + HelpExampleRpc("getzmqnotifications", "")
        );
    }

    UniValue result(UniValue::VARR);
    if (g_zmq_notification_interface != nullptr) {
        for (const auto* n : g_zmq_notification_interface->GetActiveNotifiers()) {
            UniValue obj(UniValue::VOBJ);
            obj.pushKV("type", n->GetType());
            obj.pushKV("address", n->GetAddress());
            result.push_back(obj);
        }
    }

    return result;
}

const CRPCCommand commands[] =
{ //  category              name                                actor (function)                argNames
  //  -----------------     ------------------------            -----------------------         ----------
    { "zmq",                "getzmqnotifications",              &getzmqnotifications,           {} },
};

} // anonymous namespace

void RegisterZMQRPCCommands(CRPCTable& t)
{
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
