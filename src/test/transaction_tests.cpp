// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/data/tx_invalid.json.h>
#include <test/data/tx_valid.json.h>
#include <test/util/setup_common.h>

#include <chainparams.h>
#include <checkqueue.h>
#include <clientversion.h>
#include <consensus/amount.h>
#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <key.h>
#include <nevm/nevm.h>
#include <nevm/sha3.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/solver.h>
#include <services/assetconsensus.h>
#include <streams.h>
#include <test/util/json.h>
#include <test/util/random.h>
#include <test/util/script.h>
#include <test/util/transaction_utils.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <validation.h>

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <map>
#include <string>

#include <boost/test/unit_test.hpp>

#include <univalue.h>

typedef std::vector<unsigned char> valtype;

static CFeeRate g_dust{DUST_RELAY_TX_FEE};
static bool g_bare_multi{DEFAULT_PERMIT_BAREMULTISIG};

static std::map<std::string, unsigned int> mapFlagNames = {
    {std::string("P2SH"), (unsigned int)SCRIPT_VERIFY_P2SH},
    {std::string("STRICTENC"), (unsigned int)SCRIPT_VERIFY_STRICTENC},
    {std::string("DERSIG"), (unsigned int)SCRIPT_VERIFY_DERSIG},
    {std::string("LOW_S"), (unsigned int)SCRIPT_VERIFY_LOW_S},
    {std::string("SIGPUSHONLY"), (unsigned int)SCRIPT_VERIFY_SIGPUSHONLY},
    {std::string("MINIMALDATA"), (unsigned int)SCRIPT_VERIFY_MINIMALDATA},
    {std::string("NULLDUMMY"), (unsigned int)SCRIPT_VERIFY_NULLDUMMY},
    {std::string("DISCOURAGE_UPGRADABLE_NOPS"), (unsigned int)SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS},
    {std::string("CLEANSTACK"), (unsigned int)SCRIPT_VERIFY_CLEANSTACK},
    {std::string("MINIMALIF"), (unsigned int)SCRIPT_VERIFY_MINIMALIF},
    {std::string("NULLFAIL"), (unsigned int)SCRIPT_VERIFY_NULLFAIL},
    {std::string("CHECKLOCKTIMEVERIFY"), (unsigned int)SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY},
    {std::string("CHECKSEQUENCEVERIFY"), (unsigned int)SCRIPT_VERIFY_CHECKSEQUENCEVERIFY},
    {std::string("WITNESS"), (unsigned int)SCRIPT_VERIFY_WITNESS},
    {std::string("DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM"), (unsigned int)SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM},
    {std::string("WITNESS_PUBKEYTYPE"), (unsigned int)SCRIPT_VERIFY_WITNESS_PUBKEYTYPE},
    {std::string("CONST_SCRIPTCODE"), (unsigned int)SCRIPT_VERIFY_CONST_SCRIPTCODE},
    {std::string("TAPROOT"), (unsigned int)SCRIPT_VERIFY_TAPROOT},
    {std::string("DISCOURAGE_UPGRADABLE_PUBKEYTYPE"), (unsigned int)SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE},
    {std::string("DISCOURAGE_OP_SUCCESS"), (unsigned int)SCRIPT_VERIFY_DISCOURAGE_OP_SUCCESS},
    {std::string("DISCOURAGE_UPGRADABLE_TAPROOT_VERSION"), (unsigned int)SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION},
};

unsigned int ParseScriptFlags(std::string strFlags)
{
    if (strFlags.empty() || strFlags == "NONE") return 0;
    unsigned int flags = 0;
    std::vector<std::string> words = SplitString(strFlags, ',');

    for (const std::string& word : words)
    {
        if (!mapFlagNames.count(word))
            BOOST_ERROR("Bad test: unknown verification flag '" << word << "'");
        flags |= mapFlagNames[word];
    }

    return flags;
}

// Check that all flags in STANDARD_SCRIPT_VERIFY_FLAGS are present in mapFlagNames.
bool CheckMapFlagNames()
{
    unsigned int standard_flags_missing{STANDARD_SCRIPT_VERIFY_FLAGS};
    for (const auto& pair : mapFlagNames) {
        standard_flags_missing &= ~(pair.second);
    }
    return standard_flags_missing == 0;
}

std::string FormatScriptFlags(unsigned int flags)
{
    if (flags == 0) {
        return "";
    }
    std::string ret;
    std::map<std::string, unsigned int>::const_iterator it = mapFlagNames.begin();
    while (it != mapFlagNames.end()) {
        if (flags & it->second) {
            ret += it->first + ",";
        }
        it++;
    }
    return ret.substr(0, ret.size() - 1);
}

/*
* Check that the input scripts of a transaction are valid/invalid as expected.
*/
bool CheckTxScripts(const CTransaction& tx, const std::map<COutPoint, CScript>& map_prevout_scriptPubKeys,
    const std::map<COutPoint, int64_t>& map_prevout_values, unsigned int flags,
    const PrecomputedTransactionData& txdata, const std::string& strTest, bool expect_valid)
{
    bool tx_valid = true;
    ScriptError err = expect_valid ? SCRIPT_ERR_UNKNOWN_ERROR : SCRIPT_ERR_OK;
    for (unsigned int i = 0; i < tx.vin.size() && tx_valid; ++i) {
        const CTxIn input = tx.vin[i];
        const CAmount amount = map_prevout_values.count(input.prevout) ? map_prevout_values.at(input.prevout) : 0;
        try {
            tx_valid = VerifyScript(input.scriptSig, map_prevout_scriptPubKeys.at(input.prevout),
                &input.scriptWitness, flags, TransactionSignatureChecker(&tx, i, amount, txdata, MissingDataBehavior::ASSERT_FAIL), &err);
        } catch (...) {
            BOOST_ERROR("Bad test: " << strTest);
            return true; // The test format is bad and an error is thrown. Return true to silence further error.
        }
        if (expect_valid) {
            BOOST_CHECK_MESSAGE(tx_valid, strTest);
            BOOST_CHECK_MESSAGE((err == SCRIPT_ERR_OK), ScriptErrorString(err));
            err = SCRIPT_ERR_UNKNOWN_ERROR;
        }
    }
    if (!expect_valid) {
        BOOST_CHECK_MESSAGE(!tx_valid, strTest);
        BOOST_CHECK_MESSAGE((err != SCRIPT_ERR_OK), ScriptErrorString(err));
    }
    return (tx_valid == expect_valid);
}

/*
 * Trim or fill flags to make the combination valid:
 * WITNESS must be used with P2SH
 * CLEANSTACK must be used WITNESS and P2SH
 */

unsigned int TrimFlags(unsigned int flags)
{
    // WITNESS requires P2SH
    if (!(flags & SCRIPT_VERIFY_P2SH)) flags &= ~(unsigned int)SCRIPT_VERIFY_WITNESS;

    // CLEANSTACK requires WITNESS (and transitively CLEANSTACK requires P2SH)
    if (!(flags & SCRIPT_VERIFY_WITNESS)) flags &= ~(unsigned int)SCRIPT_VERIFY_CLEANSTACK;
    Assert(IsValidFlagCombination(flags));
    return flags;
}

unsigned int FillFlags(unsigned int flags)
{
    // CLEANSTACK implies WITNESS
    if (flags & SCRIPT_VERIFY_CLEANSTACK) flags |= SCRIPT_VERIFY_WITNESS;

    // WITNESS implies P2SH (and transitively CLEANSTACK implies P2SH)
    if (flags & SCRIPT_VERIFY_WITNESS) flags |= SCRIPT_VERIFY_P2SH;
    Assert(IsValidFlagCombination(flags));
    return flags;
}

// Exclude each possible script verify flag from flags. Returns a set of these flag combinations
// that are valid and without duplicates. For example: if flags=1111 and the 4 possible flags are
// 0001, 0010, 0100, and 1000, this should return the set {0111, 1011, 1101, 1110}.
// Assumes that mapFlagNames contains all script verify flags.
std::set<unsigned int> ExcludeIndividualFlags(unsigned int flags)
{
    std::set<unsigned int> flags_combos;
    for (const auto& pair : mapFlagNames) {
        const unsigned int flags_excluding_one = TrimFlags(flags & ~(pair.second));
        if (flags != flags_excluding_one) {
            flags_combos.insert(flags_excluding_one);
        }
    }
    return flags_combos;
}

