// Copyright (c) 2012-2014 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet.h"
#include "walletdb.h"

#include <stdint.h>

#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

#ifdef ENABLE_WALLET
extern uint64_t bitcredit_nAccountingEntryNumber;
extern  Bitcredit_CDBEnv bitcredit_bitdb;
#endif

extern Bitcredit_CWallet* bitcredit_pwalletMain;

BOOST_AUTO_TEST_SUITE(accounting_tests)

static void
GetResults(Bitcredit_CWalletDB& walletdb, std::map<int64_t, CAccountingEntry>& results)
{
    std::list<CAccountingEntry> aes;

    results.clear();
    BOOST_CHECK(walletdb.ReorderTransactions(bitcredit_pwalletMain) == BITCREDIT_DB_LOAD_OK);
    walletdb.ListAccountCreditDebit("", aes);
    BOOST_FOREACH(CAccountingEntry& ae, aes)
    {
        results[ae.nOrderPos] = ae;
    }
}

BOOST_AUTO_TEST_CASE(acc_orderupgrade)
{
    Bitcredit_CWalletDB walletdb(bitcredit_pwalletMain->strWalletFile, &bitcredit_bitdb);
    std::vector<Bitcredit_CWalletTx*> vpwtx;
    Bitcredit_CWalletTx wtx;
    CAccountingEntry ae;
    std::map<int64_t, CAccountingEntry> results;

    LOCK(bitcredit_pwalletMain->cs_wallet);

    ae.strAccount = "";
    ae.nCreditDebit = 1;
    ae.nTime = 1333333333;
    ae.strOtherAccount = "b";
    ae.strComment = "";
    walletdb.WriteAccountingEntry(ae, bitcredit_nAccountingEntryNumber);

    wtx.mapValue["comment"] = "z";
    bitcredit_pwalletMain->AddToWallet(wtx);
    vpwtx.push_back(&bitcredit_pwalletMain->mapWallet[wtx.GetHash()]);
    vpwtx[0]->nTimeReceived = (unsigned int)1333333335;
    vpwtx[0]->nOrderPos = -1;

    ae.nTime = 1333333336;
    ae.strOtherAccount = "c";
    walletdb.WriteAccountingEntry(ae, bitcredit_nAccountingEntryNumber);

    GetResults(walletdb, results);

    BOOST_CHECK(bitcredit_pwalletMain->nOrderPosNext == 3);
    BOOST_CHECK(2 == results.size());
    BOOST_CHECK(results[0].nTime == 1333333333);
    BOOST_CHECK(results[0].strComment.empty());
    BOOST_CHECK(1 == vpwtx[0]->nOrderPos);
    BOOST_CHECK(results[2].nTime == 1333333336);
    BOOST_CHECK(results[2].strOtherAccount == "c");


    ae.nTime = 1333333330;
    ae.strOtherAccount = "d";
    ae.nOrderPos = bitcredit_pwalletMain->IncOrderPosNext();
    walletdb.WriteAccountingEntry(ae, bitcredit_nAccountingEntryNumber);

    GetResults(walletdb, results);

    BOOST_CHECK(results.size() == 3);
    BOOST_CHECK(bitcredit_pwalletMain->nOrderPosNext == 4);
    BOOST_CHECK(results[0].nTime == 1333333333);
    BOOST_CHECK(1 == vpwtx[0]->nOrderPos);
    BOOST_CHECK(results[2].nTime == 1333333336);
    BOOST_CHECK(results[3].nTime == 1333333330);
    BOOST_CHECK(results[3].strComment.empty());


    wtx.mapValue["comment"] = "y";
    --wtx.nLockTime;  // Just to change the hash :)
    bitcredit_pwalletMain->AddToWallet(wtx);
    vpwtx.push_back(&bitcredit_pwalletMain->mapWallet[wtx.GetHash()]);
    vpwtx[1]->nTimeReceived = (unsigned int)1333333336;

    wtx.mapValue["comment"] = "x";
    --wtx.nLockTime;  // Just to change the hash :)
    bitcredit_pwalletMain->AddToWallet(wtx);
    vpwtx.push_back(&bitcredit_pwalletMain->mapWallet[wtx.GetHash()]);
    vpwtx[2]->nTimeReceived = (unsigned int)1333333329;
    vpwtx[2]->nOrderPos = -1;

    GetResults(walletdb, results);

    BOOST_CHECK(results.size() == 3);
    BOOST_CHECK(bitcredit_pwalletMain->nOrderPosNext == 6);
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
    walletdb.WriteAccountingEntry(ae, bitcredit_nAccountingEntryNumber);

    GetResults(walletdb, results);

    BOOST_CHECK(results.size() == 4);
    BOOST_CHECK(bitcredit_pwalletMain->nOrderPosNext == 7);
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
