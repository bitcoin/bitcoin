// Copyright (c) 2018-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparamsbase.h>
#include <key_io.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <util/strencodings.h>
#include <wallet/rpcsigner.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>

#ifdef ENABLE_EXTERNAL_SIGNER

static RPCHelpMan enumeratesigners()
{
    return RPCHelpMan{
        "enumeratesigners",
        "Returns a list of external signers from -signer.",
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
        RPCExamples{""},
        [](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            const std::string command = gArgs.GetArg("-signer", "");
            if (command == "") throw JSONRPCError(RPC_WALLET_ERROR, "Error: restart bitcoind with -signer=<cmd>");
            std::string chain = gArgs.GetChainName();
            UniValue signers_res = UniValue::VARR;
            try {
                std::vector<ExternalSigner> signers;
                ExternalSigner::Enumerate(command, signers, chain);
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
    };
}

static RPCHelpMan signerdisplayaddress()
{
    return RPCHelpMan{
        "signerdisplayaddress",
        "Display address on an external signer for verification.\n",
        {
            {"address",     RPCArg::Type::STR, RPCArg::Optional::NO, /* default_val */ "", "bitcoin address to display"},
        },
        RPCResult{RPCResult::Type::NONE,"",""},
        RPCExamples{""},
        [](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
            if (!wallet) return NullUniValue;
            CWallet* const pwallet = wallet.get();

            LOCK(pwallet->cs_wallet);

            CTxDestination dest = DecodeDestination(request.params[0].get_str());

            // Make sure the destination is valid
            if (!IsValidDestination(dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }

            if (!pwallet->DisplayAddress(dest)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to display address");
            }

            UniValue result(UniValue::VOBJ);
            result.pushKV("address", request.params[0].get_str());
            return result;
        }
    };
}

Span<const CRPCCommand> GetSignerRPCCommands()
{

// clang-format off
static const CRPCCommand commands[] =
{ // category              actor (function)
  // --------------------- ------------------------
  { "signer",              &enumeratesigners,      },
  { "signer",              &signerdisplayaddress,  },
};
// clang-format on
    return MakeSpan(commands);
}


#endif // ENABLE_EXTERNAL_SIGNER
