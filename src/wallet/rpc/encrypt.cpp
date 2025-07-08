// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/util.h>
#include <scheduler.h>
#include <wallet/context.h>
#include <wallet/rpc/util.h>
#include <wallet/wallet.h>


namespace wallet {
RPCHelpMan walletpassphrase()
{
    return RPCHelpMan{
        "walletpassphrase",
        "Stores the wallet decryption key in memory for 'timeout' seconds.\n"
                "This is needed prior to performing transactions related to private keys such as sending bitcoins\n"
            "\nNote:\n"
            "Issuing the walletpassphrase command while the wallet is already unlocked will set a new unlock\n"
            "time that overrides the old one.\n",
                {
                    {"passphrase", RPCArg::Type::STR, RPCArg::Optional::NO, "The wallet passphrase"},
                    {"timeout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The time to keep the decryption key in seconds; capped at 100000000 (~3 years)."},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
            "\nUnlock the wallet for 60 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"my pass phrase\" 60") +
            "\nLock the wallet again (before 60 seconds)\n"
            + HelpExampleCli("walletlock", "") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("walletpassphrase", "\"my pass phrase\", 60")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return UniValue::VNULL;
    CWallet* const pwallet = wallet.get();

    int64_t nSleepTime;
    int64_t relock_time;
    // Prevent concurrent calls to walletpassphrase with the same wallet.
    LOCK(pwallet->m_unlock_mutex);
    {
        LOCK(pwallet->cs_wallet);

        if (!pwallet->IsCrypted()) {
            throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrase was called.");
        }

        // Note that the walletpassphrase is stored in request.params[0] which is not mlock()ed
        SecureString strWalletPass;
        strWalletPass.reserve(100);
        strWalletPass = std::string_view{request.params[0].get_str()};

        // Get the timeout
        nSleepTime = request.params[1].getInt<int64_t>();
        // Timeout cannot be negative, otherwise it will relock immediately
        if (nSleepTime < 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Timeout cannot be negative.");
        }
        // Clamp timeout
        constexpr int64_t MAX_SLEEP_TIME = 100000000; // larger values trigger a macos/libevent bug?
        if (nSleepTime > MAX_SLEEP_TIME) {
            nSleepTime = MAX_SLEEP_TIME;
        }

        if (strWalletPass.empty()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "passphrase cannot be empty");
        }

        if (!pwallet->Unlock(strWalletPass)) {
            // Check if the passphrase has a null character (see #27067 for details)
            if (strWalletPass.find('\0') == std::string::npos) {
                throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");
            } else {
                throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered is incorrect. "
                                                                    "It contains a null character (ie - a zero byte). "
                                                                    "If the passphrase was set with a version of this software prior to 25.0, "
                                                                    "please try again with only the characters up to — but not including — "
                                                                    "the first null character. If this is successful, please set a new "
                                                                    "passphrase to avoid this issue in the future.");
            }
        }

        pwallet->TopUpKeyPool();

        pwallet->nRelockTime = GetTime() + nSleepTime;
        relock_time = pwallet->nRelockTime;
    }

    // Get wallet scheduler to queue up the relock callback in the future.
    // Scheduled events don't get destructed until they are executed,
    // and they are executed in series in a single scheduler thread so
    // no cs_wallet lock is needed.
    WalletContext& context = EnsureWalletContext(request.context);
    // Keep a weak pointer to the wallet so that it is possible to unload the
    // wallet before the following callback is called. If a valid shared pointer
    // is acquired in the callback then the wallet is still loaded.
    std::weak_ptr<CWallet> weak_wallet = wallet;
    context.scheduler->scheduleFromNow([weak_wallet, relock_time] {
        if (auto shared_wallet = weak_wallet.lock()) {
            LOCK2(shared_wallet->m_relock_mutex, shared_wallet->cs_wallet);
            // Skip if this is not the most recent relock callback.
            if (shared_wallet->nRelockTime != relock_time) return;
            shared_wallet->Lock();
            shared_wallet->nRelockTime = 0;
        }
    }, std::chrono::seconds(nSleepTime));

    return UniValue::VNULL;
},
    };
}


