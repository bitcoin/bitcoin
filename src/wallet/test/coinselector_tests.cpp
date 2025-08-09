// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <node/context.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <random.h>
#include <test/util/setup_common.h>
#include <util/translation.h>
#include <wallet/coincontrol.h>
#include <wallet/coinselection.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>
#include <wallet/wallet.h>

#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <random>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(coinselector_tests, WalletTestingSetup)

// how many times to run all the tests to have a chance to catch errors that only show up with particular random shuffles
#define RUN_TESTS 100

// some tests fail 1% of the time due to bad luck.
// we repeat those tests this many times and only complain if all iterations of the test fail
#define RANDOM_REPEATS 5

typedef std::set<std::shared_ptr<COutput>> CoinSet;

static const CoinEligibilityFilter filter_standard(1, 6, 0);
static const CoinEligibilityFilter filter_confirmed(1, 1, 0);
static const CoinEligibilityFilter filter_standard_extra(6, 6, 0);
static int nextLockTime = 0;

static void add_coin(const CAmount& nValue, int nInput, SelectionResult& result)
{
    CMutableTransaction tx;
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    tx.nLockTime = nextLockTime++;        // so all transactions get different hashes
    COutput output(COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 1, /*input_bytes=*/ -1, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, /*fees=*/ 0);
    OutputGroup group;
    group.Insert(std::make_shared<COutput>(output), /*ancestors=*/ 0, /*descendants=*/ 0);
    result.AddInput(group);
}

static void add_coin(const CAmount& nValue, int nInput, SelectionResult& result, CAmount fee, CAmount long_term_fee)
{
    CMutableTransaction tx;
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    tx.nLockTime = nextLockTime++;        // so all transactions get different hashes
    std::shared_ptr<COutput> coin = std::make_shared<COutput>(COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 1, /*input_bytes=*/ 148, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, fee);
    OutputGroup group;
    group.Insert(coin, /*ancestors=*/ 0, /*descendants=*/ 0);
    coin->long_term_fee = long_term_fee; // group.Insert() will modify long_term_fee, so we need to set it afterwards
    result.AddInput(group);
}

static void add_coin(CoinsResult& available_coins, CWallet& wallet, const CAmount& nValue, CFeeRate feerate = CFeeRate(0), int nAge = 6*24, bool fIsFromMe = false, int nInput =0, bool spendable = false, int custom_size = 0)
{
    CMutableTransaction tx;
    tx.nLockTime = nextLockTime++;        // so all transactions get different hashes
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    if (spendable) {
        tx.vout[nInput].scriptPubKey = GetScriptForDestination(*Assert(wallet.GetNewDestination(OutputType::BECH32, "")));
    }
    Txid txid = tx.GetHash();

    LOCK(wallet.cs_wallet);
    auto ret = wallet.mapWallet.emplace(std::piecewise_construct, std::forward_as_tuple(txid), std::forward_as_tuple(MakeTransactionRef(std::move(tx)), TxStateInactive{}));
    assert(ret.second);
    CWalletTx& wtx = (*ret.first).second;
    const auto& txout = wtx.tx->vout.at(nInput);
    available_coins.Add(OutputType::BECH32, {COutPoint(wtx.GetHash(), nInput), txout, nAge, custom_size == 0 ? CalculateMaximumSignedInputSize(txout, &wallet, /*coin_control=*/nullptr) : custom_size, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, wtx.GetTxTime(), fIsFromMe, feerate});
}

// Helpers
std::optional<SelectionResult> KnapsackSolver(std::vector<OutputGroup>& groups, const CAmount& nTargetValue,
                                              CAmount change_target, FastRandomContext& rng)
{
    auto res{KnapsackSolver(groups, nTargetValue, change_target, rng, MAX_STANDARD_TX_WEIGHT)};
    return res ? std::optional<SelectionResult>(*res) : std::nullopt;
}

std::optional<SelectionResult> SelectCoinsBnB(std::vector<OutputGroup>& utxo_pool, const CAmount& selection_target, const CAmount& cost_of_change)
{
    auto res{SelectCoinsBnB(utxo_pool, selection_target, cost_of_change, MAX_STANDARD_TX_WEIGHT)};
    return res ? std::optional<SelectionResult>(*res) : std::nullopt;
}

/** Check if SelectionResult a is equivalent to SelectionResult b.
 * Equivalent means same input values, but maybe different inputs (i.e. same value, different prevout) */
static bool EquivalentResult(const SelectionResult& a, const SelectionResult& b)
{
    std::vector<CAmount> a_amts;
    std::vector<CAmount> b_amts;
    for (const auto& coin : a.GetInputSet()) {
        a_amts.push_back(coin->txout.nValue);
    }
    for (const auto& coin : b.GetInputSet()) {
        b_amts.push_back(coin->txout.nValue);
    }
    std::sort(a_amts.begin(), a_amts.end());
    std::sort(b_amts.begin(), b_amts.end());

    std::pair<std::vector<CAmount>::iterator, std::vector<CAmount>::iterator> ret = std::mismatch(a_amts.begin(), a_amts.end(), b_amts.begin());
    return ret.first == a_amts.end() && ret.second == b_amts.end();
}

/** Check if this selection is equal to another one. Equal means same inputs (i.e same value and prevout) */
static bool EqualResult(const SelectionResult& a, const SelectionResult& b)
{
    std::pair<CoinSet::iterator, CoinSet::iterator> ret = std::mismatch(a.GetInputSet().begin(), a.GetInputSet().end(), b.GetInputSet().begin(),
        [](const std::shared_ptr<COutput>& a, const std::shared_ptr<COutput>& b) {
            return a->outpoint == b->outpoint;
        });
    return ret.first == a.GetInputSet().end() && ret.second == b.GetInputSet().end();
}

inline std::vector<OutputGroup>& GroupCoins(const std::vector<COutput>& available_coins, bool subtract_fee_outputs = false)
{
    static std::vector<OutputGroup> static_groups;
    static_groups.clear();
    for (auto& coin : available_coins) {
        static_groups.emplace_back();
        OutputGroup& group = static_groups.back();
        group.Insert(std::make_shared<COutput>(coin), /*ancestors=*/ 0, /*descendants=*/ 0);
        group.m_subtract_fee_outputs = subtract_fee_outputs;
    }
    return static_groups;
}

inline std::vector<OutputGroup>& KnapsackGroupOutputs(const CoinsResult& available_coins, CWallet& wallet, const CoinEligibilityFilter& filter)
{
    FastRandomContext rand{};
    CoinSelectionParams coin_selection_params{
        rand,
        /*change_output_size=*/ 0,
        /*change_spend_size=*/ 0,
        /*min_change_target=*/ CENT,
        /*effective_feerate=*/ CFeeRate(0),
        /*long_term_feerate=*/ CFeeRate(0),
        /*discard_feerate=*/ CFeeRate(0),
        /*tx_noinputs_size=*/ 0,
        /*avoid_partial=*/ false,
    };
    static OutputGroupTypeMap static_groups;
    static_groups = GroupOutputs(wallet, available_coins, coin_selection_params, {{filter}})[filter];
    return static_groups.all_groups.mixed_group;
}

static std::unique_ptr<CWallet> NewWallet(const node::NodeContext& m_node, const std::string& wallet_name = "")
{
    std::unique_ptr<CWallet> wallet = std::make_unique<CWallet>(m_node.chain.get(), wallet_name, CreateMockableWalletDatabase());
    BOOST_CHECK(wallet->LoadWallet() == DBErrors::LOAD_OK);
    LOCK(wallet->cs_wallet);
    wallet->SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    wallet->SetupDescriptorScriptPubKeyMans();
    return wallet;
}

