#include "paymentservertests.h"
#include "uritests.h"

#include <QCoreApplication>
#include <QObject>
#include <QTest>

// This is all you need to run all the tests
int main(int argc, char *argv[])
{
    bool fInvalid = false;

    // Don't remove this, it's needed to access
    // QCoreApplication:: in the tests
    QCoreApplication app(argc, argv);
    app.setApplicationName("Bitcoin-Qt-test");

    URITests test1;
    if (QTest::qExec(&test1) != 0)
        fInvalid = true;

    PaymentServerTests test2;
    if (QTest::qExec(&test2) != 0)
        fInvalid = true;

    return fInvalid;
}
