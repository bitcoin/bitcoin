// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/solver.h>
#include <span.h>

#include <algorithm>
#include <cassert>
#include <string>

typedef std::vector<unsigned char> valtype;

std::string GetTxnOutputType(TxoutType t)
{
    switch (t) {
    case TxoutType::NONSTANDARD: return "nonstandard";
    case TxoutType::PUBKEY: return "pubkey";
    case TxoutType::PUBKEYHASH: return "pubkeyhash";
    case TxoutType::SCRIPTHASH: return "scripthash";
    case TxoutType::MULTISIG: return "multisig";
    case TxoutType::NULL_DATA: return "nulldata";
    case TxoutType::ANCHOR: return "anchor";
    case TxoutType::WITNESS_V0_KEYHASH: return "witness_v0_keyhash";
    case TxoutType::WITNESS_V0_SCRIPTHASH: return "witness_v0_scripthash";
    case TxoutType::WITNESS_V1_TAPROOT: return "witness_v1_taproot";
    case TxoutType::WITNESS_UNKNOWN: return "witness_unknown";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

static bool MatchPayToPubkey(const CScript& script, valtype& pubkey)
{
    if (script.size() == CPubKey::SIZE + 2 && script[0] == CPubKey::SIZE && script.back() == OP_CHECKSIG) {
        pubkey.assign(script.begin() + 1, script.begin() + CPubKey::SIZE + 1);
        return CPubKey::ValidSize(pubkey);
    }
    if (script.size() == CPubKey::COMPRESSED_SIZE + 2 && script[0] == CPubKey::COMPRESSED_SIZE && script.back() == OP_CHECKSIG) {
        pubkey.assign(script.begin() + 1, script.begin() + CPubKey::COMPRESSED_SIZE + 1);
        return CPubKey::ValidSize(pubkey);
    }
    return false;
}

static bool MatchPayToPubkeyHash(const CScript& script, valtype& pubkeyhash)
{
    if (script.size() == 25 && script[0] == OP_DUP && script[1] == OP_HASH160 && script[2] == 20 && script[23] == OP_EQUALVERIFY && script[24] == OP_CHECKSIG) {
        pubkeyhash.assign(script.begin () + 3, script.begin() + 23);
        return true;
    }
    return false;
}

/** Test for "small positive integer" script opcodes - OP_1 through OP_16. */
static constexpr bool IsSmallInteger(opcodetype opcode)
{
    return opcode >= OP_1 && opcode <= OP_16;
}

/** Retrieve a minimally-encoded number in range [min,max] from an (opcode, data) pair,
 *  whether it's OP_n or through a push. */
static std::optional<int> GetScriptNumber(opcodetype opcode, valtype data, int min, int max)
{
    int count;
    if (IsSmallInteger(opcode)) {
        count = CScript::DecodeOP_N(opcode);
    } else if (IsPushdataOp(opcode)) {
        if (!CheckMinimalPush(data, opcode)) return {};
        try {
            count = CScriptNum(data, /* fRequireMinimal = */ true).getint();
        } catch (const scriptnum_error&) {
            return {};
        }
    } else {
        return {};
    }
    if (count < min || count > max) return {};
    return count;
}

static bool MatchMultisig(const CScript& script, int& required_sigs, std::vector<valtype>& pubkeys)
{
    opcodetype opcode;
    valtype data;

    CScript::const_iterator it = script.begin();
    if (script.size() < 1 || script.back() != OP_CHECKMULTISIG) return false;

    if (!script.GetOp(it, opcode, data)) return false;
    auto req_sigs = GetScriptNumber(opcode, data, 1, MAX_PUBKEYS_PER_MULTISIG);
    if (!req_sigs) return false;
    required_sigs = *req_sigs;
    while (script.GetOp(it, opcode, data) && CPubKey::ValidSize(data)) {
        pubkeys.emplace_back(std::move(data));
    }
    auto num_keys = GetScriptNumber(opcode, data, required_sigs, MAX_PUBKEYS_PER_MULTISIG);
    if (!num_keys) return false;
    if (pubkeys.size() != static_cast<unsigned long>(*num_keys)) return false;

    return (it + 1 == script.end());
}

std::optional<std::pair<int, std::vector<std::span<const unsigned char>>>> MatchMultiA(const CScript& script)
{
    std::vector<std::span<const unsigned char>> keyspans;

    // Redundant, but very fast and selective test.
    if (script.size() == 0 || script[0] != 32 || script.back() != OP_NUMEQUAL) return {};

    // Parse keys
    auto it = script.begin();
    while (script.end() - it >= 34) {
        if (*it != 32) return {};
        ++it;
        keyspans.emplace_back(&*it, 32);
        it += 32;
        if (*it != (keyspans.size() == 1 ? OP_CHECKSIG : OP_CHECKSIGADD)) return {};
        ++it;
    }
    if (keyspans.size() == 0 || keyspans.size() > MAX_PUBKEYS_PER_MULTI_A) return {};

    // Parse threshold.
    opcodetype opcode;
    std::vector<unsigned char> data;
    if (!script.GetOp(it, opcode, data)) return {};
    if (it == script.end()) return {};
    if (*it != OP_NUMEQUAL) return {};
    ++it;
    if (it != script.end()) return {};
    auto threshold = GetScriptNumber(opcode, data, 1, (int)keyspans.size());
    if (!threshold) return {};

    // Construct result.
    return std::pair{*threshold, std::move(keyspans)};
}

TxoutType Solver(const CScript& scriptPubKey, std::vector<std::vector<unsigned char>>* vSolutionsRet)
{
    if (vSolutionsRet) vSolutionsRet->clear();

    // Shortcut for pay-to-script-hash, which are more constrained than the other types:
    // it is always OP_HASH160 20 [20 byte hash] OP_EQUAL
    if (scriptPubKey.IsPayToScriptHash())
    {
        if (vSolutionsRet) vSolutionsRet->emplace_back(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
        return TxoutType::SCRIPTHASH;
    }

    int witnessversion;
    std::vector<unsigned char> witnessprogram;
    if (scriptPubKey.IsWitnessProgram(witnessversion, &witnessprogram)) {
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_KEYHASH_SIZE) {
            if (vSolutionsRet) vSolutionsRet->push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_V0_KEYHASH;
        }
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
            if (vSolutionsRet) vSolutionsRet->push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_V0_SCRIPTHASH;
        }
        if (witnessversion == 1 && witnessprogram.size() == WITNESS_V1_TAPROOT_SIZE) {
            if (vSolutionsRet) vSolutionsRet->push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_V1_TAPROOT;
        }
        if (scriptPubKey.IsPayToAnchor()) {
            return TxoutType::ANCHOR;
        }
        if (witnessversion != 0) {
            if (vSolutionsRet) {
                vSolutionsRet->emplace_back(1, static_cast<unsigned char>(witnessversion));
                vSolutionsRet->push_back(std::move(witnessprogram));
            }
            return TxoutType::WITNESS_UNKNOWN;
        }
        return TxoutType::NONSTANDARD;
    }

    // Provably prunable, data-carrying output
    //
    // So long as script passes the IsUnspendable() test and all but the first
    // byte passes the IsPushOnly() test we don't care what exactly is in the
    // script.
    if (scriptPubKey.size() >= 1 && scriptPubKey[0] == OP_RETURN && scriptPubKey.IsPushOnly(scriptPubKey.begin()+1)) {
        return TxoutType::NULL_DATA;
    }

    std::vector<unsigned char> data;
    if (MatchPayToPubkey(scriptPubKey, data)) {
        if (vSolutionsRet) vSolutionsRet->push_back(std::move(data));
        return TxoutType::PUBKEY;
    }

    if (MatchPayToPubkeyHash(scriptPubKey, data)) {
        if (vSolutionsRet) vSolutionsRet->push_back(std::move(data));
        return TxoutType::PUBKEYHASH;
    }

    int required;
    std::vector<std::vector<unsigned char>> keys;
    if (MatchMultisig(scriptPubKey, required, keys)) {
        if (vSolutionsRet) {
            vSolutionsRet->emplace_back(1, static_cast<unsigned char>(required)); // safe as required is in range 1..20
            vSolutionsRet->insert(vSolutionsRet->end(), keys.begin(), keys.end());
            vSolutionsRet->emplace_back(1, static_cast<unsigned char>(keys.size())); // safe as size is in range 1..20
        }
        return TxoutType::MULTISIG;
    }

    if (vSolutionsRet) vSolutionsRet->clear(); //TODO: remove: redundant
    return TxoutType::NONSTANDARD;
}

CScript GetScriptForRawPubKey(const CPubKey& pubKey)
{
    return CScript() << std::vector<unsigned char>(pubKey.begin(), pubKey.end()) << OP_CHECKSIG;
}

CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys)
{
    CScript script;

    script << nRequired;
    for (const CPubKey& key : keys)
        script << ToByteVector(key);
    script << keys.size() << OP_CHECKMULTISIG;

    return script;
}