BOOST_FIXTURE_TEST_SUITE(transaction_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(tx_valid)
{
    BOOST_CHECK_MESSAGE(CheckMapFlagNames(), "mapFlagNames is missing a script verification flag");
    // Read tests from test/data/tx_valid.json
    UniValue tests = read_json(json_tests::tx_valid);

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        const UniValue& test = tests[idx];
        std::string strTest = test.write();
        if (test[0].isArray())
        {
            if (test.size() != 3 || !test[1].isStr() || !test[2].isStr())
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            std::map<COutPoint, CScript> mapprevOutScriptPubKeys;
            std::map<COutPoint, int64_t> mapprevOutValues;
            UniValue inputs = test[0].get_array();
            bool fValid = true;
            for (unsigned int inpIdx = 0; inpIdx < inputs.size(); inpIdx++) {
                const UniValue& input = inputs[inpIdx];
                if (!input.isArray()) {
                    fValid = false;
                    break;
                }
                const UniValue& vinput = input.get_array();
                if (vinput.size() < 3 || vinput.size() > 4)
                {
                    fValid = false;
                    break;
                }
                COutPoint outpoint{uint256S(vinput[0].get_str()), uint32_t(vinput[1].getInt<int>())};
                mapprevOutScriptPubKeys[outpoint] = ParseScript(vinput[2].get_str());
                if (vinput.size() >= 4)
                {
                    mapprevOutValues[outpoint] = vinput[3].getInt<int64_t>();
                }
            }
            if (!fValid)
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            std::string transaction = test[1].get_str();
            CDataStream stream(ParseHex(transaction), SER_NETWORK, PROTOCOL_VERSION);
            CTransaction tx(deserialize, stream);

            TxValidationState state;
            BOOST_CHECK_MESSAGE(CheckTransaction(tx, state), strTest);
            BOOST_CHECK(state.IsValid());

            PrecomputedTransactionData txdata(tx);
            unsigned int verify_flags = ParseScriptFlags(test[2].get_str());

            // Check that the test gives a valid combination of flags (otherwise VerifyScript will throw). Don't edit the flags.
            if (~verify_flags != FillFlags(~verify_flags)) {
                BOOST_ERROR("Bad test flags: " << strTest);
            }

            BOOST_CHECK_MESSAGE(CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, ~verify_flags, txdata, strTest, /*expect_valid=*/true),
                                "Tx unexpectedly failed: " << strTest);

            // Backwards compatibility of script verification flags: Removing any flag(s) should not invalidate a valid transaction
            for (const auto& [name, flag] : mapFlagNames) {
                // Removing individual flags
                unsigned int flags = TrimFlags(~(verify_flags | flag));
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, flags, txdata, strTest, /*expect_valid=*/true)) {
                    BOOST_ERROR("Tx unexpectedly failed with flag " << name << " unset: " << strTest);
                }
                // Removing random combinations of flags
                flags = TrimFlags(~(verify_flags | (unsigned int)InsecureRandBits(mapFlagNames.size())));
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, flags, txdata, strTest, /*expect_valid=*/true)) {
                    BOOST_ERROR("Tx unexpectedly failed with random flags " << ToString(flags) << ": " << strTest);
                }
            }

            // Check that flags are maximal: transaction should fail if any unset flags are set.
            for (auto flags_excluding_one : ExcludeIndividualFlags(verify_flags)) {
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, ~flags_excluding_one, txdata, strTest, /*expect_valid=*/false)) {
                    BOOST_ERROR("Too many flags unset: " << strTest);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(tx_invalid)
{
    // Read tests from test/data/tx_invalid.json
    UniValue tests = read_json(json_tests::tx_invalid);

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        const UniValue& test = tests[idx];
        std::string strTest = test.write();
        if (test[0].isArray())
        {
            if (test.size() != 3 || !test[1].isStr() || !test[2].isStr())
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            std::map<COutPoint, CScript> mapprevOutScriptPubKeys;
            std::map<COutPoint, int64_t> mapprevOutValues;
            UniValue inputs = test[0].get_array();
            bool fValid = true;
            for (unsigned int inpIdx = 0; inpIdx < inputs.size(); inpIdx++) {
                const UniValue& input = inputs[inpIdx];
                if (!input.isArray()) {
                    fValid = false;
                    break;
                }
                const UniValue& vinput = input.get_array();
                if (vinput.size() < 3 || vinput.size() > 4)
                {
                    fValid = false;
                    break;
                }
                COutPoint outpoint{uint256S(vinput[0].get_str()), uint32_t(vinput[1].getInt<int>())};
                mapprevOutScriptPubKeys[outpoint] = ParseScript(vinput[2].get_str());
                if (vinput.size() >= 4)
                {
                    mapprevOutValues[outpoint] = vinput[3].getInt<int64_t>();
                }
            }
            if (!fValid)
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            std::string transaction = test[1].get_str();
            CDataStream stream(ParseHex(transaction), SER_NETWORK, PROTOCOL_VERSION );
            CTransaction tx(deserialize, stream);

            TxValidationState state;
            if (!CheckTransaction(tx, state) || state.IsInvalid()) {
                BOOST_CHECK_MESSAGE(test[2].get_str() == "BADTX", strTest);
                continue;
            }

            PrecomputedTransactionData txdata(tx);
            unsigned int verify_flags = ParseScriptFlags(test[2].get_str());

            // Check that the test gives a valid combination of flags (otherwise VerifyScript will throw). Don't edit the flags.
            if (verify_flags != FillFlags(verify_flags)) {
                BOOST_ERROR("Bad test flags: " << strTest);
            }

            // Not using FillFlags() in the main test, in order to detect invalid verifyFlags combination
            BOOST_CHECK_MESSAGE(CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, verify_flags, txdata, strTest, /*expect_valid=*/false),
                                "Tx unexpectedly passed: " << strTest);

            // Backwards compatibility of script verification flags: Adding any flag(s) should not validate an invalid transaction
            for (const auto& [name, flag] : mapFlagNames) {
                unsigned int flags = FillFlags(verify_flags | flag);
                // Adding individual flags
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, flags, txdata, strTest, /*expect_valid=*/false)) {
                    BOOST_ERROR("Tx unexpectedly passed with flag " << name << " set: " << strTest);
                }
                // Adding random combinations of flags
                flags = FillFlags(verify_flags | (unsigned int)InsecureRandBits(mapFlagNames.size()));
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, flags, txdata, strTest, /*expect_valid=*/false)) {
                    BOOST_ERROR("Tx unexpectedly passed with random flags " << name << ": " << strTest);
                }
            }

            // Check that flags are minimal: transaction should succeed if any set flags are unset.
            for (auto flags_excluding_one : ExcludeIndividualFlags(verify_flags)) {
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, mapprevOutValues, flags_excluding_one, txdata, strTest, /*expect_valid=*/true)) {
                    BOOST_ERROR("Too many flags set: " << strTest);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(basic_transaction_tests)
{
    // Random real transaction (e2769b09e784f32f62ef849763d4f45b98e07ba658647343b915ff832b110436)
    unsigned char ch[] = {0x01, 0x00, 0x00, 0x00, 0x01, 0x6b, 0xff, 0x7f, 0xcd, 0x4f, 0x85, 0x65, 0xef, 0x40, 0x6d, 0xd5, 0xd6, 0x3d, 0x4f, 0xf9, 0x4f, 0x31, 0x8f, 0xe8, 0x20, 0x27, 0xfd, 0x4d, 0xc4, 0x51, 0xb0, 0x44, 0x74, 0x01, 0x9f, 0x74, 0xb4, 0x00, 0x00, 0x00, 0x00, 0x8c, 0x49, 0x30, 0x46, 0x02, 0x21, 0x00, 0xda, 0x0d, 0xc6, 0xae, 0xce, 0xfe, 0x1e, 0x06, 0xef, 0xdf, 0x05, 0x77, 0x37, 0x57, 0xde, 0xb1, 0x68, 0x82, 0x09, 0x30, 0xe3, 0xb0, 0xd0, 0x3f, 0x46, 0xf5, 0xfc, 0xf1, 0x50, 0xbf, 0x99, 0x0c, 0x02, 0x21, 0x00, 0xd2, 0x5b, 0x5c, 0x87, 0x04, 0x00, 0x76, 0xe4, 0xf2, 0x53, 0xf8, 0x26, 0x2e, 0x76, 0x3e, 0x2d, 0xd5, 0x1e, 0x7f, 0xf0, 0xbe, 0x15, 0x77, 0x27, 0xc4, 0xbc, 0x42, 0x80, 0x7f, 0x17, 0xbd, 0x39, 0x01, 0x41, 0x04, 0xe6, 0xc2, 0x6e, 0xf6, 0x7d, 0xc6, 0x10, 0xd2, 0xcd, 0x19, 0x24, 0x84, 0x78, 0x9a, 0x6c, 0xf9, 0xae, 0xa9, 0x93, 0x0b, 0x94, 0x4b, 0x7e, 0x2d, 0xb5, 0x34, 0x2b, 0x9d, 0x9e, 0x5b, 0x9f, 0xf7, 0x9a, 0xff, 0x9a, 0x2e, 0xe1, 0x97, 0x8d, 0xd7, 0xfd, 0x01, 0xdf, 0xc5, 0x22, 0xee, 0x02, 0x28, 0x3d, 0x3b, 0x06, 0xa9, 0xd0, 0x3a, 0xcf, 0x80, 0x96, 0x96, 0x8d, 0x7d, 0xbb, 0x0f, 0x91, 0x78, 0xff, 0xff, 0xff, 0xff, 0x02, 0x8b, 0xa7, 0x94, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x19, 0x76, 0xa9, 0x14, 0xba, 0xde, 0xec, 0xfd, 0xef, 0x05, 0x07, 0x24, 0x7f, 0xc8, 0xf7, 0x42, 0x41, 0xd7, 0x3b, 0xc0, 0x39, 0x97, 0x2d, 0x7b, 0x88, 0xac, 0x40, 0x94, 0xa8, 0x02, 0x00, 0x00, 0x00, 0x00, 0x19, 0x76, 0xa9, 0x14, 0xc1, 0x09, 0x32, 0x48, 0x3f, 0xec, 0x93, 0xed, 0x51, 0xf5, 0xfe, 0x95, 0xe7, 0x25, 0x59, 0xf2, 0xcc, 0x70, 0x43, 0xf9, 0x88, 0xac, 0x00, 0x00, 0x00, 0x00, 0x00};
    std::vector<unsigned char> vch(ch, ch + sizeof(ch) -1);
    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);
    CMutableTransaction tx;
    stream >> tx;
    TxValidationState state;
    BOOST_CHECK_MESSAGE(CheckTransaction(CTransaction(tx), state) && state.IsValid(), "Simple deserialized transaction should be valid.");

    // Check that duplicate txins fail
    tx.vin.push_back(tx.vin[0]);
    BOOST_CHECK_MESSAGE(!CheckTransaction(CTransaction(tx), state) || !state.IsValid(), "Transaction with duplicate txins should be invalid.");
}

