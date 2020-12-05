#include <qt/trafficgraphdata.h>

const int TrafficGraphData::RangeMinutes[] = {5,10,15,30,60,120,180,360,720,1440};
const int TrafficGraphData::DESIRED_DATA_SAMPLES = TrafficGraphData::RangeMinutes[TrafficGraphData::Range_5m] * 60; // i.e. one data sample per second for Range_5m
const int TrafficGraphData::DesiredQueueSizes[] = {
        TrafficGraphData::DESIRED_DATA_SAMPLES,     //Range_5m
        TrafficGraphData::DESIRED_DATA_SAMPLES/2,   //Range_10m
        TrafficGraphData::DESIRED_DATA_SAMPLES*2/3, //Range_15m
        TrafficGraphData::DESIRED_DATA_SAMPLES/2,   //Range_30m
        TrafficGraphData::DESIRED_DATA_SAMPLES/2,   //Range_1h
        TrafficGraphData::DESIRED_DATA_SAMPLES/2,   //Range_2h
        TrafficGraphData::DESIRED_DATA_SAMPLES*2/3, //Range_3h
        TrafficGraphData::DESIRED_DATA_SAMPLES/2,   //Range_6h
        TrafficGraphData::DESIRED_DATA_SAMPLES/2,   //Range_12h
        TrafficGraphData::DESIRED_DATA_SAMPLES/2,   //Range_24h
    } ;

const int TrafficGraphData::SMALLEST_SAMPLE_PERIOD =
        TrafficGraphData::RangeMinutes[TrafficGraphData::Range_5m] * 60 * 1000 / TrafficGraphData::DESIRED_DATA_SAMPLES;

TrafficGraphData::TrafficGraphData(GraphRange range)
    :currentGraphRange(range),
    currentSampleCounter(0),
    nLastBytesIn(0),
    nLastBytesOut(0)
{
}

void TrafficGraphData::tryAddingSampleToStash(GraphRange range)
{
    SampleQueue& queue = sampleMap[range];
    if (queue.size() > DesiredQueueSizes[range]) {
       sampleStash[range].push_front(queue.at(DesiredQueueSizes[range]));
    }
}

void TrafficGraphData::tryUpdateNextWithLast2Samples(GraphRange range, GraphRange nextRange)
{
    SampleQueue& queue = sampleMap[range];
    if (queue.size() == DesiredQueueSizes[range] + 2) {
        update(nextRange, queue.takeLast() + queue.takeLast());
    }
}

void TrafficGraphData::tryUpdateNextWithLast3Samples(GraphRange range, GraphRange nextRange)
{
    SampleQueue& stashQueue = sampleStash[range];
    if (stashQueue.size() == 3) {
        update(nextRange, stashQueue.takeLast() + stashQueue.takeLast() + stashQueue.takeLast());
    }
}

void TrafficGraphData::setLastBytes(quint64 nLastBytesIn, quint64 nLastBytesOut)
{
    this->nLastBytesIn = nLastBytesIn;
    this->nLastBytesOut = nLastBytesOut;
}

bool TrafficGraphData::update(quint64 totalBytesRecv, quint64 totalBytesSent)
{
    float inRate = totalBytesRecv - nLastBytesIn;
    float outRate = totalBytesSent - nLastBytesOut;
    nLastBytesIn = totalBytesRecv;
    nLastBytesOut = totalBytesSent;
    return update(TrafficSample(inRate, outRate));
}

bool TrafficGraphData::update(const TrafficSample& trafficSample)
{
    update(Range_5m, trafficSample);

    currentSampleCounter++;

    if (RangeMinutes[currentGraphRange] / RangeMinutes[Range_5m] == currentSampleCounter) {
        currentSampleCounter = 0;
        return true;
    }
    return false;
}

void TrafficGraphData::update(GraphRange range, const TrafficSample& trafficSample)
{
    SampleQueue& queue = sampleMap[range];
    queue.push_front(trafficSample);

    switch(range) {
        case Range_5m: {
            tryAddingSampleToStash(Range_5m);
            tryUpdateNextWithLast2Samples(Range_5m, Range_10m);
            tryUpdateNextWithLast3Samples(Range_5m, Range_15m);
            return;
        }
        case Range_15m: {
            tryUpdateNextWithLast2Samples(Range_15m, Range_30m);
            return;
        }
        case Range_30m: {
            tryUpdateNextWithLast2Samples(Range_30m, Range_1h);
            return;
        }
        case Range_1h: {
            tryAddingSampleToStash(Range_1h);
            tryUpdateNextWithLast2Samples(Range_1h, Range_2h);
            tryUpdateNextWithLast3Samples(Range_1h, Range_3h);
            return;
        }
        case Range_3h: {
            tryUpdateNextWithLast2Samples(Range_3h, Range_6h);
            return;
        }
        case Range_6h: {
            tryUpdateNextWithLast2Samples( Range_6h, Range_12h);
            return;
        }
        case Range_12h: {
            tryUpdateNextWithLast2Samples(Range_12h, Range_24h);
            return;
        }
        default: {
            if (queue.size() > DesiredQueueSizes[range]) {
                queue.removeLast();
            }
            return;
        }
    }
}