// Branch and bound coin selection tests
BOOST_AUTO_TEST_CASE(bnb_search_test)
{
    FastRandomContext rand{};
    // Setup
    std::vector<COutput> utxo_pool;
    SelectionResult expected_result(CAmount(0), SelectionAlgorithm::BNB);

    ////////////////////
    // Behavior tests //
    ////////////////////

    // Make sure that effective value is working in AttemptSelection when BnB is used
    CoinSelectionParams coin_selection_params_bnb{
        rand,
        /*change_output_size=*/ 31,
        /*change_spend_size=*/ 68,
        /*min_change_target=*/ 0,
        /*effective_feerate=*/ CFeeRate(3000),
        /*long_term_feerate=*/ CFeeRate(1000),
        /*discard_feerate=*/ CFeeRate(1000),
        /*tx_noinputs_size=*/ 0,
        /*avoid_partial=*/ false,
    };
    coin_selection_params_bnb.m_change_fee = coin_selection_params_bnb.m_effective_feerate.GetFee(coin_selection_params_bnb.change_output_size);
    coin_selection_params_bnb.m_cost_of_change = coin_selection_params_bnb.m_effective_feerate.GetFee(coin_selection_params_bnb.change_spend_size) + coin_selection_params_bnb.m_change_fee;
    coin_selection_params_bnb.min_viable_change = coin_selection_params_bnb.m_effective_feerate.GetFee(coin_selection_params_bnb.change_spend_size);

    {
        std::unique_ptr<CWallet> wallet = NewWallet(m_node);

        CoinsResult available_coins;

        add_coin(available_coins, *wallet, 1, coin_selection_params_bnb.m_effective_feerate);
        available_coins.All().at(0).input_bytes = 40; // Make sure that it has a negative effective value. The next check should assert if this somehow got through. Otherwise it will fail
        BOOST_CHECK(!SelectCoinsBnB(GroupCoins(available_coins.All()), 1 * CENT, coin_selection_params_bnb.m_cost_of_change));

        // Test fees subtracted from output:
        available_coins.Clear();
        add_coin(available_coins, *wallet, 1 * CENT, coin_selection_params_bnb.m_effective_feerate);
        available_coins.All().at(0).input_bytes = 40;
        const auto result9 = SelectCoinsBnB(GroupCoins(available_coins.All()), 1 * CENT, coin_selection_params_bnb.m_cost_of_change);
        BOOST_CHECK(result9);
        BOOST_CHECK_EQUAL(result9->GetSelectedValue(), 1 * CENT);
    }

    {
        std::unique_ptr<CWallet> wallet = NewWallet(m_node);

        CoinsResult available_coins;

        coin_selection_params_bnb.m_effective_feerate = CFeeRate(0);
        add_coin(available_coins, *wallet, 5 * CENT, coin_selection_params_bnb.m_effective_feerate, 6 * 24, false, 0, true);
        add_coin(available_coins, *wallet, 3 * CENT, coin_selection_params_bnb.m_effective_feerate, 6 * 24, false, 0, true);
        add_coin(available_coins, *wallet, 2 * CENT, coin_selection_params_bnb.m_effective_feerate, 6 * 24, false, 0, true);
        CCoinControl coin_control;
        coin_control.m_allow_other_inputs = true;
        COutput select_coin = available_coins.All().at(0);
        coin_control.Select(select_coin.outpoint);
        PreSelectedInputs selected_input;
        selected_input.Insert(select_coin, coin_selection_params_bnb.m_subtract_fee_outputs);
        available_coins.Erase({available_coins.coins[OutputType::BECH32].begin()->outpoint});

        LOCK(wallet->cs_wallet);
        const auto result10 = SelectCoins(*wallet, available_coins, selected_input, 10 * CENT, coin_control, coin_selection_params_bnb);
        BOOST_CHECK(result10);
    }
    {
        std::unique_ptr<CWallet> wallet = NewWallet(m_node);
        LOCK(wallet->cs_wallet); // Every 'SelectCoins' call requires it

        CoinsResult available_coins;

        // pre selected coin should be selected even if disadvantageous
        coin_selection_params_bnb.m_effective_feerate = CFeeRate(5000);
        coin_selection_params_bnb.m_long_term_feerate = CFeeRate(3000);

        // Add selectable outputs, increasing their raw amounts by their input fee to make the effective value equal to the raw amount
        CAmount input_fee = coin_selection_params_bnb.m_effective_feerate.GetFee(/*virtual_bytes=*/68); // bech32 input size (default test output type)
        add_coin(available_coins, *wallet, 10 * CENT + input_fee, coin_selection_params_bnb.m_effective_feerate, 6 * 24, false, 0, true);
        add_coin(available_coins, *wallet, 9 * CENT + input_fee, coin_selection_params_bnb.m_effective_feerate, 6 * 24, false, 0, true);
        add_coin(available_coins, *wallet, 1 * CENT + input_fee, coin_selection_params_bnb.m_effective_feerate, 6 * 24, false, 0, true);

        expected_result.Clear();
        add_coin(9 * CENT + input_fee, 2, expected_result);
        add_coin(1 * CENT + input_fee, 2, expected_result);
        CCoinControl coin_control;
        coin_control.m_allow_other_inputs = true;
        COutput select_coin = available_coins.All().at(1); // pre select 9 coin
        coin_control.Select(select_coin.outpoint);
        PreSelectedInputs selected_input;
        selected_input.Insert(select_coin, coin_selection_params_bnb.m_subtract_fee_outputs);
        available_coins.Erase({(++available_coins.coins[OutputType::BECH32].begin())->outpoint});
        const auto result13 = SelectCoins(*wallet, available_coins, selected_input, 10 * CENT, coin_control, coin_selection_params_bnb);
        BOOST_CHECK(EquivalentResult(expected_result, *result13));
    }

    {
        // Test bnb max weight exceeded
        // Inputs set [10, 9, 8, 5, 3, 1], Selection Target = 16 and coin 5 exceeding the max weight.

        std::unique_ptr<CWallet> wallet = NewWallet(m_node);

        CoinsResult available_coins;
        add_coin(available_coins, *wallet, 10 * CENT, coin_selection_params_bnb.m_effective_feerate, 6 * 24, false, 0, true);
        add_coin(available_coins, *wallet, 9 * CENT, coin_selection_params_bnb.m_effective_feerate, 6 * 24, false, 0, true);
        add_coin(available_coins, *wallet, 8 * CENT, coin_selection_params_bnb.m_effective_feerate, 6 * 24, false, 0, true);
        add_coin(available_coins, *wallet, 5 * CENT, coin_selection_params_bnb.m_effective_feerate, 6 * 24, false, 0, true, /*custom_size=*/MAX_STANDARD_TX_WEIGHT);
        add_coin(available_coins, *wallet, 3 * CENT, coin_selection_params_bnb.m_effective_feerate, 6 * 24, false, 0, true);
        add_coin(available_coins, *wallet, 1 * CENT, coin_selection_params_bnb.m_effective_feerate, 6 * 24, false, 0, true);

        CAmount selection_target = 16 * CENT;
        const auto& no_res = SelectCoinsBnB(GroupCoins(available_coins.All(), /*subtract_fee_outputs*/true),
                                            selection_target, /*cost_of_change=*/0, MAX_STANDARD_TX_WEIGHT);
        BOOST_REQUIRE(!no_res);
        BOOST_CHECK(util::ErrorString(no_res).original.find("The inputs size exceeds the maximum weight") != std::string::npos);

        // Now add same coin value with a good size and check that it gets selected
        add_coin(available_coins, *wallet, 5 * CENT, coin_selection_params_bnb.m_effective_feerate, 6 * 24, false, 0, true);
        const auto& res = SelectCoinsBnB(GroupCoins(available_coins.All(), /*subtract_fee_outputs*/true), selection_target, /*cost_of_change=*/0);

        expected_result.Clear();
        add_coin(8 * CENT, 2, expected_result);
        add_coin(5 * CENT, 2, expected_result);
        add_coin(3 * CENT, 2, expected_result);
        BOOST_CHECK(EquivalentResult(expected_result, *res));
    }
}

BOOST_AUTO_TEST_CASE(bnb_sffo_restriction)
{
    // Verify the coin selection process does not produce a BnB solution when SFFO is enabled.
    // This is currently problematic because it could require a change output. And BnB is specialized on changeless solutions.
    std::unique_ptr<CWallet> wallet = NewWallet(m_node);
    WITH_LOCK(wallet->cs_wallet, wallet->SetLastBlockProcessed(300, uint256{})); // set a high block so internal UTXOs are selectable

    FastRandomContext rand{};
    CoinSelectionParams params{
            rand,
            /*change_output_size=*/ 31,  // unused value, p2wpkh output size (wallet default change type)
            /*change_spend_size=*/ 68,   // unused value, p2wpkh input size (high-r signature)
            /*min_change_target=*/ 0,    // dummy, set later
            /*effective_feerate=*/ CFeeRate(3000),
            /*long_term_feerate=*/ CFeeRate(1000),
            /*discard_feerate=*/ CFeeRate(1000),
            /*tx_noinputs_size=*/ 0,
            /*avoid_partial=*/ false,
    };
    params.m_subtract_fee_outputs = true;
    params.m_change_fee = params.m_effective_feerate.GetFee(params.change_output_size);
    params.m_cost_of_change = params.m_discard_feerate.GetFee(params.change_spend_size) + params.m_change_fee;
    params.m_min_change_target = params.m_cost_of_change + 1;
    // Add spendable coin at the BnB selection upper bound
    CoinsResult available_coins;
    add_coin(available_coins, *wallet, COIN + params.m_cost_of_change, /*feerate=*/params.m_effective_feerate, /*nAge=*/6, /*fIsFromMe=*/true, /*nInput=*/0, /*spendable=*/true);
    add_coin(available_coins, *wallet, 0.5 * COIN + params.m_cost_of_change, /*feerate=*/params.m_effective_feerate, /*nAge=*/6, /*fIsFromMe=*/true, /*nInput=*/0, /*spendable=*/true);
    add_coin(available_coins, *wallet, 0.5 * COIN, /*feerate=*/params.m_effective_feerate, /*nAge=*/6, /*fIsFromMe=*/true, /*nInput=*/0, /*spendable=*/true);
    // Knapsack will only find a changeless solution on an exact match to the satoshi, SRD doesn’t look for changeless
    // If BnB were run, it would produce a single input solution with the best waste score
    auto result = WITH_LOCK(wallet->cs_wallet, return SelectCoins(*wallet, available_coins, /*pre_set_inputs=*/{}, COIN, /*coin_control=*/{}, params));
    BOOST_CHECK(result.has_value());
    BOOST_CHECK_NE(result->GetAlgo(), SelectionAlgorithm::BNB);
    BOOST_CHECK(result->GetInputSet().size() == 2);
    // We have only considered BnB, SRD, and Knapsack. Test needs to be reevaluated if new algo is added
    BOOST_CHECK(result->GetAlgo() == SelectionAlgorithm::SRD || result->GetAlgo() == SelectionAlgorithm::KNAPSACK);
}

