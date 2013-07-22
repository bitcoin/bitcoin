#include <QTest>
#include <QObject>

#include "uritests.h"
#include "paymentservertests.h"

// This is all you need to run all the tests
int main(int argc, char *argv[])
{
    bool fInvalid = false;

    URITests test1;
    if (QTest::qExec(&test1) != 0)
        fInvalid = true;

    PaymentServerTests test2;
    if (QTest::qExec(&test2) != 0)
        fInvalid = true;

    return fInvalid;
}
