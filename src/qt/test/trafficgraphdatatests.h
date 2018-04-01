#ifndef BITCOIN_QT_TEST_TRAFFICGRAPHDATATESTS_H
#define BITCOIN_QT_TEST_TRAFFICGRAPHDATATESTS_H

#include <QObject>
#include <QTest>

class TrafficGraphDataTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void simpleCurrentSampleQueueTests();
    void accumulationCurrentSampleQueueTests();
    void getRangeTests();
    void switchRangeTests();
    void clearTests();
    void averageBandwidthTest();
    void averageBandwidthEvery2EmptyTest();

};


#endif // BITCOIN_QT_TEST_TRAFFICGRAPHDATATESTS_H
