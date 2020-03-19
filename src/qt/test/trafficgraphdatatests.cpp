#include <qt/test/trafficgraphdatatests.h>
#include <qt/trafficgraphdata.h>
#include <algorithm>
#include <sstream>
#include <QTime>

void TrafficGraphDataTests::simpleCurrentSampleQueueTests()
{
    TrafficGraphData trafficGraphData(TrafficGraphData::Range_5m);
    for (int i = 0; i < TrafficGraphData::DESIRED_DATA_SAMPLES; i++)
        QVERIFY(trafficGraphData.update(TrafficSample(i, i)));

    TrafficGraphData::SampleQueue queue = trafficGraphData.getCurrentRangeQueue();
    QCOMPARE(queue.size(), TrafficGraphData::DESIRED_DATA_SAMPLES);
    for (int i = 0; i < TrafficGraphData::DESIRED_DATA_SAMPLES; i++){
        QCOMPARE((int)queue.at(i).in, TrafficGraphData::DESIRED_DATA_SAMPLES - i - 1);
        QCOMPARE((int)queue.at(i).out, TrafficGraphData::DESIRED_DATA_SAMPLES - i - 1);
    }

    QVERIFY(trafficGraphData.update(TrafficSample(0, 0)));
    queue = trafficGraphData.getCurrentRangeQueue();
    QCOMPARE(queue.size(), TrafficGraphData::DESIRED_DATA_SAMPLES);
    QCOMPARE((int)queue.at(0).in, 0);
    QCOMPARE((int)queue.at(0).out, 0);
}

namespace
{
void checkQueue(const TrafficGraphData::SampleQueue& queue, int value)
{
    for (int i = 0; i < queue.size(); i++){
        std::ostringstream oss;
        oss<< "i:" << i << " value:" << value << " actual:" << queue.at(i).in;
        QVERIFY2(value == (int)queue.at(i).in, oss.str().c_str());
        QVERIFY2(value == (int)queue.at(i).out, oss.str().c_str());
    }
}

void testQueueFill(TrafficGraphData::GraphRange range, int multiplier)
{
    int size = 10000;
    TrafficGraphData trafficGraphData(range);
    for (int i = 1; i <= size; i++){
        bool result = trafficGraphData.update(TrafficSample(1, 1));
        std::ostringstream oss;
        oss<< "result:" << result << " multiplier:" << multiplier << " i:" << i << " range:" << range;
        if (i == 1){
            if (range == TrafficGraphData::Range_5m)
                QVERIFY2(result, oss.str().c_str());
            else
                QVERIFY2(!result, oss.str().c_str());
        }
        else if (i % multiplier == 0){
            QVERIFY2(result, oss.str().c_str());
        }
        else {
            QVERIFY2(!result, oss.str().c_str());
        }
    }
    TrafficGraphData::SampleQueue queue = trafficGraphData.getCurrentRangeQueue();
    QCOMPARE(queue.size(), std::min(size / multiplier, TrafficGraphData::DESIRED_DATA_SAMPLES));
    checkQueue(queue, multiplier);
}
}

void TrafficGraphDataTests::accumulationCurrentSampleQueueTests()
{
    testQueueFill(TrafficGraphData::Range_10m, 2);
    testQueueFill(TrafficGraphData::Range_15m, 3);
    testQueueFill(TrafficGraphData::Range_30m, 6);
    testQueueFill(TrafficGraphData::Range_1h, 12);
    testQueueFill(TrafficGraphData::Range_2h, 24);
    testQueueFill(TrafficGraphData::Range_3h, 36);
    testQueueFill(TrafficGraphData::Range_6h, 72);
    testQueueFill(TrafficGraphData::Range_12h, 144);
    testQueueFill(TrafficGraphData::Range_24h, 288);
}

