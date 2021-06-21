// Copyright (c) 2017-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <node/context.h>
#include <primitives/transaction.h>
#include <random.h>
#include <test/util/setup_common.h>
#include <wallet/coincontrol.h>
#include <wallet/coinselection.h>
#include <wallet/test/wallet_test_fixture.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>
#include <random>

BOOST_FIXTURE_TEST_SUITE(coinselector_tests, WalletTestingSetup)

// how many times to run all the tests to have a chance to catch errors that only show up with particular random shuffles
#define RUN_TESTS 100

// some tests fail 1% of the time due to bad luck.
// we repeat those tests this many times and only complain if all iterations of the test fail
#define RANDOM_REPEATS 5

typedef std::set<CInputCoin> CoinSet;

static std::vector<COutput> vCoins;
static NodeContext testNode;
static auto testChain = interfaces::MakeChain(testNode);
static CWallet testWallet(testChain.get(), "", CreateDummyWalletDatabase());
static CAmount balance = 0;

CoinEligibilityFilter filter_standard(1, 6, 0);
CoinEligibilityFilter filter_confirmed(1, 1, 0);
CoinEligibilityFilter filter_standard_extra(6, 6, 0);
CoinSelectionParams coin_selection_params(/* change_output_size= */ 0,
                                          /* change_spend_size= */ 0, /* effective_feerate= */ CFeeRate(0),
                                          /* long_term_feerate= */ CFeeRate(0), /* discard_feerate= */ CFeeRate(0),
                                          /* tx_no_inputs_size= */ 0, /* avoid_partial= */ false);

static void add_coin(const CAmount& nValue, int nInput, std::vector<CInputCoin>& set)
{
    CMutableTransaction tx;
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    set.emplace_back(MakeTransactionRef(tx), nInput);
}

static void add_coin(const CAmount& nValue, int nInput, CoinSet& set)
{
    CMutableTransaction tx;
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    set.emplace(MakeTransactionRef(tx), nInput);
}

static void add_coin(CWallet& wallet, const CAmount& nValue, int nAge = 6*24, bool fIsFromMe = false, int nInput=0, bool spendable = false)
{
    balance += nValue;
    static int nextLockTime = 0;
    CMutableTransaction tx;
    tx.nLockTime = nextLockTime++;        // so all transactions get different hashes
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    if (spendable) {
        CTxDestination dest;
        std::string error;
        const bool destination_ok = wallet.GetNewDestination(OutputType::BECH32, "", dest, error);
        assert(destination_ok);
        tx.vout[nInput].scriptPubKey = GetScriptForDestination(dest);
    }
    if (fIsFromMe) {
        // IsFromMe() returns (GetDebit() > 0), and GetDebit() is 0 if vin.empty(),
        // so stop vin being empty, and cache a non-zero Debit to fake out IsFromMe()
        tx.vin.resize(1);
    }
    CWalletTx* wtx = wallet.AddToWallet(MakeTransactionRef(std::move(tx)), /* confirm= */ {});
    if (fIsFromMe)
    {
        wtx->m_amounts[CWalletTx::DEBIT].Set(ISMINE_SPENDABLE, 1);
        wtx->m_is_cache_empty = false;
    }
    COutput output(wtx, nInput, nAge, true /* spendable */, true /* solvable */, true /* safe */);
    vCoins.push_back(output);
}
static void add_coin(const CAmount& nValue, int nAge = 6*24, bool fIsFromMe = false, int nInput=0, bool spendable = false)
{
    add_coin(testWallet, nValue, nAge, fIsFromMe, nInput, spendable);
}

static void empty_wallet(void)
{
    vCoins.clear();
    balance = 0;
}

static bool equal_sets(CoinSet a, CoinSet b)
{
    std::pair<CoinSet::iterator, CoinSet::iterator> ret = mismatch(a.begin(), a.end(), b.begin());
    return ret.first == a.end() && ret.second == b.end();
}

