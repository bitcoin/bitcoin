// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>
#include <wallet/coinselection.h>
#include <amount.h>
#include <primitives/transaction.h>
#include <random.h>
#include <test/test_bitcoin.h>
#include <wallet/test/wallet_test_fixture.h>

#include <boost/test/unit_test.hpp>
#include <random>

BOOST_FIXTURE_TEST_SUITE(coinselector_tests, WalletTestingSetup)

// how many times to run all the tests to have a chance to catch errors that only show up with particular random shuffles
#define RUN_TESTS 100

// some tests fail 1% of the time due to bad luck.
// we repeat those tests this many times and only complain if all iterations of the test fail
#define RANDOM_REPEATS 5

std::vector<std::unique_ptr<CWalletTx>> wtxn;

typedef std::set<CInputCoin> CoinSet;

static std::vector<COutput> vCoins;
static const CWallet testWallet("dummy", WalletDatabase::CreateDummy());
static CAmount balance = 0;

CoinEligibilityFilter filter_standard(1, 6, 0);
CoinEligibilityFilter filter_confirmed(1, 1, 0);
CoinEligibilityFilter filter_standard_extra(6, 6, 0);
CoinSelectionParams coin_selection_params(false, 0, 0, CFeeRate(0), 0);

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

static void add_coin(const CAmount& nValue, int nAge = 6*24, bool fIsFromMe = false, int nInput=0)
{
    balance += nValue;
    static int nextLockTime = 0;
    CMutableTransaction tx;
    tx.nLockTime = nextLockTime++;        // so all transactions get different hashes
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    if (fIsFromMe) {
        // IsFromMe() returns (GetDebit() > 0), and GetDebit() is 0 if vin.empty(),
        // so stop vin being empty, and cache a non-zero Debit to fake out IsFromMe()
        tx.vin.resize(1);
    }
    std::unique_ptr<CWalletTx> wtx(new CWalletTx(&testWallet, MakeTransactionRef(std::move(tx))));
    if (fIsFromMe)
    {
        wtx->fDebitCached = true;
        wtx->nDebitCached = 1;
    }
    COutput output(wtx.get(), nInput, nAge, true /* spendable */, true /* solvable */, true /* safe */);
    vCoins.push_back(output);
    wtxn.emplace_back(std::move(wtx));
}

static void empty_wallet(void)
{
    vCoins.clear();
    wtxn.clear();
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
    for (auto& coin : coins) static_groups.emplace_back(coin, 0, true, 0, 0);
    return static_groups;
}

inline std::vector<OutputGroup>& GroupCoins(const std::vector<COutput>& coins)
{
    static std::vector<OutputGroup> static_groups;
    static_groups.clear();
    for (auto& coin : coins) static_groups.emplace_back(coin.GetInputCoin(), coin.nDepth, coin.tx->fDebitCached && coin.tx->nDebitCached == 1 /* HACK: we can't figure out the is_me flag so we use the conditions defined above; perhaps set safe to false for !fIsFromMe in add_coin() */, 0, 0);
    return static_groups;
}