BOOST_AUTO_TEST_CASE(knapsack_solver_test)
{
    FastRandomContext rand{};
    const auto temp1{[&rand](std::vector<OutputGroup>& g, const CAmount& v, CAmount c) { return KnapsackSolver(g, v, c, rand); }};
    const auto KnapsackSolver{temp1};
    std::unique_ptr<CWallet> wallet = NewWallet(m_node);

    CoinsResult available_coins;

    // test multiple times to allow for differences in the shuffle order
    for (int i = 0; i < RUN_TESTS; i++)
    {
        available_coins.Clear();

        // with an empty wallet we can't even pay one cent
        BOOST_CHECK(!KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_standard), 1 * CENT, CENT));

        add_coin(available_coins, *wallet, 1*CENT, CFeeRate(0), 4);        // add a new 1 cent coin

        // with a new 1 cent coin, we still can't find a mature 1 cent
        BOOST_CHECK(!KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_standard), 1 * CENT, CENT));

        // but we can find a new 1 cent
        const auto result1 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 1 * CENT, CENT);
        BOOST_CHECK(result1);
        BOOST_CHECK_EQUAL(result1->GetSelectedValue(), 1 * CENT);

        add_coin(available_coins, *wallet, 2*CENT);           // add a mature 2 cent coin

        // we can't make 3 cents of mature coins
        BOOST_CHECK(!KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_standard), 3 * CENT, CENT));

        // we can make 3 cents of new coins
        const auto result2 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 3 * CENT, CENT);
        BOOST_CHECK(result2);
        BOOST_CHECK_EQUAL(result2->GetSelectedValue(), 3 * CENT);

        add_coin(available_coins, *wallet, 5*CENT);           // add a mature 5 cent coin,
        add_coin(available_coins, *wallet, 10*CENT, CFeeRate(0), 3, true); // a new 10 cent coin sent from one of our own addresses
        add_coin(available_coins, *wallet, 20*CENT);          // and a mature 20 cent coin

        // now we have new: 1+10=11 (of which 10 was self-sent), and mature: 2+5+20=27.  total = 38

        // we can't make 38 cents only if we disallow new coins:
        BOOST_CHECK(!KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_standard), 38 * CENT, CENT));
        // we can't even make 37 cents if we don't allow new coins even if they're from us
        BOOST_CHECK(!KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_standard_extra), 38 * CENT, CENT));
        // but we can make 37 cents if we accept new coins from ourself
        const auto result3 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_standard), 37 * CENT, CENT);
        BOOST_CHECK(result3);
        BOOST_CHECK_EQUAL(result3->GetSelectedValue(), 37 * CENT);
        // and we can make 38 cents if we accept all new coins
        const auto result4 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 38 * CENT, CENT);
        BOOST_CHECK(result4);
        BOOST_CHECK_EQUAL(result4->GetSelectedValue(), 38 * CENT);

        // try making 34 cents from 1,2,5,10,20 - we can't do it exactly
        const auto result5 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 34 * CENT, CENT);
        BOOST_CHECK(result5);
        BOOST_CHECK_EQUAL(result5->GetSelectedValue(), 35 * CENT);       // but 35 cents is closest
        BOOST_CHECK_EQUAL(result5->GetInputSet().size(), 3U);     // the best should be 20+10+5.  it's incredibly unlikely the 1 or 2 got included (but possible)

        // when we try making 7 cents, the smaller coins (1,2,5) are enough.  We should see just 2+5
        const auto result6 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 7 * CENT, CENT);
        BOOST_CHECK(result6);
        BOOST_CHECK_EQUAL(result6->GetSelectedValue(), 7 * CENT);
        BOOST_CHECK_EQUAL(result6->GetInputSet().size(), 2U);

        // when we try making 8 cents, the smaller coins (1,2,5) are exactly enough.
        const auto result7 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 8 * CENT, CENT);
        BOOST_CHECK(result7);
        BOOST_CHECK(result7->GetSelectedValue() == 8 * CENT);
        BOOST_CHECK_EQUAL(result7->GetInputSet().size(), 3U);

        // when we try making 9 cents, no subset of smaller coins is enough, and we get the next bigger coin (10)
        const auto result8 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 9 * CENT, CENT);
        BOOST_CHECK(result8);
        BOOST_CHECK_EQUAL(result8->GetSelectedValue(), 10 * CENT);
        BOOST_CHECK_EQUAL(result8->GetInputSet().size(), 1U);

        // now clear out the wallet and start again to test choosing between subsets of smaller coins and the next biggest coin
        available_coins.Clear();

        add_coin(available_coins, *wallet,  6*CENT);
        add_coin(available_coins, *wallet,  7*CENT);
        add_coin(available_coins, *wallet,  8*CENT);
        add_coin(available_coins, *wallet, 20*CENT);
        add_coin(available_coins, *wallet, 30*CENT); // now we have 6+7+8+20+30 = 71 cents total

        // check that we have 71 and not 72
        const auto result9 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 71 * CENT, CENT);
        BOOST_CHECK(result9);
        BOOST_CHECK(!KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 72 * CENT, CENT));

        // now try making 16 cents.  the best smaller coins can do is 6+7+8 = 21; not as good at the next biggest coin, 20
        const auto result10 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 16 * CENT, CENT);
        BOOST_CHECK(result10);
        BOOST_CHECK_EQUAL(result10->GetSelectedValue(), 20 * CENT); // we should get 20 in one coin
        BOOST_CHECK_EQUAL(result10->GetInputSet().size(), 1U);

        add_coin(available_coins, *wallet,  5*CENT); // now we have 5+6+7+8+20+30 = 75 cents total

        // now if we try making 16 cents again, the smaller coins can make 5+6+7 = 18 cents, better than the next biggest coin, 20
        const auto result11 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 16 * CENT, CENT);
        BOOST_CHECK(result11);
        BOOST_CHECK_EQUAL(result11->GetSelectedValue(), 18 * CENT); // we should get 18 in 3 coins
        BOOST_CHECK_EQUAL(result11->GetInputSet().size(), 3U);

        add_coin(available_coins, *wallet,  18*CENT); // now we have 5+6+7+8+18+20+30

        // and now if we try making 16 cents again, the smaller coins can make 5+6+7 = 18 cents, the same as the next biggest coin, 18
        const auto result12 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 16 * CENT, CENT);
        BOOST_CHECK(result12);
        BOOST_CHECK_EQUAL(result12->GetSelectedValue(), 18 * CENT);  // we should get 18 in 1 coin
        BOOST_CHECK_EQUAL(result12->GetInputSet().size(), 1U); // because in the event of a tie, the biggest coin wins

        // now try making 11 cents.  we should get 5+6
        const auto result13 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 11 * CENT, CENT);
        BOOST_CHECK(result13);
        BOOST_CHECK_EQUAL(result13->GetSelectedValue(), 11 * CENT);
        BOOST_CHECK_EQUAL(result13->GetInputSet().size(), 2U);

        // check that the smallest bigger coin is used
        add_coin(available_coins, *wallet,  1*COIN);
        add_coin(available_coins, *wallet,  2*COIN);
        add_coin(available_coins, *wallet,  3*COIN);
        add_coin(available_coins, *wallet,  4*COIN); // now we have 5+6+7+8+18+20+30+100+200+300+400 = 1094 cents
        const auto result14 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 95 * CENT, CENT);
        BOOST_CHECK(result14);
        BOOST_CHECK_EQUAL(result14->GetSelectedValue(), 1 * COIN);  // we should get 1 BTC in 1 coin
        BOOST_CHECK_EQUAL(result14->GetInputSet().size(), 1U);

        const auto result15 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 195 * CENT, CENT);
        BOOST_CHECK(result15);
        BOOST_CHECK_EQUAL(result15->GetSelectedValue(), 2 * COIN);  // we should get 2 BTC in 1 coin
        BOOST_CHECK_EQUAL(result15->GetInputSet().size(), 1U);

        // empty the wallet and start again, now with fractions of a cent, to test small change avoidance

        available_coins.Clear();
        add_coin(available_coins, *wallet, CENT * 1 / 10);
        add_coin(available_coins, *wallet, CENT * 2 / 10);
        add_coin(available_coins, *wallet, CENT * 3 / 10);
        add_coin(available_coins, *wallet, CENT * 4 / 10);
        add_coin(available_coins, *wallet, CENT * 5 / 10);

        // try making 1 * CENT from the 1.5 * CENT
        // we'll get change smaller than CENT whatever happens, so can expect CENT exactly
        const auto result16 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), CENT, CENT);
        BOOST_CHECK(result16);
        BOOST_CHECK_EQUAL(result16->GetSelectedValue(), CENT);

        // but if we add a bigger coin, small change is avoided
        add_coin(available_coins, *wallet, 1111*CENT);

        // try making 1 from 0.1 + 0.2 + 0.3 + 0.4 + 0.5 + 1111 = 1112.5
        const auto result17 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 1 * CENT, CENT);
        BOOST_CHECK(result17);
        BOOST_CHECK_EQUAL(result17->GetSelectedValue(), 1 * CENT); // we should get the exact amount

        // if we add more small coins:
        add_coin(available_coins, *wallet, CENT * 6 / 10);
        add_coin(available_coins, *wallet, CENT * 7 / 10);

        // and try again to make 1.0 * CENT
        const auto result18 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 1 * CENT, CENT);
        BOOST_CHECK(result18);
        BOOST_CHECK_EQUAL(result18->GetSelectedValue(), 1 * CENT); // we should get the exact amount

        // run the 'mtgox' test (see https://blockexplorer.com/tx/29a3efd3ef04f9153d47a990bd7b048a4b2d213daaa5fb8ed670fb85f13bdbcf)
        // they tried to consolidate 10 50k coins into one 500k coin, and ended up with 50k in change
        available_coins.Clear();
        for (int j = 0; j < 20; j++)
            add_coin(available_coins, *wallet, 50000 * COIN);

        const auto result19 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 500000 * COIN, CENT);
        BOOST_CHECK(result19);
        BOOST_CHECK_EQUAL(result19->GetSelectedValue(), 500000 * COIN); // we should get the exact amount
        BOOST_CHECK_EQUAL(result19->GetInputSet().size(), 10U); // in ten coins

        // if there's not enough in the smaller coins to make at least 1 * CENT change (0.5+0.6+0.7 < 1.0+1.0),
        // we need to try finding an exact subset anyway

        // sometimes it will fail, and so we use the next biggest coin:
        available_coins.Clear();
        add_coin(available_coins, *wallet, CENT * 5 / 10);
        add_coin(available_coins, *wallet, CENT * 6 / 10);
        add_coin(available_coins, *wallet, CENT * 7 / 10);
        add_coin(available_coins, *wallet, 1111 * CENT);
        const auto result20 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 1 * CENT, CENT);
        BOOST_CHECK(result20);
        BOOST_CHECK_EQUAL(result20->GetSelectedValue(), 1111 * CENT); // we get the bigger coin
        BOOST_CHECK_EQUAL(result20->GetInputSet().size(), 1U);

        // but sometimes it's possible, and we use an exact subset (0.4 + 0.6 = 1.0)
        available_coins.Clear();
        add_coin(available_coins, *wallet, CENT * 4 / 10);
        add_coin(available_coins, *wallet, CENT * 6 / 10);
        add_coin(available_coins, *wallet, CENT * 8 / 10);
        add_coin(available_coins, *wallet, 1111 * CENT);
        const auto result21 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), CENT, CENT);
        BOOST_CHECK(result21);
        BOOST_CHECK_EQUAL(result21->GetSelectedValue(), CENT);   // we should get the exact amount
        BOOST_CHECK_EQUAL(result21->GetInputSet().size(), 2U); // in two coins 0.4+0.6

        // test avoiding small change
        available_coins.Clear();
        add_coin(available_coins, *wallet, CENT * 5 / 100);
        add_coin(available_coins, *wallet, CENT * 1);
        add_coin(available_coins, *wallet, CENT * 100);

        // trying to make 100.01 from these three coins
        const auto result22 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), CENT * 10001 / 100, CENT);
        BOOST_CHECK(result22);
        BOOST_CHECK_EQUAL(result22->GetSelectedValue(), CENT * 10105 / 100); // we should get all coins
        BOOST_CHECK_EQUAL(result22->GetInputSet().size(), 3U);

        // but if we try to make 99.9, we should take the bigger of the two small coins to avoid small change
        const auto result23 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), CENT * 9990 / 100, CENT);
        BOOST_CHECK(result23);
        BOOST_CHECK_EQUAL(result23->GetSelectedValue(), 101 * CENT);
        BOOST_CHECK_EQUAL(result23->GetInputSet().size(), 2U);
    }

    // test with many inputs
    for (CAmount amt=1500; amt < COIN; amt*=10) {
        available_coins.Clear();
        // Create 676 inputs (=  (old MAX_STANDARD_TX_SIZE == 100000)  / 148 bytes per input)
        for (uint16_t j = 0; j < 676; j++)
            add_coin(available_coins, *wallet, amt);

        // We only create the wallet once to save time, but we still run the coin selection RUN_TESTS times.
        for (int i = 0; i < RUN_TESTS; i++) {
            const auto result24 = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_confirmed), 2000, CENT);
            BOOST_CHECK(result24);

            if (amt - 2000 < CENT) {
                // needs more than one input:
                uint16_t returnSize = std::ceil((2000.0 + CENT)/amt);
                CAmount returnValue = amt * returnSize;
                BOOST_CHECK_EQUAL(result24->GetSelectedValue(), returnValue);
                BOOST_CHECK_EQUAL(result24->GetInputSet().size(), returnSize);
            } else {
                // one input is sufficient:
                BOOST_CHECK_EQUAL(result24->GetSelectedValue(), amt);
                BOOST_CHECK_EQUAL(result24->GetInputSet().size(), 1U);
            }
        }
    }

    // test randomness
    {
        available_coins.Clear();
        for (int i2 = 0; i2 < 100; i2++)
            add_coin(available_coins, *wallet, COIN);

        // Again, we only create the wallet once to save time, but we still run the coin selection RUN_TESTS times.
        for (int i = 0; i < RUN_TESTS; i++) {
            // picking 50 from 100 coins doesn't depend on the shuffle,
            // but does depend on randomness in the stochastic approximation code
            const auto result25 = KnapsackSolver(GroupCoins(available_coins.All()), 50 * COIN, CENT);
            BOOST_CHECK(result25);
            const auto result26 = KnapsackSolver(GroupCoins(available_coins.All()), 50 * COIN, CENT);
            BOOST_CHECK(result26);
            BOOST_CHECK(!EqualResult(*result25, *result26));

            int fails = 0;
            for (int j = 0; j < RANDOM_REPEATS; j++)
            {
                // Test that the KnapsackSolver selects randomly from equivalent coins (same value and same input size).
                // When choosing 1 from 100 identical coins, 1% of the time, this test will choose the same coin twice
                // which will cause it to fail.
                // To avoid that issue, run the test RANDOM_REPEATS times and only complain if all of them fail
                const auto result27 = KnapsackSolver(GroupCoins(available_coins.All()), COIN, CENT);
                BOOST_CHECK(result27);
                const auto result28 = KnapsackSolver(GroupCoins(available_coins.All()), COIN, CENT);
                BOOST_CHECK(result28);
                if (EqualResult(*result27, *result28))
                    fails++;
            }
            BOOST_CHECK_NE(fails, RANDOM_REPEATS);
        }

        // add 75 cents in small change.  not enough to make 90 cents,
        // then try making 90 cents.  there are multiple competing "smallest bigger" coins,
        // one of which should be picked at random
        add_coin(available_coins, *wallet, 5 * CENT);
        add_coin(available_coins, *wallet, 10 * CENT);
        add_coin(available_coins, *wallet, 15 * CENT);
        add_coin(available_coins, *wallet, 20 * CENT);
        add_coin(available_coins, *wallet, 25 * CENT);

        for (int i = 0; i < RUN_TESTS; i++) {
            int fails = 0;
            for (int j = 0; j < RANDOM_REPEATS; j++)
            {
                const auto result29 = KnapsackSolver(GroupCoins(available_coins.All()), 90 * CENT, CENT);
                BOOST_CHECK(result29);
                const auto result30 = KnapsackSolver(GroupCoins(available_coins.All()), 90 * CENT, CENT);
                BOOST_CHECK(result30);
                if (EqualResult(*result29, *result30))
                    fails++;
            }
            BOOST_CHECK_NE(fails, RANDOM_REPEATS);
        }
    }
}