RPCHelpMan walletpassphrasechange()
{
    return RPCHelpMan{
        "walletpassphrasechange",
        "Changes the wallet passphrase from 'oldpassphrase' to 'newpassphrase'.\n",
                {
                    {"oldpassphrase", RPCArg::Type::STR, RPCArg::Optional::NO, "The current passphrase"},
                    {"newpassphrase", RPCArg::Type::STR, RPCArg::Optional::NO, "The new passphrase"},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
                    HelpExampleCli("walletpassphrasechange", "\"old one\" \"new one\"")
            + HelpExampleRpc("walletpassphrasechange", "\"old one\", \"new one\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    if (!pwallet->IsCrypted()) {
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrasechange was called.");
    }

    if (pwallet->IsScanningWithPassphrase()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: the wallet is currently being used to rescan the blockchain for related transactions. Please call `abortrescan` before changing the passphrase.");
    }

    LOCK2(pwallet->m_relock_mutex, pwallet->cs_wallet);

    SecureString strOldWalletPass;
    strOldWalletPass.reserve(100);
    strOldWalletPass = std::string_view{request.params[0].get_str()};

    SecureString strNewWalletPass;
    strNewWalletPass.reserve(100);
    strNewWalletPass = std::string_view{request.params[1].get_str()};

    if (strOldWalletPass.empty() || strNewWalletPass.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "passphrase cannot be empty");
    }

    if (!pwallet->ChangeWalletPassphrase(strOldWalletPass, strNewWalletPass)) {
        // Check if the old passphrase had a null character (see #27067 for details)
        if (strOldWalletPass.find('\0') == std::string::npos) {
            throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");
        } else {
            throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The old wallet passphrase entered is incorrect. "
                                                                "It contains a null character (ie - a zero byte). "
                                                                "If the old passphrase was set with a version of this software prior to 25.0, "
                                                                "please try again with only the characters up to — but not including — "
                                                                "the first null character.");
        }
    }

    return UniValue::VNULL;
},
    };
}


RPCHelpMan walletlock()
{
    return RPCHelpMan{
        "walletlock",
        "Removes the wallet encryption key from memory, locking the wallet.\n"
                "After calling this method, you will need to call walletpassphrase again\n"
                "before being able to call any methods which require the wallet to be unlocked.\n",
                {},
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
            "\nSet the passphrase for 2 minutes to perform a transaction\n"
            + HelpExampleCli("walletpassphrase", "\"my pass phrase\" 120") +
            "\nPerform a send (requires passphrase set)\n"
            + HelpExampleCli("sendtoaddress", "\"" + EXAMPLE_ADDRESS[0] + "\" 1.0") +
            "\nClear the passphrase since we are done before 2 minutes is up\n"
            + HelpExampleCli("walletlock", "") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("walletlock", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    if (!pwallet->IsCrypted()) {
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletlock was called.");
    }

    if (pwallet->IsScanningWithPassphrase()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: the wallet is currently being used to rescan the blockchain for related transactions. Please call `abortrescan` before locking the wallet.");
    }

    LOCK2(pwallet->m_relock_mutex, pwallet->cs_wallet);

    pwallet->Lock();
    pwallet->nRelockTime = 0;

    return UniValue::VNULL;
},
    };
}


RPCHelpMan encryptwallet()
{
    return RPCHelpMan{
        "encryptwallet",
        "Encrypts the wallet with 'passphrase'. This is for first time encryption.\n"
        "After this, any calls that interact with private keys such as sending or signing \n"
        "will require the passphrase to be set prior to making these calls.\n"
                "Use the walletpassphrase call for this, and then walletlock call.\n"
                "If the wallet is already encrypted, use the walletpassphrasechange call.\n"
                "** IMPORTANT **\n"
                "For security reasons, the encryption process will generate a new HD seed, resulting\n"
                "in the creation of a fresh set of active descriptors. Therefore, it is crucial to\n"
                "securely back up the newly generated wallet file using the backupwallet RPC.\n",
                {
                    {"passphrase", RPCArg::Type::STR, RPCArg::Optional::NO, "The pass phrase to encrypt the wallet with. It must be at least 1 character, but should be long."},
                },
                RPCResult{RPCResult::Type::STR, "", "A string with further instructions"},
                RPCExamples{
            "\nEncrypt your wallet\n"
            + HelpExampleCli("encryptwallet", "\"my pass phrase\"") +
            "\nNow set the passphrase to use the wallet, such as for signing or sending bitcoin\n"
            + HelpExampleCli("walletpassphrase", "\"my pass phrase\"") +
            "\nNow we can do something like sign\n"
            + HelpExampleCli("signmessage", "\"address\" \"test message\"") +
            "\nNow lock the wallet again by removing the passphrase\n"
            + HelpExampleCli("walletlock", "") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("encryptwallet", "\"my pass phrase\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    if (pwallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        throw JSONRPCError(RPC_WALLET_ENCRYPTION_FAILED, "Error: wallet does not contain private keys, nothing to encrypt.");
    }

    if (pwallet->IsCrypted()) {
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an encrypted wallet, but encryptwallet was called.");
    }

    if (pwallet->IsScanningWithPassphrase()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: the wallet is currently being used to rescan the blockchain for related transactions. Please call `abortrescan` before encrypting the wallet.");
    }

    LOCK2(pwallet->m_relock_mutex, pwallet->cs_wallet);

    SecureString strWalletPass;
    strWalletPass.reserve(100);
    strWalletPass = std::string_view{request.params[0].get_str()};

    if (strWalletPass.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "passphrase cannot be empty");
    }

    if (!pwallet->EncryptWallet(strWalletPass)) {
        throw JSONRPCError(RPC_WALLET_ENCRYPTION_FAILED, "Error: Failed to encrypt the wallet.");
    }

    return "wallet encrypted; The keypool has been flushed and a new HD seed was generated. You need to make a new backup with the backupwallet RPC.";
},
    };
}
} // namespace wallet
