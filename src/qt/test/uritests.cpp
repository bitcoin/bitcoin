// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/test/uritests.h>

#include <qt/guiutil.h>
#include <qt/walletmodel.h>

#include <QUrl>

void URITests::uriTests()
{
    SendCoinsRecipient rv;
    QUrl uri;
    uri.setUrl(QString("chaincoin:CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU?req-dontexist="));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("chaincoin:CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU?dontexist="));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("chaincoin:CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU?label=Genesis Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU"));
    QVERIFY(rv.label == QString("Genesis Example Address"));
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("chaincoin:CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU?amount=0.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100000);

    uri.setUrl(QString("chaincoin:CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU?amount=1.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100100000);

    uri.setUrl(QString("chaincoin:CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU?amount=100&label=Genesis Example"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU"));
    QVERIFY(rv.amount == 10000000000LL);
    QVERIFY(rv.label == QString("Genesis Example"));

    uri.setUrl(QString("chaincoin:CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU?message=Genesis Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU"));
    QVERIFY(rv.label == QString());

    QVERIFY(GUIUtil::parseBitcoinURI("bitcoin:CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU?message=Genesis Example Address", &rv));
    QVERIFY(rv.address == QString("CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU"));
    QVERIFY(rv.label == QString());

    uri.setUrl(QString("chaincoin:CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU?req-message=Genesis Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("chaincoin:CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU?amount=1,000&label=Genesis Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("chaincoin:CMZCCTuDipQrPrxwJSxwagGfcyDcteaTUU?amount=1,000.0&label=Genesis Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));
}