BOOST_AUTO_TEST_CASE(ApproximateBestSubset)
{
    FastRandomContext rand{};
    std::unique_ptr<CWallet> wallet = NewWallet(m_node);

    CoinsResult available_coins;

    // Test vValue sort order
    for (int i = 0; i < 1000; i++)
        add_coin(available_coins, *wallet, 1000 * COIN);
    add_coin(available_coins, *wallet, 3 * COIN);

    const auto result = KnapsackSolver(KnapsackGroupOutputs(available_coins, *wallet, filter_standard), 1003 * COIN, CENT, rand);
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(result->GetSelectedValue(), 1003 * COIN);
    BOOST_CHECK_EQUAL(result->GetInputSet().size(), 2U);
}

// Tests that with the ideal conditions, the coin selector will always be able to find a solution that can pay the target value
BOOST_AUTO_TEST_CASE(SelectCoins_test)
{
    std::unique_ptr<CWallet> wallet = NewWallet(m_node);
    LOCK(wallet->cs_wallet); // Every 'SelectCoins' call requires it

    // Random generator stuff
    std::default_random_engine generator;
    std::exponential_distribution<double> distribution (100);
    FastRandomContext rand;

    // Run this test 100 times
    for (int i = 0; i < 100; ++i)
    {
        CoinsResult available_coins;
        CAmount balance{0};

        // Make a wallet with 1000 exponentially distributed random inputs
        for (int j = 0; j < 1000; ++j)
        {
            CAmount val = distribution(generator)*10000000;
            add_coin(available_coins, *wallet, val);
            balance += val;
        }

        // Generate a random fee rate in the range of 100 - 400
        CFeeRate rate(rand.randrange(300) + 100);

        // Generate a random target value between 1000 and wallet balance
        CAmount target = rand.randrange(balance - 1000) + 1000;

        // Perform selection
        CoinSelectionParams cs_params{
            rand,
            /*change_output_size=*/ 34,
            /*change_spend_size=*/ 148,
            /*min_change_target=*/ CENT,
            /*effective_feerate=*/ CFeeRate(0),
            /*long_term_feerate=*/ CFeeRate(0),
            /*discard_feerate=*/ CFeeRate(0),
            /*tx_noinputs_size=*/ 0,
            /*avoid_partial=*/ false,
        };
        cs_params.m_cost_of_change = 1;
        cs_params.min_viable_change = 1;
        CCoinControl cc;
        const auto result = SelectCoins(*wallet, available_coins, /*pre_set_inputs=*/{}, target, cc, cs_params);
        BOOST_CHECK(result);
        BOOST_CHECK_GE(result->GetSelectedValue(), target);
    }
}

