// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_OUTPUTTYPE_H
#define BITCOIN_OUTPUTTYPE_H

#include <addresstype.h>
#include <script/signingprovider.h>

#include <array>
#include <optional>
#include <string_view>
#include <vector>

/**
 * @enum OutputType
 * Address script output formats supported by the wallet.
 *
 * LEGACY      Traditional pay-to-pubkey-hash.
 * P2SH_SEGWIT Pay-to-witness-v0 (segwit) nested in P2SH.
 * BECH32      Native witness v0 (bech32). Segwit.
 * BECH32M     Native witness v1+ (bech32m). Taproot.
 * UNKNOWN     Fallback for unrecognized or future types.
 */
enum class OutputType {
    LEGACY,
    P2SH_SEGWIT,
    BECH32,
    BECH32M,
    UNKNOWN,
};

/**
 * List of all “user-visible” OutputType variants.
 *
 * Excludes UNKNOWN, for use in UIs and configuration enumeration.
 */
static constexpr std::array<OutputType, 4> OUTPUT_TYPES{{
    OutputType::LEGACY,
    OutputType::P2SH_SEGWIT,
    OutputType::BECH32,
    OutputType::BECH32M,
}};

/**
 * All OutputType string literals in enum order.
 *
 * Includes a final entry for UNKNOWN.
 */
static constexpr std::array<std::string_view, 5> OUTPUT_TYPE_STRINGS_ALL{{
    "legacy",
    "p2sh-segwit",
    "bech32",
    "bech32m",
    "unknown"
}};

/**
 * Convert an OutputType to its string label.
 * @param t  The output type to format.
 * @return A compile-time string_view corresponding to t.
 *
 * @note No heap allocation or dynamic initialization occurs.
 */
inline std::string_view FormatOutputType(OutputType t) noexcept
{
    // static_cast<size_t>(t) is guaranteed 0…4
    return OUTPUT_TYPE_STRINGS_ALL[static_cast<size_t>(t)];
}

/**
 * Parse a string into an OutputType enum.
 * @param s  The string label to parse.
 * @return The matching OutputType, or std::nullopt if unrecognized.
 */
inline std::optional<OutputType> ParseOutputType(std::string_view s) noexcept
{
    for (size_t i = 0; i < OUTPUT_TYPE_STRINGS_ALL.size(); ++i) {
        if (OUTPUT_TYPE_STRINGS_ALL[i] == s) {
            return static_cast<OutputType>(i);
        }
    }
    return std::nullopt;
}

/**
 * Retrieve the array of labels for all user-visible output types.
 * @return A constexpr array of string_views: {"legacy", "p2sh-segwit", "bech32", "bech32m"}.
 *
 * @note This never triggers any dynamic initialization.
 */
static constexpr auto GetOutputTypeStrings() noexcept
{
    std::array<std::string_view, OUTPUT_TYPES.size()> ret{};
    for (size_t i = 0; i < OUTPUT_TYPES.size(); ++i) {
        ret[i] = FormatOutputType(OUTPUT_TYPES[i]);
    }
    return ret;
}

/**
 * Get a destination of the requested type (if possible) to the specified public key.
 * The caller must make sure LearnRelatedScripts has been called beforehand.
 * @param key   The public key.
 * @param type  The target OutputType.
 * @return The corresponding script destination.
 */
CTxDestination GetDestinationForKey(const CPubKey& key, OutputType type);

/**
 * Get all destinations (potentially) supported by the wallet for the given public key.
 * @param key  The public key.
 * @return Vector of supported CTxDestination variants.
 */
std::vector<CTxDestination> GetAllDestinationsForKey(const CPubKey& key);

/**
 * Get a destination of the requested type (if possible) to the specified script.
 * This function will automatically add the script (and any other necessary scripts) to the keystore.
 * @param keystore  The signing provider to which scripts are added.
 * @param script    The redeem or witness script.
 * @param type      The desired OutputType.
 * @return The script’s CTxDestination.
 */
CTxDestination AddAndGetDestinationForScript(FlatSigningProvider& keystore, const CScript& script, OutputType type);

/**
 * Get the OutputType for a CTxDestination.
 * @param dest  The destination variant.
 * @return The matching OutputType, or std::nullopt if not one of the known types.
 */
std::optional<OutputType> OutputTypeFromDestination(const CTxDestination& dest);

#endif // BITCOIN_OUTPUTTYPE_H
