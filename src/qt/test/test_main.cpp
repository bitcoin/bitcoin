#include <QTest>
#include <QObject>

#include "urltests.h"

// This is all you need to run all the tests
int main(int argc, char *argv[])
{
    URLTests test1;
    QTest::qExec(&test1);
}