BOOST_AUTO_TEST_CASE(waste_test)
{
    const CAmount fee{100};
    const CAmount min_viable_change{300};
    const CAmount change_cost{125};
    const CAmount change_fee{30};
    const CAmount fee_diff{40};
    const CAmount in_amt{3 * COIN};
    const CAmount target{2 * COIN};
    const CAmount excess{80};
    const CAmount exact_target{in_amt - fee * 2}; // Maximum spendable amount after fees: no change, no excess

    // In the following, we test that the waste is calculated correctly in various scenarios.
    // Usually, RecalculateWaste would compute change_fee and change_cost on basis of the
    // change output type, current feerate, and discard_feerate, but we use fixed values
    // across this test to make the test easier to understand.
    {
        // Waste with change is the change cost and difference between fee and long term fee
        SelectionResult selection1{target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection1, /*fee=*/fee, /*long_term_fee=*/fee - fee_diff);
        add_coin(2 * COIN, 2, selection1, fee, fee - fee_diff);
        selection1.RecalculateWaste(min_viable_change, change_cost, change_fee);
        BOOST_CHECK_EQUAL(fee_diff * 2 + change_cost, selection1.GetWaste());

        // Waste will be greater when fee is greater, but long term fee is the same
        SelectionResult selection2{target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection2, fee * 2, fee - fee_diff);
        add_coin(2 * COIN, 2, selection2, fee * 2, fee - fee_diff);
        selection2.RecalculateWaste(min_viable_change, change_cost, change_fee);
        BOOST_CHECK_GT(selection2.GetWaste(), selection1.GetWaste());

        // Waste with change is the change cost and difference between fee and long term fee
        // With long term fee greater than fee, waste should be less than when long term fee is less than fee
        SelectionResult selection3{target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection3, fee, fee + fee_diff);
        add_coin(2 * COIN, 2, selection3, fee, fee + fee_diff);
        selection3.RecalculateWaste(min_viable_change, change_cost, change_fee);
        BOOST_CHECK_EQUAL(fee_diff * -2 + change_cost, selection3.GetWaste());
        BOOST_CHECK_LT(selection3.GetWaste(), selection1.GetWaste());
    }

    {
        // Waste without change is the excess and difference between fee and long term fee
        SelectionResult selection_nochange1{exact_target - excess, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection_nochange1, fee, fee - fee_diff);
        add_coin(2 * COIN, 2, selection_nochange1, fee, fee - fee_diff);
        selection_nochange1.RecalculateWaste(min_viable_change, change_cost, change_fee);
        BOOST_CHECK_EQUAL(fee_diff * 2 + excess, selection_nochange1.GetWaste());

        // Waste without change is the excess and difference between fee and long term fee
        // With long term fee greater than fee, waste should be less than when long term fee is less than fee
        SelectionResult selection_nochange2{exact_target - excess, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection_nochange2, fee, fee + fee_diff);
        add_coin(2 * COIN, 2, selection_nochange2, fee, fee + fee_diff);
        selection_nochange2.RecalculateWaste(min_viable_change, change_cost, change_fee);
        BOOST_CHECK_EQUAL(fee_diff * -2 + excess, selection_nochange2.GetWaste());
        BOOST_CHECK_LT(selection_nochange2.GetWaste(), selection_nochange1.GetWaste());
    }

    {
        // Waste with change and fee == long term fee is just cost of change
        SelectionResult selection{target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection, fee, fee);
        add_coin(2 * COIN, 2, selection, fee, fee);
        selection.RecalculateWaste(min_viable_change, change_cost, change_fee);
        BOOST_CHECK_EQUAL(change_cost, selection.GetWaste());
    }

    {
        // Waste without change and fee == long term fee is just the excess
        SelectionResult selection{exact_target - excess, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection, fee, fee);
        add_coin(2 * COIN, 2, selection, fee, fee);
        selection.RecalculateWaste(min_viable_change, change_cost, change_fee);
        BOOST_CHECK_EQUAL(excess, selection.GetWaste());
    }

    {
        // Waste is 0 when fee == long_term_fee, no change, and no excess
        SelectionResult selection{exact_target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection, fee, fee);
        add_coin(2 * COIN, 2, selection, fee, fee);
        selection.RecalculateWaste(min_viable_change, change_cost , change_fee);
        BOOST_CHECK_EQUAL(0, selection.GetWaste());
    }

    {
        // Waste is 0 when (fee - long_term_fee) == (-cost_of_change), and no excess
        SelectionResult selection{target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection, fee, fee + fee_diff);
        add_coin(2 * COIN, 2, selection, fee, fee + fee_diff);
        selection.RecalculateWaste(min_viable_change, /*change_cost=*/fee_diff * 2, change_fee);
        BOOST_CHECK_EQUAL(0, selection.GetWaste());
    }

    {
        // Waste is 0 when (fee - long_term_fee) == (-excess), no change cost
        const CAmount new_target{exact_target - /*excess=*/fee_diff * 2};
        SelectionResult selection{new_target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection, fee, fee + fee_diff);
        add_coin(2 * COIN, 2, selection, fee, fee + fee_diff);
        selection.RecalculateWaste(min_viable_change, change_cost, change_fee);
        BOOST_CHECK_EQUAL(0, selection.GetWaste());
    }

    {
        // Negative waste when the long term fee is greater than the current fee and the selected value == target
        SelectionResult selection{exact_target, SelectionAlgorithm::MANUAL};
        const CAmount target_waste1{-2 * fee_diff}; // = (2 * fee) - (2 * (fee + fee_diff))
        add_coin(1 * COIN, 1, selection, fee, fee + fee_diff);
        add_coin(2 * COIN, 2, selection, fee, fee + fee_diff);
        selection.RecalculateWaste(min_viable_change, change_cost, change_fee);
        BOOST_CHECK_EQUAL(target_waste1, selection.GetWaste());
    }

    {
        // Negative waste when the long term fee is greater than the current fee and change_cost < - (inputs * (fee - long_term_fee))
        SelectionResult selection{target, SelectionAlgorithm::MANUAL};
        const CAmount large_fee_diff{90};
        const CAmount target_waste2{-2 * large_fee_diff + change_cost};
        // = (2 * fee) - (2 * (fee + large_fee_diff)) + change_cost
        // = (2 * 100) - (2 * (100 + 90)) + 125
        // = 200 - 380 + 125 = -55
        assert(target_waste2 == -55);
        add_coin(1 * COIN, 1, selection, fee, fee + large_fee_diff);
        add_coin(2 * COIN, 2, selection, fee, fee + large_fee_diff);
        selection.RecalculateWaste(min_viable_change, change_cost, change_fee);
        BOOST_CHECK_EQUAL(target_waste2, selection.GetWaste());
    }
}


BOOST_AUTO_TEST_CASE(bump_fee_test)
{
    const CAmount fee{100};
    const CAmount min_viable_change{200};
    const CAmount change_cost{125};
    const CAmount change_fee{35};
    const CAmount fee_diff{40};
    const CAmount target{2 * COIN};

    {
        SelectionResult selection{target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection, /*fee=*/fee, /*long_term_fee=*/fee + fee_diff);
        add_coin(2 * COIN, 2, selection, fee, fee + fee_diff);
        const std::vector<std::shared_ptr<COutput>> inputs = selection.GetShuffledInputVector();

        for (size_t i = 0; i < inputs.size(); ++i) {
            inputs[i]->ApplyBumpFee(20*(i+1));
        }

        selection.RecalculateWaste(min_viable_change, change_cost, change_fee);
        CAmount expected_waste = fee_diff * -2 + change_cost + /*bump_fees=*/60;
        BOOST_CHECK_EQUAL(expected_waste, selection.GetWaste());

        selection.SetBumpFeeDiscount(30);
        selection.RecalculateWaste(min_viable_change, change_cost, change_fee);
        expected_waste = fee_diff * -2 + change_cost + /*bump_fees=*/60 - /*group_discount=*/30;
        BOOST_CHECK_EQUAL(expected_waste, selection.GetWaste());
    }

    {
        // Test with changeless transaction
        //
        // Bump fees and excess both contribute fully to the waste score,
        // therefore, a bump fee group discount will not change the waste
        // score as long as we do not create change in both instances.
        CAmount changeless_target = 3 * COIN - 2 * fee - 100;
        SelectionResult selection{changeless_target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection, /*fee=*/fee, /*long_term_fee=*/fee + fee_diff);
        add_coin(2 * COIN, 2, selection, fee, fee + fee_diff);
        const std::vector<std::shared_ptr<COutput>> inputs = selection.GetShuffledInputVector();

        for (size_t i = 0; i < inputs.size(); ++i) {
            inputs[i]->ApplyBumpFee(20*(i+1));
        }

        selection.RecalculateWaste(min_viable_change, change_cost, change_fee);
        CAmount expected_waste = fee_diff * -2 + /*bump_fees=*/60 + /*excess = 100 - bump_fees*/40;
        BOOST_CHECK_EQUAL(expected_waste, selection.GetWaste());

        selection.SetBumpFeeDiscount(30);
        selection.RecalculateWaste(min_viable_change, change_cost, change_fee);
        expected_waste = fee_diff * -2 + /*bump_fees=*/60 - /*group_discount=*/30 + /*excess = 100 - bump_fees + group_discount*/70;
        BOOST_CHECK_EQUAL(expected_waste, selection.GetWaste());
    }
}

