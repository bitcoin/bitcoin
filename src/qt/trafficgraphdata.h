// Copyright (c) 2017-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TRAFFICGRAPHDATA_H
#define BITCOIN_QT_TRAFFICGRAPHDATA_H

#include <QHash>
#include <QQueue>

struct TrafficSample
{
    float in;
    float out;

    TrafficSample(float in, float out)
    {
        this->in = in;
        this->out = out;
    }

    TrafficSample operator+(const TrafficSample&  other) const
    {
        return TrafficSample(this->in + other.in, this->out + other.out);
    }

    TrafficSample& operator+=(const TrafficSample& rhs)
    {
          this->in += rhs.in;
          this->out += rhs.out;
          return *this;
    }
};

class TrafficGraphData
{
public:
    enum GraphRange
    {
        Range_5m,
        Range_10m,
        Range_15m,
        Range_30m,
        Range_1h,
        Range_2h,
        Range_3h,
        Range_6h,
        Range_12h,
        Range_24h,
    };

    static const int RangeMinutes[];
    static const int DESIRED_DATA_SAMPLES;
    static const int SMALLEST_SAMPLE_PERIOD;

    typedef QQueue<TrafficSample> SampleQueue;
    typedef QHash<GraphRange,SampleQueue> SampleQueueMap;

    TrafficGraphData(GraphRange range);
    bool update(const TrafficSample& trafficSample);
    bool update(quint64 totalBytesRecv, quint64 totalBytesSent);
    void switchRange(GraphRange newRange);
    SampleQueue getRangeQueue(GraphRange range);
    SampleQueue getCurrentRangeQueue();
    SampleQueue getCurrentRangeQueueWithAverageBandwidth();
    void clear();
    void setLastBytes(quint64 nLastBytesIn, quint64 nLastBytesOut);
    quint64 getLastBytesIn() { return nLastBytesIn; }
    quint64 getLastBytesOut() { return nLastBytesOut; }

private:
    static const int DesiredQueueSizes[];

    SampleQueueMap sampleMap;
    SampleQueueMap sampleStash;

    GraphRange currentGraphRange;
    int currentSampleCounter;

    quint64 nLastBytesIn;
    quint64 nLastBytesOut;

    void update(GraphRange range, const TrafficSample &trafficSample);

    void tryAddingSampleToStash(GraphRange range);
    void tryUpdateNextWithLast2Samples(GraphRange range, GraphRange nextRange);
    void tryUpdateNextWithLast3Samples(GraphRange range, GraphRange nextRange);

    SampleQueue sumEach2Samples(const SampleQueue &rangeQueue);
    SampleQueue sumEach3Samples(const SampleQueue &rangeQueue, GraphRange range);

    float convertSampleToBandwidth(float dataAmount);
    TrafficGraphData(const TrafficGraphData& that);
    TrafficGraphData& operator=(TrafficGraphData const&);
};

#endif // BITCOIN_QT_TRAFFICGRAPHDATA_H