static CAmount make_hard_case(int utxos, std::vector<CInputCoin>& utxo_pool)
{
    utxo_pool.clear();
    CAmount target = 0;
    for (int i = 0; i < utxos; ++i) {
        target += (CAmount)1 << (utxos+i);
        add_coin((CAmount)1 << (utxos+i), 2*i, utxo_pool);
        add_coin(((CAmount)1 << (utxos+i)) + ((CAmount)1 << (utxos-1-i)), 2*i + 1, utxo_pool);
    }
    return target;
}

inline std::vector<OutputGroup>& GroupCoins(const std::vector<CInputCoin>& coins)
{
    static std::vector<OutputGroup> static_groups;
    static_groups.clear();
    for (auto& coin : coins) {
        static_groups.emplace_back();
        static_groups.back().Insert(coin, 0, true, 0, 0, false);
    }
    return static_groups;
}

inline std::vector<OutputGroup>& GroupCoins(const std::vector<COutput>& coins)
{
    static std::vector<OutputGroup> static_groups;
    static_groups.clear();
    for (auto& coin : coins) {
        static_groups.emplace_back();
        static_groups.back().Insert(coin.GetInputCoin(), coin.nDepth, coin.tx->m_amounts[CWalletTx::DEBIT].m_cached[ISMINE_SPENDABLE] && coin.tx->m_amounts[CWalletTx::DEBIT].m_value[ISMINE_SPENDABLE] == 1 /* HACK: we can't figure out the is_me flag so we use the conditions defined above; perhaps set safe to false for !fIsFromMe in add_coin() */, 0, 0, false);
    }
    return static_groups;
}

