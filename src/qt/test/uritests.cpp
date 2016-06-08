#include "uritests.h"

#include "guiutil.h"
#include "walletmodel.h"

#include <QUrl>

void URITests::uriTests()
{
    SendCoinsRecipient rv;
    QUrl uri;
    uri.setUrl(QString("crowncoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?req-dontexist="));
    QVERIFY(!GUIUtil::parseCrowncoinURI(uri, &rv));

    uri.setUrl(QString("crowncoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?dontexist="));
    QVERIFY(GUIUtil::parseCrowncoinURI(uri, &rv));
    QVERIFY(rv.recipient == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("crowncoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?label=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseCrowncoinURI(uri, &rv));
    QVERIFY(rv.recipient == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString("Wikipedia Example Address"));
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("crowncoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=0.001"));
    QVERIFY(GUIUtil::parseCrowncoinURI(uri, &rv));
    QVERIFY(rv.recipient == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100000);

    uri.setUrl(QString("crowncoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=1.001"));
    QVERIFY(GUIUtil::parseCrowncoinURI(uri, &rv));
    QVERIFY(rv.recipient == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100100000);

    uri.setUrl(QString("crowncoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=100&label=Wikipedia Example"));
    QVERIFY(GUIUtil::parseCrowncoinURI(uri, &rv));
    QVERIFY(rv.recipient == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.amount == 10000000000LL);
    QVERIFY(rv.label == QString("Wikipedia Example"));

    uri.setUrl(QString("crowncoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?message=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseCrowncoinURI(uri, &rv));
    QVERIFY(rv.recipient == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());

    QVERIFY(GUIUtil::parseCrowncoinURI("crowncoin://175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?message=Wikipedia Example Address", &rv));
    QVERIFY(rv.recipient == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());

    uri.setUrl(QString("crowncoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?req-message=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseCrowncoinURI(uri, &rv));

    uri.setUrl(QString("crowncoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=1,000&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseCrowncoinURI(uri, &rv));

    uri.setUrl(QString("crowncoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=1,000.0&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseCrowncoinURI(uri, &rv));
}
