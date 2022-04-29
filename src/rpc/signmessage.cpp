// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <key_io.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <univalue.h>
#include <util/message.h>

#include <string>

static RPCHelpMan verifymessage()
{
    return RPCHelpMan{"verifymessage",
        "Verify a signed message.",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The syscoin address to use for the signature."},
            {"signature", RPCArg::Type::STR, RPCArg::Optional::NO, "The signature provided by the signer in base 64 encoding (see signmessage)."},
            {"message", RPCArg::Type::STR, RPCArg::Optional::NO, "The message that was signed."},
        },
        RPCResult{
            RPCResult::Type::BOOL, "", "If the signature is verified or not."
        },
        RPCExamples{
            "\nUnlock the wallet for 30 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"mypassphrase\" 30") +
            "\nCreate the signature\n"
            + HelpExampleCli("signmessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\" \"signature\" \"my message\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\", \"signature\", \"my message\"")
        },
        [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
        {
            std::string strAddress = request.params[0].get_str();
            std::string strSign = request.params[1].get_str();
            std::string strMessage = request.params[2].get_str();

            switch (MessageVerify(strAddress, strSign, strMessage)) {
            case MessageVerificationResult::ERR_INVALID_ADDRESS:
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            case MessageVerificationResult::ERR_ADDRESS_NO_KEY:
                throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");
            case MessageVerificationResult::ERR_MALFORMED_SIGNATURE:
                throw JSONRPCError(RPC_TYPE_ERROR, "Malformed base64 encoding");
            case MessageVerificationResult::ERR_PUBKEY_NOT_RECOVERED:
            case MessageVerificationResult::ERR_NOT_SIGNED:
                return false;
            case MessageVerificationResult::OK:
                return true;
            }

            return false;
        },
    };
}

static RPCHelpMan signmessagewithprivkey()
{
    return RPCHelpMan{"signmessagewithprivkey",
        "\nSign a message with the private key of an address\n",
        {
            {"privkey", RPCArg::Type::STR, RPCArg::Optional::NO, "The private key to sign the message with."},
            {"message", RPCArg::Type::STR, RPCArg::Optional::NO, "The message to create a signature of."},
        },
        RPCResult{
            RPCResult::Type::STR, "signature", "The signature of the message encoded in base 64"
        },
        RPCExamples{
            "\nCreate the signature\n"
            + HelpExampleCli("signmessagewithprivkey", "\"privkey\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\" \"signature\" \"my message\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("signmessagewithprivkey", "\"privkey\", \"my message\"")
        },
        [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
        {
            std::string strPrivkey = request.params[0].get_str();
            std::string strMessage = request.params[1].get_str();

            CKey key = DecodeSecret(strPrivkey);
            if (!key.IsValid()) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
            }

            std::string signature;

            if (!MessageSign(key, strMessage, signature)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sign failed");
            }

            return signature;
        },
    };
}

void RegisterSignMessageRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"util", &verifymessage},
        {"util", &signmessagewithprivkey},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