BOOST_AUTO_TEST_CASE(effective_value_test)
{
    const int input_bytes = 148;
    const CFeeRate feerate(1000);
    const CAmount nValue = 10000;
    const int nInput = 0;

    CMutableTransaction tx;
    tx.vout.resize(1);
    tx.vout[nInput].nValue = nValue;

    // standard case, pass feerate in constructor
    COutput output1(COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 1, input_bytes, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, feerate);
    const CAmount expected_ev1 = 9852; // 10000 - 148
    BOOST_CHECK_EQUAL(output1.GetEffectiveValue(), expected_ev1);

    // input bytes unknown (input_bytes = -1), pass feerate in constructor
    COutput output2(COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 1, /*input_bytes=*/ -1, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, feerate);
    BOOST_CHECK_EQUAL(output2.GetEffectiveValue(), nValue); // The effective value should be equal to the absolute value if input_bytes is -1

    // negative effective value, pass feerate in constructor
    COutput output3(COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 1, input_bytes, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, CFeeRate(100000));
    const CAmount expected_ev3 = -4800; // 10000 - 14800
    BOOST_CHECK_EQUAL(output3.GetEffectiveValue(), expected_ev3);

    // standard case, pass fees in constructor
    const CAmount fees = 148;
    COutput output4(COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 1, input_bytes, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, fees);
    BOOST_CHECK_EQUAL(output4.GetEffectiveValue(), expected_ev1);

    // input bytes unknown (input_bytes = -1), pass fees in constructor
    COutput output5(COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 1, /*input_bytes=*/ -1, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, /*fees=*/ 0);
    BOOST_CHECK_EQUAL(output5.GetEffectiveValue(), nValue); // The effective value should be equal to the absolute value if input_bytes is -1
}

static util::Result<SelectionResult> CoinGrinder(const CAmount& target,
                                                    const CoinSelectionParams& cs_params,
                                                    const node::NodeContext& m_node,
                                                    int max_selection_weight,
                                                    std::function<CoinsResult(CWallet&)> coin_setup)
{
    std::unique_ptr<CWallet> wallet = NewWallet(m_node);
    CoinEligibilityFilter filter(0, 0, 0); // accept all coins without ancestors
    Groups group = GroupOutputs(*wallet, coin_setup(*wallet), cs_params, {{filter}})[filter].all_groups;
    return CoinGrinder(group.positive_group, target, cs_params.m_min_change_target, max_selection_weight);
}

