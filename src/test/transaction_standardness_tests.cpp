// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <consensus/amount.h>
#include <consensus/validation.h>
#include <key.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <primitives/transaction_identifier.h>
#include <script/script.h>
#include <script/solver.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace util::hex_literals;

BOOST_FIXTURE_TEST_SUITE(transaction_standardness_tests, BasicTestingSetup)

namespace {
/// Policy settings passed to IsStandardTx, defaulting to the node defaults.
struct StandardPolicy {
    unsigned int max_op_return_relay{MAX_OP_RETURN_RELAY};
    bool permit_bare_multisig{DEFAULT_PERMIT_BAREMULTISIG};
    CFeeRate dust_relay_fee{DUST_RELAY_TX_FEE};
};

void CheckIsStandard(const CMutableTransaction& t, const StandardPolicy& policy = {})
{
    std::string reason;
    BOOST_CHECK(IsStandardTx(CTransaction{t}, policy.max_op_return_relay, policy.permit_bare_multisig, policy.dust_relay_fee, reason));
    BOOST_CHECK(reason.empty());
}

void CheckIsNotStandard(const CMutableTransaction& t, const std::string& reason_in, const StandardPolicy& policy = {})
{
    std::string reason;
    BOOST_CHECK(!IsStandardTx(CTransaction{t}, policy.max_op_return_relay, policy.permit_bare_multisig, policy.dust_relay_fee, reason));
    BOOST_CHECK_EQUAL(reason_in, reason);
}

CScript P2PKHScript()
{
    return GetScriptForDestination(PKHash(GenerateRandomKey().GetPubKey()));
}

/// A standard output, for cases that do not test the outputs.
CTxOut StandardOutput()
{
    return CTxOut{90 * CENT, P2PKHScript()};
}

/// A transaction with a single input. IsStandardTx does not look up the spent
/// outputs, so the prevout is arbitrary. The default scriptSig is standard.
CMutableTransaction MakeTx(std::vector<CTxOut> vout, CScript script_sig = CScript() << std::vector<unsigned char>(65, 0))
{
    CMutableTransaction t;
    t.vin = {
        CTxIn{COutPoint{Txid::FromUint256(uint256::ONE), 1}, std::move(script_sig)},
    };
    t.vout = std::move(vout);
    return t;
}

/// Check that an output with this output script is not dust at `threshold`, but
/// is dust one satoshi below it.
void CheckDustThreshold(const CScript& output_script, CAmount threshold, const StandardPolicy& policy = {})
{
    const auto tx_with_value{[&](CAmount value) {
        std::vector<CTxOut> vout{CTxOut{value, output_script}};
        vout.insert(vout.end(), MAX_DUST_OUTPUTS_PER_TX, CTxOut{0, P2PKHScript()});
        return MakeTx(std::move(vout));
    }};
    CheckIsStandard(tx_with_value(threshold), policy);
    CheckIsNotStandard(tx_with_value(threshold - 1), "dust", policy);
}
} // namespace

BOOST_AUTO_TEST_CASE(test_IsStandard)
{
    CheckIsStandard(MakeTx({StandardOutput()}));
}

BOOST_AUTO_TEST_CASE(test_IsStandard_dust_limit)
{
    // Check dust with default relay fee:
    const CAmount dust_threshold{182 * CFeeRate{DUST_RELAY_TX_FEE}.GetFeePerK() / 1000};
    BOOST_CHECK_EQUAL(dust_threshold, 546);

    // Add dust outputs up to allowed maximum, still standard!
    CMutableTransaction t{MakeTx({StandardOutput()})};
    for (size_t i{0}; i < MAX_DUST_OUTPUTS_PER_TX; ++i) {
        t.vout.emplace_back(0, t.vout[0].scriptPubKey);
        CheckIsStandard(t);
    }

    CheckDustThreshold(P2PKHScript(), dust_threshold);

    // Check dust with odd relay fee to verify rounding:
    // dust_threshold = 182 * 3702 / 1000
    CheckDustThreshold(P2PKHScript(), 674, {.dust_relay_fee = CFeeRate{3702}});
}

BOOST_AUTO_TEST_CASE(test_IsStandard_version)
{
    const auto tx_with_version{[](uint32_t version) {
        CMutableTransaction t{MakeTx({StandardOutput()})};
        t.version = version;
        return t;
    }};

    // Disallowed version
    CheckIsNotStandard(tx_with_version(std::numeric_limits<uint32_t>::max()), "version");
    CheckIsNotStandard(tx_with_version(0), "version");
    CheckIsNotStandard(tx_with_version(TX_MAX_STANDARD_VERSION + 1), "version");

    // Allowed version
    CheckIsStandard(tx_with_version(1));
    CheckIsStandard(tx_with_version(2));
}

