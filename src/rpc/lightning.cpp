// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>

#include <rusty/src/rust_bridge.h>

#include <rpc/util.h>
#include <util/strencodings.h>

#include <univalue.h>

//! Pointer to Lightning node that needs to be declared as a global to be accessible
//! RPC methods. Due to limitations of the RPC framework, there's currently no
//! direct way to pass in state to RPC methods without globals.
extern void* lightning_node;

static UniValue lnconnect(const JSONRPCRequest& request)
{
    RPCHelpMan{"lnconnect",
               "\nConnect to a Lightning Network node.\n",
               {{"node", RPCArg::Type::STR, RPCArg::Optional::NO, "Node to connect to in pubkey@host:port format."},},
               RPCResult{
                       "n          (numeric) FIX\n"
               },
               RPCExamples{
                       HelpExampleCli("lnconnect", "pubkey@host:port")
                       + HelpExampleRpc("lnconnect", "pubkey@host:port")
               },
    }.Check(request);

    std::string strNode = request.params[0].get_str();

    return lightning::connect_peer(lightning_node, strNode.c_str());
}

static UniValue lngetpeers(const JSONRPCRequest& request)
{
    RPCHelpMan{"lngetpeers",
               "\nList all connected Lightning Network nodes.\n",
               {},
               RPCResult{
                       "n          (numeric) FIXME\n"
               },
               RPCExamples{
                       HelpExampleCli("lngetpeers", "")
                       + HelpExampleRpc("lngetpeers", "")
               },
    }.Check(request);

    auto peers = lightning::get_peers(lightning_node);
    std::vector<lightning::Peer> vpeers(peers.ptr, peers.ptr + peers.len);

    UniValue ret(UniValue::VARR);

    for (auto peer : vpeers) {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("id", peer.id);
        ret.push_back(obj);
    };

    return ret;
}

static UniValue lnfundchannel(const JSONRPCRequest& request)
{
    RPCHelpMan{"lnfundchannel",
               "\nFund a Lightning Network channel.\n",
               {{"node_id", RPCArg::Type::STR, RPCArg::Optional::NO, "Node to open a channel with."},
                {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount."},},
               RPCResults{},
               RPCExamples{
                       HelpExampleCli("lnfundchannel", "pubkey amount")
                       + HelpExampleRpc("lnfundchannel", "pubkey amount")
               },
    }.Check(request);

    std::string strNode = request.params[0].get_str();

    CAmount nAmount = AmountFromValue(request.params[1]);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");

    lightning::fund_channel(lightning_node, strNode.c_str(), nAmount);
    return NullUniValue;
}

static UniValue lngetchannels(const JSONRPCRequest& request)
{
    RPCHelpMan{"lngetchannels",
               "\nList all Lightning Network channels.\n",
               {},
               RPCResult{
                       "n          (numeric) FIXME\n"
               },
               RPCExamples{
                       HelpExampleCli("lngetchannels", "")
                       + HelpExampleRpc("lngetchannels", "")
               },
    }.Check(request);


    auto channels = lightning::get_channels(lightning_node);
    std::vector<lightning::Channel> vchannels(channels.ptr, channels.ptr + channels.len);

    UniValue ret(UniValue::VARR);

    for (auto channel : vchannels) {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("id", channel.id);
        obj.pushKV("shortid", channel.short_id);
        obj.pushKV("capacity", channel.capacity);
        obj.pushKV("status", channel.short_id ? "confirmed" : "unconfirmed");
        ret.push_back(obj);
    };

    return ret;
}

static UniValue lnpayinvoice(const JSONRPCRequest& request)
{
    RPCHelpMan{"lnpayinvoice",
               "\nPay a Lightning Network invoice.\n",
               {{"invoice", RPCArg::Type::STR, RPCArg::Optional::NO, "BOLT-11 encoded invoice string"},},
               RPCResults{},
               RPCExamples{
                       HelpExampleCli("lnpayinvoice", "invoice")
                       + HelpExampleRpc("lnpayinvoice", "invoice")
               },
    }.Check(request);

    std::string bolt11 = request.params[0].get_str();

    lightning::pay_invoice(lightning_node, bolt11.c_str());
    return NullUniValue;
}

static UniValue lncreateinvoice(const JSONRPCRequest& request)
{
    RPCHelpMan{"lncreateinvoice",
               "\nFund a Lightning Network channel.\n",
               {{"description", RPCArg::Type::STR, RPCArg::Optional::NO, "Invoice description."},
                {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount."},},
               RPCResult{
                       "s          (string) BOLT-11 encoded invoice\n"
               },
               RPCExamples{
                       HelpExampleCli("lncreateinvoice", "description amount")
                       + HelpExampleRpc("lncreateinvoice", "description amount")
               },
    }.Check(request);

    std::string description = request.params[0].get_str();

    CAmount nAmount = AmountFromValue(request.params[1]);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");

    return lightning::create_invoice(lightning_node, description.c_str(), nAmount);
}

static UniValue lnclosechannel(const JSONRPCRequest& request)
{
    RPCHelpMan{"lnclosechannel",
               "\nClose a Lightning Network channel.\n",
               {{"channel_id", RPCArg::Type::STR, RPCArg::Optional::NO, "Channel ID."},},
               RPCResults{},
               RPCExamples{
                       HelpExampleCli("lnclosechannel", "channel_id")
                       + HelpExampleRpc("lnclosechannel", "channel_id")
               },
    }.Check(request);

    std::string strChannelId = request.params[0].get_str();

    lightning::close_channel(lightning_node, strChannelId.c_str());
    return NullUniValue;
}

// clang-format off
static const CRPCCommand commands[] =
        { //  category              name                      actor (function)         argNames
                //  --------------------- ------------------------  -----------------------  ----------
                { "lightning",            "lnconnect",                &lnconnect,                {"node"} },
                { "lightning",            "lngetpeers",               &lngetpeers,               {} },
                { "lightning",            "lnfundchannel",            &lnfundchannel,            {"node_id", "amount"} },
                { "lightning",            "lngetchannels",            &lngetchannels,            {} },
                { "lightning",            "lnpayinvoice",             &lnpayinvoice,             {"invoice"} },
                { "lightning",            "lncreateinvoice",          &lncreateinvoice,          {"description", "amount"} },
                { "lightning",            "lnclosechannel",           &lnclosechannel,           {"channel_id"} },
        };
// clang-format on

void RegisterLightningRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}