BOOST_AUTO_TEST_CASE(coin_grinder_tests)
{
    // Test Coin Grinder:
    // 1) Insufficient funds, select all provided coins and fail.
    // 2) Exceeded max weight, coin selection always surpasses the max allowed weight.
    // 3) Select coins without surpassing the max weight (some coins surpasses the max allowed weight, some others not)
    // 4) Test that two less valuable UTXOs with a combined lower weight are preferred over a more valuable heavier UTXO
    // 5) Test finding a solution in a UTXO pool with mixed weights
    // 6) Test that the lightest solution among many clones is found
    // 7) Test that lots of tiny UTXOs can be skipped if they are too heavy while there are enough funds in lookahead

    FastRandomContext rand;
    CoinSelectionParams dummy_params{ // Only used to provide the 'avoid_partial' flag.
            rand,
            /*change_output_size=*/34,
            /*change_spend_size=*/68,
            /*min_change_target=*/CENT,
            /*effective_feerate=*/CFeeRate(5000),
            /*long_term_feerate=*/CFeeRate(2000),
            /*discard_feerate=*/CFeeRate(1000),
            /*tx_noinputs_size=*/10 + 34, // static header size + output size
            /*avoid_partial=*/false,
    };

    {
        // #########################################################
        // 1) Insufficient funds, select all provided coins and fail
        // #########################################################
        CAmount target = 49.5L * COIN;
        int max_selection_weight = 10'000; // high enough to not fail for this reason.
        const auto& res = CoinGrinder(target, dummy_params, m_node, max_selection_weight, [&](CWallet& wallet) {
            CoinsResult available_coins;
            for (int j = 0; j < 10; ++j) {
                add_coin(available_coins, wallet, CAmount(1 * COIN));
                add_coin(available_coins, wallet, CAmount(2 * COIN));
            }
            return available_coins;
        });
        BOOST_CHECK(!res);
        BOOST_CHECK(util::ErrorString(res).empty()); // empty means "insufficient funds"
    }

    {
        // ###########################
        // 2) Test max weight exceeded
        // ###########################
        CAmount target = 29.5L * COIN;
        int max_selection_weight = 3000;
        const auto& res = CoinGrinder(target, dummy_params, m_node, max_selection_weight, [&](CWallet& wallet) {
            CoinsResult available_coins;
            for (int j = 0; j < 10; ++j) {
                add_coin(available_coins, wallet, CAmount(1 * COIN), CFeeRate(5000), 144, false, 0, true);
                add_coin(available_coins, wallet, CAmount(2 * COIN), CFeeRate(5000), 144, false, 0, true);
            }
            return available_coins;
        });
        BOOST_CHECK(!res);
        BOOST_CHECK(util::ErrorString(res).original.find("The inputs size exceeds the maximum weight") != std::string::npos);
    }

    {
        // ###############################################################################################################
        // 3) Test that the lowest-weight solution is found when some combinations would exceed the allowed weight
        // ################################################################################################################
        CAmount target = 25.33L * COIN;
        int max_selection_weight = 10'000; // WU
        const auto& res = CoinGrinder(target, dummy_params, m_node, max_selection_weight, [&](CWallet& wallet) {
            CoinsResult available_coins;
            for (int j = 0; j < 60; ++j) { // 60 UTXO --> 19,8 BTC total --> 60 × 272 WU = 16320 WU
                add_coin(available_coins, wallet, CAmount(0.33 * COIN), CFeeRate(5000), 144, false, 0, true);
            }
            for (int i = 0; i < 10; i++) { // 10 UTXO --> 20 BTC total --> 10 × 272 WU = 2720 WU
                add_coin(available_coins, wallet, CAmount(2 * COIN), CFeeRate(5000), 144, false, 0, true);
            }
            return available_coins;
        });
        SelectionResult expected_result(CAmount(0), SelectionAlgorithm::CG);
        for (int i = 0; i < 10; ++i) {
            add_coin(2 * COIN, i, expected_result);
        }
        for (int j = 0; j < 17; ++j) {
            add_coin(0.33 * COIN, j + 10, expected_result);
        }
        BOOST_CHECK(EquivalentResult(expected_result, *res));
        // Demonstrate how following improvements reduce iteration count and catch any regressions in the future.
        size_t expected_attempts = 37;
        BOOST_CHECK_MESSAGE(res->GetSelectionsEvaluated() == expected_attempts, strprintf("Expected %i attempts, but got %i", expected_attempts, res->GetSelectionsEvaluated()));
    }

    {
        // #################################################################################################################
        // 4) Test that two less valuable UTXOs with a combined lower weight are preferred over a more valuable heavier UTXO
        // #################################################################################################################
        CAmount target =  1.9L * COIN;
        int max_selection_weight = 400'000; // WU
        const auto& res = CoinGrinder(target, dummy_params, m_node, max_selection_weight, [&](CWallet& wallet) {
            CoinsResult available_coins;
            add_coin(available_coins, wallet, CAmount(2 * COIN), CFeeRate(5000), 144, false, 0, true, 148);
            add_coin(available_coins, wallet, CAmount(1 * COIN), CFeeRate(5000), 144, false, 0, true, 68);
            add_coin(available_coins, wallet, CAmount(1 * COIN), CFeeRate(5000), 144, false, 0, true, 68);
            return available_coins;
        });
        SelectionResult expected_result(CAmount(0), SelectionAlgorithm::CG);
        add_coin(1 * COIN, 1, expected_result);
        add_coin(1 * COIN, 2, expected_result);
        BOOST_CHECK(EquivalentResult(expected_result, *res));
        // Demonstrate how following improvements reduce iteration count and catch any regressions in the future.
        size_t expected_attempts = 3;
        BOOST_CHECK_MESSAGE(res->GetSelectionsEvaluated() == expected_attempts, strprintf("Expected %i attempts, but got %i", expected_attempts, res->GetSelectionsEvaluated()));
    }

    {
        // ###############################################################################################################
        // 5) Test finding a solution in a UTXO pool with mixed weights
        // ################################################################################################################
        CAmount target = 30L * COIN;
        int max_selection_weight = 400'000; // WU
        const auto& res = CoinGrinder(target, dummy_params, m_node, max_selection_weight, [&](CWallet& wallet) {
            CoinsResult available_coins;
            for (int j = 0; j < 5; ++j) {
                // Add heavy coins {3, 6, 9, 12, 15}
                add_coin(available_coins, wallet, CAmount((3 + 3 * j) * COIN), CFeeRate(5000), 144, false, 0, true, 350);
                // Add medium coins {2, 5, 8, 11, 14}
                add_coin(available_coins, wallet, CAmount((2 + 3 * j) * COIN), CFeeRate(5000), 144, false, 0, true, 250);
                // Add light coins {1, 4, 7, 10, 13}
                add_coin(available_coins, wallet, CAmount((1 + 3 * j) * COIN), CFeeRate(5000), 144, false, 0, true, 150);
            }
            return available_coins;
        });
        BOOST_CHECK(res);
        SelectionResult expected_result(CAmount(0), SelectionAlgorithm::CG);
        add_coin(14 * COIN, 1, expected_result);
        add_coin(13 * COIN, 2, expected_result);
        add_coin(4 * COIN, 3, expected_result);
        BOOST_CHECK(EquivalentResult(expected_result, *res));
        // Demonstrate how following improvements reduce iteration count and catch any regressions in the future.
        size_t expected_attempts = 92;
        BOOST_CHECK_MESSAGE(res->GetSelectionsEvaluated() == expected_attempts, strprintf("Expected %i attempts, but got %i", expected_attempts, res->GetSelectionsEvaluated()));
    }

    {
        // #################################################################################################################
        // 6) Test that the lightest solution among many clones is found
        // #################################################################################################################
        CAmount target =  9.9L * COIN;
        int max_selection_weight = 400'000; // WU
        const auto& res = CoinGrinder(target, dummy_params, m_node, max_selection_weight, [&](CWallet& wallet) {
            CoinsResult available_coins;
            // Expected Result: 4 + 3 + 2 + 1 = 10 BTC at 400 vB
            add_coin(available_coins, wallet, CAmount(4 * COIN), CFeeRate(5000), 144, false, 0, true, 100);
            add_coin(available_coins, wallet, CAmount(3 * COIN), CFeeRate(5000), 144, false, 0, true, 100);
            add_coin(available_coins, wallet, CAmount(2 * COIN), CFeeRate(5000), 144, false, 0, true, 100);
            add_coin(available_coins, wallet, CAmount(1 * COIN), CFeeRate(5000), 144, false, 0, true, 100);
            // Distracting clones:
            for (int j = 0; j < 100; ++j) {
                add_coin(available_coins, wallet, CAmount(8 * COIN), CFeeRate(5000), 144, false, 0, true, 1000);
            }
            for (int j = 0; j < 100; ++j) {
                add_coin(available_coins, wallet, CAmount(7 * COIN), CFeeRate(5000), 144, false, 0, true, 800);
            }
            for (int j = 0; j < 100; ++j) {
                add_coin(available_coins, wallet, CAmount(6 * COIN), CFeeRate(5000), 144, false, 0, true, 600);
            }
            for (int j = 0; j < 100; ++j) {
                add_coin(available_coins, wallet, CAmount(5 * COIN), CFeeRate(5000), 144, false, 0, true, 400);
            }
            return available_coins;
        });
        SelectionResult expected_result(CAmount(0), SelectionAlgorithm::CG);
        add_coin(4 * COIN, 0, expected_result);
        add_coin(3 * COIN, 0, expected_result);
        add_coin(2 * COIN, 0, expected_result);
        add_coin(1 * COIN, 0, expected_result);
        BOOST_CHECK(EquivalentResult(expected_result, *res));
        // Demonstrate how following improvements reduce iteration count and catch any regressions in the future.
        size_t expected_attempts = 38;
        BOOST_CHECK_MESSAGE(res->GetSelectionsEvaluated() == expected_attempts, strprintf("Expected %i attempts, but got %i", expected_attempts, res->GetSelectionsEvaluated()));
    }

    {
        // #################################################################################################################
        // 7) Test that lots of tiny UTXOs can be skipped if they are too heavy while there are enough funds in lookahead
        // #################################################################################################################
        CAmount target =  1.9L * COIN;
        int max_selection_weight = 40000; // WU
        const auto& res = CoinGrinder(target, dummy_params, m_node, max_selection_weight, [&](CWallet& wallet) {
            CoinsResult available_coins;
            add_coin(available_coins, wallet, CAmount(1.8 * COIN), CFeeRate(5000), 144, false, 0, true, 2500);
            add_coin(available_coins, wallet, CAmount(1 * COIN), CFeeRate(5000), 144, false, 0, true, 1000);
            add_coin(available_coins, wallet, CAmount(1 * COIN), CFeeRate(5000), 144, false, 0, true, 1000);
            for (int j = 0; j < 100; ++j) {
                // make a 100 unique coins only differing by one sat
                add_coin(available_coins, wallet, CAmount(0.01 * COIN + j), CFeeRate(5000), 144, false, 0, true, 110);
            }
            return available_coins;
        });
        SelectionResult expected_result(CAmount(0), SelectionAlgorithm::CG);
        add_coin(1 * COIN, 1, expected_result);
        add_coin(1 * COIN, 2, expected_result);
        BOOST_CHECK(EquivalentResult(expected_result, *res));
        // Demonstrate how following improvements reduce iteration count and catch any regressions in the future.
        size_t expected_attempts = 7;
        BOOST_CHECK_MESSAGE(res->GetSelectionsEvaluated() == expected_attempts, strprintf("Expected %i attempts, but got %i", expected_attempts, res->GetSelectionsEvaluated()));
    }
}

static util::Result<SelectionResult> SelectCoinsSRD(const CAmount& target,
                                                    const CoinSelectionParams& cs_params,
                                                    const node::NodeContext& m_node,
                                                    int max_selection_weight,
                                                    std::function<CoinsResult(CWallet&)> coin_setup)
{
    std::unique_ptr<CWallet> wallet = NewWallet(m_node);
    CoinEligibilityFilter filter(0, 0, 0); // accept all coins without ancestors
    Groups group = GroupOutputs(*wallet, coin_setup(*wallet), cs_params, {{filter}})[filter].all_groups;
    return SelectCoinsSRD(group.positive_group, target, cs_params.m_change_fee, cs_params.rng_fast, max_selection_weight);
}

BOOST_AUTO_TEST_CASE(srd_tests)
{
    // Test SRD:
    // 1) Insufficient funds, select all provided coins and fail.
    // 2) Exceeded max weight, coin selection always surpasses the max allowed weight.
    // 3) Select coins without surpassing the max weight (some coins surpasses the max allowed weight, some others not)

    FastRandomContext rand;
    CoinSelectionParams dummy_params{ // Only used to provide the 'avoid_partial' flag.
            rand,
            /*change_output_size=*/34,
            /*change_spend_size=*/68,
            /*min_change_target=*/CENT,
            /*effective_feerate=*/CFeeRate(0),
            /*long_term_feerate=*/CFeeRate(0),
            /*discard_feerate=*/CFeeRate(0),
            /*tx_noinputs_size=*/10 + 34, // static header size + output size
            /*avoid_partial=*/false,
    };

    {
        // #########################################################
        // 1) Insufficient funds, select all provided coins and fail
        // #########################################################
        CAmount target = 49.5L * COIN;
        int max_selection_weight = 10000; // high enough to not fail for this reason.
        const auto& res = SelectCoinsSRD(target, dummy_params, m_node, max_selection_weight, [&](CWallet& wallet) {
            CoinsResult available_coins;
            for (int j = 0; j < 10; ++j) {
                add_coin(available_coins, wallet, CAmount(1 * COIN));
                add_coin(available_coins, wallet, CAmount(2 * COIN));
            }
            return available_coins;
        });
        BOOST_CHECK(!res);
        BOOST_CHECK(util::ErrorString(res).empty()); // empty means "insufficient funds"
    }

    {
        // ###########################
        // 2) Test max weight exceeded
        // ###########################
        CAmount target = 49.5L * COIN;
        int max_selection_weight = 3000;
        const auto& res = SelectCoinsSRD(target, dummy_params, m_node, max_selection_weight, [&](CWallet& wallet) {
            CoinsResult available_coins;
            for (int j = 0; j < 10; ++j) {
                /* 10 × 1 BTC + 10 × 2 BTC = 30 BTC. 20 × 272 WU = 5440 WU */
                add_coin(available_coins, wallet, CAmount(1 * COIN), CFeeRate(0), 144, false, 0, true);
                add_coin(available_coins, wallet, CAmount(2 * COIN), CFeeRate(0), 144, false, 0, true);
            }
            return available_coins;
        });
        BOOST_CHECK(!res);
        BOOST_CHECK(util::ErrorString(res).original.find("The inputs size exceeds the maximum weight") != std::string::npos);
    }

    {
        // ################################################################################################################
        // 3) Test that SRD result does not exceed the max weight
        // ################################################################################################################
        CAmount target = 25.33L * COIN;
        int max_selection_weight = 10000; // WU
        const auto& res = SelectCoinsSRD(target, dummy_params, m_node, max_selection_weight, [&](CWallet& wallet) {
            CoinsResult available_coins;
            for (int j = 0; j < 60; ++j) { // 60 UTXO --> 19,8 BTC total --> 60 × 272 WU = 16320 WU
                add_coin(available_coins, wallet, CAmount(0.33 * COIN), CFeeRate(0), 144, false, 0, true);
            }
            for (int i = 0; i < 10; i++) { // 10 UTXO --> 20 BTC total --> 10 × 272 WU = 2720 WU
                add_coin(available_coins, wallet, CAmount(2 * COIN), CFeeRate(0), 144, false, 0, true);
            }
            return available_coins;
        });
        BOOST_CHECK(res);
        BOOST_CHECK(res->GetWeight() <= max_selection_weight);
    }
}