// Post-fork canonical asset commitment rules (CheckAssetAllocationCanonical).
BOOST_AUTO_TEST_CASE(syscoin_reject_noncanonical_asset_commitments)
{
    const uint32_t nForkHeight = (uint32_t)Params().GetConsensus().nCLReceiptStartBlock;

    // Two assets (guids differ) each assigning output index 1, on a burn-to-NEVM (version 141)
    // transaction. The single-asset rule fires before the duplicate-output rule.
    {
        const std::string tx_hex =
            "8d000000021c20c6f72930a91983304748f2d4ac2afdaa24caccecaade2ef164dd0b4eb2360000000000fdffffffe7723d25853300be42aa3c4cbed44f7184a9598a3d2e6a7edd6c570e50bd81f10000000000fdffffff02aca4ed902e0000001600144773f668af479ce0a8d8ba5b77e669a732e1029000000000000000002a6a280286c340010191cf96e30004010191cf96e3001471b216bd593c215cf3d1617718e364908755476900000000";

        CDataStream stream(ParseHex(tx_hex), SER_NETWORK, PROTOCOL_VERSION);
        const CTransaction tx(deserialize, stream);

        TxValidationState state;
        NEVMMintTxSet setMintTxs;
        CAssetsMap mapAssetIn;
        CAssetsMap mapAssetOut;
        BOOST_CHECK(!CheckSyscoinInputs(Params().GetConsensus(), tx, tx.GetHash(), state,
            nForkHeight, true, setMintTxs, mapAssetIn, mapAssetOut));
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "assetallocation-single-asset");
    }

    // A single asset whose commitment assigns output index 1 twice: passes the single-asset
    // rule, rejected by the duplicate-output rule. Also proves the rule is fork-gated: the
    // same transaction is accepted one block before the fork height (historically such
    // commitments deserialized with last-writer-wins semantics).
    {
        const std::string tx_hex =
            "8d000000021c20c6f72930a91983304748f2d4ac2afdaa24caccecaade2ef164dd0b4eb2360000000000fdffffffe7723d25853300be42aa3c4cbed44f7184a9598a3d2e6a7edd6c570e50bd81f10000000000fdffffff02aca4ed902e0000001600144773f668af479ce0a8d8ba5b77e669a732e102900000000000000000286a260186c340020191cf96e3000191cf96e3001471b216bd593c215cf3d1617718e364908755476900000000";

        CDataStream stream(ParseHex(tx_hex), SER_NETWORK, PROTOCOL_VERSION);
        const CTransaction tx(deserialize, stream);

        {
            TxValidationState state;
            NEVMMintTxSet setMintTxs;
            CAssetsMap mapAssetIn;
            CAssetsMap mapAssetOut;
            BOOST_CHECK(!CheckSyscoinInputs(Params().GetConsensus(), tx, tx.GetHash(), state,
                nForkHeight, true, setMintTxs, mapAssetIn, mapAssetOut));
            BOOST_CHECK_EQUAL(state.GetRejectReason(), "assetallocation-duplicate-output");
        }
        {
            TxValidationState state;
            NEVMMintTxSet setMintTxs;
            CAssetsMap mapAssetIn;
            CAssetsMap mapAssetOut;
            BOOST_CHECK(!CheckSyscoinInputs(Params().GetConsensus(), tx, tx.GetHash(), state,
                nForkHeight + 1, true, setMintTxs, mapAssetIn, mapAssetOut));
            BOOST_CHECK_EQUAL(state.GetRejectReason(), "assetallocation-duplicate-output");
        }
        // Pre-fork the canonical rules must NOT apply (deserialization stays permissive and
        // no consensus rule rejects the duplicate assignment), or reindex of historical
        // blocks would fail.
        {
            TxValidationState state;
            NEVMMintTxSet setMintTxs;
            CAssetsMap mapAssetIn;
            CAssetsMap mapAssetOut;
            BOOST_CHECK(CheckSyscoinInputs(Params().GetConsensus(), tx, tx.GetHash(), state,
                nForkHeight - 1, true, setMintTxs, mapAssetIn, mapAssetOut));
            BOOST_CHECK(state.IsValid());
        }
    }

    // The same asset guid serialized as two CAssetOut entries with *different* output
    // indexes. This is the exact shape a distinct-asset-count check (mapAssetOut.size())
    // would have missed: the two entries collapse to one map key. Uses an allocation send
    // (version 142) because on burn-to-NEVM/mint the single-asset rule fires first;
    // post-fork the duplicate-asset rule must reject it for every asset tx version.
    {
        const std::string tx_hex =
            "8e000000021c20c6f72930a91983304748f2d4ac2afdaa24caccecaade2ef164dd0b4eb2360000000000fdffffffe7723d25853300be42aa3c4cbed44f7184a9598a3d2e6a7edd6c570e50bd81f10000000000fdffffff02aca4ed902e0000001600144773f668af479ce0a8d8ba5b77e669a732e10290000000000000000017" "6a150286c340010091cf96e30086c340010191cf96e300" "00000000";

        CDataStream stream(ParseHex(tx_hex), SER_NETWORK, PROTOCOL_VERSION);
        const CTransaction tx(deserialize, stream);

        TxValidationState state;
        NEVMMintTxSet setMintTxs;
        CAssetsMap mapAssetIn;
        CAssetsMap mapAssetOut;
        BOOST_CHECK(!CheckSyscoinInputs(Params().GetConsensus(), tx, tx.GetHash(), state,
            nForkHeight, true, setMintTxs, mapAssetIn, mapAssetOut));
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "assetallocation-duplicate-asset");
    }
}

BOOST_AUTO_TEST_CASE(syscoin_data_short_witness_markers)
{
    const std::array<unsigned char, 4> marker{0xaa, 0x21, 0xa9, 0xed};
    for (size_t size = 0; size <= 40; ++size) {
        std::vector<unsigned char> pushed(size, 0);
        std::copy_n(marker.begin(), std::min(size, marker.size()), pushed.begin());
        const CScript script = CScript() << OP_RETURN << pushed;
        std::vector<unsigned char> parsed;
        const bool expected = size < 36;
        BOOST_CHECK_EQUAL(GetSyscoinData(script, parsed), expected);
        if (expected) {
            BOOST_CHECK(parsed == pushed);
        }
    }

    std::vector<unsigned char> commitment(36, 0);
    std::copy(marker.begin(), marker.end(), commitment.begin());
    const std::vector<unsigned char> syscoin_data{1, 2, 3};
    const CScript script = CScript() << OP_RETURN << commitment << syscoin_data;
    std::vector<unsigned char> parsed;
    BOOST_CHECK(GetSyscoinData(script, parsed));
    BOOST_CHECK(parsed == syscoin_data);
}

BOOST_AUTO_TEST_CASE(syscoin_mint_parent_node_offsets)
{
    auto check = [](const std::vector<unsigned char>& tx_nodes, uint16_t pos_tx,
                    const std::vector<unsigned char>& receipt_nodes, uint16_t pos_receipt,
                    bool expected) {
        CMintSyscoin encoded;
        encoded.voutAssets.emplace_back(1, std::vector<CAssetOutValue>{{0, 1}});
        encoded.vchTxParentNodes = tx_nodes;
        encoded.posTx = pos_tx;
        encoded.vchReceiptParentNodes = receipt_nodes;
        encoded.posReceipt = pos_receipt;
        std::vector<unsigned char> data;
        encoded.SerializeData(data);

        CMintSyscoin decoded;
        BOOST_CHECK_EQUAL(decoded.UnserializeFromData(data) == 0, expected);
    };

    check({}, 0, {}, 0, false);
    check({0x80}, 0, {0x80}, 0, true);
    check({0x80}, 1, {0x80}, 0, false);
    check({0x80}, 0, {0x80}, 1, false);
    check({0x80}, 2, {0x80}, 0, false);
    check({0x80}, 0, {0x80}, 2, false);
    check({}, std::numeric_limits<uint16_t>::max(), {}, 0, false);
    check({}, 0, {}, std::numeric_limits<uint16_t>::max(), false);
}

BOOST_AUTO_TEST_CASE(syscoin_mint_compares_roots_before_proof_parsing)
{
    auto previous_roots_db = std::move(pnevmtxrootsdb);
    auto previous_mint_db = std::move(pnevmtxmintdb);
    pnevmtxrootsdb = std::make_unique<CNEVMTxRootsDB>(DBParams{
        .path = "mint_root_order",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true,
        .wipe_data = true});
    pnevmtxmintdb = std::make_unique<CNEVMMintedTxDB>(DBParams{
        .path = "mint_root_order_txs",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true,
        .wipe_data = true});

    const uint256 block_hash = uint256S("01");
    NEVMTxRoot stored_roots;
    stored_roots.nTxRoot = uint256S("02");
    stored_roots.nReceiptRoot = uint256S("03");
    pnevmtxrootsdb->FlushDataToCache({{block_hash, stored_roots}});

    CMintSyscoin mint;
    mint.nBlockHash = block_hash;
    mint.nTxRoot = uint256S("04");
    mint.nReceiptRoot = stored_roots.nReceiptRoot;
    // Empty parent-node buffers are deliberately invalid RLP. Root mismatch must win.
    TxValidationState state;
    NEVMMintTxSet mint_txs;
    uint64_t asset_guid{0};
    CAmount amount{0};
    std::string address;
    BOOST_CHECK(!CheckSyscoinMintInternal(
        mint, state, true, false, mint_txs, asset_guid, amount, address));
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "mint-mismatching-txroot");

    mint.nTxRoot = stored_roots.nTxRoot;
    mint.vchTxParentNodes = {0xc1, 0xc0};
    mint.posTx = 1;
    mint.vchReceiptParentNodes = {0xc1, 0xc0};
    mint.posReceipt = 1;
    const dev::bytes invalid_tx_value{0xc0};
    const dev::h256 invalid_tx_hash = dev::sha3(
        dev::bytesConstRef(invalid_tx_value.data(), invalid_tx_value.size()));
    std::vector<unsigned char> invalid_tx_hash_bytes = invalid_tx_hash.asBytes();
    std::reverse(invalid_tx_hash_bytes.begin(), invalid_tx_hash_bytes.end());
    mint.nTxHash = uint256S(HexStr(invalid_tx_hash_bytes));
    TxValidationState proof_state;
    BOOST_CHECK(!CheckSyscoinMintInternal(
        mint, proof_state, true, false, mint_txs, asset_guid, amount, address));
    BOOST_CHECK_EQUAL(proof_state.GetRejectReason(), "mint-verify-receipt-proof");

    pnevmtxmintdb->FlushDataToCache({mint.nTxHash});
    TxValidationState replay_state;
    BOOST_CHECK(!CheckSyscoinMintInternal(
        mint, replay_state, true, false, mint_txs, asset_guid, amount, address));
    BOOST_CHECK_EQUAL(replay_state.GetRejectReason(), "mint-exists");

    mint.nTxHash = uint256S("01");
    TxValidationState forged_hash_state;
    BOOST_CHECK(!CheckSyscoinMintInternal(
        mint, forged_hash_state, true, false, mint_txs, asset_guid, amount, address));
    BOOST_CHECK_EQUAL(forged_hash_state.GetRejectReason(), "mint-verify-tx-hash");

    pnevmtxrootsdb = std::move(previous_roots_db);
    pnevmtxmintdb = std::move(previous_mint_db);
}

