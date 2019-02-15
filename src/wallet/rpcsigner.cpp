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

#ifdef HAVE_BOOST_PROCESS

static UniValue enumeratesigners(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();

    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
            RPCHelpMan{"enumeratesigners\n",
                "Returns a list of external signers from -signer and associates them\n"
                "with the wallet until you stop bitcoind.\n",
                {},
                RPCResult{
                    "{\n"
                    "  \"signers\" : [                              (json array of objects)\n"
                    "    {\n"
                    "      \"masterkeyfingerprint\" : \"fingerprint\" (string) Master key fingerprint\n"
                    "    }\n"
                    "    ,...\n"
                    "  ]\n"
                    "}\n"
                },
                RPCExamples{""}
            }.ToString()
        );
    }

    const std::string command = gArgs.GetArg("-signer", DEFAULT_EXTERNAL_SIGNER);
    if (command == "") throw JSONRPCError(RPC_WALLET_ERROR, "Error: restart bitcoind with -signer=<cmd>");
    std::string chain = gArgs.GetChainName();
    const bool mainnet = chain == CBaseChainParams::MAIN;
    UniValue signers;
    try {
        signers = ExternalSigner::Enumerate(command, pwallet->m_external_signers, mainnet);
    } catch (const ExternalSignerException& e) {
        throw JSONRPCError(RPC_WALLET_ERROR, e.what());
    }
    UniValue result(UniValue::VOBJ);
    result.pushKV("signers", signers);
    return result;
}

// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                                actor (function)                argNames
    //  --------------------- ------------------------          -----------------------         ----------
    { "signer",             "enumeratesigners",                 &enumeratesigners,              {} },
};
// clang-format on

void RegisterSignerRPCCommands(interfaces::Chain& chain, std::vector<std::unique_ptr<interfaces::Handler>>& handlers)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        handlers.emplace_back(chain.handleRpc(commands[vcidx]));
}
#endif // HAVE_BOOST_PROCESS
