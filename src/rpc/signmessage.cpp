// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/signmessage.h>
#include <rpc/register.h> // IWYU pragma: associated

#include <addresstype.h>
#include <key.h>
#include <key_io.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <univalue.h>

#include <string>
#include <string_view>
#include <variant>
#include <vector>

static RPCMethod verifymessage()
{
    return RPCMethod{"verifymessage",
        "Verify a signed message. P2PKH uses legacy compact signatures; native P2WPKH, P2WSH and P2TR use BIP322 simple proofs.\n"
        "Full signatures, proofs of funds and P2SH (including wrapped SegWit) are unsupported. No wallet or chain data is required.\n"
        "A valid message proof does not establish a current balance or a past payment.",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The bitcoin address to use for the signature."},
            {"signature", RPCArg::Type::STR, RPCArg::Optional::NO, "Legacy base64 compact signature, or a BIP322 simple proof: smp followed by a base64-encoded serialized witness stack. Older unprefixed simple proofs are also accepted. Malformed base64 or witness serialization raises an error."},
            {"message", RPCArg::Type::STR, RPCArg::Optional::NO, "The message that was signed."},
        },
        RPCResult{
            RPCResult::Type::BOOL, "", "True for a verified signature; false for invalid, unsupported or inconclusive proofs."
        },
        RPCExamples{
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"" + EXAMPLE_ADDRESS[0] + "\" \"signature\" \"my message\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("verifymessage", "\"" + EXAMPLE_ADDRESS[0] + "\", \"signature\", \"my message\"")
        },
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
        {
            const std::string address{self.Arg<std::string_view>("address")};
            const auto signature = self.Arg<std::string_view>("signature");
            const auto message = self.Arg<std::string_view>("message");
            const CTxDestination destination = DecodeDestination(address);
            if (std::holds_alternative<WitnessV0KeyHash>(destination) ||
                std::holds_alternative<WitnessV0ScriptHash>(destination) ||
                std::holds_alternative<WitnessV1Taproot>(destination)) {
                switch (BIP322Verify(destination, signature, message)) {
                case BIP322VerificationResult::OK:
                    return true;
                case BIP322VerificationResult::MALFORMED_BASE64:
                    throw JSONRPCError(RPC_TYPE_ERROR, "Malformed base64 encoding");
                case BIP322VerificationResult::MALFORMED_WITNESS:
                    throw JSONRPCError(RPC_TYPE_ERROR, "Malformed BIP322 witness");
                case BIP322VerificationResult::UNSUPPORTED_ADDRESS:
                    throw JSONRPCError(RPC_TYPE_ERROR, "Unsupported address type");
                case BIP322VerificationResult::INVALID:
                case BIP322VerificationResult::INCONCLUSIVE:
                case BIP322VerificationResult::UNSUPPORTED_FORMAT:
                    return false;
                }
                return false;
            }

            switch (MessageVerify(address, std::string{signature}, std::string{message})) {
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

static RPCMethod signmessagewithprivkey()
{
    return RPCMethod{
        "signmessagewithprivkey",
        "Sign a message with a private key using the legacy compact format (for P2PKH addresses only).\n",
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
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
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