BOOST_AUTO_TEST_CASE(syscoin_mint_canonical_receipt_activation)
{
    auto previous_roots_db = std::move(pnevmtxrootsdb);
    auto previous_mint_db = std::move(pnevmtxmintdb);
    pnevmtxrootsdb = std::make_unique<CNEVMTxRootsDB>(DBParams{
        .path = "mint_canonical_roots",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true,
        .wipe_data = true});
    pnevmtxmintdb = std::make_unique<CNEVMMintedTxDB>(DBParams{
        .path = "mint_canonical_txs",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true,
        .wipe_data = true});

    const dev::bytes manager = Params().GetConsensus().vchSyscoinVaultManager;
    const dev::bytes freeze_topic = Params().GetConsensus().vchTokenFreezeMethod;
    dev::bytes guid_topic(32, 0);
    guid_topic[0] = 1; // Legacy ignores these high bits.
    guid_topic[31] = 1;
    dev::RLPStream topics(4);
    topics.append(freeze_topic);
    topics.append(guid_topic);
    topics.append(dev::bytes{1}); // Legacy does not inspect the freezer topic shape.
    topics.append(dev::bytes{});  // Legacy accepts additional topics.

    const std::string witness{"abc"};
    dev::bytes event_data(128, 0);
    event_data[31] = 1; // amount
    event_data[32] = 1; // Legacy ignores the offset's high bits.
    event_data[63] = 64;
    event_data[64] = 1; // Legacy ignores the length's high bits.
    event_data[95] = witness.size();
    std::copy(witness.begin(), witness.end(), event_data.begin() + 96);

    dev::RLPStream log(4);
    log.append(manager);
    log.appendRaw(topics.out());
    log.append(event_data);
    log.append(dev::bytes{}); // Legacy accepts additional log fields.
    dev::RLPStream logs(1);
    logs.appendRaw(log.out());
    dev::RLPStream receipt(4);
    receipt.append(1U);
    receipt.append(0U);
    receipt.append(dev::bytes(256, 0));
    receipt.appendRaw(logs.out());
    const dev::bytes receipt_value = receipt.out();

    const uint64_t chain_id = Params().GetConsensus().nNEVMChainID;
    dev::RLPStream eth_tx(9);
    eth_tx.append(0U);
    eth_tx.append(0U);
    eth_tx.append(0U);
    eth_tx.append(manager);
    eth_tx.append(0U);
    eth_tx.append(dev::bytes{});
    eth_tx.append(static_cast<unsigned>(chain_id * 2 + 35));
    eth_tx.append(0U);
    eth_tx.append(0U);
    const dev::bytes tx_value = eth_tx.out();

    auto make_proof = [](const dev::bytes& value, uint16_t& value_pos, uint256& root) {
        dev::RLPStream leaf(2);
        leaf.append(dev::bytes{0x20}); // Leaf with an empty remaining nibble path.
        leaf.append(value);
        const dev::bytes leaf_data = leaf.out();
        dev::RLPStream parents(1);
        parents.appendRaw(leaf_data);
        const dev::bytes parent_data = parents.out();
        const auto value_it = std::search(parent_data.begin(), parent_data.end(), value.begin(), value.end());
        BOOST_REQUIRE(value_it != parent_data.end());
        value_pos = static_cast<uint16_t>(std::distance(parent_data.begin(), value_it));
        const dev::bytes root_bytes = dev::sha3(
            dev::bytesConstRef(leaf_data.data(), leaf_data.size())).asBytes();
        std::copy(root_bytes.begin(), root_bytes.end(), root.begin());
        return std::vector<unsigned char>(parent_data.begin(), parent_data.end());
    };

    CMintSyscoin mint;
    mint.nBlockHash = uint256S("11");
    mint.vchReceiptParentNodes = make_proof(receipt_value, mint.posReceipt, mint.nReceiptRoot);
    mint.vchTxParentNodes = make_proof(tx_value, mint.posTx, mint.nTxRoot);
    const dev::h256 tx_hash = dev::sha3(
        dev::bytesConstRef(tx_value.data(), tx_value.size()));
    std::vector<unsigned char> tx_hash_bytes = tx_hash.asBytes();
    std::reverse(tx_hash_bytes.begin(), tx_hash_bytes.end());
    mint.nTxHash = uint256S(HexStr(tx_hash_bytes));
    pnevmtxrootsdb->FlushDataToCache({{mint.nBlockHash, {mint.nTxRoot, mint.nReceiptRoot}}});

    NEVMMintTxSet mint_txs;
    uint64_t asset_guid{0};
    CAmount amount{0};
    std::string address;
    TxValidationState legacy_state;
    BOOST_CHECK(CheckSyscoinMintInternal(
        mint, legacy_state, true, false, mint_txs, asset_guid, amount, address));
    BOOST_CHECK_EQUAL(asset_guid, 1U);
    BOOST_CHECK_EQUAL(amount, 1);
    BOOST_CHECK_EQUAL(address, witness);

    TxValidationState canonical_state;
    BOOST_CHECK(!CheckSyscoinMintInternal(
        mint, canonical_state, true, true, mint_txs, asset_guid, amount, address));
    BOOST_CHECK_EQUAL(canonical_state.GetRejectReason(), "mint-log-invalid-field-count");

    pnevmtxrootsdb = std::move(previous_roots_db);
    pnevmtxmintdb = std::move(previous_mint_db);
}

BOOST_AUTO_TEST_CASE(syscoin_mint_typed_transaction_schemas)
{
    auto previous_roots_db = std::move(pnevmtxrootsdb);
    auto previous_mint_db = std::move(pnevmtxmintdb);
    pnevmtxrootsdb = std::make_unique<CNEVMTxRootsDB>(DBParams{
        .path = "mint_typed_roots",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true,
        .wipe_data = true});
    pnevmtxmintdb = std::make_unique<CNEVMMintedTxDB>(DBParams{
        .path = "mint_typed_txs",
        .cache_bytes = static_cast<size_t>(1 << 20),
        .memory_only = true,
        .wipe_data = true});

    const dev::bytes manager = Params().GetConsensus().vchSyscoinVaultManager;
    const dev::bytes freeze_topic = Params().GetConsensus().vchTokenFreezeMethod;
    dev::bytes guid_topic(32, 0);
    guid_topic[31] = 1;
    dev::bytes freezer_topic(32, 0);
    freezer_topic[31] = 1;
    dev::RLPStream topics(3);
    topics.append(freeze_topic);
    topics.append(guid_topic);
    topics.append(freezer_topic);

    const std::string witness{"abc"};
    dev::bytes event_data(128, 0);
    event_data[31] = 1;
    event_data[63] = 64;
    event_data[95] = witness.size();
    std::copy(witness.begin(), witness.end(), event_data.begin() + 96);

    dev::RLPStream log(3);
    log.append(manager);
    log.appendRaw(topics.out());
    log.append(event_data);
    dev::RLPStream logs(1);
    logs.appendRaw(log.out());
    dev::RLPStream receipt(4);
    receipt.append(1U);
    receipt.append(0U);
    receipt.append(dev::bytes(256, 0));
    receipt.appendRaw(logs.out());
    const dev::bytes receipt_value = receipt.out();

    const uint64_t chain_id = Params().GetConsensus().nNEVMChainID;
    const dev::RLPStream empty_list(0);
    dev::RLPStream type1_tx(11);
    type1_tx.append(chain_id);
    type1_tx.append(0U);
    type1_tx.append(0U);
    type1_tx.append(0U);
    type1_tx.append(manager);
    type1_tx.append(0U);
    type1_tx.append(dev::bytes{});
    type1_tx.appendRaw(empty_list.out());
    type1_tx.append(0U);
    type1_tx.append(0U);
    type1_tx.append(0U);
    const dev::bytes type1_value = type1_tx.out();

    dev::RLPStream type2_tx(12);
    type2_tx.append(chain_id);
    type2_tx.append(0U);
    type2_tx.append(0U);
    type2_tx.append(0U);
    type2_tx.append(0U);
    type2_tx.append(manager);
    type2_tx.append(0U);
    type2_tx.append(dev::bytes{});
    type2_tx.appendRaw(empty_list.out());
    type2_tx.append(0U);
    type2_tx.append(0U);
    type2_tx.append(0U);
    const dev::bytes type2_value = type2_tx.out();

    auto make_proof = [](const dev::bytes& value,
                         const std::optional<uint8_t>& envelope_type,
                         uint16_t& value_pos,
                         uint256& root) {
        dev::bytes trie_value;
        if (envelope_type.has_value()) {
            trie_value.push_back(*envelope_type);
        }
        trie_value.insert(trie_value.end(), value.begin(), value.end());

        dev::RLPStream leaf(2);
        leaf.append(dev::bytes{0x20});
        leaf.append(trie_value);
        const dev::bytes leaf_data = leaf.out();
        dev::RLPStream parents(1);
        parents.appendRaw(leaf_data);
        const dev::bytes parent_data = parents.out();
        const auto value_it = std::search(
            parent_data.begin(), parent_data.end(), value.begin(), value.end());
        BOOST_REQUIRE(value_it != parent_data.end());
        value_pos = static_cast<uint16_t>(
            std::distance(parent_data.begin(), value_it));
        const dev::bytes root_bytes = dev::sha3(
            dev::bytesConstRef(leaf_data.data(), leaf_data.size())).asBytes();
        std::copy(root_bytes.begin(), root_bytes.end(), root.begin());
        return std::vector<unsigned char>(parent_data.begin(), parent_data.end());
    };

    auto check = [&](const dev::bytes& tx_value,
                     uint8_t tx_type,
                     uint8_t receipt_type,
                     bool expected,
                     const std::string& reason = "") {
        CMintSyscoin mint;
        mint.nBlockHash = uint256S("22");
        mint.vchReceiptParentNodes = make_proof(
            receipt_value, receipt_type, mint.posReceipt, mint.nReceiptRoot);
        mint.vchTxParentNodes = make_proof(
            tx_value, tx_type, mint.posTx, mint.nTxRoot);
        const dev::h256 tx_hash = dev::sha3(
            dev::bytesConstRef(tx_value.data(), tx_value.size()));
        std::vector<unsigned char> tx_hash_bytes = tx_hash.asBytes();
        std::reverse(tx_hash_bytes.begin(), tx_hash_bytes.end());
        mint.nTxHash = uint256S(HexStr(tx_hash_bytes));
        pnevmtxrootsdb->FlushDataToCache(
            {{mint.nBlockHash, {mint.nTxRoot, mint.nReceiptRoot}}});

        NEVMMintTxSet mint_txs;
        uint64_t asset_guid{0};
        CAmount amount{0};
        std::string address;
        TxValidationState state;
        BOOST_CHECK_EQUAL(CheckSyscoinMintInternal(
            mint, state, true, true, mint_txs, asset_guid, amount, address), expected);
        if (expected) {
            BOOST_CHECK_EQUAL(asset_guid, 1U);
            BOOST_CHECK_EQUAL(amount, 1);
            BOOST_CHECK_EQUAL(address, witness);
        } else {
            BOOST_CHECK_EQUAL(state.GetRejectReason(), reason);
        }
    };

    check(type1_value, 1, 1, true);
    check(type2_value, 2, 2, true);
    check(type1_value, 2, 2, false, "mint-unsupported-tx-format");
    check(type1_value, 1, 2, false, "mint-envelope-type-mismatch");
    check(type1_value, 3, 3, false, "mint-unsupported-tx-format");
    check(type1_value, 0x7f, 0x7f, false, "mint-unsupported-tx-format");

    pnevmtxrootsdb = std::move(previous_roots_db);
    pnevmtxmintdb = std::move(previous_mint_db);
}

