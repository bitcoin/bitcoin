// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <coins.h>
#include <consensus/amount.h>
#include <core_io.h>
#include <netbase.h>
#include <node/privbcast.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <rpc/server.h>
#include <script/script.h>
#include <script/solver.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <univalue.h>
#include <util/string.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using node::PrivbcastQueue;

namespace {

/** The signature operations of SIGOPS_SCRIPT. */
constexpr int64_t SIGOPS{100};
/** A witness script whose signature operations are never executed: they count toward the policy
 *  size of a transaction that spends it, not toward its BIP141 size. */
const CScript SIGOPS_SCRIPT{[] {
    CScript script;
    script << OP_0 << OP_IF;
    for (int64_t i{0}; i < SIGOPS; ++i) script << OP_CHECKSIG;
    script << OP_ENDIF << OP_1;
    return script;
}()};
/** A witness script anyone can spend, with no signature operation. */
const CScript TRUE_SCRIPT{CScript{} << OP_TRUE};

CTxOut WshOut(const CScript& witness_script, CAmount value)
{
    return CTxOut{value, GetScriptForDestination(WitnessV0ScriptHash{witness_script})};
}

/** A transaction that spends each of coins, paid to witness_script, to these outputs. */
CTransactionRef Spend(const std::vector<COutPoint>& coins, const CScript& witness_script, std::vector<CTxOut> outputs)
{
    CMutableTransaction tx;
    for (const COutPoint& coin : coins) {
        tx.vin.emplace_back(coin);
        tx.vin.back().scriptWitness.stack.emplace_back(witness_script.begin(), witness_script.end());
    }
    tx.vout = std::move(outputs);
    return MakeTransactionRef(std::move(tx));
}

/** A node with -privatebroadcast, whose queue starts no job, and confirmed coins of COIN_VALUE paid
 *  to each of the two witness scripts. */
struct PrivatePackageSetup : public TestChain100Setup {
    static constexpr size_t COINS{8};
    static constexpr CAmount COIN_VALUE{3 * COIN};
    std::vector<COutPoint> sigops_coins;
    std::vector<COutPoint> true_coins;
    /** Whether the node has an onion proxy, as the queue sees it. Without one, nothing is queued.
     *  As no job starts here, nothing connects through it. */
    std::atomic<bool> onion_proxy{true};