// Branch and bound coin selection tests
BOOST_AUTO_TEST_CASE(bnb_search_test)
{

    LOCK(testWallet.cs_wallet);

    // Setup
    std::vector<CInputCoin> utxo_pool;
    CoinSet selection;
    CoinSet actual_selection;
    CAmount value_ret = 0;
    CAmount not_input_fees = 0;

    /////////////////////////
    // Known Outcome tests //
    /////////////////////////
    BOOST_TEST_MESSAGE("Testing known outcomes");

    // Empty utxo pool
    BOOST_CHECK(!SelectCoinsBnB(GroupCoins(utxo_pool), 1 * CENT, 0.5 * CENT, selection, value_ret, not_input_fees));
    selection.clear();

    // Add utxos
    add_coin(1 * CENT, 1, utxo_pool);
    add_coin(2 * CENT, 2, utxo_pool);
    add_coin(3 * CENT, 3, utxo_pool);
    add_coin(4 * CENT, 4, utxo_pool);

    // Select 1 Cent
    add_coin(1 * CENT, 1, actual_selection);
    BOOST_CHECK(SelectCoinsBnB(GroupCoins(utxo_pool), 1 * CENT, 0.5 * CENT, selection, value_ret, not_input_fees));
    BOOST_CHECK(equal_sets(selection, actual_selection));
    actual_selection.clear();
    selection.clear();

    // Select 2 Cent
    add_coin(2 * CENT, 2, actual_selection);
    BOOST_CHECK(SelectCoinsBnB(GroupCoins(utxo_pool), 2 * CENT, 0.5 * CENT, selection, value_ret, not_input_fees));
    BOOST_CHECK(equal_sets(selection, actual_selection));
    actual_selection.clear();
    selection.clear();

    // Select 5 Cent
    add_coin(3 * CENT, 3, actual_selection);
    add_coin(2 * CENT, 2, actual_selection);
    BOOST_CHECK(SelectCoinsBnB(GroupCoins(utxo_pool), 5 * CENT, 0.5 * CENT, selection, value_ret, not_input_fees));
    BOOST_CHECK(equal_sets(selection, actual_selection));
    actual_selection.clear();
    selection.clear();

    // Select 11 Cent, not possible
    BOOST_CHECK(!SelectCoinsBnB(GroupCoins(utxo_pool), 11 * CENT, 0.5 * CENT, selection, value_ret, not_input_fees));
    actual_selection.clear();
    selection.clear();

    // Select 10 Cent
    add_coin(5 * CENT, 5, utxo_pool);
    add_coin(4 * CENT, 4, actual_selection);
    add_coin(3 * CENT, 3, actual_selection);
    add_coin(2 * CENT, 2, actual_selection);
    add_coin(1 * CENT, 1, actual_selection);
    BOOST_CHECK(SelectCoinsBnB(GroupCoins(utxo_pool), 10 * CENT, 0.5 * CENT, selection, value_ret, not_input_fees));
    BOOST_CHECK(equal_sets(selection, actual_selection));
    actual_selection.clear();
    selection.clear();

    // Negative effective value
    // Select 10 Cent but have 1 Cent not be possible because too small
    add_coin(5 * CENT, 5, actual_selection);
    add_coin(3 * CENT, 3, actual_selection);
    add_coin(2 * CENT, 2, actual_selection);
    BOOST_CHECK(SelectCoinsBnB(GroupCoins(utxo_pool), 10 * CENT, 5000, selection, value_ret, not_input_fees));

    // Select 0.25 Cent, not possible
    BOOST_CHECK(!SelectCoinsBnB(GroupCoins(utxo_pool), 0.25 * CENT, 0.5 * CENT, selection, value_ret, not_input_fees));
    actual_selection.clear();
    selection.clear();

    // Iteration exhaustion test
    CAmount target = make_hard_case(17, utxo_pool);
    BOOST_CHECK(!SelectCoinsBnB(GroupCoins(utxo_pool), target, 0, selection, value_ret, not_input_fees)); // Should exhaust
    target = make_hard_case(14, utxo_pool);
    BOOST_CHECK(SelectCoinsBnB(GroupCoins(utxo_pool), target, 0, selection, value_ret, not_input_fees)); // Should not exhaust

    // Test same value early bailout optimization
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
    BOOST_CHECK(SelectCoinsBnB(GroupCoins(utxo_pool), 30 * CENT, 5000, selection, value_ret, not_input_fees));

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
        BOOST_CHECK(!SelectCoinsBnB(GroupCoins(utxo_pool), 1 * CENT, 2 * CENT, selection, value_ret, not_input_fees));
    }

    // Make sure that effective value is working in SelectCoinsMinConf when BnB is used
    CoinSelectionParams coin_selection_params_bnb(true, 0, 0, CFeeRate(3000), 0);
    CoinSet setCoinsRet;
    CAmount nValueRet;
    bool bnb_used;
    empty_wallet();
    add_coin(1);
    vCoins.at(0).nInputBytes = 40; // Make sure that it has a negative effective value. The next check should assert if this somehow got through. Otherwise it will fail
    BOOST_CHECK(!testWallet.SelectCoinsMinConf( 1 * CENT, filter_standard, GroupCoins(vCoins), setCoinsRet, nValueRet, coin_selection_params_bnb, bnb_used));
}

// Tests that with the ideal conditions, the coin selector will always be able to find a solution that can pay the target value
BOOST_AUTO_TEST_CASE(SelectCoins_test)
{
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
        CoinSelectionParams coin_selection_params_knapsack(false, 34, 148, CFeeRate(0), 0);
        CoinSelectionParams coin_selection_params_bnb(true, 34, 148, CFeeRate(0), 0);
        CoinSet out_set;
        CAmount out_value = 0;
        bool bnb_used = false;
        BOOST_CHECK(testWallet.SelectCoinsMinConf(target, filter_standard, GroupCoins(vCoins), out_set, out_value, coin_selection_params_bnb, bnb_used) ||
                    testWallet.SelectCoinsMinConf(target, filter_standard, GroupCoins(vCoins), out_set, out_value, coin_selection_params_knapsack, bnb_used));
        BOOST_CHECK_GE(out_value, target);
    }
}

BOOST_AUTO_TEST_SUITE_END()