BOOST_AUTO_TEST_CASE(syscoin_bridge_raw_allocation_canonicality)
{
    const uint32_t fork_height = (uint32_t)Params().GetConsensus().nCLReceiptStartBlock;
    auto make_tx = [](const std::vector<unsigned char>& data,
                      int32_t version = SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM) {
        CMutableTransaction mtx;
        mtx.nVersion = version;
        mtx.vout.emplace_back(0, CScript() << OP_TRUE);
        mtx.vout.emplace_back(0, CScript() << OP_RETURN << data);
        mtx.LoadAssets();
        return CTransaction(mtx);
    };
    auto check = [&](const CTransaction& tx, uint32_t height, const std::string& reason) {
        TxValidationState state;
        NEVMMintTxSet set_mint_txs;
        CAssetsMap assets_in;
        CAssetsMap assets_out;
        BOOST_CHECK(!CheckSyscoinInputs(Params().GetConsensus(), tx, tx.GetHash(), state,
                                       height, true, set_mint_txs, assets_in, assets_out));
        BOOST_CHECK_EQUAL(state.GetRejectReason(), reason);
    };
    auto check_valid = [&](const CTransaction& tx, uint32_t height) {
        TxValidationState state;
        NEVMMintTxSet set_mint_txs;
        CAssetsMap assets_in;
        CAssetsMap assets_out;
        BOOST_CHECK(CheckSyscoinInputs(Params().GetConsensus(), tx, tx.GetHash(), state,
                                      height, true, set_mint_txs, assets_in, assets_out));
        BOOST_CHECK(state.IsValid());
    };

    CAssetAllocation null_guid;
    null_guid.voutAssets.emplace_back(0, std::vector<CAssetOutValue>{{1, 1}});
    std::vector<unsigned char> null_guid_data;
    null_guid.SerializeData(null_guid_data);
    const CTransaction null_guid_tx = make_tx(null_guid_data);
    check(null_guid_tx, fork_height, "assetallocation-null-asset");
    check(null_guid_tx, fork_height + 1, "assetallocation-null-asset");

    CMutableTransaction malformed_script_mtx;
    malformed_script_mtx.nVersion = SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM;
    malformed_script_mtx.vout.emplace_back(0, CScript() << OP_TRUE);
    CScript malformed_script = CScript() << OP_RETURN << null_guid_data;
    malformed_script.push_back(OP_PUSHDATA1); // Truncated trailing push.
    malformed_script_mtx.vout.emplace_back(0, malformed_script);
    malformed_script_mtx.LoadAssets();
    const CTransaction malformed_script_tx(malformed_script_mtx);
    check_valid(malformed_script_tx, fork_height - 1);
    check(malformed_script_tx, fork_height, "assetallocation-noncanonical-script");

    CDataStream noncanonical(SER_NETWORK, PROTOCOL_VERSION);
    uint64_t asset_count{1};
    uint64_t asset_guid{1};
    uint64_t value_count{1};
    uint64_t output_index{1};
    uint64_t overflowing_compressed_amount{17073621928676916506ULL};
    noncanonical << COMPACTSIZE(asset_count) << VARINT(asset_guid)
                 << COMPACTSIZE(value_count) << COMPACTSIZE(output_index)
                 << VARINT(overflowing_compressed_amount);
    const auto noncanonical_span = MakeUCharSpan(noncanonical);
    std::vector<unsigned char> noncanonical_data(
        noncanonical_span.begin(), noncanonical_span.end());
    // Keep the destination valid so this vector isolates amount canonicality.
    noncanonical_data.push_back(20);
    noncanonical_data.insert(noncanonical_data.end(), 20, 1);
    const CTransaction noncanonical_tx = make_tx(noncanonical_data);
    check(noncanonical_tx, fork_height, "assetallocation-noncanonical-encoding");
    check(noncanonical_tx, fork_height + 1, "assetallocation-noncanonical-encoding");
    check_valid(noncanonical_tx, fork_height - 1);

    CAssetAllocation canonical_allocation;
    canonical_allocation.voutAssets.emplace_back(
        1, std::vector<CAssetOutValue>{{1, 1}});
    std::vector<unsigned char> allocation_data;
    canonical_allocation.SerializeData(allocation_data);

    std::vector<unsigned char> valid_burn_data = allocation_data;
    valid_burn_data.push_back(20);
    valid_burn_data.insert(valid_burn_data.end(), 20, 1);
    check_valid(make_tx(valid_burn_data), fork_height);

    check_valid(make_tx(allocation_data), fork_height - 1);
    check(make_tx(allocation_data), fork_height,
          "assetallocation-invalid-nevm-destination");

    std::vector<unsigned char> short_destination = allocation_data;
    short_destination.push_back(20);
    short_destination.insert(short_destination.end(), 19, 1);
    check(make_tx(short_destination), fork_height,
          "assetallocation-invalid-nevm-destination");

    std::vector<unsigned char> wrong_length = allocation_data;
    wrong_length.push_back(19);
    wrong_length.insert(wrong_length.end(), 19, 1);
    check(make_tx(wrong_length), fork_height,
          "assetallocation-invalid-nevm-destination");

    std::vector<unsigned char> trailing_data = valid_burn_data;
    trailing_data.push_back(0);
    check(make_tx(trailing_data), fork_height,
          "assetallocation-invalid-nevm-destination");

    std::vector<unsigned char> zero_destination = allocation_data;
    zero_destination.push_back(20);
    zero_destination.insert(zero_destination.end(), 20, 0);
    check(make_tx(zero_destination), fork_height,
          "assetallocation-null-nevm-destination");

    // The representation mismatch matters at the bridge boundary. Ordinary
    // SPT accounting uses the decoded, MoneyRange-checked value consistently.
    const CTransaction noncanonical_send =
        make_tx(noncanonical_data, SYSCOIN_TX_VERSION_ALLOCATION_SEND);
    check_valid(noncanonical_send, fork_height);
}

BOOST_AUTO_TEST_CASE(syscoin_bridge_uses_clreceipt_activation)
{
    const auto main = CreateChainParams(*m_node.args, ChainType::MAIN);
    const auto testnet = CreateChainParams(*m_node.args, ChainType::TESTNET);
    BOOST_CHECK_EQUAL(main->GetConsensus().nCLReceiptStartBlock, std::numeric_limits<int>::max());
    BOOST_CHECK_EQUAL(testnet->GetConsensus().nCLReceiptStartBlock, 1746000);

    ArgsManager args;
    args.ForceSetArg("-clreceiptstartheight", "123");
    const auto regtest = CreateChainParams(args, ChainType::REGTEST);
    BOOST_CHECK_EQUAL(regtest->GetConsensus().nCLReceiptStartBlock, 123);
}

BOOST_AUTO_TEST_CASE(asset_amount_aggregate_range_checks)
{
    constexpr uint64_t SYSX = 123456;
    constexpr CAmount M = MAX_MONEY;
    // R = 2^64 - 18*M; each component is individually MoneyRange-valid.
    constexpr CAmount R = 446744073709551634LL;
    BOOST_REQUIRE(MoneyRange(R));
    BOOST_REQUIRE(MoneyRange(M));
    // Pairwise sums of MoneyRange values fit in int64.
    BOOST_REQUIRE(M <= std::numeric_limits<CAmount>::max() - M);

    std::vector<CAmount> amounts;
    amounts.push_back(1);
    for (int i = 0; i < 18; ++i) {
        amounts.push_back(M);
    }
    amounts.push_back(R);
    BOOST_REQUIRE_EQUAL(amounts.size(), 20U);

    // Per-output amounts are each in range and use unique indices.
    std::unordered_set<uint32_t> setOutputs;
    for (size_t i = 0; i < amounts.size(); ++i) {
        BOOST_CHECK(MoneyRange(amounts[i]));
        BOOST_CHECK(setOutputs.insert(static_cast<uint32_t>(i)).second);
    }

    // Without an aggregate range check, uint64 modular sum of this vector is 1
    // (mathematical total is 2^64+1). Use unsigned addition for defined wrap.
    uint64_t unchecked_sum = 0;
    for (CAmount a : amounts) {
        unchecked_sum += static_cast<uint64_t>(a);
    }
    BOOST_CHECK_EQUAL(unchecked_sum, uint64_t{1});

    // Consensus GetAssetValueOut must reject the same vector.
    CMutableTransaction mtx;
    mtx.nVersion = SYSCOIN_TX_VERSION_ALLOCATION_SEND;
    mtx.vout.resize(amounts.size());
    for (size_t i = 0; i < amounts.size(); ++i) {
        mtx.vout[i].nValue = 0;
        mtx.vout[i].scriptPubKey = CScript() << OP_TRUE;
        mtx.vout[i].assetInfo = CAssetCoinInfo(SYSX, amounts[i]);
    }
    CAssetsMap mapAssetOut;
    std::string err;
    BOOST_CHECK(!CTransaction(mtx).GetAssetValueOut(mapAssetOut, err));
    BOOST_CHECK_EQUAL(err, "bad-txns-asset-out-outofrange");

    // Two MAX_MONEY outputs for one asset are also rejected before the total
    // is updated past MAX_MONEY.
    BOOST_CHECK(!MoneyRange(M + M));
    CMutableTransaction two_max;
    two_max.nVersion = SYSCOIN_TX_VERSION_ALLOCATION_SEND;
    two_max.vout.resize(2);
    two_max.vout[0].assetInfo = CAssetCoinInfo(SYSX, M);
    two_max.vout[1].assetInfo = CAssetCoinInfo(SYSX, M);
    CAssetsMap two_max_map;
    std::string two_max_err;
    BOOST_CHECK(!CTransaction(two_max).GetAssetValueOut(two_max_map, two_max_err));
    BOOST_CHECK_EQUAL(two_max_err, "bad-txns-asset-out-outofrange");
    BOOST_CHECK(two_max_map.find(SYSX) == two_max_map.end() ||
                two_max_map.at(SYSX) == M);

    // In-range aggregate still succeeds.
    CMutableTransaction ok_mtx;
    ok_mtx.nVersion = SYSCOIN_TX_VERSION_ALLOCATION_SEND;
    ok_mtx.vout.resize(2);
    ok_mtx.vout[0].assetInfo = CAssetCoinInfo(SYSX, 1);
    ok_mtx.vout[1].assetInfo = CAssetCoinInfo(SYSX, 2);
    CAssetsMap ok_map;
    std::string ok_err;
    BOOST_CHECK(CTransaction(ok_mtx).GetAssetValueOut(ok_map, ok_err));
    BOOST_CHECK_EQUAL(ok_map.at(SYSX), 3);
}

