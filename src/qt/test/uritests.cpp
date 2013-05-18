#include "uritests.h"
#include "../guiutil.h"
#include "../walletmodel.h"

#include <QUrl>

void URITests::uriTests()
{
    SendCoinsRecipient rv;
    QUrl uri;
    uri.setUrl(QString("litecoin:LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9?req-dontexist="));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("litecoin:LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9?dontexist="));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("litecoin:LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9?label=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9"));
    QVERIFY(rv.label == QString("Wikipedia Example Address"));
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("litecoin:LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9?amount=0.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100000);

    uri.setUrl(QString("litecoin:LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9?amount=1.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100100000);

    uri.setUrl(QString("litecoin:LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9?amount=100&label=Wikipedia Example"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9"));
    QVERIFY(rv.amount == 10000000000LL);
    QVERIFY(rv.label == QString("Wikipedia Example"));

    uri.setUrl(QString("litecoin:LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9?message=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9"));
    QVERIFY(rv.label == QString());

    QVERIFY(GUIUtil::parseBitcoinURI("litecoin://LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9?message=Wikipedia Example Address", &rv));
    QVERIFY(rv.address == QString("LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9"));
    QVERIFY(rv.label == QString());

    // We currently don't implement the message parameter (ok, yea, we break spec...)
    uri.setUrl(QString("litecoin:LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9?req-message=Wikipedia Example Address"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("litecoin:LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9?amount=1,000&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("litecoin:LQDPC5rbjDB72fGFVHu4enYhxGAZuRiFh9?amount=1,000.0&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));
}
