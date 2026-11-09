// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <common/signmessage.h>
#include <key_io.h>
#include <rpc/util.h>
#include <wallet/rpc/util.h>
#include <wallet/wallet.h>

#include <univalue.h>

#include <variant>

namespace wallet {
RPCMethod signmessage()
{
    return RPCMethod{
        "signmessage",
        "Sign a message for a wallet address. P2PKH uses the legacy compact format; native P2WPKH, P2WSH and P2TR use BIP322 simple proofs.\n"
        "BIP322 signing requires a complete satisfaction using the wallet's available descriptor keys, scripts and other data. Partial proofs are not returned.\n"
        "BIP322 simple proofs fix transaction version, locktime and input sequence to zero.\n"
        "Timelock requirements incompatible with these fields cannot be satisfied by waiting or importing more keys.\n"
        "Signing may still succeed through another satisfiable script branch or a Taproot key path.\n"
        "Full signatures, proofs of funds and P2SH (including wrapped SegWit) are unsupported. signmessagewithprivkey remains legacy-only.\n"
        "A message proof does not establish a current balance or a past payment." +
          HELP_REQUIRING_PASSPHRASE,
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The wallet's P2PKH, native P2WPKH, P2WSH or P2TR address."},
            {"message", RPCArg::Type::STR, RPCArg::Optional::NO, "The message to create a signature of."},
        },
        RPCResult{
            RPCResult::Type::STR, "signature", "Legacy base64 compact signature, or smp followed by a base64-encoded serialized witness stack."
        },
        RPCExamples{
            "\nUnlock the wallet for 30 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"mypassphrase\" 30") +
            "\nCreate the signature\n"
            + HelpExampleCli("signmessage", "\"" + EXAMPLE_ADDRESS[0] + "\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"" + EXAMPLE_ADDRESS[0] + "\" \"signature\" \"my message\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("signmessage", "\"" + EXAMPLE_ADDRESS[0] + "\", \"my message\"")
        },
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
        {
            const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) return UniValue::VNULL;

            LOCK(pwallet->cs_wallet);

            EnsureWalletIsUnlocked(*pwallet);

            std::string strAddress = request.params[0].get_str();
            std::string strMessage = request.params[1].get_str();

            CTxDestination dest = DecodeDestination(strAddress);
            if (!IsValidDestination(dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }

            std::string signature;
            SigningResult err;
            if (const auto pkhash = std::get_if<PKHash>(&dest)) {
                err = pwallet->SignMessage(strMessage, *pkhash, signature);
            } else if (std::holds_alternative<WitnessV0KeyHash>(dest) ||
                       std::holds_alternative<WitnessV0ScriptHash>(dest) ||
                       std::holds_alternative<WitnessV1Taproot>(dest)) {
                err = pwallet->SignBIP322Message(strMessage, dest, signature);
            } else {
                throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");
            }

            if (err == SigningResult::SIGNING_FAILED) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, SigningResultString(err));
            } else if (err != SigningResult::OK) {
                throw JSONRPCError(RPC_WALLET_ERROR, SigningResultString(err));
            }

            return signature;
        },
    };
}
} // namespace wallet
