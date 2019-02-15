// Copyright (c) 2018-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparamsbase.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <util/strencodings.h>
#include <wallet/rpcsigner.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>

#ifdef ENABLE_EXTERNAL_SIGNER

static UniValue enumeratesigners(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
            RPCHelpMan{"enumeratesigners\n",
                "Returns a list of external signers from -signer.\n",
                {},
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::ARR, "signers", /* optional */ false, "",
                        {
                            {RPCResult::Type::STR_HEX, "masterkeyfingerprint", "Master key fingerprint"},
                            {RPCResult::Type::STR, "name", "Device name"},
                        },
                        }
                    }
                },
                RPCExamples{""}
            }.ToString()
        );
    }

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    const std::string command = gArgs.GetArg("-signer", DEFAULT_EXTERNAL_SIGNER);
    if (command == "") throw JSONRPCError(RPC_WALLET_ERROR, "Error: restart bitcoind with -signer=<cmd>");
    std::string chain = gArgs.GetChainName();
    const bool mainnet = chain == CBaseChainParams::MAIN;
    UniValue signers_res = UniValue::VARR;
    try {
        std::vector<ExternalSigner> signers;
        ExternalSigner::Enumerate(command, signers, mainnet);
        for (ExternalSigner signer : signers) {
            UniValue signer_res = UniValue::VOBJ;
            signer_res.pushKV("fingerprint", signer.m_fingerprint);
            signer_res.pushKV("name", signer.m_name);
            signers_res.push_back(signer_res);
        }
    } catch (const ExternalSignerException& e) {
        throw JSONRPCError(RPC_WALLET_ERROR, e.what());
    }
    UniValue result(UniValue::VOBJ);
    result.pushKV("signers", signers_res);
    return result;
}


Span<const CRPCCommand> GetSignerRPCCommands()
{

// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                                actor (function)                argNames
    //  --------------------- ------------------------          -----------------------         ----------
    { "signer",             "enumeratesigners",                 &enumeratesigners,              {} },
};
// clang-format on
    return MakeSpan(commands);
}


#endif // ENABLE_EXTERNAL_SIGNER