// Branch and bound coin selection tests
BOOST_AUTO_TEST_CASE(bnb_search_test)
{

    LOCK(testWallet.cs_wallet);
    testWallet.SetupLegacyScriptPubKeyMan();

    // Setup
    std::vector<CInputCoin> utxo_pool;
    CoinSet selection;
    CoinSet actual_selection;
    CAmount value_ret = 0;

    /////////////////////////
    // Known Outcome tests //
    /////////////////////////

    // Empty utxo pool
    BOOST_CHECK(!SelectCoinsBnB(GroupCoins(utxo_pool), 1 * CENT, 0.5 * CENT, selection, value_ret));
    selection.clear();

    // Add utxos
    add_coin(1 * CENT, 1, utxo_pool);
    add_coin(2 * CENT, 2, utxo_pool);
    add_coin(3 * CENT, 3, utxo_pool);
    add_coin(4 * CENT, 4, utxo_pool);

    // Select 1 Cent
    add_coin(1 * CENT, 1, actual_selection);
    BOOST_CHECK(SelectCoinsBnB(GroupCoins(utxo_pool), 1 * CENT, 0.5 * CENT, selection, value_ret));
    BOOST_CHECK(equal_sets(selection, actual_selection));
    BOOST_CHECK_EQUAL(value_ret, 1 * CENT);
    actual_selection.clear();
    selection.clear();

    // Select 2 Cent
    add_coin(2 * CENT, 2, actual_selection);
    BOOST_CHECK(SelectCoinsBnB(GroupCoins(utxo_pool), 2 * CENT, 0.5 * CENT, selection, value_ret));
    BOOST_CHECK(equal_sets(selection, actual_selection));
    BOOST_CHECK_EQUAL(value_ret, 2 * CENT);
    actual_selection.clear();
    selection.clear();

    // Select 5 Cent
    add_coin(4 * CENT, 4, actual_selection);
    add_coin(1 * CENT, 1, actual_selection);
    BOOST_CHECK(SelectCoinsBnB(GroupCoins(utxo_pool), 5 * CENT, 0.5 * CENT, selection, value_ret));
    BOOST_CHECK(equal_sets(selection, actual_selection));
    BOOST_CHECK_EQUAL(value_ret, 5 * CENT);
    actual_selection.clear();
    selection.clear();

    // Select 11 Cent, not possible
    BOOST_CHECK(!SelectCoinsBnB(GroupCoins(utxo_pool), 11 * CENT, 0.5 * CENT, selection, value_ret));
    actual_selection.clear();
    selection.clear();

    // Cost of change is greater than the difference between target value and utxo sum
    add_coin(1 * CENT, 1, actual_selection);
    BOOST_CHECK(SelectCoinsBnB(GroupCoins(utxo_pool), 0.9 * CENT, 0.5 * CENT, selection, value_ret));
    BOOST_CHECK_EQUAL(value_ret, 1 * CENT);
    BOOST_CHECK(equal_sets(selection, actual_selection));
    actual_selection.clear();
    selection.clear();

    // Cost of change is less than the difference between target value and utxo sum
    BOOST_CHECK(!SelectCoinsBnB(GroupCoins(utxo_pool), 0.9 * CENT, 0, selection, value_ret));
    actual_selection.clear();
    selection.clear();

    // Select 10 Cent
    add_coin(5 * CENT, 5, utxo_pool);
    add_coin(5 * CENT, 5, actual_selection);
    add_coin(4 * CENT, 4, actual_selection);
    add_coin(1 * CENT, 1, actual_selection);
    BOOST_CHECK(SelectCoinsBnB(GroupCoins(utxo_pool), 10 * CENT, 0.5 * CENT, selection, value_ret));
    BOOST_CHECK(equal_sets(selection, actual_selection));
    BOOST_CHECK_EQUAL(value_ret, 10 * CENT);
    actual_selection.clear();
    selection.clear();

    // Negative effective value
    // Select 10 Cent but have 1 Cent not be possible because too small
    add_coin(5 * CENT, 5, actual_selection);
    add_coin(3 * CENT, 3, actual_selection);
    add_coin(2 * CENT, 2, actual_selection);
    BOOST_CHECK(SelectCoinsBnB(GroupCoins(utxo_pool), 10 * CENT, 5000, selection, value_ret));
    BOOST_CHECK_EQUAL(value_ret, 10 * CENT);
    // FIXME: this test is redundant with the above, because 1 Cent is selected, not "too small"
    // BOOST_CHECK(equal_sets(selection, actual_selection));

    // Select 0.25 Cent, not possible
    BOOST_CHECK(!SelectCoinsBnB(GroupCoins(utxo_pool), 0.25 * CENT, 0.5 * CENT, selection, value_ret));
    actual_selection.clear();
    selection.clear();

    // Iteration exhaustion test
    CAmount target = make_hard_case(17, utxo_pool);
    BOOST_CHECK(!SelectCoinsBnB(GroupCoins(utxo_pool), target, 0, selection, value_ret)); // Should exhaust
    target = make_hard_case(14, utxo_pool);
    BOOST_CHECK(SelectCoinsBnB(GroupCoins(utxo_pool), target, 0, selection, value_ret)); // Should not exhaust

    // Test same value early bailout optimization
    utxo_pool.clear();
    add_coin(7 * CENT, 7, actual_selection);
    add_coin(7 * CENT, 7, actual_selection);
    add_coin(7 * CENT, 7, actual_selection);
    add_coin(7 * CENT, 7, actual_selection);
    add_coin(2 * CENT, 7, actual_selection);
    add_coin(7 * CENT, 7, utxo_pool);
    add_coin(7 * CENT, 7, utxo_pool);
    add_coin(7 * CENT, 7, utxo_pool);
    add_coin(7 * CENT, 7, utxo_pool);
    add_coin(2 * CENT, 7, utxo_pool);
    for (int i = 0; i < 50000; ++i) {
        add_coin(5 * CENT, 7, utxo_pool);
    }
    BOOST_CHECK(SelectCoinsBnB(GroupCoins(utxo_pool), 30 * CENT, 5000, selection, value_ret));
    BOOST_CHECK_EQUAL(value_ret, 30 * CENT);
    BOOST_CHECK(equal_sets(selection, actual_selection));

    ////////////////////
    // Behavior tests //
    ////////////////////
    // Select 1 Cent with pool of only greater than 5 Cent
    utxo_pool.clear();
    for (int i = 5; i <= 20; ++i) {
        add_coin(i * CENT, i, utxo_pool);
    }
    // Run 100 times, to make sure it is never finding a solution
    for (int i = 0; i < 100; ++i) {
        BOOST_CHECK(!SelectCoinsBnB(GroupCoins(utxo_pool), 1 * CENT, 2 * CENT, selection, value_ret));
    }

    // Make sure that effective value is working in AttemptSelection when BnB is used
    CoinSelectionParams coin_selection_params_bnb(/* change_output_size= */ 0,
                                                  /* change_spend_size= */ 0, /* effective_feerate= */ CFeeRate(3000),
                                                  /* long_term_feerate= */ CFeeRate(1000), /* discard_feerate= */ CFeeRate(1000),
                                                  /* tx_no_inputs_size= */ 0, /* avoid_partial= */ false);
    CoinSet setCoinsRet;
    CAmount nValueRet;
    empty_wallet();
    add_coin(1);
    vCoins.at(0).nInputBytes = 40; // Make sure that it has a negative effective value. The next check should assert if this somehow got through. Otherwise it will fail
    BOOST_CHECK(!testWallet.AttemptSelection( 1 * CENT, filter_standard, vCoins, setCoinsRet, nValueRet, coin_selection_params_bnb));

    // Test fees subtracted from output:
    empty_wallet();
    add_coin(1 * CENT);
    vCoins.at(0).nInputBytes = 40;
    coin_selection_params_bnb.m_subtract_fee_outputs = true;
    BOOST_CHECK(testWallet.AttemptSelection( 1 * CENT, filter_standard, vCoins, setCoinsRet, nValueRet, coin_selection_params_bnb));
    BOOST_CHECK_EQUAL(nValueRet, 1 * CENT);

    // Make sure that can use BnB when there are preset inputs
    empty_wallet();
    {
        std::unique_ptr<CWallet> wallet = std::make_unique<CWallet>(m_node.chain.get(), "", CreateMockWalletDatabase());
        wallet->LoadWallet();
        wallet->SetupLegacyScriptPubKeyMan();
        LOCK(wallet->cs_wallet);
        add_coin(*wallet, 5 * CENT, 6 * 24, false, 0, true);
        add_coin(*wallet, 3 * CENT, 6 * 24, false, 0, true);
        add_coin(*wallet, 2 * CENT, 6 * 24, false, 0, true);
        CCoinControl coin_control;
        coin_control.fAllowOtherInputs = true;
        coin_control.Select(COutPoint(vCoins.at(0).tx->GetHash(), vCoins.at(0).i));
        coin_selection_params_bnb.m_effective_feerate = CFeeRate(0);
        BOOST_CHECK(wallet->SelectCoins(vCoins, 10 * CENT, setCoinsRet, nValueRet, coin_control, coin_selection_params_bnb));
    }
}

