// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <outputtype.h>

#include <pubkey.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <util/vector.h>

#include <assert.h>
#include <string>

static const std::string OUTPUT_TYPE_STRING_LEGACY = "legacy";
static const std::string OUTPUT_TYPE_STRING_UNKNOWN = "unknown";

const std::array<OutputType, 2> OUTPUT_TYPES = {OutputType::LEGACY, OutputType::UNKNOWN};

bool ParseOutputType(const std::string& type, OutputType& output_type)
{
    if (type == OUTPUT_TYPE_STRING_LEGACY) {
        output_type = OutputType::LEGACY;
        return true;
    }
    return false;
}

const std::string& FormatOutputType(OutputType type)
{
    switch (type) {
    case OutputType::LEGACY: return OUTPUT_TYPE_STRING_LEGACY;
    case OutputType::UNKNOWN: return OUTPUT_TYPE_STRING_UNKNOWN;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

CTxDestination GetDestinationForKey(const CPubKey& key, OutputType type)
{
    switch (type) {
    case OutputType::LEGACY: return PKHash(key);
    case OutputType::UNKNOWN: {} // This function should never be used with UNKNOWN, so let it assert
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

std::vector<CTxDestination> GetAllDestinationsForKey(const CPubKey& key)
{
    PKHash keyid(key);
    CTxDestination p2pkh{keyid};
    return Vector(std::move(p2pkh));
}

CTxDestination AddAndGetDestinationForScript(FillableSigningProvider& keystore, const CScript& script, OutputType type)
{
    // Add script to keystore
    keystore.AddCScript(script);
    ScriptHash sh(script);
    // Note that scripts over 520 bytes are not yet supported.
    switch (type) {
    case OutputType::LEGACY:
        keystore.AddCScript(GetScriptForDestination(sh));
        return sh;
    case OutputType::UNKNOWN: {} // This function should not be used for UNKNOWN, so let it assert
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}
