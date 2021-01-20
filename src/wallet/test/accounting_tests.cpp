// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>
#include <validation.h> // for cs_main because of AddToWallet

#include <wallet/test/wallet_test_fixture.h>

#include <stdint.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(accounting_tests, WalletTestingSetup)

static void
GetResults(CWallet& wallet, std::map<CAmount, CAccountingEntry>& results)
{
    std::list<CAccountingEntry> aes;

    results.clear();
    BOOST_CHECK(wallet.ReorderTransactions() == DBErrors::LOAD_OK);
    wallet.ListAccountCreditDebit("", aes);
    for (CAccountingEntry& ae : aes)
    {
        results[ae.nOrderPos] = ae;
    }
}

BOOST_AUTO_TEST_CASE(acc_orderupgrade)
{
    std::vector<CWalletTx*> vpwtx;
    CWalletTx wtx(nullptr /* pwallet */, MakeTransactionRef());
    CAccountingEntry ae;
    std::map<CAmount, CAccountingEntry> results;

    LOCK2(cs_main, m_wallet.cs_wallet);

    ae.strAccount = "";
    ae.nCreditDebit = 1;
    ae.nTime = 1333333333;
    ae.strOtherAccount = "b";
    ae.strComment = "";
    m_wallet.AddAccountingEntry(ae);

    wtx.mapValue["comment"] = "z";
    m_wallet.AddToWallet(wtx);
    vpwtx.push_back(&m_wallet.mapWallet.at(wtx.GetHash()));
    vpwtx[0]->nTimeReceived = (unsigned int)1333333335;
    vpwtx[0]->nOrderPos = -1;

    ae.nTime = 1333333336;
    ae.strOtherAccount = "c";
    m_wallet.AddAccountingEntry(ae);

    GetResults(m_wallet, results);

    BOOST_CHECK(m_wallet.nOrderPosNext == 3);
    BOOST_CHECK(2 == results.size());
    BOOST_CHECK(results[0].nTime == 1333333333);
    BOOST_CHECK(results[0].strComment.empty());
    BOOST_CHECK(1 == vpwtx[0]->nOrderPos);
    BOOST_CHECK(results[2].nTime == 1333333336);
    BOOST_CHECK(results[2].strOtherAccount == "c");


    ae.nTime = 1333333330;
    ae.strOtherAccount = "d";
    ae.nOrderPos = m_wallet.IncOrderPosNext();
    m_wallet.AddAccountingEntry(ae);

    GetResults(m_wallet, results);

    BOOST_CHECK(results.size() == 3);
    BOOST_CHECK(m_wallet.nOrderPosNext == 4);
    BOOST_CHECK(results[0].nTime == 1333333333);
    BOOST_CHECK(1 == vpwtx[0]->nOrderPos);
    BOOST_CHECK(results[2].nTime == 1333333336);
    BOOST_CHECK(results[3].nTime == 1333333330);
    BOOST_CHECK(results[3].strComment.empty());


    wtx.mapValue["comment"] = "y";
    {
        CMutableTransaction tx(*wtx.tx);
        ++tx.nLockTime;  // Just to change the hash :)
        wtx.SetTx(MakeTransactionRef(std::move(tx)));
    }
    m_wallet.AddToWallet(wtx);
    vpwtx.push_back(&m_wallet.mapWallet.at(wtx.GetHash()));
    vpwtx[1]->nTimeReceived = (unsigned int)1333333336;

    wtx.mapValue["comment"] = "x";
    {
        CMutableTransaction tx(*wtx.tx);
        ++tx.nLockTime;  // Just to change the hash :)
        wtx.SetTx(MakeTransactionRef(std::move(tx)));
    }
    m_wallet.AddToWallet(wtx);
    vpwtx.push_back(&m_wallet.mapWallet.at(wtx.GetHash()));
    vpwtx[2]->nTimeReceived = (unsigned int)1333333329;
    vpwtx[2]->nOrderPos = -1;

    GetResults(m_wallet, results);

    BOOST_CHECK(results.size() == 3);
    BOOST_CHECK(m_wallet.nOrderPosNext == 6);
    BOOST_CHECK(0 == vpwtx[2]->nOrderPos);
    BOOST_CHECK(results[1].nTime == 1333333333);
    BOOST_CHECK(2 == vpwtx[0]->nOrderPos);
    BOOST_CHECK(results[3].nTime == 1333333336);
    BOOST_CHECK(results[4].nTime == 1333333330);
    BOOST_CHECK(results[4].strComment.empty());
    BOOST_CHECK(5 == vpwtx[1]->nOrderPos);


    ae.nTime = 1333333334;
    ae.strOtherAccount = "e";
    ae.nOrderPos = -1;
    m_wallet.AddAccountingEntry(ae);

    GetResults(m_wallet, results);

    BOOST_CHECK(results.size() == 4);
    BOOST_CHECK(m_wallet.nOrderPosNext == 7);
    BOOST_CHECK(0 == vpwtx[2]->nOrderPos);
    BOOST_CHECK(results[1].nTime == 1333333333);
    BOOST_CHECK(2 == vpwtx[0]->nOrderPos);
    BOOST_CHECK(results[3].nTime == 1333333336);
    BOOST_CHECK(results[3].strComment.empty());
    BOOST_CHECK(results[4].nTime == 1333333330);
    BOOST_CHECK(results[4].strComment.empty());
    BOOST_CHECK(results[5].nTime == 1333333334);
    BOOST_CHECK(6 == vpwtx[1]->nOrderPos);
}

BOOST_AUTO_TEST_SUITE_END()