void TrafficGraphData::switchRange(GraphRange newRange)
{
    currentGraphRange = newRange;
    currentSampleCounter = 0;
}

TrafficGraphData::SampleQueue TrafficGraphData::sumEach2Samples(const SampleQueue& rangeQueue)
{
    SampleQueue result;
    int i = rangeQueue.size() - 1;

    while (i - 1 >= 0) {
        result.push_front(rangeQueue.at(i) + rangeQueue.at(i - 1));
        i -= 2;
    }
    return result;
}

TrafficGraphData::SampleQueue TrafficGraphData::sumEach3Samples(const SampleQueue& rangeQueue, GraphRange range)
{
    SampleQueue result;
    int lastUnusedSample = std::min(rangeQueue.size() - 1, DESIRED_DATA_SAMPLES - 1);

    // use stash first
    SampleQueue& stashQueue = sampleStash[range];
    TrafficSample sum(0, 0);
    for (int i = 0; i < stashQueue.size(); ++i) {
       sum += stashQueue.at(i);
    }
    int toFullSample = 3 - stashQueue.size();
    if (toFullSample > rangeQueue.size())
        return result;

    // append to stash data to create whole sample
    for (int i = 0; i < toFullSample; ++i) {
        sum += rangeQueue.at(lastUnusedSample);
        --lastUnusedSample;
    }
    result.push_front(sum);

    while (lastUnusedSample - 2 >= 0) {
        result.push_front(rangeQueue.at(lastUnusedSample) + rangeQueue.at(lastUnusedSample - 1) + rangeQueue.at(lastUnusedSample - 2));
        lastUnusedSample -= 3;
    }
    return result;
}

TrafficGraphData::SampleQueue TrafficGraphData::getRangeQueue(GraphRange range)
{
    switch(range) {
        case Range_5m: {
            return sampleMap[Range_5m];
        }
        case Range_10m: {
            SampleQueue queue = sumEach2Samples(getRangeQueue(Range_5m));
            queue.append(sampleMap[Range_10m]);
            return queue;
        }
        case Range_15m: {
            SampleQueue queue = sumEach3Samples(getRangeQueue(Range_5m), Range_5m);
            queue.append(sampleMap[Range_15m]);
            return queue;
        }
        case Range_30m: {
            SampleQueue queue = sumEach2Samples(getRangeQueue(Range_15m));
            queue.append(sampleMap[Range_30m]);
            return queue;
        }
        case Range_1h: {
            SampleQueue queue = sumEach2Samples(getRangeQueue(Range_30m));
            queue.append(sampleMap[Range_1h]);
            return queue;
        }
        case Range_2h: {
            SampleQueue queue = sumEach2Samples(getRangeQueue(Range_1h));
            queue.append(sampleMap[Range_2h]);
            return queue;
        }
        case Range_3h: {
            SampleQueue queue = sumEach3Samples(getRangeQueue(Range_1h),Range_1h);
            queue.append(sampleMap[Range_3h]);
            return queue;
        }
        case Range_6h: {
            SampleQueue queue = sumEach2Samples(getRangeQueue(Range_3h));
            queue.append(sampleMap[Range_6h]);
            return queue;
        }
        case Range_12h: {
            SampleQueue queue = sumEach2Samples(getRangeQueue(Range_6h));
            queue.append(sampleMap[Range_12h]);
            return queue;
        }
        case Range_24h: {
            SampleQueue queue = sumEach2Samples(getRangeQueue(Range_12h));
            queue.append(sampleMap[Range_24h]);
            return queue;
        }
        default: {
            return SampleQueue();
        }
    }
}

TrafficGraphData::SampleQueue TrafficGraphData::getCurrentRangeQueue()
{
    SampleQueue newQueue;
    getRangeQueue(currentGraphRange).mid(0, DESIRED_DATA_SAMPLES).swap(newQueue);
    return newQueue;
}

float TrafficGraphData::convertSampleToBandwidth(float dataAmount)
{
    // to base range
    float result  = dataAmount / RangeMinutes[currentGraphRange] * RangeMinutes[TrafficGraphData::Range_5m];
    // to B/s
    result = result * 1000 / SMALLEST_SAMPLE_PERIOD;
    // to KB/s
    result = result / 1024;

    return result;

}

TrafficGraphData::SampleQueue TrafficGraphData::getCurrentRangeQueueWithAverageBandwidth()
{
    SampleQueue newQueue;
    getRangeQueue(currentGraphRange).mid(0, DESIRED_DATA_SAMPLES).swap(newQueue);
    for (auto& sample : newQueue){
        sample.in = convertSampleToBandwidth(sample.in);
        sample.out = convertSampleToBandwidth(sample.out);
    }
    return newQueue;
}

void TrafficGraphData::clear()
{
    sampleMap.clear();
    sampleMap.clear();
    currentSampleCounter = 0;
    nLastBytesIn = 0;
    nLastBytesOut = 0;
}
