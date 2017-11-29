#include "uritests.h"
#include "../guiutil.h"
#include "../walletmodel.h"

#include <QUrl>

void URITests::uriTests()
{
    SendCoinsRecipient rv;
    QUrl uri;
    uri.setUrl(QString("peercoin:PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j?req-dontexist="));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("peercoin:PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j?dontexist="));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("peercoin:PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j?label=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j"));
    QVERIFY(rv.label == QString("Wikipedia Example Address"));
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("peercoin:PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j?amount=0.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 1000);

    uri.setUrl(QString("peercoin:PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j?amount=1.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 1001000);

    uri.setUrl(QString("peercoin:PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j?amount=10000&label=Wikipedia Example"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j"));
    QVERIFY(rv.amount == 10000000000LL);
    QVERIFY(rv.label == QString("Wikipedia Example"));

    uri.setUrl(QString("peercoin:PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j?message=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j"));
    QVERIFY(rv.label == QString());

    QVERIFY(GUIUtil::parseBitcoinURI("peercoin://PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j?message=Wikipedia Example Address", &rv));
    QVERIFY(rv.address == QString("PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j"));
    QVERIFY(rv.label == QString());

    // We currently don't implement the message parameter (ok, yea, we break spec...)
    uri.setUrl(QString("peercoin:PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j?req-message=Wikipedia Example Address"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("peercoin:PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j?amount=1,000&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("peercoin:PAbnnK7dJ3zbaqML1fFqcBbNAjCgg4CK7j?amount=1,000.0&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));
}