BOOST_AUTO_TEST_CASE(knapsack_solver_test)
{
    CoinSet setCoinsRet, setCoinsRet2;
    CAmount nValueRet;

    LOCK(testWallet.cs_wallet);
    testWallet.SetupLegacyScriptPubKeyMan();

    // test multiple times to allow for differences in the shuffle order
    for (int i = 0; i < RUN_TESTS; i++)
    {
        empty_wallet();

        // with an empty wallet we can't even pay one cent
        BOOST_CHECK(!testWallet.AttemptSelection( 1 * CENT, filter_standard, vCoins, setCoinsRet, nValueRet, coin_selection_params));

        add_coin(1*CENT, 4);        // add a new 1 cent coin

        // with a new 1 cent coin, we still can't find a mature 1 cent
        BOOST_CHECK(!testWallet.AttemptSelection( 1 * CENT, filter_standard, vCoins, setCoinsRet, nValueRet, coin_selection_params));

        // but we can find a new 1 cent
        BOOST_CHECK( testWallet.AttemptSelection( 1 * CENT, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 1 * CENT);

        add_coin(2*CENT);           // add a mature 2 cent coin

        // we can't make 3 cents of mature coins
        BOOST_CHECK(!testWallet.AttemptSelection( 3 * CENT, filter_standard, vCoins, setCoinsRet, nValueRet, coin_selection_params));

        // we can make 3 cents of new coins
        BOOST_CHECK( testWallet.AttemptSelection( 3 * CENT, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 3 * CENT);

        add_coin(5*CENT);           // add a mature 5 cent coin,
        add_coin(10*CENT, 3, true); // a new 10 cent coin sent from one of our own addresses
        add_coin(20*CENT);          // and a mature 20 cent coin

        // now we have new: 1+10=11 (of which 10 was self-sent), and mature: 2+5+20=27.  total = 38

        // we can't make 38 cents only if we disallow new coins:
        BOOST_CHECK(!testWallet.AttemptSelection(38 * CENT, filter_standard, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        // we can't even make 37 cents if we don't allow new coins even if they're from us
        BOOST_CHECK(!testWallet.AttemptSelection(38 * CENT, filter_standard_extra, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        // but we can make 37 cents if we accept new coins from ourself
        BOOST_CHECK( testWallet.AttemptSelection(37 * CENT, filter_standard, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 37 * CENT);
        // and we can make 38 cents if we accept all new coins
        BOOST_CHECK( testWallet.AttemptSelection(38 * CENT, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 38 * CENT);

        // try making 34 cents from 1,2,5,10,20 - we can't do it exactly
        BOOST_CHECK( testWallet.AttemptSelection(34 * CENT, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 35 * CENT);       // but 35 cents is closest
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 3U);     // the best should be 20+10+5.  it's incredibly unlikely the 1 or 2 got included (but possible)

        // when we try making 7 cents, the smaller coins (1,2,5) are enough.  We should see just 2+5
        BOOST_CHECK( testWallet.AttemptSelection( 7 * CENT, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 7 * CENT);
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 2U);

        // when we try making 8 cents, the smaller coins (1,2,5) are exactly enough.
        BOOST_CHECK( testWallet.AttemptSelection( 8 * CENT, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK(nValueRet == 8 * CENT);
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 3U);

        // when we try making 9 cents, no subset of smaller coins is enough, and we get the next bigger coin (10)
        BOOST_CHECK( testWallet.AttemptSelection( 9 * CENT, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 10 * CENT);
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 1U);

        // now clear out the wallet and start again to test choosing between subsets of smaller coins and the next biggest coin
        empty_wallet();

        add_coin( 6*CENT);
        add_coin( 7*CENT);
        add_coin( 8*CENT);
        add_coin(20*CENT);
        add_coin(30*CENT); // now we have 6+7+8+20+30 = 71 cents total

        // check that we have 71 and not 72
        BOOST_CHECK( testWallet.AttemptSelection(71 * CENT, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK(!testWallet.AttemptSelection(72 * CENT, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));

        // now try making 16 cents.  the best smaller coins can do is 6+7+8 = 21; not as good at the next biggest coin, 20
        BOOST_CHECK( testWallet.AttemptSelection(16 * CENT, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 20 * CENT); // we should get 20 in one coin
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 1U);

        add_coin( 5*CENT); // now we have 5+6+7+8+20+30 = 75 cents total

        // now if we try making 16 cents again, the smaller coins can make 5+6+7 = 18 cents, better than the next biggest coin, 20
        BOOST_CHECK( testWallet.AttemptSelection(16 * CENT, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 18 * CENT); // we should get 18 in 3 coins
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 3U);

        add_coin( 18*CENT); // now we have 5+6+7+8+18+20+30

        // and now if we try making 16 cents again, the smaller coins can make 5+6+7 = 18 cents, the same as the next biggest coin, 18
        BOOST_CHECK( testWallet.AttemptSelection(16 * CENT, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 18 * CENT);  // we should get 18 in 1 coin
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 1U); // because in the event of a tie, the biggest coin wins

        // now try making 11 cents.  we should get 5+6
        BOOST_CHECK( testWallet.AttemptSelection(11 * CENT, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 11 * CENT);
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 2U);

        // check that the smallest bigger coin is used
        add_coin( 1*COIN);
        add_coin( 2*COIN);
        add_coin( 3*COIN);
        add_coin( 4*COIN); // now we have 5+6+7+8+18+20+30+100+200+300+400 = 1094 cents
        BOOST_CHECK( testWallet.AttemptSelection(95 * CENT, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 1 * COIN);  // we should get 1 BTC in 1 coin
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 1U);

        BOOST_CHECK( testWallet.AttemptSelection(195 * CENT, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 2 * COIN);  // we should get 2 BTC in 1 coin
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 1U);

        // empty the wallet and start again, now with fractions of a cent, to test small change avoidance

        empty_wallet();
        add_coin(MIN_CHANGE * 1 / 10);
        add_coin(MIN_CHANGE * 2 / 10);
        add_coin(MIN_CHANGE * 3 / 10);
        add_coin(MIN_CHANGE * 4 / 10);
        add_coin(MIN_CHANGE * 5 / 10);

        // try making 1 * MIN_CHANGE from the 1.5 * MIN_CHANGE
        // we'll get change smaller than MIN_CHANGE whatever happens, so can expect MIN_CHANGE exactly
        BOOST_CHECK( testWallet.AttemptSelection(MIN_CHANGE, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, MIN_CHANGE);

        // but if we add a bigger coin, small change is avoided
        add_coin(1111*MIN_CHANGE);

        // try making 1 from 0.1 + 0.2 + 0.3 + 0.4 + 0.5 + 1111 = 1112.5
        BOOST_CHECK( testWallet.AttemptSelection(1 * MIN_CHANGE, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 1 * MIN_CHANGE); // we should get the exact amount

        // if we add more small coins:
        add_coin(MIN_CHANGE * 6 / 10);
        add_coin(MIN_CHANGE * 7 / 10);

        // and try again to make 1.0 * MIN_CHANGE
        BOOST_CHECK( testWallet.AttemptSelection(1 * MIN_CHANGE, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 1 * MIN_CHANGE); // we should get the exact amount

        // run the 'mtgox' test (see https://blockexplorer.com/tx/29a3efd3ef04f9153d47a990bd7b048a4b2d213daaa5fb8ed670fb85f13bdbcf)
        // they tried to consolidate 10 50k coins into one 500k coin, and ended up with 50k in change
        empty_wallet();
        for (int j = 0; j < 20; j++)
            add_coin(50000 * COIN);

        BOOST_CHECK( testWallet.AttemptSelection(500000 * COIN, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 500000 * COIN); // we should get the exact amount
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 10U); // in ten coins

        // if there's not enough in the smaller coins to make at least 1 * MIN_CHANGE change (0.5+0.6+0.7 < 1.0+1.0),
        // we need to try finding an exact subset anyway

        // sometimes it will fail, and so we use the next biggest coin:
        empty_wallet();
        add_coin(MIN_CHANGE * 5 / 10);
        add_coin(MIN_CHANGE * 6 / 10);
        add_coin(MIN_CHANGE * 7 / 10);
        add_coin(1111 * MIN_CHANGE);
        BOOST_CHECK( testWallet.AttemptSelection(1 * MIN_CHANGE, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 1111 * MIN_CHANGE); // we get the bigger coin
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 1U);

        // but sometimes it's possible, and we use an exact subset (0.4 + 0.6 = 1.0)
        empty_wallet();
        add_coin(MIN_CHANGE * 4 / 10);
        add_coin(MIN_CHANGE * 6 / 10);
        add_coin(MIN_CHANGE * 8 / 10);
        add_coin(1111 * MIN_CHANGE);
        BOOST_CHECK( testWallet.AttemptSelection(MIN_CHANGE, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, MIN_CHANGE);   // we should get the exact amount
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 2U); // in two coins 0.4+0.6

        // test avoiding small change
        empty_wallet();
        add_coin(MIN_CHANGE * 5 / 100);
        add_coin(MIN_CHANGE * 1);
        add_coin(MIN_CHANGE * 100);

        // trying to make 100.01 from these three coins
        BOOST_CHECK(testWallet.AttemptSelection(MIN_CHANGE * 10001 / 100, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, MIN_CHANGE * 10105 / 100); // we should get all coins
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 3U);

        // but if we try to make 99.9, we should take the bigger of the two small coins to avoid small change
        BOOST_CHECK(testWallet.AttemptSelection(MIN_CHANGE * 9990 / 100, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));
        BOOST_CHECK_EQUAL(nValueRet, 101 * MIN_CHANGE);
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 2U);
      }

      // test with many inputs
      for (CAmount amt=1500; amt < COIN; amt*=10) {
           empty_wallet();
           // Create 676 inputs (=  (old MAX_STANDARD_TX_SIZE == 100000)  / 148 bytes per input)
           for (uint16_t j = 0; j < 676; j++)
               add_coin(amt);

           // We only create the wallet once to save time, but we still run the coin selection RUN_TESTS times.
           for (int i = 0; i < RUN_TESTS; i++) {
             BOOST_CHECK(testWallet.AttemptSelection(2000, filter_confirmed, vCoins, setCoinsRet, nValueRet, coin_selection_params));

             if (amt - 2000 < MIN_CHANGE) {
                 // needs more than one input:
                 uint16_t returnSize = std::ceil((2000.0 + MIN_CHANGE)/amt);
                 CAmount returnValue = amt * returnSize;
                 BOOST_CHECK_EQUAL(nValueRet, returnValue);
                 BOOST_CHECK_EQUAL(setCoinsRet.size(), returnSize);
             } else {
                 // one input is sufficient:
                 BOOST_CHECK_EQUAL(nValueRet, amt);
                 BOOST_CHECK_EQUAL(setCoinsRet.size(), 1U);
             }
           }
      }

      // test randomness
      {
          empty_wallet();
          for (int i2 = 0; i2 < 100; i2++)
              add_coin(COIN);

          // Again, we only create the wallet once to save time, but we still run the coin selection RUN_TESTS times.
          for (int i = 0; i < RUN_TESTS; i++) {
            // picking 50 from 100 coins doesn't depend on the shuffle,
            // but does depend on randomness in the stochastic approximation code
            BOOST_CHECK(KnapsackSolver(50 * COIN, GroupCoins(vCoins), setCoinsRet, nValueRet));
            BOOST_CHECK(KnapsackSolver(50 * COIN, GroupCoins(vCoins), setCoinsRet2, nValueRet));
            BOOST_CHECK(!equal_sets(setCoinsRet, setCoinsRet2));

            int fails = 0;
            for (int j = 0; j < RANDOM_REPEATS; j++)
            {
                // Test that the KnapsackSolver selects randomly from equivalent coins (same value and same input size).
                // When choosing 1 from 100 identical coins, 1% of the time, this test will choose the same coin twice
                // which will cause it to fail.
                // To avoid that issue, run the test RANDOM_REPEATS times and only complain if all of them fail
                BOOST_CHECK(KnapsackSolver(COIN, GroupCoins(vCoins), setCoinsRet, nValueRet));
                BOOST_CHECK(KnapsackSolver(COIN, GroupCoins(vCoins), setCoinsRet2, nValueRet));
                if (equal_sets(setCoinsRet, setCoinsRet2))
                    fails++;
            }
            BOOST_CHECK_NE(fails, RANDOM_REPEATS);
          }

          // add 75 cents in small change.  not enough to make 90 cents,
          // then try making 90 cents.  there are multiple competing "smallest bigger" coins,
          // one of which should be picked at random
          add_coin(5 * CENT);
          add_coin(10 * CENT);
          add_coin(15 * CENT);
          add_coin(20 * CENT);
          add_coin(25 * CENT);

          for (int i = 0; i < RUN_TESTS; i++) {
            int fails = 0;
            for (int j = 0; j < RANDOM_REPEATS; j++)
            {
                BOOST_CHECK(KnapsackSolver(90*CENT, GroupCoins(vCoins), setCoinsRet, nValueRet));
                BOOST_CHECK(KnapsackSolver(90*CENT, GroupCoins(vCoins), setCoinsRet2, nValueRet));
                if (equal_sets(setCoinsRet, setCoinsRet2))
                    fails++;
            }
            BOOST_CHECK_NE(fails, RANDOM_REPEATS);
          }
      }

    empty_wallet();
}

BOOST_AUTO_TEST_CASE(ApproximateBestSubset)
{
    CoinSet setCoinsRet;
    CAmount nValueRet;

    LOCK(testWallet.cs_wallet);
    testWallet.SetupLegacyScriptPubKeyMan();

    empty_wallet();

    // Test vValue sort order
    for (int i = 0; i < 1000; i++)
        add_coin(1000 * COIN);
    add_coin(3 * COIN);

    BOOST_CHECK(testWallet.AttemptSelection(1003 * COIN, filter_standard, vCoins, setCoinsRet, nValueRet, coin_selection_params));
    BOOST_CHECK_EQUAL(nValueRet, 1003 * COIN);
    BOOST_CHECK_EQUAL(setCoinsRet.size(), 2U);

    empty_wallet();
}

// Tests that with the ideal conditions, the coin selector will always be able to find a solution that can pay the target value
BOOST_AUTO_TEST_CASE(SelectCoins_test)
{
    LOCK(testWallet.cs_wallet);
    testWallet.SetupLegacyScriptPubKeyMan();

    // Random generator stuff
    std::default_random_engine generator;
    std::exponential_distribution<double> distribution (100);
    FastRandomContext rand;

    // Run this test 100 times
    for (int i = 0; i < 100; ++i)
    {
        empty_wallet();

        // Make a wallet with 1000 exponentially distributed random inputs
        for (int j = 0; j < 1000; ++j)
        {
            add_coin((CAmount)(distribution(generator)*10000000));
        }

        // Generate a random fee rate in the range of 100 - 400
        CFeeRate rate(rand.randrange(300) + 100);

        // Generate a random target value between 1000 and wallet balance
        CAmount target = rand.randrange(balance - 1000) + 1000;

        // Perform selection
        CoinSelectionParams cs_params(/* change_output_size= */ 34,
                                      /* change_spend_size= */ 148, /* effective_feerate= */ CFeeRate(0),
                                      /* long_term_feerate= */ CFeeRate(0), /* discard_feerate= */ CFeeRate(0),
                                      /* tx_no_inputs_size= */ 0, /* avoid_partial= */ false);
        CoinSet out_set;
        CAmount out_value = 0;
        CCoinControl cc;
        BOOST_CHECK(testWallet.SelectCoins(vCoins, target, out_set, out_value, cc, cs_params));
        BOOST_CHECK_GE(out_value, target);
    }
}

BOOST_AUTO_TEST_SUITE_END()