static util::Result<SelectionResult> select_coins(const CAmount& target, const CoinSelectionParams& cs_params, const CCoinControl& cc, std::function<CoinsResult(CWallet&)> coin_setup, const node::NodeContext& m_node)
{
    std::unique_ptr<CWallet> wallet = NewWallet(m_node);
    auto available_coins = coin_setup(*wallet);

    LOCK(wallet->cs_wallet);
    auto result = SelectCoins(*wallet, available_coins, /*pre_set_inputs=*/ {}, target, cc, cs_params);
    if (result) {
        const auto signedTxSize = 10 + 34 + 68 * result->GetInputSet().size(); // static header size + output size + inputs size (P2WPKH)
        BOOST_CHECK_LE(signedTxSize * WITNESS_SCALE_FACTOR, MAX_STANDARD_TX_WEIGHT);

        BOOST_CHECK_GE(result->GetSelectedValue(), target);
    }
    return result;
}

static bool has_coin(const CoinSet& set, CAmount amount)
{
    return std::any_of(set.begin(), set.end(), [&](const auto& coin) { return coin->GetEffectiveValue() == amount; });
}

BOOST_AUTO_TEST_CASE(check_max_selection_weight)
{
    const CAmount target = 49.5L * COIN;
    CCoinControl cc;

    FastRandomContext rand;
    CoinSelectionParams cs_params{
        rand,
        /*change_output_size=*/34,
        /*change_spend_size=*/68,
        /*min_change_target=*/CENT,
        /*effective_feerate=*/CFeeRate(0),
        /*long_term_feerate=*/CFeeRate(0),
        /*discard_feerate=*/CFeeRate(0),
        /*tx_noinputs_size=*/10 + 34, // static header size + output size
        /*avoid_partial=*/false,
    };

    int max_weight = MAX_STANDARD_TX_WEIGHT - WITNESS_SCALE_FACTOR * (cs_params.tx_noinputs_size + cs_params.change_output_size);
    {
        // Scenario 1:
        // The actor starts with 1x 50.0 BTC and 1515x 0.033 BTC (~100.0 BTC total) unspent outputs
        // Then tries to spend 49.5 BTC
        // The 50.0 BTC output should be selected, because the transaction would otherwise be too large

        // Perform selection

        const auto result = select_coins(
            target, cs_params, cc, [&](CWallet& wallet) {
                CoinsResult available_coins;
                for (int j = 0; j < 1515; ++j) {
                    add_coin(available_coins, wallet, CAmount(0.033 * COIN), CFeeRate(0), 144, false, 0, true);
                }

                add_coin(available_coins, wallet, CAmount(50 * COIN), CFeeRate(0), 144, false, 0, true);
                return available_coins;
            },
            m_node);

        BOOST_CHECK(result);
        // Verify that the 50 BTC UTXO was selected, and result is below max_weight
        BOOST_CHECK(has_coin(result->GetInputSet(), CAmount(50 * COIN)));
        BOOST_CHECK_LE(result->GetWeight(), max_weight);
    }

    {
        // Scenario 2:

        // The actor starts with 400x 0.0625 BTC and 2000x 0.025 BTC (75.0 BTC total) unspent outputs
        // Then tries to spend 49.5 BTC
        // A combination of coins should be selected, such that the created transaction is not too large

        // Perform selection
        const auto result = select_coins(
            target, cs_params, cc, [&](CWallet& wallet) {
                CoinsResult available_coins;
                for (int j = 0; j < 400; ++j) {
                    add_coin(available_coins, wallet, CAmount(0.0625 * COIN), CFeeRate(0), 144, false, 0, true);
                }
                for (int j = 0; j < 2000; ++j) {
                    add_coin(available_coins, wallet, CAmount(0.025 * COIN), CFeeRate(0), 144, false, 0, true);
                }
                return available_coins;
            },
            m_node);

        BOOST_CHECK(has_coin(result->GetInputSet(), CAmount(0.0625 * COIN)));
        BOOST_CHECK(has_coin(result->GetInputSet(), CAmount(0.025 * COIN)));
        BOOST_CHECK_LE(result->GetWeight(), max_weight);
    }

    {
        // Scenario 3:

        // The actor starts with 1515x 0.033 BTC (49.995 BTC total) unspent outputs
        // No results should be returned, because the transaction would be too large

        // Perform selection
        const auto result = select_coins(
            target, cs_params, cc, [&](CWallet& wallet) {
                CoinsResult available_coins;
                for (int j = 0; j < 1515; ++j) {
                    add_coin(available_coins, wallet, CAmount(0.033 * COIN), CFeeRate(0), 144, false, 0, true);
                }
                return available_coins;
            },
            m_node);

        // No results
        // 1515 inputs * 68 bytes = 103,020 bytes
        // 103,020 bytes * 4 = 412,080 weight, which is above the MAX_STANDARD_TX_WEIGHT of 400,000
        BOOST_CHECK(!result);
    }
}

BOOST_AUTO_TEST_CASE(SelectCoins_effective_value_test)
{
    // Test that the effective value is used to check whether preset inputs provide sufficient funds when subtract_fee_outputs is not used.
    // This test creates a coin whose value is higher than the target but whose effective value is lower than the target.
    // The coin is selected using coin control, with m_allow_other_inputs = false. SelectCoins should fail due to insufficient funds.

    std::unique_ptr<CWallet> wallet = NewWallet(m_node);

    CoinsResult available_coins;
    {
        std::unique_ptr<CWallet> dummyWallet = NewWallet(m_node, /*wallet_name=*/"dummy");
        add_coin(available_coins, *dummyWallet, 100000); // 0.001 BTC
    }

    CAmount target{99900}; // 0.000999 BTC

    FastRandomContext rand;
    CoinSelectionParams cs_params{
        rand,
        /*change_output_size=*/34,
        /*change_spend_size=*/148,
        /*min_change_target=*/1000,
        /*effective_feerate=*/CFeeRate(3000),
        /*long_term_feerate=*/CFeeRate(1000),
        /*discard_feerate=*/CFeeRate(1000),
        /*tx_noinputs_size=*/0,
        /*avoid_partial=*/false,
    };
    CCoinControl cc;
    cc.m_allow_other_inputs = false;
    COutput output = available_coins.All().at(0);
    cc.SetInputWeight(output.outpoint, 148);
    cc.Select(output.outpoint).SetTxOut(output.txout);

    LOCK(wallet->cs_wallet);
    const auto preset_inputs = *Assert(FetchSelectedInputs(*wallet, cc, cs_params));
    available_coins.Erase({available_coins.coins[OutputType::BECH32].begin()->outpoint});

    const auto result = SelectCoins(*wallet, available_coins, preset_inputs, target, cc, cs_params);
    BOOST_CHECK(!result);
}

BOOST_FIXTURE_TEST_CASE(wallet_coinsresult_test, BasicTestingSetup)
{
    // Test case to verify CoinsResult object sanity.
    CoinsResult available_coins;
    {
        std::unique_ptr<CWallet> dummyWallet = NewWallet(m_node, /*wallet_name=*/"dummy");

        // Add some coins to 'available_coins'
        for (int i=0; i<10; i++) {
            add_coin(available_coins, *dummyWallet, 1 * COIN);
        }
    }

    {
        // First test case, check that 'CoinsResult::Erase' function works as expected.
        // By trying to erase two elements from the 'available_coins' object.
        std::unordered_set<COutPoint, SaltedOutpointHasher> outs_to_remove;
        const auto& coins = available_coins.All();
        for (int i = 0; i < 2; i++) {
            outs_to_remove.emplace(coins[i].outpoint);
        }
        available_coins.Erase(outs_to_remove);

        // Check that the elements were actually removed.
        const auto& updated_coins = available_coins.All();
        for (const auto& out: outs_to_remove) {
            auto it = std::find_if(updated_coins.begin(), updated_coins.end(), [&out](const COutput &coin) {
                return coin.outpoint == out;
            });
            BOOST_CHECK(it == updated_coins.end());
        }
        // And verify that no extra element were removed
        BOOST_CHECK_EQUAL(available_coins.Size(), 8);
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