BOOST_AUTO_TEST_CASE(test_Get)
{
    FillableSigningProvider keystore;
    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    std::vector<CMutableTransaction> dummyTransactions =
        SetupDummyInputs(keystore, coins, {11*CENT, 50*CENT, 21*CENT, 22*CENT});

    CMutableTransaction t1;
    t1.vin.resize(3);
    t1.vin[0].prevout.hash = dummyTransactions[0].GetHash();
    t1.vin[0].prevout.n = 1;
    t1.vin[0].scriptSig << std::vector<unsigned char>(65, 0);
    t1.vin[1].prevout.hash = dummyTransactions[1].GetHash();
    t1.vin[1].prevout.n = 0;
    t1.vin[1].scriptSig << std::vector<unsigned char>(65, 0) << std::vector<unsigned char>(33, 4);
    t1.vin[2].prevout.hash = dummyTransactions[1].GetHash();
    t1.vin[2].prevout.n = 1;
    t1.vin[2].scriptSig << std::vector<unsigned char>(65, 0) << std::vector<unsigned char>(33, 4);
    t1.vout.resize(2);
    t1.vout[0].nValue = 90*CENT;
    t1.vout[0].scriptPubKey << OP_1;

    BOOST_CHECK(AreInputsStandard(CTransaction(t1), coins));
}

static void CreateCreditAndSpend(const FillableSigningProvider& keystore, const CScript& outscript, CTransactionRef& output, CMutableTransaction& input, bool success = true)
{
    CMutableTransaction outputm;
    outputm.nVersion = 1;
    outputm.vin.resize(1);
    outputm.vin[0].prevout.SetNull();
    outputm.vin[0].scriptSig = CScript();
    outputm.vout.resize(1);
    outputm.vout[0].nValue = 1;
    outputm.vout[0].scriptPubKey = outscript;
    CDataStream ssout(SER_NETWORK, PROTOCOL_VERSION);
    ssout << outputm;
    ssout >> output;
    assert(output->vin.size() == 1);
    assert(output->vin[0] == outputm.vin[0]);
    assert(output->vout.size() == 1);
    assert(output->vout[0] == outputm.vout[0]);

    CMutableTransaction inputm;
    inputm.nVersion = 1;
    inputm.vin.resize(1);
    inputm.vin[0].prevout.hash = output->GetHash();
    inputm.vin[0].prevout.n = 0;
    inputm.vout.resize(1);
    inputm.vout[0].nValue = 1;
    inputm.vout[0].scriptPubKey = CScript();
    SignatureData empty;
    bool ret = SignSignature(keystore, *output, inputm, 0, SIGHASH_ALL, empty);
    assert(ret == success);
    CDataStream ssin(SER_NETWORK, PROTOCOL_VERSION);
    ssin << inputm;
    ssin >> input;
    assert(input.vin.size() == 1);
    assert(input.vin[0] == inputm.vin[0]);
    assert(input.vout.size() == 1);
    assert(input.vout[0] == inputm.vout[0]);
    assert(input.vin[0].scriptWitness.stack == inputm.vin[0].scriptWitness.stack);
}

static void CheckWithFlag(const CTransactionRef& output, const CMutableTransaction& input, uint32_t flags, bool success)
{
    ScriptError error;
    CTransaction inputi(input);
    bool ret = VerifyScript(inputi.vin[0].scriptSig, output->vout[0].scriptPubKey, &inputi.vin[0].scriptWitness, flags, TransactionSignatureChecker(&inputi, 0, output->vout[0].nValue, MissingDataBehavior::ASSERT_FAIL), &error);
    assert(ret == success);
}

static CScript PushAll(const std::vector<valtype>& values)
{
    CScript result;
    for (const valtype& v : values) {
        if (v.size() == 0) {
            result << OP_0;
        } else if (v.size() == 1 && v[0] >= 1 && v[0] <= 16) {
            result << CScript::EncodeOP_N(v[0]);
        } else if (v.size() == 1 && v[0] == 0x81) {
            result << OP_1NEGATE;
        } else {
            result << v;
        }
    }
    return result;
}

static void ReplaceRedeemScript(CScript& script, const CScript& redeemScript)
{
    std::vector<valtype> stack;
    EvalScript(stack, script, SCRIPT_VERIFY_STRICTENC, BaseSignatureChecker(), SigVersion::BASE);
    assert(stack.size() > 0);
    stack.back() = std::vector<unsigned char>(redeemScript.begin(), redeemScript.end());
    script = PushAll(stack);
}

BOOST_AUTO_TEST_CASE(test_big_witness_transaction)
{
    CMutableTransaction mtx;
    mtx.nVersion = 1;

    CKey key;
    key.MakeNewKey(true); // Need to use compressed keys in segwit or the signing will fail
    FillableSigningProvider keystore;
    BOOST_CHECK(keystore.AddKeyPubKey(key, key.GetPubKey()));
    CKeyID hash = key.GetPubKey().GetID();
    CScript scriptPubKey = CScript() << OP_0 << std::vector<unsigned char>(hash.begin(), hash.end());

    std::vector<int> sigHashes;
    sigHashes.push_back(SIGHASH_NONE | SIGHASH_ANYONECANPAY);
    sigHashes.push_back(SIGHASH_SINGLE | SIGHASH_ANYONECANPAY);
    sigHashes.push_back(SIGHASH_ALL | SIGHASH_ANYONECANPAY);
    sigHashes.push_back(SIGHASH_NONE);
    sigHashes.push_back(SIGHASH_SINGLE);
    sigHashes.push_back(SIGHASH_ALL);

    // create a big transaction of 4500 inputs signed by the same key
    for(uint32_t ij = 0; ij < 4500; ij++) {
        uint32_t i = mtx.vin.size();
        uint256 prevId;
        prevId.SetHex("0000000000000000000000000000000000000000000000000000000000000100");
        COutPoint outpoint(prevId, i);

        mtx.vin.resize(mtx.vin.size() + 1);
        mtx.vin[i].prevout = outpoint;
        mtx.vin[i].scriptSig = CScript();

        mtx.vout.resize(mtx.vout.size() + 1);
        mtx.vout[i].nValue = 1000;
        mtx.vout[i].scriptPubKey = CScript() << OP_1;
    }

    // sign all inputs
    for(uint32_t i = 0; i < mtx.vin.size(); i++) {
        SignatureData empty;
        bool hashSigned = SignSignature(keystore, scriptPubKey, mtx, i, 1000, sigHashes.at(i % sigHashes.size()), empty);
        assert(hashSigned);
    }

    CDataStream ssout(SER_NETWORK, PROTOCOL_VERSION);
    ssout << mtx;
    CTransaction tx(deserialize, ssout);

    // check all inputs concurrently, with the cache
    PrecomputedTransactionData txdata(tx);
    CCheckQueue<CScriptCheck> scriptcheckqueue(128);
    CCheckQueueControl<CScriptCheck> control(&scriptcheckqueue);

    scriptcheckqueue.StartWorkerThreads(20);

    std::vector<Coin> coins;
    for(uint32_t i = 0; i < mtx.vin.size(); i++) {
        Coin coin;
        coin.nHeight = 1;
        coin.fCoinBase = false;
        coin.out.nValue = 1000;
        coin.out.scriptPubKey = scriptPubKey;
        coins.emplace_back(std::move(coin));
    }

    for(uint32_t i = 0; i < mtx.vin.size(); i++) {
        std::vector<CScriptCheck> vChecks;
        vChecks.emplace_back(coins[tx.vin[i].prevout.n].out, tx, i, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, false, &txdata);
        control.Add(std::move(vChecks));
    }

    bool controlCheck = control.Wait();
    assert(controlCheck);
    scriptcheckqueue.StopWorkerThreads();
}

SignatureData CombineSignatures(const CMutableTransaction& input1, const CMutableTransaction& input2, const CTransactionRef tx)
{
    SignatureData sigdata;
    sigdata = DataFromTransaction(input1, 0, tx->vout[0]);
    sigdata.MergeSignatureData(DataFromTransaction(input2, 0, tx->vout[0]));
    ProduceSignature(DUMMY_SIGNING_PROVIDER, MutableTransactionSignatureCreator(input1, 0, tx->vout[0].nValue, SIGHASH_ALL), tx->vout[0].scriptPubKey, sigdata);
    return sigdata;
}

