// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINPARAMS_H
#define BITCOIN_CHAINPARAMS_H

#include <kernel/chainparams.h> // IWYU pragma: export

#include <memory>

class ArgsManager;

/**
 * Creates and returns a std::unique_ptr<CChainParams> of the chosen chain.
 */
std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const ChainType chain);

/**
 * Return the currently selected parameters. This won't change after app
 * startup, except for unit tests.
 */
const CChainParams &Params();

/**
 * Sets the params returned by Params() to those for the given chain type.
 */
void SelectParams(const ChainType chain);

/**
 * Extracts signet params and signet challenge from wrapped signet challenge.
 * Format of wrapped signet challenge is:
 * If the challenge is in the form "OP_RETURN PUSHDATA<params> PUSHDATA<actual challenge>",
 * If the input challenge does not start with OP_RETURN,
 * sets outParams="" and outChallenge=input.
 * If the input challenge starts with OP_RETURN, but does not satisfy the format,
 * throws an exception.
 */
void ParseWrappedSignetChallenge(const std::vector<uint8_t>& wrappedChallenge, std::vector<uint8_t>& outParams, std::vector<uint8_t>& outChallenge);

/**
 * Parses signet options.
 * The format currently supports only setting pow_target_spacing, but
 * can be extended in the future.
 * Possible values:
 *  - Empty (then do nothing)
 *  - 0x01 (pow_target_spacing as int64_t little endian) => set pow_target_spacing.
 * If the format is wrong, throws an exception.
 */
void ParseSignetParams(const std::vector<uint8_t>& params, CChainParams::SigNetOptions& options);

#endif // BITCOIN_CHAINPARAMS_H