    explicit PrivatePackageSetup(TestOpts opts = {}) : TestChain100Setup{ChainType::REGTEST, opts}
    {
        if (RPCIsInWarmup(nullptr)) SetRPCWarmupFinished();
        m_node.privbcast = std::make_unique<PrivbcastQueue>(
            privbcast::SeedMaterial{.dns_seeds = {"a.seed."}, .default_port = 18444, .chain = "regtest"},
            /*network_active=*/[] { return true; },
            /*onion_proxy=*/[this]() -> std::optional<Proxy> {
                if (!onion_proxy) return std::nullopt;
                return Proxy{LookupNumeric("127.0.0.1", 9050), /*tor_stream_isolation=*/true};
            },
            /*in_mempool=*/[this](const Txid& txid) { return m_node.mempool->exists(txid); });
        std::vector<CTxOut> outputs;
        for (size_t i{0}; i < COINS; ++i) {
            outputs.push_back(WshOut(SIGOPS_SCRIPT, COIN_VALUE));
            outputs.push_back(WshOut(TRUE_SCRIPT, COIN_VALUE));
        }
        const CTransactionRef& coinbase{m_coinbase_txns[0]};
        const CMutableTransaction funding{CreateValidTransaction({coinbase}, {COutPoint{coinbase->GetHash(), 0}}, /*input_height=*/1,
                                                                 {coinbaseKey}, outputs, /*feerate=*/std::nullopt, /*fee_output=*/std::nullopt)
                                              .first};
        CreateAndProcessBlock({funding}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        for (uint32_t i{0}; i < 2 * COINS; i += 2) {
            sigops_coins.emplace_back(funding.GetHash(), i);
            true_coins.emplace_back(funding.GetHash(), i + 1);
        }
        LOCK(cs_main);
        BOOST_REQUIRE(m_node.chainman->ActiveChainstate().CoinsTip().HaveCoin(true_coins.back()));
    }

    ~PrivatePackageSetup() { m_node.privbcast.reset(); }

    /** sendrawtransaction, as a client calls it. */
    UniValue SendRawTransaction(const CTransactionRef& tx)
    {
        JSONRPCRequest request;
        request.context = &m_node;
        request.strMethod = "sendrawtransaction";
        request.params = UniValue{UniValue::VARR};
        request.params.push_back(EncodeHexTx(*tx));
        return tableRPC.execute(request);
    }

    /** submitpackage, as a client calls it. */
    UniValue SubmitPackage(const std::vector<CTransactionRef>& txs, const std::optional<CFeeRate>& max_fee_rate = std::nullopt)
    {
        UniValue package{UniValue::VARR};
        for (const CTransactionRef& tx : txs) package.push_back(EncodeHexTx(*tx));
        JSONRPCRequest request;
        request.context = &m_node;
        request.strMethod = "submitpackage";
        request.params = UniValue{UniValue::VARR};
        request.params.push_back(std::move(package));
        if (max_fee_rate) request.params.push_back(ValueFromAmount(max_fee_rate->GetFeePerK()));
        return tableRPC.execute(request);
    }

    /** The error the result of submitpackage gives tx, or "" if none. */
    static std::string Error(const UniValue& result, const CTransactionRef& tx)
    {
        const UniValue& error{result["tx-results"][tx->GetWitnessHash().GetHex()]["error"]};
        return error.isNull() ? "" : error.get_str();
    }

    /** The jobs the queue holds. */
    size_t Jobs() const { return m_node.privbcast->Info().size(); }

    /** Put tx in the mempool. */
    void Accept(const CTransactionRef& tx)
    {
        LOCK(cs_main);
        BOOST_REQUIRE(m_node.chainman->ProcessTransaction(tx).m_result_type == MempoolAcceptResult::ResultType::VALID);
    }
};

/** As above, with a mempool whose cluster limit takes two transactions that together weigh more
 *  than a package may. */
struct LargeClusterSetup : public PrivatePackageSetup {
    LargeClusterSetup() : PrivatePackageSetup{{.extra_args = {"-limitclustersize=200"}}} {}
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(privbcast_rpc_tests, PrivatePackageSetup)

BOOST_AUTO_TEST_CASE(onion_proxy_from_the_queue)
{
    // Interface/Node, Extension: Interface: without an onion proxy for jobs, as the queue reports,
    // sendrawtransaction and submitpackage fail with RPC_MISC_ERROR; with one, they queue jobs.
    const CTransactionRef tx{Spend({true_coins[0]}, TRUE_SCRIPT, {WshOut(TRUE_SCRIPT, COIN_VALUE - 1'000)})};
    const CTransactionRef other{Spend({true_coins[1]}, TRUE_SCRIPT, {WshOut(TRUE_SCRIPT, COIN_VALUE - 1'000)})};
    const auto no_proxy{[](const UniValue& error) { return error["code"].getValStr() == util::ToString(RPC_MISC_ERROR); }};
    onion_proxy = false;
    BOOST_CHECK_EXCEPTION(SendRawTransaction(tx), UniValue, no_proxy);
    BOOST_CHECK_EXCEPTION(SubmitPackage({tx}), UniValue, no_proxy);
    BOOST_CHECK_EQUAL(Jobs(), 0U);
    onion_proxy = true;
    BOOST_CHECK_EQUAL(SendRawTransaction(tx).get_str(), tx->GetHash().GetHex());
    BOOST_CHECK_EQUAL(SubmitPackage({other})["package_msg"].get_str(), "success");
    BOOST_CHECK_EQUAL(Jobs(), 2U);
}

BOOST_AUTO_TEST_CASE(package_feerate_sizes)
{
    // Extension: Interface: a parent that fails alone is test-accepted with its child at their
    // package feerate, on the sigop-adjusted sizes; here the parent's exceeds its BIP141 size, and a child that pays
    // the minimum relay feerate for the two only on their BIP141 sizes is refused.
    const CTransactionRef parent{Spend({sigops_coins[0]}, SIGOPS_SCRIPT, {WshOut(TRUE_SCRIPT, COIN_VALUE)})};
    const int64_t parent_vsize{GetVirtualTransactionSize(*parent, SIGOPS, DEFAULT_BYTES_PER_SIGOP)};
    BOOST_REQUIRE_GT(parent_vsize, GetVirtualTransactionSize(*parent));
    const auto child{[&](CAmount fee) { return Spend({{parent->GetHash(), 0}}, TRUE_SCRIPT, {WshOut(TRUE_SCRIPT, COIN_VALUE - fee)}); }};
    const int64_t child_vsize{GetVirtualTransactionSize(*child(0))};
    const CFeeRate& min_relay_feerate{m_node.mempool->m_opts.min_relay_feerate};
    const CAmount bip141_fee{min_relay_feerate.GetFee(GetVirtualTransactionSize(*parent) + child_vsize)};
    const CAmount policy_fee{min_relay_feerate.GetFee(parent_vsize + child_vsize)};
    BOOST_REQUIRE_LT(bip141_fee, policy_fee - 1);
    for (const CAmount fee : {bip141_fee, policy_fee - 1}) {
        const CTransactionRef refused{child(fee)};
        const UniValue result{SubmitPackage({parent, refused})};
        BOOST_CHECK_EQUAL(result["package_msg"].get_str(), "transaction failed");
        BOOST_CHECK(Error(result, parent).starts_with("min relay fee not met"));
        BOOST_CHECK(Error(result, refused).starts_with("min relay fee not met"));
    }
    BOOST_CHECK_EQUAL(Jobs(), 0U);
    BOOST_CHECK_EQUAL(SubmitPackage({parent, child(policy_fee)})["package_msg"].get_str(), "success");
    BOOST_CHECK_EQUAL(Jobs(), 1U);

    // maxfeerate holds for the child on its sigop-adjusted size, as submitpackage applies it: the
    // most it allows goes out, a satoshi more does not.
    const CFeeRate max_fee_rate{10'000};
    const CTransactionRef free_parent{Spend({true_coins[0]}, TRUE_SCRIPT, {WshOut(SIGOPS_SCRIPT, COIN_VALUE)})};
    const auto sigops_child{[&](CAmount fee) { return Spend({{free_parent->GetHash(), 0}}, SIGOPS_SCRIPT, {WshOut(TRUE_SCRIPT, COIN_VALUE - fee)}); }};
    const CAmount max_fee{max_fee_rate.GetFee(GetVirtualTransactionSize(*sigops_child(0), SIGOPS, DEFAULT_BYTES_PER_SIGOP))};
    BOOST_REQUIRE_GT(max_fee, max_fee_rate.GetFee(GetVirtualTransactionSize(*sigops_child(0))));
    BOOST_CHECK_EQUAL(SubmitPackage({free_parent, sigops_child(max_fee)}, max_fee_rate)["package_msg"].get_str(), "success");
    const CTransactionRef over{sigops_child(max_fee + 1)};
    const UniValue result{SubmitPackage({free_parent, over}, max_fee_rate)};
    BOOST_CHECK_EQUAL(Error(result, over), "max feerate exceeded");
    BOOST_CHECK_EQUAL(Jobs(), 2U);
}

BOOST_AUTO_TEST_CASE(max_fee_rate)
{
    // Extension: Interface: maxfeerate holds for each of two test-accepted as a package as
    // submitpackage applies it (modified fee over the sigop-adjusted size), and for one
    // test-accepted alone as sendrawtransaction does (fee against the BIP141 size). Every
    // transaction checked has the same sizes, the sigop-adjusted one the larger.
    const CFeeRate max_fee_rate{10'000};
    const auto sigops_spend{[](const COutPoint& coin, CAmount value, CAmount fee) {
        return Spend({coin}, SIGOPS_SCRIPT, {WshOut(TRUE_SCRIPT, value - fee)});
    }};
    const CTransactionRef sample{sigops_spend(sigops_coins[0], COIN_VALUE, 0)};
    const CAmount policy_max{max_fee_rate.GetFee(GetVirtualTransactionSize(*sample, SIGOPS, DEFAULT_BYTES_PER_SIGOP))};
    const CAmount bip141_max{max_fee_rate.GetFee(GetVirtualTransactionSize(*sample))};
    BOOST_REQUIRE_LT(bip141_max, policy_max);
    const auto child{[](const CTransactionRef& parent) {
        return Spend({{parent->GetHash(), 0}}, TRUE_SCRIPT, {WshOut(TRUE_SCRIPT, parent->vout[0].nValue - 500)});
    }};

    // Two test-accepted as a package: a parent that pays the most allowed for its policy size goes
    // out with its child. One that pays a satoshi more does not, nor one prioritised by a satoshi,
    // and its child, tried alone, misses its input, as submitpackage reports it.
    const CTransactionRef at_max{sigops_spend(sigops_coins[0], COIN_VALUE, policy_max)};
    BOOST_CHECK_EQUAL(SubmitPackage({at_max, child(at_max)}, max_fee_rate)["package_msg"].get_str(), "success");
    BOOST_CHECK_EQUAL(Jobs(), 1U);
    const CTransactionRef over_max{sigops_spend(sigops_coins[1], COIN_VALUE, policy_max + 1)};
    const CTransactionRef prioritised{sigops_spend(sigops_coins[2], COIN_VALUE, policy_max)};
    m_node.mempool->PrioritiseTransaction(prioritised->GetHash(), 1);
    for (const CTransactionRef& parent : {over_max, prioritised}) {
        const CTransactionRef refused_child{child(parent)};
        const UniValue result{SubmitPackage({parent, refused_child}, max_fee_rate)};
        BOOST_CHECK_EQUAL(result["package_msg"].get_str(), "transaction failed");
        BOOST_CHECK_EQUAL(Error(result, parent), "max feerate exceeded");
        BOOST_CHECK_EQUAL(Error(result, refused_child), "bad-txns-inputs-missingorspent");
    }
    BOOST_CHECK_EQUAL(Jobs(), 1U);

    // One test-accepted alone: the most allowed for its BIP141 size, whatever its priority, goes
    // out, and a satoshi more does not.
    const CTransactionRef alone{sigops_spend(sigops_coins[3], COIN_VALUE, bip141_max)};
    m_node.mempool->PrioritiseTransaction(alone->GetHash(), COIN);
    BOOST_CHECK_EQUAL(SubmitPackage({alone}, max_fee_rate)["package_msg"].get_str(), "success");
    const CTransactionRef alone_over{sigops_spend(sigops_coins[4], COIN_VALUE, bip141_max + 1)};
    BOOST_CHECK_EQUAL(Error(SubmitPackage({alone_over}, max_fee_rate), alone_over), "max feerate exceeded");
    BOOST_CHECK_EQUAL(Jobs(), 2U);

    // So too a child test-accepted alone, its parent being in the mempool.
    const CTransactionRef parent{Spend({true_coins[0]}, TRUE_SCRIPT, {WshOut(SIGOPS_SCRIPT, COIN_VALUE - 1'000)})};
    Accept(parent);
    const CTransactionRef child_at_max{sigops_spend({parent->GetHash(), 0}, parent->vout[0].nValue, bip141_max)};
    BOOST_CHECK_EQUAL(SubmitPackage({parent, child_at_max}, max_fee_rate)["package_msg"].get_str(), "success");
    const CTransactionRef child_over{sigops_spend({parent->GetHash(), 0}, parent->vout[0].nValue, bip141_max + 1)};
    const UniValue result{SubmitPackage({parent, child_over}, max_fee_rate)};
    BOOST_CHECK_EQUAL(Error(result, parent), "");
    BOOST_CHECK_EQUAL(Error(result, child_over), "max feerate exceeded");
    BOOST_CHECK_EQUAL(Jobs(), 3U);
}

BOOST_AUTO_TEST_CASE(package_priority)
{
    // Extension: Interface: a child test-accepted with its parent counts its prioritisetransaction
    // delta, against maxfeerate and in what the two pay together.
    const CFeeRate max_fee_rate{10'000};
    const CTransactionRef parent{Spend({true_coins[0]}, TRUE_SCRIPT, {WshOut(SIGOPS_SCRIPT, COIN_VALUE)})};
    const auto child{[&](CAmount fee) { return Spend({{parent->GetHash(), 0}}, SIGOPS_SCRIPT, {WshOut(TRUE_SCRIPT, COIN_VALUE - fee)}); }};
    const int64_t child_vsize{GetVirtualTransactionSize(*child(0), SIGOPS, DEFAULT_BYTES_PER_SIGOP)};
    const CAmount max_fee{max_fee_rate.GetFee(child_vsize)};
    const CAmount min_fee{m_node.mempool->m_opts.min_relay_feerate.GetFee(GetVirtualTransactionSize(*parent) + child_vsize)};
    BOOST_REQUIRE_LT(min_fee, max_fee);
    // The most maxfeerate allows, prioritised by a satoshi: too much.
    const CTransactionRef too_much{child(max_fee)};
    m_node.mempool->PrioritiseTransaction(too_much->GetHash(), 1);
    BOOST_CHECK_EQUAL(Error(SubmitPackage({parent, too_much}, max_fee_rate), too_much), "max feerate exceeded");
    // A satoshi short of the minimum relay feerate for the two, prioritised by a satoshi: enough.
    const CTransactionRef short_by_one{child(min_fee - 1)};
    BOOST_CHECK(Error(SubmitPackage({parent, short_by_one}, max_fee_rate), short_by_one).starts_with("min relay fee not met"));
    m_node.mempool->PrioritiseTransaction(short_by_one->GetHash(), 1);
    BOOST_CHECK_EQUAL(SubmitPackage({parent, short_by_one}, max_fee_rate)["package_msg"].get_str(), "success");
    BOOST_CHECK_EQUAL(Jobs(), 1U);
}

BOOST_AUTO_TEST_CASE(package_priority_saturates)
{
    // Extension: Interface: for a parent and child test-accepted together, the deltas and the two
    // fees are added saturating, so no delta wraps a fee around.
    const auto child{[](const CTransactionRef& parent) {
        return Spend({{parent->GetHash(), 0}}, TRUE_SCRIPT, {WshOut(TRUE_SCRIPT, COIN_VALUE - 1'000)});
    }};
    // Both prioritised by -5,000,000,000,000,000,000: the sum of their fees, below INT64_MIN,
    // counts as INT64_MIN, and the pair is refused.
    const CTransactionRef parent{Spend({true_coins[0]}, TRUE_SCRIPT, {WshOut(TRUE_SCRIPT, COIN_VALUE)})};
    const CTransactionRef refused{child(parent)};
    for (const CTransactionRef& tx : {parent, refused}) m_node.mempool->PrioritiseTransaction(tx->GetHash(), -5'000'000'000'000'000'000);
    const UniValue result{SubmitPackage({parent, refused})};
    BOOST_CHECK_EQUAL(result["package_msg"].get_str(), "transaction failed");
    BOOST_CHECK(Error(result, refused).starts_with("min relay fee not met"));
    // A child prioritised by INT64_MAX: its fee counts as INT64_MAX, beyond maxfeerate, and the
    // pair is refused, the parent for its own fee.
    const CTransactionRef other_parent{Spend({true_coins[1]}, TRUE_SCRIPT, {WshOut(TRUE_SCRIPT, COIN_VALUE)})};
    const CTransactionRef prioritised{child(other_parent)};
    m_node.mempool->PrioritiseTransaction(prioritised->GetHash(), std::numeric_limits<CAmount>::max());
    const UniValue over{SubmitPackage({other_parent, prioritised})};
    BOOST_CHECK_EQUAL(Error(over, prioritised), "max feerate exceeded");
    BOOST_CHECK(Error(over, other_parent).starts_with("min relay fee not met"));
    BOOST_CHECK_EQUAL(Jobs(), 0U);
}

BOOST_FIXTURE_TEST_CASE(package_weight, LargeClusterSetup)
{
    // Extension: Interface: a parent and its child weigh at most MAX_PACKAGE_WEIGHT together,
    // whether neither, one or both are in the mempool already.
    const auto wide{[](const COutPoint& coin, CAmount value) {
        std::vector<CTxOut> outputs(1'200, WshOut(TRUE_SCRIPT, 1'000));
        outputs[0].nValue = value - static_cast<CAmount>(outputs.size() - 1) * 1'000 - 100'000;
        return Spend({coin}, TRUE_SCRIPT, std::move(outputs));
    }};
    const CTransactionRef parent{wide(true_coins[0], COIN_VALUE)};
    const CTransactionRef child{wide({parent->GetHash(), 0}, parent->vout[0].nValue)};
    for (const CTransactionRef& tx : {parent, child}) BOOST_REQUIRE_LE(GetTransactionWeight(*tx), MAX_STANDARD_TX_WEIGHT);
    BOOST_REQUIRE_GT(GetTransactionWeight(*parent) + GetTransactionWeight(*child), MAX_PACKAGE_WEIGHT);
    const auto refused{[&] {
        const UniValue result{SubmitPackage({parent, child})};
        BOOST_CHECK_EQUAL(result["package_msg"].get_str(), "package-too-large");
        for (const CTransactionRef& tx : {parent, child}) BOOST_CHECK_EQUAL(Error(result, tx), "package-not-validated");
    }};
    refused();
    BOOST_CHECK_EQUAL(Jobs(), 0U);
    // The parent in the mempool: the child alone goes out, and the two do not.
    Accept(parent);
    refused();
    BOOST_CHECK_EQUAL(Jobs(), 0U);
    BOOST_CHECK_EQUAL(SubmitPackage({child})["package_msg"].get_str(), "success");
    BOOST_CHECK_EQUAL(Jobs(), 1U);
    // Both in the mempool.
    Accept(child);
    refused();
    BOOST_CHECK_EQUAL(Jobs(), 1U);
}

BOOST_AUTO_TEST_SUITE_END()