BOOST_AUTO_TEST_CASE(test_witness)
{
    FillableSigningProvider keystore, keystore2;
    CKey key1, key2, key3, key1L, key2L;
    CPubKey pubkey1, pubkey2, pubkey3, pubkey1L, pubkey2L;
    key1.MakeNewKey(true);
    key2.MakeNewKey(true);
    key3.MakeNewKey(true);
    key1L.MakeNewKey(false);
    key2L.MakeNewKey(false);
    pubkey1 = key1.GetPubKey();
    pubkey2 = key2.GetPubKey();
    pubkey3 = key3.GetPubKey();
    pubkey1L = key1L.GetPubKey();
    pubkey2L = key2L.GetPubKey();
    BOOST_CHECK(keystore.AddKeyPubKey(key1, pubkey1));
    BOOST_CHECK(keystore.AddKeyPubKey(key2, pubkey2));
    BOOST_CHECK(keystore.AddKeyPubKey(key1L, pubkey1L));
    BOOST_CHECK(keystore.AddKeyPubKey(key2L, pubkey2L));
    CScript scriptPubkey1, scriptPubkey2, scriptPubkey1L, scriptPubkey2L, scriptMulti;
    scriptPubkey1 << ToByteVector(pubkey1) << OP_CHECKSIG;
    scriptPubkey2 << ToByteVector(pubkey2) << OP_CHECKSIG;
    scriptPubkey1L << ToByteVector(pubkey1L) << OP_CHECKSIG;
    scriptPubkey2L << ToByteVector(pubkey2L) << OP_CHECKSIG;
    std::vector<CPubKey> oneandthree;
    oneandthree.push_back(pubkey1);
    oneandthree.push_back(pubkey3);
    scriptMulti = GetScriptForMultisig(2, oneandthree);
    BOOST_CHECK(keystore.AddCScript(scriptPubkey1));
    BOOST_CHECK(keystore.AddCScript(scriptPubkey2));
    BOOST_CHECK(keystore.AddCScript(scriptPubkey1L));
    BOOST_CHECK(keystore.AddCScript(scriptPubkey2L));
    BOOST_CHECK(keystore.AddCScript(scriptMulti));
    CScript destination_script_1, destination_script_2, destination_script_1L, destination_script_2L, destination_script_multi;
    destination_script_1 = GetScriptForDestination(WitnessV0KeyHash(pubkey1));
    destination_script_2 = GetScriptForDestination(WitnessV0KeyHash(pubkey2));
    destination_script_1L = GetScriptForDestination(WitnessV0KeyHash(pubkey1L));
    destination_script_2L = GetScriptForDestination(WitnessV0KeyHash(pubkey2L));
    destination_script_multi = GetScriptForDestination(WitnessV0ScriptHash(scriptMulti));
    BOOST_CHECK(keystore.AddCScript(destination_script_1));
    BOOST_CHECK(keystore.AddCScript(destination_script_2));
    BOOST_CHECK(keystore.AddCScript(destination_script_1L));
    BOOST_CHECK(keystore.AddCScript(destination_script_2L));
    BOOST_CHECK(keystore.AddCScript(destination_script_multi));
    BOOST_CHECK(keystore2.AddCScript(scriptMulti));
    BOOST_CHECK(keystore2.AddCScript(destination_script_multi));
    BOOST_CHECK(keystore2.AddKeyPubKey(key3, pubkey3));

    CTransactionRef output1, output2;
    CMutableTransaction input1, input2;

    // Normal pay-to-compressed-pubkey.
    CreateCreditAndSpend(keystore, scriptPubkey1, output1, input1);
    CreateCreditAndSpend(keystore, scriptPubkey2, output2, input2);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);
    CheckWithFlag(output1, input2, 0, false);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, STANDARD_SCRIPT_VERIFY_FLAGS, false);

    // P2SH pay-to-compressed-pubkey.
    CreateCreditAndSpend(keystore, GetScriptForDestination(ScriptHash(scriptPubkey1)), output1, input1);
    CreateCreditAndSpend(keystore, GetScriptForDestination(ScriptHash(scriptPubkey2)), output2, input2);
    ReplaceRedeemScript(input2.vin[0].scriptSig, scriptPubkey1);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);
    CheckWithFlag(output1, input2, 0, true);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, STANDARD_SCRIPT_VERIFY_FLAGS, false);

    // Witness pay-to-compressed-pubkey (v0).
    CreateCreditAndSpend(keystore, destination_script_1, output1, input1);
    CreateCreditAndSpend(keystore, destination_script_2, output2, input2);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);
    CheckWithFlag(output1, input2, 0, true);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, STANDARD_SCRIPT_VERIFY_FLAGS, false);

    // P2SH witness pay-to-compressed-pubkey (v0).
    CreateCreditAndSpend(keystore, GetScriptForDestination(ScriptHash(destination_script_1)), output1, input1);
    CreateCreditAndSpend(keystore, GetScriptForDestination(ScriptHash(destination_script_2)), output2, input2);
    ReplaceRedeemScript(input2.vin[0].scriptSig, destination_script_1);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);
    CheckWithFlag(output1, input2, 0, true);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, STANDARD_SCRIPT_VERIFY_FLAGS, false);

    // Normal pay-to-uncompressed-pubkey.
    CreateCreditAndSpend(keystore, scriptPubkey1L, output1, input1);
    CreateCreditAndSpend(keystore, scriptPubkey2L, output2, input2);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);
    CheckWithFlag(output1, input2, 0, false);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, STANDARD_SCRIPT_VERIFY_FLAGS, false);

    // P2SH pay-to-uncompressed-pubkey.
    CreateCreditAndSpend(keystore, GetScriptForDestination(ScriptHash(scriptPubkey1L)), output1, input1);
    CreateCreditAndSpend(keystore, GetScriptForDestination(ScriptHash(scriptPubkey2L)), output2, input2);
    ReplaceRedeemScript(input2.vin[0].scriptSig, scriptPubkey1L);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);
    CheckWithFlag(output1, input2, 0, true);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false);
    CheckWithFlag(output1, input2, STANDARD_SCRIPT_VERIFY_FLAGS, false);

    // Signing disabled for witness pay-to-uncompressed-pubkey (v1).
    CreateCreditAndSpend(keystore, destination_script_1L, output1, input1, false);
    CreateCreditAndSpend(keystore, destination_script_2L, output2, input2, false);

    // Signing disabled for P2SH witness pay-to-uncompressed-pubkey (v1).
    CreateCreditAndSpend(keystore, GetScriptForDestination(ScriptHash(destination_script_1L)), output1, input1, false);
    CreateCreditAndSpend(keystore, GetScriptForDestination(ScriptHash(destination_script_2L)), output2, input2, false);

    // Normal 2-of-2 multisig
    CreateCreditAndSpend(keystore, scriptMulti, output1, input1, false);
    CheckWithFlag(output1, input1, 0, false);
    CreateCreditAndSpend(keystore2, scriptMulti, output2, input2, false);
    CheckWithFlag(output2, input2, 0, false);
    BOOST_CHECK(*output1 == *output2);
    UpdateInput(input1.vin[0], CombineSignatures(input1, input2, output1));
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);

    // P2SH 2-of-2 multisig
    CreateCreditAndSpend(keystore, GetScriptForDestination(ScriptHash(scriptMulti)), output1, input1, false);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, false);
    CreateCreditAndSpend(keystore2, GetScriptForDestination(ScriptHash(scriptMulti)), output2, input2, false);
    CheckWithFlag(output2, input2, 0, true);
    CheckWithFlag(output2, input2, SCRIPT_VERIFY_P2SH, false);
    BOOST_CHECK(*output1 == *output2);
    UpdateInput(input1.vin[0], CombineSignatures(input1, input2, output1));
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);

    // Witness 2-of-2 multisig
    CreateCreditAndSpend(keystore, destination_script_multi, output1, input1, false);
    CheckWithFlag(output1, input1, 0, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, false);
    CreateCreditAndSpend(keystore2, destination_script_multi, output2, input2, false);
    CheckWithFlag(output2, input2, 0, true);
    CheckWithFlag(output2, input2, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, false);
    BOOST_CHECK(*output1 == *output2);
    UpdateInput(input1.vin[0], CombineSignatures(input1, input2, output1));
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);

    // P2SH witness 2-of-2 multisig
    CreateCreditAndSpend(keystore, GetScriptForDestination(ScriptHash(destination_script_multi)), output1, input1, false);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, false);
    CreateCreditAndSpend(keystore2, GetScriptForDestination(ScriptHash(destination_script_multi)), output2, input2, false);
    CheckWithFlag(output2, input2, SCRIPT_VERIFY_P2SH, true);
    CheckWithFlag(output2, input2, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, false);
    BOOST_CHECK(*output1 == *output2);
    UpdateInput(input1.vin[0], CombineSignatures(input1, input2, output1));
    CheckWithFlag(output1, input1, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, true);
    CheckWithFlag(output1, input1, STANDARD_SCRIPT_VERIFY_FLAGS, true);
}