namespace
{
void checkRange(TrafficGraphData& trafficGraphData, int size, TrafficGraphData::GraphRange toRange, int multiplier)
{
    TrafficGraphData::SampleQueue queue = trafficGraphData.getRangeQueue(toRange);
    QCOMPARE(queue.size(), std::min(size / multiplier, TrafficGraphData::DESIRED_DATA_SAMPLES));
    checkQueue(queue,multiplier);
}

void testQueueFillAndCheckRangesForSize(int size)
{
    TrafficGraphData trafficGraphData(TrafficGraphData::Range_5m);
    for (int i = 1; i <= size; i++){
        trafficGraphData.update(TrafficSample(1, 1));
    }

    checkRange(trafficGraphData, size, TrafficGraphData::Range_10m, 2);
    checkRange(trafficGraphData, size, TrafficGraphData::Range_15m, 3);
    checkRange(trafficGraphData, size, TrafficGraphData::Range_30m, 6);
    checkRange(trafficGraphData, size, TrafficGraphData::Range_1h, 12);
    checkRange(trafficGraphData, size, TrafficGraphData::Range_2h, 24);
    checkRange(trafficGraphData, size, TrafficGraphData::Range_3h, 36);
    checkRange(trafficGraphData, size, TrafficGraphData::Range_6h, 72);
    checkRange(trafficGraphData, size, TrafficGraphData::Range_12h, 144);
    checkRange(trafficGraphData, size, TrafficGraphData::Range_24h, 288);
}
}

void TrafficGraphDataTests::getRangeTests()
{
    testQueueFillAndCheckRangesForSize(TrafficGraphData::DESIRED_DATA_SAMPLES);
    testQueueFillAndCheckRangesForSize(TrafficGraphData::DESIRED_DATA_SAMPLES * 2);
    testQueueFillAndCheckRangesForSize(TrafficGraphData::DESIRED_DATA_SAMPLES * 3);
    testQueueFillAndCheckRangesForSize(TrafficGraphData::DESIRED_DATA_SAMPLES * 6);
    testQueueFillAndCheckRangesForSize(TrafficGraphData::DESIRED_DATA_SAMPLES * 12);
    testQueueFillAndCheckRangesForSize(TrafficGraphData::DESIRED_DATA_SAMPLES * 24);
    testQueueFillAndCheckRangesForSize(TrafficGraphData::DESIRED_DATA_SAMPLES * 36);
    testQueueFillAndCheckRangesForSize(TrafficGraphData::DESIRED_DATA_SAMPLES * 72);
    testQueueFillAndCheckRangesForSize(TrafficGraphData::DESIRED_DATA_SAMPLES * 144);
    testQueueFillAndCheckRangesForSize(TrafficGraphData::DESIRED_DATA_SAMPLES * 288);
}

namespace
{
void compareQueues(const TrafficGraphData::SampleQueue& expected, const TrafficGraphData::SampleQueue& actual)
{
    QCOMPARE(expected.size(), actual.size());
    for (int i = 0; i < expected.size(); i++){
        std::ostringstream oss;
        oss<< "i:" << i << " expected:" << expected.at(i).in << " actual:" << actual.at(i).in;
        QVERIFY2((int)expected.at(i).in == (int)actual.at(i).in, oss.str().c_str());
        QVERIFY2((int)expected.at(i).out == (int)actual.at(i).out, oss.str().c_str());
    }
}

void testRangeSwitch(TrafficGraphData::GraphRange baseRange, TrafficGraphData::GraphRange toRange,int size)
{
    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());
    TrafficGraphData trafficGraphDataBase(baseRange);
    TrafficGraphData trafficGraphData(toRange);
    for (int i = 1; i <= size; i++){
        int in = qrand() % 1000;
        int out = qrand() % 1000;
        trafficGraphData.update(TrafficSample(in, out));
        trafficGraphDataBase.update(TrafficSample(in, out));
    }
    trafficGraphDataBase.switchRange(toRange);
    compareQueues(trafficGraphData.getCurrentRangeQueue(),trafficGraphDataBase.getCurrentRangeQueue());
}
}

void TrafficGraphDataTests::switchRangeTests()
{
    testRangeSwitch(TrafficGraphData::Range_5m, TrafficGraphData::Range_10m, 10000);
    testRangeSwitch(TrafficGraphData::Range_5m, TrafficGraphData::Range_30m, 20000);
    testRangeSwitch(TrafficGraphData::Range_5m, TrafficGraphData::Range_15m, 8000 * 2 - 1);
    testRangeSwitch(TrafficGraphData::Range_5m, TrafficGraphData::Range_24h, 8000 * 288 - 1);
}



