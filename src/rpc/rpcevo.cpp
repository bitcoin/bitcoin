// Copyright (c) 2018-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <core_io.h>
#include <init.h>
#include <rpc/server.h>
#include <util/moneystr.h>
#include <validation.h>

#include <evo/deterministicmns.h>

#include <bls/bls.h>

#include <masternode/masternodemeta.h>
#include <rpc/util.h>
#include <rpc/blockchain.h>
#include <node/context.h>
#include <rpc/server_util.h>
#include <llmq/quorums_utils.h>
UniValue BuildDMNListEntry(const node::NodeContext& node, const CDeterministicMN& dmn, bool detailed)
{
    if (!detailed) {
        return dmn.proTxHash.ToString();
    }
    UniValue o(UniValue::VOBJ);

    dmn.ToJson(*node.chain, o);
    std::map<COutPoint, Coin> coins;
    coins[dmn.collateralOutpoint]; 
    node.chain->findCoins(coins);
    int confirmations = 0;
    const Coin &coin = coins.at(dmn.collateralOutpoint);
    if (!coin.IsSpent()) {
        confirmations = *node.chain->getHeight() - coin.nHeight;
    }
    o.pushKV("confirmations", confirmations);
    auto metaInfo = mmetaman->GetMetaInfo(dmn.proTxHash);
    o.pushKV("metaInfo", metaInfo->ToJson());

    return o;
}

static RPCHelpMan protx_list()
{
    return RPCHelpMan{"protx_list",
        "\nLists all ProTxs on-chain, depending on the given type.\n"
        "This will also include ProTx which failed PoSe verification.\n",
        {
             {"type", RPCArg::Type::STR, RPCArg::Default{"registered"},
            "\nAvailable types:\n"
            "  registered   - List all ProTx which are registered at the given chain height.\n"
            "                 This will also include ProTx which failed PoSe verification.\n"
            "  valid        - List only ProTx which are active/valid at the given chain height.\n"},
            {"detailed", RPCArg::Type::BOOL,  RPCArg::Default{false}, "If true, only the hashes of the ProTx will be returned."},
            {"height", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Height to look for ProTx transactions, if not specified defaults to current chain-tip"},                   
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("protx_list", "registered true")
            + HelpExampleRpc("protx_list", "\"registered\", true")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{

    const node::NodeContext& node = EnsureAnyNodeContext(request.context);
    std::string type = "registered";
    if (!request.params[0].isNull()) {
        type = request.params[0].get_str();
    }

    UniValue ret(UniValue::VARR);

    if (type == "valid" || type == "registered") {
        CDeterministicMNList mnList;
        bool detailed = !request.params[1].isNull() ? request.params[1].get_bool() : false;
        {
            LOCK(cs_main);
            int height = !request.params[2].isNull() ? request.params[2].getInt<int>() : node.chainman->ActiveHeight();
            if (height < 1 || height > node.chainman->ActiveHeight()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");
            }
            mnList = deterministicMNManager->GetListForBlock(node.chainman->ActiveChain()[height]);
        }
        bool onlyValid = type == "valid";
        mnList.ForEachMN(onlyValid, [&](const auto& dmn) {
            ret.push_back(BuildDMNListEntry(node, dmn, detailed));
        });

    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid type specified");
    }
    

    return ret;
},
    };
} 

static RPCHelpMan protx_info()
{
    return RPCHelpMan{"protx_info",
        "\nReturns detailed information about a deterministic masternode.\n",
        {
            {"proTxHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hash of the initial ProRegTx."},                 
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("protx_info", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("protx_info", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    const node::NodeContext& node = EnsureAnyNodeContext(request.context);
    uint256 proTxHash = ParseHashV(request.params[0], "proTxHash");
    auto mnList = deterministicMNManager->GetListAtChainTip();
    auto dmn = mnList.GetMN(proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s not found", proTxHash.ToString()));
    }
   
    return BuildDMNListEntry(node, *dmn, true);
},
    };
}

static RPCHelpMan bls_generate()
{
     return RPCHelpMan{"bls_generate",
        "\nReturns a BLS secret/public key pair.\n",
        {   
            {"legacy", RPCArg::Type::BOOL, RPCArg::Default{true}, "Use legacy BLS scheme"},           
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("bls_generate", "")
            + HelpExampleRpc("bls_generate", "")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    node::NodeContext& node = request.nodeContext? *request.nodeContext: EnsureAnyNodeContext (request.context);
    CBLSSecretKey sk;
    sk.MakeNewKey();
    bool bls_legacy_scheme;
    {
        LOCK(cs_main);
        bls_legacy_scheme = !llmq::CLLMQUtils::IsV19Active(node.chainman->ActiveHeight());
        if (!request.params[0].isNull()) {
            bls_legacy_scheme = request.params[0].get_bool();
        }
    }
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("secret", sk.ToString());
    ret.pushKV("public", sk.GetPublicKey().ToString(bls_legacy_scheme));
    std::string bls_scheme_str = bls_legacy_scheme ? "legacy" : "basic";
    ret.pushKV("scheme", bls_scheme_str);
    return ret;
},
    };
} 
static CBLSSecretKey ParseBLSSecretKey(const std::string& hexKey, const std::string& paramName)
{
    CBLSSecretKey secKey;
    if (!secKey.SetHexStr(hexKey)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid BLS secret key", paramName));
    }
    return secKey;
}
static RPCHelpMan bls_fromsecret()
{
     return RPCHelpMan{"bls_fromsecret",
        "\nParses a BLS secret key and returns the secret/public key pair.\n",
        {              
             {"secret", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The BLS secret key."},  
             {"legacy", RPCArg::Type::BOOL, RPCArg::Default{true}, "Use legacy BLS scheme"},
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("bls_fromsecret", "")
            + HelpExampleRpc("bls_fromsecret", "")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    node::NodeContext& node = request.nodeContext? *request.nodeContext: EnsureAnyNodeContext (request.context);
    CBLSSecretKey sk = ParseBLSSecretKey(request.params[0].get_str(), "secretKey");
    bool bls_legacy_scheme;
    {
        LOCK(cs_main);
        bls_legacy_scheme = !llmq::CLLMQUtils::IsV19Active(node.chainman->ActiveHeight());
        if (!request.params[1].isNull()) {
            bls_legacy_scheme = request.params[1].get_bool();
        }
    }
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("secret", sk.ToString());
    ret.pushKV("public", sk.GetPublicKey().ToString(bls_legacy_scheme));
    std::string bls_scheme_str = bls_legacy_scheme ? "legacy" : "basic";
    ret.pushKV("scheme", bls_scheme_str);
    return ret;
},
    };
}


void RegisterEvoRPCCommands(CRPCTable &t)
{
    static const CRPCCommand commands[]{
        {"evo", &bls_generate},
        {"evo", &bls_fromsecret},
        {"evo", &protx_list},
        {"evo", &protx_info},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
