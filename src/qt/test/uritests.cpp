// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "uritests.h"

#include "guiutil.h"
#include "walletmodel.h"

#include <QUrl>

void URITests::uriTests()
{
    SendCoinsRecipient rv;
    QUrl uri;
    uri.setUrl(QString("iop:pALt2cJGyYomBRsERBndibwKpogg6Zn3eL?req-dontexist="));
    QVERIFY(!GUIUtil::parseIoPURI(uri, &rv));

    uri.setUrl(QString("iop:pALt2cJGyYomBRsERBndibwKpogg6Zn3eL?dontexist="));
    QVERIFY(GUIUtil::parseIoPURI(uri, &rv));
    QVERIFY(rv.address == QString("pALt2cJGyYomBRsERBndibwKpogg6Zn3eL"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("iop:pALt2cJGyYomBRsERBndibwKpogg6Zn3eL?label=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseIoPURI(uri, &rv));
    QVERIFY(rv.address == QString("pALt2cJGyYomBRsERBndibwKpogg6Zn3eL"));
    QVERIFY(rv.label == QString("Wikipedia Example Address"));
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("iop:pALt2cJGyYomBRsERBndibwKpogg6Zn3eL?amount=0.001"));
    QVERIFY(GUIUtil::parseIoPURI(uri, &rv));
    QVERIFY(rv.address == QString("pALt2cJGyYomBRsERBndibwKpogg6Zn3eL"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100000);

    uri.setUrl(QString("iop:pALt2cJGyYomBRsERBndibwKpogg6Zn3eL?amount=1.001"));
    QVERIFY(GUIUtil::parseIoPURI(uri, &rv));
    QVERIFY(rv.address == QString("pALt2cJGyYomBRsERBndibwKpogg6Zn3eL"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100100000);

    uri.setUrl(QString("iop:pALt2cJGyYomBRsERBndibwKpogg6Zn3eL?amount=100&label=Wikipedia Example"));
    QVERIFY(GUIUtil::parseIoPURI(uri, &rv));
    QVERIFY(rv.address == QString("pALt2cJGyYomBRsERBndibwKpogg6Zn3eL"));
    QVERIFY(rv.amount == 10000000000LL);
    QVERIFY(rv.label == QString("Wikipedia Example"));

    uri.setUrl(QString("iop:pALt2cJGyYomBRsERBndibwKpogg6Zn3eL?message=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseIoPURI(uri, &rv));
    QVERIFY(rv.address == QString("pALt2cJGyYomBRsERBndibwKpogg6Zn3eL"));
    QVERIFY(rv.label == QString());

    QVERIFY(GUIUtil::parseIoPURI("iop://pALt2cJGyYomBRsERBndibwKpogg6Zn3eL?message=Wikipedia Example Address", &rv));
    QVERIFY(rv.address == QString("pALt2cJGyYomBRsERBndibwKpogg6Zn3eL"));
    QVERIFY(rv.label == QString());

    uri.setUrl(QString("iop:pALt2cJGyYomBRsERBndibwKpogg6Zn3eL?req-message=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseIoPURI(uri, &rv));

    uri.setUrl(QString("iop:pALt2cJGyYomBRsERBndibwKpogg6Zn3eL?amount=1,000&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseIoPURI(uri, &rv));

    uri.setUrl(QString("iop:pALt2cJGyYomBRsERBndibwKpogg6Zn3eL?amount=1,000.0&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseIoPURI(uri, &rv));
}
