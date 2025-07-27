// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/data/tx_invalid.json.h>
#include <test/data/tx_valid.json.h>
#include <test/util/setup_common.h>

#include <checkqueue.h>
#include <clientversion.h>
#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <key.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/signingprovider.h>
#include <streams.h>
#include <test/util/json.h>
#include <test/util/transaction_utils.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <validation.h>

#include <functional>
#include <map>
#include <string>

#include <boost/test/unit_test.hpp>

#include <univalue.h>

static std::map<std::string, unsigned int> mapFlagNames = {
    {std::string("P2SH"), (unsigned int) SCRIPT_VERIFY_P2SH},
    {std::string("STRICTENC"), (unsigned int) SCRIPT_VERIFY_STRICTENC},
    {std::string("DERSIG"), (unsigned int) SCRIPT_VERIFY_DERSIG},
    {std::string("LOW_S"), (unsigned int) SCRIPT_VERIFY_LOW_S},
    {std::string("SIGPUSHONLY"), (unsigned int) SCRIPT_VERIFY_SIGPUSHONLY},
    {std::string("MINIMALDATA"), (unsigned int) SCRIPT_VERIFY_MINIMALDATA},
    {std::string("NULLDUMMY"), (unsigned int) SCRIPT_VERIFY_NULLDUMMY},
    {std::string("DISCOURAGE_UPGRADABLE_NOPS"), (unsigned int) SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS},
    {std::string("CLEANSTACK"), (unsigned int) SCRIPT_VERIFY_CLEANSTACK},
    {std::string("NULLFAIL"), (unsigned int) SCRIPT_VERIFY_NULLFAIL},
    {std::string("CHECKLOCKTIMEVERIFY"), (unsigned int) SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY},
    {std::string("CHECKSEQUENCEVERIFY"), (unsigned int) SCRIPT_VERIFY_CHECKSEQUENCEVERIFY},
    {std::string("CONST_SCRIPTCODE"), (unsigned int)SCRIPT_VERIFY_CONST_SCRIPTCODE},
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
    unsigned int flags,
    const PrecomputedTransactionData& txdata, const std::string& strTest, bool expect_valid)
{
    bool tx_valid = true;
    ScriptError err = expect_valid ? SCRIPT_ERR_UNKNOWN_ERROR : SCRIPT_ERR_OK;
    for (unsigned int i = 0; i < tx.vin.size() && tx_valid; ++i) {
        const CTxIn input = tx.vin[i];
        const CAmount amount = 0;
        try {
            tx_valid = VerifyScript(input.scriptSig, map_prevout_scriptPubKeys.at(input.prevout),
                flags, TransactionSignatureChecker(&tx, i, amount, txdata, MissingDataBehavior::ASSERT_FAIL), &err);
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
 * Trim or fill flags to make the combination valid, used for SegWit code only
 */

unsigned int TrimFlags(unsigned int flags)
{
    // CLEANSTACK requires P2SH)
    if (!(flags & SCRIPT_VERIFY_P2SH)) flags &= ~(unsigned int)SCRIPT_VERIFY_CLEANSTACK;

    return flags;
}

unsigned int FillFlags(unsigned int flags)
{
    // CLEANSTACK implies P2SH
    if (flags & SCRIPT_VERIFY_CLEANSTACK) flags |= SCRIPT_VERIFY_P2SH;

    return flags;
}

// Return valid flags that are all except one flag for each flag
std::vector<unsigned int> ExcludeIndividualFlags(unsigned int flags)
{
    std::vector<unsigned int> flags_combos;
    for (unsigned int i = 0; i < mapFlagNames.size(); ++i) {
        const unsigned int flags_excluding_i = TrimFlags(flags & ~(1U << i));
        if (flags != flags_excluding_i && std::find(flags_combos.begin(), flags_combos.end(), flags_excluding_i) != flags_combos.end()) {
            flags_combos.push_back(flags_excluding_i);
        }
    }
    return flags_combos;
}

BOOST_FIXTURE_TEST_SUITE(transaction_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(tx_valid)
{
    // Read tests from test/data/tx_valid.json
    UniValue tests = read_json(std::string(json_tests::tx_valid, json_tests::tx_valid + sizeof(json_tests::tx_valid)));

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        UniValue test = tests[idx];
        std::string strTest = test.write();
        if (test[0].isArray())
        {
            if (test.size() != 3 || !test[1].isStr() || !test[2].isStr())
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            std::map<COutPoint, CScript> mapprevOutScriptPubKeys;
            UniValue inputs = test[0].get_array();
            bool fValid = true;
            for (unsigned int inpIdx = 0; inpIdx < inputs.size(); inpIdx++) {
                const UniValue& input = inputs[inpIdx];
                if (!input.isArray()) {
                    fValid = false;
                    break;
                }
                UniValue vinput = input.get_array();
                if (vinput.size() != 3)
                {
                    fValid = false;
                    break;
                }

                mapprevOutScriptPubKeys[COutPoint{uint256S(vinput[0].get_str()), uint32_t(vinput[1].getInt<int>())}] = ParseScript(vinput[2].get_str());
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

            if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, ~verify_flags, txdata, strTest, /*expect_valid=*/true)) {
                BOOST_ERROR("Tx unexpectedly failed: " << strTest);
            }

            // Backwards compatibility of script verification flags: Removing any flag(s) should not invalidate a valid transaction
            for (size_t i = 0; i < mapFlagNames.size(); ++i) {
                // Removing individual flags
                unsigned int flags = TrimFlags(~(verify_flags | (1U << i)));
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, flags, txdata, strTest, /*expect_valid=*/true)) {
                    BOOST_ERROR("Tx unexpectedly failed with flag " << ToString(i) << " unset: " << strTest);
                }
                // Removing random combinations of flags
                flags = TrimFlags(~(verify_flags | (unsigned int)InsecureRandBits(mapFlagNames.size())));
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, flags, txdata, strTest, /*expect_valid=*/true)) {
                    BOOST_ERROR("Tx unexpectedly failed with random flags " << ToString(flags) << ": " << strTest);
                }
            }

            // Check that flags are maximal: transaction should fail if any unset flags are set.
            for (auto flags_excluding_one: ExcludeIndividualFlags(verify_flags)) {
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, ~flags_excluding_one, txdata, strTest, /*expect_valid=*/false)) {
                    BOOST_ERROR("Too many flags unset: " << strTest);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(tx_invalid)
{
    // Read tests from test/data/tx_invalid.json
    UniValue tests = read_json(std::string(json_tests::tx_invalid, json_tests::tx_invalid + sizeof(json_tests::tx_invalid)));

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        UniValue test = tests[idx];
        std::string strTest = test.write();
        if (test[0].isArray())
        {
            if (test.size() != 3 || !test[1].isStr() || !test[2].isStr())
            {
                BOOST_ERROR("Bad test: " << strTest);
                continue;
            }

            std::map<COutPoint, CScript> mapprevOutScriptPubKeys;
            UniValue inputs = test[0].get_array();
            bool fValid = true;
            for (unsigned int inpIdx = 0; inpIdx < inputs.size(); inpIdx++) {
                const UniValue& input = inputs[inpIdx];
                if (!input.isArray()) {
                    fValid = false;
                    break;
                }
                UniValue vinput = input.get_array();
                if (vinput.size() != 3)
                {
                    fValid = false;
                    break;
                }

                mapprevOutScriptPubKeys[COutPoint{uint256S(vinput[0].get_str()), uint32_t(vinput[1].getInt<int>())}] = ParseScript(vinput[2].get_str());
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
            if (!CheckTransaction(tx, state) || state.IsInvalid()) {
                BOOST_CHECK_MESSAGE(test[2].get_str() == "BADTX", strTest);
                continue;
            }

            PrecomputedTransactionData txdata(tx);
            unsigned int verify_flags = ParseScriptFlags(test[2].get_str());

            // Not using FillFlags() in the main test, in order to detect invalid verifyFlags combination
            if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, verify_flags, txdata, strTest, /*expect_valid=*/false)) {
                BOOST_ERROR("Tx unexpectedly passed: " << strTest);
            }

            // Backwards compatibility of script verification flags: Adding any flag(s) should not validate an invalid transaction
            for (size_t i = 0; i < mapFlagNames.size(); i++) {
                unsigned int flags = FillFlags(verify_flags | (1U << i));
                // Adding individual flags
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, flags, txdata, strTest, /*expect_valid=*/false)) {
                    BOOST_ERROR("Tx unexpectedly passed with flag " << ToString(i) << " set: " << strTest);
                }
                // Adding random combinations of flags
                flags = FillFlags(verify_flags | (unsigned int)InsecureRandBits(mapFlagNames.size()));
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, flags, txdata, strTest, /*expect_valid=*/false)) {
                    BOOST_ERROR("Tx unexpectedly passed with random flags " << ToString(flags) << ": " << strTest);
                }
            }

            // Check that flags are minimal: transaction should succeed if any set flags are unset.
            for (auto flags_excluding_one: ExcludeIndividualFlags(verify_flags)) {
                if (!CheckTxScripts(tx, mapprevOutScriptPubKeys, flags_excluding_one, txdata, strTest, /*expect_valid=*/true)) {
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

BOOST_AUTO_TEST_CASE(test_IsStandard)
{
    LOCK(cs_main);
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
        BOOST_CHECK(IsStandardTx(CTransaction(t), reason));
        BOOST_CHECK(reason.empty());
    };
    constexpr auto CheckIsNotStandard = [](const auto& t, const std::string& reason_in) {
        std::string reason;
        BOOST_CHECK(!IsStandardTx(CTransaction(t), reason));
        BOOST_CHECK_EQUAL(reason_in, reason);
    };

    CheckIsStandard(t);

    // Check dust with default relay fee:
    CAmount nDustThreshold = 182 * dustRelayFee.GetFeePerK() / 1000;
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

    t.nVersion = 4;
    CheckIsNotStandard(t, "version");

    // Allowed nVersion
    t.nVersion = 1;
    CheckIsStandard(t);

    t.nVersion = 2;
    CheckIsStandard(t);

    t.nVersion = 3;
    CheckIsStandard(t);
    // Check dust with odd relay fee to verify rounding:
    // nDustThreshold = 182 * 3702 / 1000
    minRelayTxFee = CFeeRate(3702);
    // dust:
    t.vout[0].nValue = 546 - 1;
    CheckIsNotStandard(t, "dust");
    // not dust:
    t.vout[0].nValue = 546;
    CheckIsStandard(t);
    minRelayTxFee = CFeeRate(DUST_RELAY_TX_FEE);

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

    // Check bare multisig (standard if policy flag fIsBareMultisigStd is set)
    fIsBareMultisigStd = true;
    t.vout[0].scriptPubKey = GetScriptForMultisig(1, {key.GetPubKey()}); // simple 1-of-1
    t.vin[0].scriptSig = CScript() << std::vector<unsigned char>(65, 0);
    CheckIsStandard(t);

    fIsBareMultisigStd = false;
    CheckIsNotStandard(t, "bare-multisig");
    fIsBareMultisigStd = DEFAULT_PERMIT_BAREMULTISIG;
}

BOOST_AUTO_TEST_SUITE_END()
