// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#ifndef BITCOIN_WALLET_DESCRIPTOR_ANALYZER_H
#define BITCOIN_WALLET_DESCRIPTOR_ANALYZER_H

#include <addresstype.h>
#include <script/descriptor_analyzer.h>

#include <memory>
#include <optional>
#include <span>
#include <vector>

class SigningProvider;
struct Descriptor;

namespace wallet {
class CWallet;

/** Which of a descriptor's keys this wallet holds, plus its multisig analysis. */
struct WalletDescriptorAnalysis {
    /** Whether a key occurrence belongs to this wallet.
     *
     * An extended key matches if it is one of the wallet's HD keys, or if its
     * origin fingerprint is one of theirs and deriving the origin path from it
     * gives the same key. A constant key matches if the wallet has its private
     * key, or in the same way from its origin: the one in the descriptor, or the
     * one the wallet recorded when it derived the key. MuSig aggregates are never
     * matched; their participants are.
     */
    enum class KeyMatch {
        NOT_MINE,
        MINE,       //!< The wallet holds the private key, so it can sign for it.
        WATCH_ONLY, //!< The wallet has the key, but not its private key.
        UNKNOWN,    //!< A wallet key has the origin fingerprint, but the wallet is locked, so the path can't be derived.
    };

    //! The wallet-free part. Key IDs below refer to `analysis.policy.keys`.
    DescriptorAnalysis analysis;
    //! One entry per key ID.
    std::vector<KeyMatch> keys;
    //! The first receive address (index 0 of the first path), if it can be derived.
    std::optional<CTxDestination> first_address;

    //! Whether the wallet holds the private key for at least one key, or
    //! nullopt if it can't tell because the wallet is locked. This holds for any
    //! descriptor; whether it is a safe k-of-n is `analysis.IsRecognized()`.
    std::optional<bool> CanSign() const;
};

/** Analyze the descriptors returned by Parse() for one descriptor string, of
 *  any kind, and match their keys against the wallet's keys. `provider` is the
 *  one Parse() filled, so private keys in the descriptor can derive the first
 *  address. A locked wallet is not an error: keys that need a private key to
 *  check are UNKNOWN. Only the candidate private key is decrypted for each key
 *  that needs it. */
WalletDescriptorAnalysis AnalyzeDescriptor(const CWallet& wallet, std::span<const std::unique_ptr<Descriptor>> descriptors, const SigningProvider& provider);

} // namespace wallet

#endif // BITCOIN_WALLET_DESCRIPTOR_ANALYZER_H