void TrafficGraphDataTests::clearTests()
{
    TrafficGraphData trafficGraphData(TrafficGraphData::Range_5m);
    for (int i = 1; i <= TrafficGraphData::DESIRED_DATA_SAMPLES; i++){
        trafficGraphData.update(TrafficSample(1, 1));
    }
    QCOMPARE(trafficGraphData.getCurrentRangeQueue().size(),TrafficGraphData::DESIRED_DATA_SAMPLES);
    trafficGraphData.clear();
    QCOMPARE(trafficGraphData.getCurrentRangeQueue().size(), 0);
    for (int i = 1; i <= TrafficGraphData::DESIRED_DATA_SAMPLES; i++){
        trafficGraphData.update(TrafficSample(1, 1));
    }
    QCOMPARE(trafficGraphData.getCurrentRangeQueue().size(), TrafficGraphData::DESIRED_DATA_SAMPLES);
}

void TrafficGraphDataTests::averageBandwidthTest()
{
    TrafficGraphData trafficGraphData(TrafficGraphData::Range_5m);
    int step = 123; // in bytes
    float rate = step * TrafficGraphData::DESIRED_DATA_SAMPLES / (TrafficGraphData::RangeMinutes[TrafficGraphData::Range_5m] * 60.0f) / 1024.0f; // in KB/s
    quint64 totalBytesRecv = 0;
    quint64 totalBytesSent = 0;
    for (int i = 1; i <= TrafficGraphData::DESIRED_DATA_SAMPLES; i++){
        totalBytesRecv += step;
        totalBytesSent += step;
        trafficGraphData.update(totalBytesRecv, totalBytesSent);
    }

    // check rate at initial range
    QCOMPARE(trafficGraphData.getCurrentRangeQueueWithAverageBandwidth().size(), TrafficGraphData::DESIRED_DATA_SAMPLES);
    for (const auto& sample : trafficGraphData.getCurrentRangeQueueWithAverageBandwidth()){
        QCOMPARE(sample.in, rate);
        QCOMPARE(sample.out, rate);
    }

    // rate should be the same regardless of the current range
    trafficGraphData.switchRange(TrafficGraphData::Range_10m);

    QCOMPARE(trafficGraphData.getCurrentRangeQueueWithAverageBandwidth().size(), TrafficGraphData::DESIRED_DATA_SAMPLES / 2);
    for (const auto& sample : trafficGraphData.getCurrentRangeQueueWithAverageBandwidth()){
        QCOMPARE(sample.in, rate);
        QCOMPARE(sample.out, rate);
    }
}

void TrafficGraphDataTests::averageBandwidthEvery2EmptyTest()
{
    TrafficGraphData trafficGraphData(TrafficGraphData::Range_5m);
    int step = 123; // in bytes
    float rate = step * TrafficGraphData::DESIRED_DATA_SAMPLES / (TrafficGraphData::RangeMinutes[TrafficGraphData::Range_5m] * 60.0f) / 1024.0f / 2.0f; // in KB/s
    quint64 totalBytesRecv = 0;
    quint64 totalBytesSent = 0;
    for (int i = 1; i <= TrafficGraphData::DESIRED_DATA_SAMPLES; i++){
        if (i % 2 == 0) {
            totalBytesRecv += step;
            totalBytesSent += step;
        }
        trafficGraphData.update(totalBytesRecv, totalBytesSent);
    }

    // rate should average out at larger range
    trafficGraphData.switchRange(TrafficGraphData::Range_10m);

    QCOMPARE(trafficGraphData.getCurrentRangeQueueWithAverageBandwidth().size(), TrafficGraphData::DESIRED_DATA_SAMPLES / 2);
    for (const auto& sample : trafficGraphData.getCurrentRangeQueueWithAverageBandwidth()){
        QCOMPARE(sample.in, rate);
        QCOMPARE(sample.out, rate);
    }
}