BOOST_AUTO_TEST_CASE(test_IsStandard_scriptpubkey)
{
    CheckIsNotStandard(MakeTx({CTxOut{90 * CENT, CScript() << OP_1}}), "scriptpubkey");
}

BOOST_AUTO_TEST_CASE(test_IsStandard_op_return)
{
    const auto tx_with_op_return{[](CScript output_script) {
        return MakeTx({CTxOut{0, std::move(output_script)}});
    }};

    // Custom 83-byte TxoutType::NULL_DATA (standard with max_op_return_relay of 83)
    const CScript op_return_83{CScript() << OP_RETURN << "04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3804678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38"_hex};
    BOOST_CHECK_EQUAL(83, op_return_83.size());
    CheckIsStandard(tx_with_op_return(op_return_83), {.max_op_return_relay = 83});

    // Non-standard if max_op_return_relay datacarrier arg is one less
    CheckIsNotStandard(tx_with_op_return(op_return_83), "datacarrier", {.max_op_return_relay = 82});

    // Data payload can be encoded in any way...
    CheckIsStandard(tx_with_op_return(CScript() << OP_RETURN << ""_hex));
    CheckIsStandard(tx_with_op_return(CScript() << OP_RETURN << "00"_hex << "01"_hex));
    // OP_RESERVED *is* considered to be a PUSHDATA type opcode by IsPushOnly()!
    CheckIsStandard(tx_with_op_return(CScript() << OP_RETURN << OP_RESERVED << -1 << 0 << "01"_hex << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11 << 12 << 13 << 14 << 15 << 16));
    CheckIsStandard(tx_with_op_return(CScript() << OP_RETURN << 0 << "01"_hex << 2 << "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"_hex));

    // ...so long as it only contains PUSHDATA's
    CheckIsNotStandard(tx_with_op_return(CScript() << OP_RETURN << OP_RETURN), "scriptpubkey");

    // TxoutType::NULL_DATA w/o PUSHDATA
    CheckIsStandard(tx_with_op_return(CScript() << OP_RETURN));
}

BOOST_AUTO_TEST_CASE(test_IsStandard_multiple_op_return)
{
    const CScript op_return_42{CScript() << OP_RETURN << "04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38"_hex};
    const CScript op_return_83{CScript() << OP_RETURN << "04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef3804678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38"_hex};
    BOOST_CHECK_EQUAL(42, op_return_42.size());
    BOOST_CHECK_EQUAL(83, op_return_83.size());

    // Multiple TxoutType::NULL_DATA are permitted
    CheckIsStandard(MakeTx({CTxOut{0, op_return_42}, CTxOut{0, op_return_42}}));
    CheckIsStandard(MakeTx({CTxOut{0, op_return_42}, CTxOut{0, CScript() << OP_RETURN}}));
    CheckIsStandard(MakeTx({CTxOut{0, CScript() << OP_RETURN}, CTxOut{0, CScript() << OP_RETURN}}));

    const CMutableTransaction t{MakeTx({CTxOut{0, op_return_83}, CTxOut{0, op_return_83}})};
    const unsigned int datacarrier_size = op_return_83.size() * 2;
    CheckIsStandard(t); // Default max relay should never trigger
    CheckIsStandard(t, {.max_op_return_relay = datacarrier_size});
    CheckIsNotStandard(t, "datacarrier", {.max_op_return_relay = datacarrier_size - 1});
}

BOOST_AUTO_TEST_CASE(test_IsStandard_scriptsig_size)
{
    // Check large scriptSig (non-standard if size is >1650 bytes)
    // OP_PUSHDATA2 with len (3 bytes) + data (1647 bytes) = 1650 bytes
    CheckIsStandard(MakeTx({StandardOutput()}, CScript() << std::vector<unsigned char>(1647, 0))); // 1650
    CheckIsNotStandard(MakeTx({StandardOutput()}, CScript() << std::vector<unsigned char>(1648, 0)), "scriptsig-size"); // 1651
}

