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
#include <evo/simplifiedmns.h>

#include <bls/bls.h>

#include <masternode/masternodemeta.h>
#include <rpc/util.h>
#include <rpc/blockchain.h>
#include <node/context.h>


UniValue BuildDMNListEntry(const NodeContext& node, const CDeterministicMNCPtr& dmn, bool detailed)
{
    if (!detailed) {
        return dmn->proTxHash.ToString();
    }
    UniValue o(UniValue::VOBJ);

    dmn->ToJson(o);

    int confirmations = GetUTXOConfirmations(dmn->collateralOutpoint);
    o.pushKV("confirmations", confirmations);
    auto metaInfo = mmetaman.GetMetaInfo(dmn->proTxHash);
    o.pushKV("metaInfo", metaInfo->ToJson());

    return o;
}

static RPCHelpMan protx_list()
{
    return RPCHelpMan{"protx_list",
        "\nLists all ProTxs on-chain, depending on the given type.\n"
        "This will also include ProTx which failed PoSe verification.\n",
        {
             {"type", RPCArg::Type::STR, "registered", "Type of ProTx.\n",
            "\nAvailable types:\n"
            "  registered   - List all ProTx which are registered at the given chain height.\n"
            "                 This will also include ProTx which failed PoSe verification.\n"
            "  valid        - List only ProTx which are active/valid at the given chain height.\n"},
            {"detailed", RPCArg::Type::BOOL, "false", "If true, only the hashes of the ProTx will be returned."},
            {"height", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Height to look for ProTx transactions, if not specified defaults to current chain-tip"},                   
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("protx_list", "registered true")
            + HelpExampleRpc("protx_list", "\"registered\", true")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    const NodeContext& node = EnsureAnyNodeContext(request.context);
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
            int height = !request.params[2].isNull() ? request.params[2].get_int() : ::ChainActive().Height();
            if (height < 1 || height > ::ChainActive().Height()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");
            }
            if(deterministicMNManager)
                deterministicMNManager->GetListForBlock(::ChainActive()[height], mnList);
        }
        bool onlyValid = type == "valid";
        mnList.ForEachMN(onlyValid, [&](const CDeterministicMNCPtr& dmn) {
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
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    uint256 proTxHash = ParseHashV(request.params[0], "proTxHash");
    CDeterministicMNList mnList;
    if(deterministicMNManager)
        deterministicMNManager->GetListAtChainTip(mnList);
    auto dmn = mnList.GetMN(proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s not found", proTxHash.ToString()));
    }
   
    return BuildDMNListEntry(node, dmn, true);
},
    };
} 

static uint256 ParseBlock(const UniValue& v, std::string strName)
{
    try {
        return ParseHashV(v, strName);
    } catch (...) {
        LOCK(cs_main);
        int h = v.get_int();
        if (h < 1 || h > ::ChainActive().Height())
            throw std::runtime_error(strprintf("%s must be a block hash or chain height and not %s", strName, v.getValStr()));
        return *::ChainActive()[h]->phashBlock;
    }
}

static RPCHelpMan protx_diff()
{
    return RPCHelpMan{"protx_diff",
        "\nReturns detailed information about a deterministic masternode.\n",
        {
            {"baseBlock", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The starting block hash."},  
            {"block", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The ending block hash."},                 
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("protx_diff", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("protx_diff", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    uint256 baseBlockHash = ParseBlock(request.params[0], "baseBlock");
    uint256 blockHash = ParseBlock(request.params[1], "block");

    CSimplifiedMNListDiff mnListDiff;
    std::string strError;
    if (!BuildSimplifiedMNListDiff(baseBlockHash, blockHash, mnListDiff, strError)) {
        throw std::runtime_error(strError);
    }

    UniValue ret;
    mnListDiff.ToJson(ret);
    return ret;
},
    };
} 

static RPCHelpMan bls_generate()
{
     return RPCHelpMan{"bls_generate",
        "\nReturns a BLS secret/public key pair.\n",
        {              
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("bls_generate", "")
            + HelpExampleRpc("bls_generate", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    CBLSSecretKey sk;
    sk.MakeNewKey();

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("secret", sk.ToString());
    ret.pushKV("public", sk.GetPublicKey().ToString());
    return ret;
},
    };
} 

static RPCHelpMan bls_fromsecret()
{
     return RPCHelpMan{"bls_fromsecret",
        "\nParses a BLS secret key and returns the secret/public key pair.\n",
        {              
             {"secret", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The BLS secret key."},  
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("bls_fromsecret", "")
            + HelpExampleRpc("bls_fromsecret", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    CBLSSecretKey sk;
    if (!sk.SetHexStr(request.params[0].get_str())) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Secret key must be a valid hex string of length %d", sk.SerSize*2));
    }

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("secret", sk.ToString());
    ret.pushKV("public", sk.GetPublicKey().ToString());
    return ret;
},
    };
} 


void RegisterEvoRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)
  //  --------------------- ------------------------  -----------------------
    { "evo",                &bls_generate,                },
    { "evo",                &bls_fromsecret,              },
    { "evo",                &protx_list,                  },
    { "evo",                &protx_info,                  },
    { "evo",                &protx_diff,                  },
};
// clang-format on
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