BOOST_AUTO_TEST_CASE(test_IsStandard)
{
    FillableSigningProvider keystore;
    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    std::vector<CMutableTransaction> dummyTransactions =
        SetupDummyInputs(keystore, coins, {11*CENT, 50*CENT, 21*CENT, 22*CENT});

    CMutableTransaction t;
    t.vin.resize(1);
    t.vin[0].prevout.hash = dummyTransactions[0].GetHash();
    t.vin[0].prevout.n = 1;
    t.vin[0].scriptSig << std::vector<unsigned char>(65, 0);
    t.vout.resize(1);
    t.vout[0].nValue = 90*CENT;
    CKey key;
    key.MakeNewKey(true);
    t.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));

    constexpr auto CheckIsStandard = [](const auto& t) {
        std::string reason;
        BOOST_CHECK(IsStandardTx(CTransaction{t}, MAX_OP_RETURN_RELAY, g_bare_multi, g_dust, reason));
        BOOST_CHECK(reason.empty());
    };
    constexpr auto CheckIsNotStandard = [](const auto& t, const std::string& reason_in) {
        std::string reason;
        BOOST_CHECK(!IsStandardTx(CTransaction{t}, MAX_OP_RETURN_RELAY, g_bare_multi, g_dust, reason));
        BOOST_CHECK_EQUAL(reason_in, reason);
    };

    CheckIsStandard(t);

    // Check dust with default relay fee:
    CAmount nDustThreshold = 182 * g_dust.GetFeePerK() / 1000;
    BOOST_CHECK_EQUAL(nDustThreshold, 546);
    // dust:
    t.vout[0].nValue = nDustThreshold - 1;
    CheckIsNotStandard(t, "dust");
    // not dust:
    t.vout[0].nValue = nDustThreshold;
    CheckIsStandard(t);

    // Disallowed nVersion
    t.nVersion = -1;
    CheckIsNotStandard(t, "version");

    t.nVersion = 0;
    CheckIsNotStandard(t, "version");

    t.nVersion = 3;
    CheckIsNotStandard(t, "version");

    // Allowed nVersion
    t.nVersion = 1;
    CheckIsStandard(t);

    t.nVersion = 2;
    CheckIsStandard(t);

    // Check dust with odd relay fee to verify rounding:
    // nDustThreshold = 182 * 3702 / 1000
    g_dust = CFeeRate(3702);
    // dust:
    t.vout[0].nValue = 674 - 1;
    CheckIsNotStandard(t, "dust");
    // not dust:
    t.vout[0].nValue = 674;
    CheckIsStandard(t);
    g_dust = CFeeRate{DUST_RELAY_TX_FEE};

    t.vout[0].scriptPubKey = CScript() << OP_1;
    CheckIsNotStandard(t, "scriptpubkey");

    // MAX_OP_RETURN_RELAY-byte TxoutType::NULL_DATA (standard)
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3804678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    BOOST_CHECK_EQUAL(MAX_OP_RETURN_RELAY, t.vout[0].scriptPubKey.size());
    CheckIsStandard(t);

    // MAX_OP_RETURN_RELAY+1-byte TxoutType::NULL_DATA (non-standard)
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3804678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3800");
    BOOST_CHECK_EQUAL(MAX_OP_RETURN_RELAY + 1, t.vout[0].scriptPubKey.size());
    CheckIsNotStandard(t, "scriptpubkey");

    // Data payload can be encoded in any way...
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("");
    CheckIsStandard(t);
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("00") << ParseHex("01");
    CheckIsStandard(t);
    // OP_RESERVED *is* considered to be a PUSHDATA type opcode by IsPushOnly()!
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << OP_RESERVED << -1 << 0 << ParseHex("01") << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11 << 12 << 13 << 14 << 15 << 16;
    CheckIsStandard(t);
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << 0 << ParseHex("01") << 2 << ParseHex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    CheckIsStandard(t);

    // ...so long as it only contains PUSHDATA's
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << OP_RETURN;
    CheckIsNotStandard(t, "scriptpubkey");

    // TxoutType::NULL_DATA w/o PUSHDATA
    t.vout.resize(1);
    t.vout[0].scriptPubKey = CScript() << OP_RETURN;
    CheckIsStandard(t);

    // Only one TxoutType::NULL_DATA permitted in all cases
    t.vout.resize(2);
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    t.vout[0].nValue = 0;
    t.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    t.vout[1].nValue = 0;
    CheckIsNotStandard(t, "multi-op-return");

    t.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38");
    t.vout[1].scriptPubKey = CScript() << OP_RETURN;
    CheckIsNotStandard(t, "multi-op-return");

    t.vout[0].scriptPubKey = CScript() << OP_RETURN;
    t.vout[1].scriptPubKey = CScript() << OP_RETURN;
    CheckIsNotStandard(t, "multi-op-return");

    // Check large scriptSig (non-standard if size is >1650 bytes)
    t.vout.resize(1);
    t.vout[0].nValue = MAX_MONEY;
    t.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));
    // OP_PUSHDATA2 with len (3 bytes) + data (1647 bytes) = 1650 bytes
    t.vin[0].scriptSig = CScript() << std::vector<unsigned char>(1647, 0); // 1650
    CheckIsStandard(t);

    t.vin[0].scriptSig = CScript() << std::vector<unsigned char>(1648, 0); // 1651
    CheckIsNotStandard(t, "scriptsig-size");

    // Check scriptSig format (non-standard if there are any other ops than just PUSHs)
    t.vin[0].scriptSig = CScript()
        << OP_TRUE << OP_0 << OP_1NEGATE << OP_16 // OP_n (single byte pushes: n = 1, 0, -1, 16)
        << std::vector<unsigned char>(75, 0)      // OP_PUSHx [...x bytes...]
        << std::vector<unsigned char>(235, 0)     // OP_PUSHDATA1 x [...x bytes...]
        << std::vector<unsigned char>(1234, 0)    // OP_PUSHDATA2 x [...x bytes...]
        << OP_9;
    CheckIsStandard(t);

    const std::vector<unsigned char> non_push_ops = { // arbitrary set of non-push operations
        OP_NOP, OP_VERIFY, OP_IF, OP_ROT, OP_3DUP, OP_SIZE, OP_EQUAL, OP_ADD, OP_SUB,
        OP_HASH256, OP_CODESEPARATOR, OP_CHECKSIG, OP_CHECKLOCKTIMEVERIFY };

    CScript::const_iterator pc = t.vin[0].scriptSig.begin();
    while (pc < t.vin[0].scriptSig.end()) {
        opcodetype opcode;
        CScript::const_iterator prev_pc = pc;
        t.vin[0].scriptSig.GetOp(pc, opcode); // advance to next op
        // for the sake of simplicity, we only replace single-byte push operations
        if (opcode >= 1 && opcode <= OP_PUSHDATA4)
            continue;

        int index = prev_pc - t.vin[0].scriptSig.begin();
        unsigned char orig_op = *prev_pc; // save op
        // replace current push-op with each non-push-op
        for (auto op : non_push_ops) {
            t.vin[0].scriptSig[index] = op;
            CheckIsNotStandard(t, "scriptsig-not-pushonly");
        }
        t.vin[0].scriptSig[index] = orig_op; // restore op
        CheckIsStandard(t);
    }

    // Check tx-size (non-standard if transaction weight is > MAX_STANDARD_TX_WEIGHT)
    t.vin.clear();
    t.vin.resize(2438); // size per input (empty scriptSig): 41 bytes
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << std::vector<unsigned char>(19, 0); // output size: 30 bytes
    // tx header:                12 bytes =>     48 vbytes
    // 2438 inputs: 2438*41 = 99958 bytes => 399832 vbytes
    //    1 output:              30 bytes =>    120 vbytes
    //                      ===============================
    //                                total: 400000 vbytes
    BOOST_CHECK_EQUAL(GetTransactionWeight(CTransaction(t)), 400000);
    CheckIsStandard(t);

    // increase output size by one byte, so we end up with 400004 vbytes
    t.vout[0].scriptPubKey = CScript() << OP_RETURN << std::vector<unsigned char>(20, 0); // output size: 31 bytes
    BOOST_CHECK_EQUAL(GetTransactionWeight(CTransaction(t)), 400004);
    CheckIsNotStandard(t, "tx-size");

    // Check bare multisig (standard if policy flag g_bare_multi is set)
    g_bare_multi = true;
    t.vout[0].scriptPubKey = GetScriptForMultisig(1, {key.GetPubKey()}); // simple 1-of-1
    t.vin.resize(1);
    t.vin[0].scriptSig = CScript() << std::vector<unsigned char>(65, 0);
    CheckIsStandard(t);

    g_bare_multi = false;
    CheckIsNotStandard(t, "bare-multisig");
    g_bare_multi = DEFAULT_PERMIT_BAREMULTISIG;

    // Check compressed P2PK outputs dust threshold (must have leading 02 or 03)
    t.vout[0].scriptPubKey = CScript() << std::vector<unsigned char>(33, 0x02) << OP_CHECKSIG;
    t.vout[0].nValue = 576;
    CheckIsStandard(t);
    t.vout[0].nValue = 575;
    CheckIsNotStandard(t, "dust");

    // Check uncompressed P2PK outputs dust threshold (must have leading 04/06/07)
    t.vout[0].scriptPubKey = CScript() << std::vector<unsigned char>(65, 0x04) << OP_CHECKSIG;
    t.vout[0].nValue = 672;
    CheckIsStandard(t);
    t.vout[0].nValue = 671;
    CheckIsNotStandard(t, "dust");

    // Check P2PKH outputs dust threshold
    t.vout[0].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << std::vector<unsigned char>(20, 0) << OP_EQUALVERIFY << OP_CHECKSIG;
    t.vout[0].nValue = 546;
    CheckIsStandard(t);
    t.vout[0].nValue = 545;
    CheckIsNotStandard(t, "dust");

    // Check P2SH outputs dust threshold
    t.vout[0].scriptPubKey = CScript() << OP_HASH160 << std::vector<unsigned char>(20, 0) << OP_EQUAL;
    t.vout[0].nValue = 540;
    CheckIsStandard(t);
    t.vout[0].nValue = 539;
    CheckIsNotStandard(t, "dust");

    // Check P2WPKH outputs dust threshold
    t.vout[0].scriptPubKey = CScript() << OP_0 << std::vector<unsigned char>(20, 0);
    t.vout[0].nValue = 294;
    CheckIsStandard(t);
    t.vout[0].nValue = 293;
    CheckIsNotStandard(t, "dust");

    // Check P2WSH outputs dust threshold
    t.vout[0].scriptPubKey = CScript() << OP_0 << std::vector<unsigned char>(32, 0);
    t.vout[0].nValue = 330;
    CheckIsStandard(t);
    t.vout[0].nValue = 329;
    CheckIsNotStandard(t, "dust");

    // Check P2TR outputs dust threshold (Invalid xonly key ok!)
    t.vout[0].scriptPubKey = CScript() << OP_1 << std::vector<unsigned char>(32, 0);
    t.vout[0].nValue = 330;
    CheckIsStandard(t);
    t.vout[0].nValue = 329;
    CheckIsNotStandard(t, "dust");

    // Check future Witness Program versions dust threshold (non-32-byte pushes are undefined for version 1)
    for (int op = OP_1; op <= OP_16; op += 1) {
        t.vout[0].scriptPubKey = CScript() << (opcodetype)op << std::vector<unsigned char>(2, 0);
        t.vout[0].nValue = 240;
        CheckIsStandard(t);

        t.vout[0].nValue = 239;
        CheckIsNotStandard(t, "dust");
    }
}

BOOST_AUTO_TEST_SUITE_END()