BOOST_AUTO_TEST_CASE(test_IsStandard_scriptsig_pushonly)
{
    // Check scriptSig format (non-standard if there are any other ops than just PUSHs)
    CMutableTransaction t{MakeTx({StandardOutput()}, CScript()
        << OP_TRUE << OP_0 << OP_1NEGATE << OP_16 // OP_n (single byte pushes: n = 1, 0, -1, 16)
        << std::vector<unsigned char>(75, 0)      // OP_PUSHx [...x bytes...]
        << std::vector<unsigned char>(235, 0)     // OP_PUSHDATA1 x [...x bytes...]
        << std::vector<unsigned char>(1234, 0)    // OP_PUSHDATA2 x [...x bytes...]
        << OP_9)};
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
}

BOOST_AUTO_TEST_CASE(test_IsStandard_tx_size)
{
    // Check tx-size (non-standard if transaction weight is > MAX_STANDARD_TX_WEIGHT)
    const auto tx_with_op_return_size{[](size_t data_size) {
        CMutableTransaction t;
        t.vin = std::vector<CTxIn>(2438, CTxIn{COutPoint{}}); // size per input (empty scriptSig): 41 bytes
        t.vout = {
            CTxOut{0, CScript() << OP_RETURN << std::vector<unsigned char>(data_size, 0)},
        };
        return t;
    }};

    const CMutableTransaction t{tx_with_op_return_size(19)}; // output size: 30 bytes
    // tx header:                12 bytes =>     48 weight units
    // 2438 inputs: 2438*41 = 99958 bytes => 399832 weight units
    //    1 output:              30 bytes =>    120 weight units
    //                      ======================================
    //                                total: 400000 weight units
    BOOST_CHECK_EQUAL(GetTransactionWeight(CTransaction(t)), 400000);
    CheckIsStandard(t);

    // increase output size by one byte, so we end up with 400004 weight units
    const CMutableTransaction t_too_large{tx_with_op_return_size(20)}; // output size: 31 bytes
    BOOST_CHECK_EQUAL(GetTransactionWeight(CTransaction(t_too_large)), 400004);
    CheckIsNotStandard(t_too_large, "tx-size");
}

BOOST_AUTO_TEST_CASE(test_IsStandard_bare_multisig)
{
    // Check bare multisig (standard if policy flag permit_bare_multisig is set)
    const CKey key{GenerateRandomKey()};
    const CMutableTransaction t{MakeTx({CTxOut{90 * CENT, GetScriptForMultisig(1, {key.GetPubKey()})}})}; // simple 1-of-1
    CheckIsStandard(t, {.permit_bare_multisig = true});
    CheckIsNotStandard(t, "bare-multisig", {.permit_bare_multisig = false});
}

BOOST_AUTO_TEST_CASE(test_IsStandard_dust_thresholds)
{
    // Check compressed P2PK outputs dust threshold (must have leading 02 or 03)
    CheckDustThreshold(CScript() << std::vector<unsigned char>(33, 0x02) << OP_CHECKSIG, 576);

    // Check uncompressed P2PK outputs dust threshold (must have leading 04/06/07)
    CheckDustThreshold(CScript() << std::vector<unsigned char>(65, 0x04) << OP_CHECKSIG, 672);

    // Check P2PKH outputs dust threshold
    CheckDustThreshold(CScript() << OP_DUP << OP_HASH160 << std::vector<unsigned char>(20, 0) << OP_EQUALVERIFY << OP_CHECKSIG, 546);

    // Check P2SH outputs dust threshold
    CheckDustThreshold(CScript() << OP_HASH160 << std::vector<unsigned char>(20, 0) << OP_EQUAL, 540);

    // Check P2WPKH outputs dust threshold
    CheckDustThreshold(CScript() << OP_0 << std::vector<unsigned char>(20, 0), 294);

    // Check P2WSH outputs dust threshold
    CheckDustThreshold(CScript() << OP_0 << std::vector<unsigned char>(32, 0), 330);

    // Check P2TR outputs dust threshold (Invalid xonly key ok!)
    CheckDustThreshold(CScript() << OP_1 << std::vector<unsigned char>(32, 0), 330);

    // Check future Witness Program versions dust threshold (non-32-byte pushes are undefined for version 1)
    for (int op = OP_1; op <= OP_16; op += 1) {
        CheckDustThreshold(CScript() << (opcodetype)op << std::vector<unsigned char>(2, 0), 240);
    }

    // Check anchor outputs
    const CScript anchor{CScript() << OP_1 << ANCHOR_BYTES};
    BOOST_CHECK(anchor.IsPayToAnchor());
    CheckDustThreshold(anchor, 240);
}

BOOST_AUTO_TEST_SUITE_END()
