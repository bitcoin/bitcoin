#include <QTest>
#include <QObject>

#include "uritests.h"

// This is all you need to run all the tests
int main(int argc, char *argv[])
{
    URITests test1;
    QTest::qExec(&test1);
}
