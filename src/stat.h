// Copyright (c) 2016-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STAT_H
#define STAT_H

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <chrono>
// c++11 #include <type_traits>
#include "univalue/include/univalue.h"

#include "sync.h"

class CStatBase;

extern CCriticalSection cs_statMap;

enum StatOperation
{
    STAT_OP_SUM = 1,
    STAT_OP_AVE = 2,
    STAT_OP_MAX = 4,
    STAT_OP_MIN = 8,
    STAT_KEEP = 0x10, // Do not clear the value when it it moved into history
    STAT_KEEP_COUNT = 0x20, // do not reset the sample count when it it moved into history
    STAT_INDIVIDUAL = 0x40, // Every sample is a data point, do not aggregate by time
};

// typedef boost::reference_wrapper<std::string> CStatKey;
typedef std::string CStatKey;
typedef std::map<CStatKey, CStatBase *> CStatMap;

extern CStatMap statistics;
extern boost::asio::io_service stat_io_service;
extern std::chrono::milliseconds statMinInterval;

template <typename NUM>
void statAverage(NUM &tally, const NUM &cur, unsigned int sampleCounts)
{
    tally = ((tally * ((NUM)sampleCounts - 1)) + cur) / sampleCounts;
}

template void statAverage<uint16_t>(uint16_t &tally, const uint16_t &cur, unsigned int sampleCounts);
template void statAverage<unsigned int>(unsigned int &tally, const unsigned int &cur, unsigned int sampleCounts);
template void statAverage<uint64_t>(uint64_t &tally, const uint64_t &cur, unsigned int sampleCounts);
template void statAverage<int16_t>(int16_t &tally, const int16_t &cur, unsigned int sampleCounts);
template void statAverage<int>(int &tally, const int &cur, unsigned int sampleCounts);
template void statAverage<int64_t>(int64_t &tally, const int64_t &cur, unsigned int sampleCounts);
template void statAverage<float>(float &tally, const float &cur, unsigned int sampleCounts);
template void statAverage<double>(double &tally, const double &cur, unsigned int sampleCounts);
// template void statAverage<ZZ>(ZZ& tally,const ZZ& cur,unsigned int sampleCounts);

template <typename NUM>
void statReset(NUM &tally, uint64_t flags)
{
    if (!(flags & STAT_KEEP))
        tally = NUM();
}


class CStatBase
{
public:
    CStatBase(){};
    virtual ~CStatBase(){};
    virtual UniValue GetNow() = 0; // Returns the current value of this statistic
    virtual UniValue GetTotal() = 0; // Returns the cumulative value of this statistic
    virtual UniValue GetSeries(const std::string &name, int count) = 0; // Returns the historical or series data
    // Returns the historical or series data along with timestamp
    virtual UniValue GetSeriesTime(const std::string &name, int count) = 0;
};

template <class DataType, class RecordType = DataType>
class CStat : public CStatBase
{
public:
protected:
    RecordType value;
    std::string name;

public:
    CStat()
    {
        value = RecordType(); // = 0;
    }
    CStat(const char *namep) : name(namep)
    {
        LOCK(cs_statMap);
        value = RecordType(); // = 0;
        statistics[CStatKey(name)] = this;
    }
    CStat(const std::string &namep) : name(namep)
    {
        LOCK(cs_statMap);
        value = RecordType(); // = 0;
        statistics[CStatKey(name)] = this;
    }

    void init(const char *namep)
    {
        LOCK(cs_statMap);
        name = namep;
        value = RecordType(); // = 0;
        statistics[CStatKey(name)] = this;
    }

    void init(const std::string &namep)
    {
        LOCK(cs_statMap);
        name = namep;
        value = RecordType(); // = 0;
        statistics[CStatKey(name)] = this;
    }

    void cleanup()
    {
        LOCK(cs_statMap);
        statistics.erase(CStatKey(name));
        name.clear();
    }

    CStat &operator=(const DataType &arg)
    {
        value = arg;
        return *this;
    }

    CStat &operator+=(const DataType &rhs)
    {
        value += rhs;
        return *this;
    }
    CStat &operator-=(const DataType &rhs)
    {
        value -= rhs;
        return *this;
    }

    RecordType &operator()() { return value; }
    virtual UniValue GetNow() { return UniValue(value); }
    virtual UniValue GetTotal() { return NullUniValue; }
    virtual UniValue GetSeries(const std::string &name, int count)
    {
        return NullUniValue; // Has no series data
    }
    virtual UniValue GetSeriesTime(const std::string &name, int count)
    {
        return NullUniValue; // Has no series data
    }

    virtual ~CStat()
    {
        LOCK(cs_statMap);
        if (name.size())
        {
            statistics.erase(CStatKey(name));
            name.clear();
        }
    }
};


extern const char *sampleNames[];
extern int operateSampleCount[]; // Even though there may be 1000 samples, it takes this many samples to produce an
// element in the next series.
extern int interruptIntervals[]; // When to calculate the next series, in multiples of the interrupt time.

// accumulate(accumulator,datapt);


enum
{
    STATISTICS_NUM_RANGES = 5,
    STATISTICS_SAMPLES = 300,
};

template <class DataType, class RecordType = DataType>
class CStatHistory : public CStat<DataType, RecordType>
{
protected:
    unsigned int op;
    boost::asio::steady_timer timer;
    RecordType history[STATISTICS_NUM_RANGES][STATISTICS_SAMPLES];
    int64_t historyTime[STATISTICS_NUM_RANGES][STATISTICS_SAMPLES];
    int loc[STATISTICS_NUM_RANGES];
    int len[STATISTICS_NUM_RANGES];
    uint64_t timerCount;
    std::chrono::steady_clock::time_point timerStartSteady;
    unsigned int sampleCount;
    RecordType total;

public:
    CStatHistory() : CStat<DataType, RecordType>(), op(STAT_OP_SUM | STAT_KEEP_COUNT), timer(stat_io_service)
    {
        Clear(false);
    }
    CStatHistory(const char *name, unsigned int operation = STAT_OP_SUM)
        : CStat<DataType, RecordType>(name), op(operation), timer(stat_io_service)
    {
        Clear();
    }

    CStatHistory(const std::string &name, unsigned int operation = STAT_OP_SUM)
        : CStat<DataType, RecordType>(name), op(operation), timer(stat_io_service)
    {
        Clear();
    }

    void init(const char *name, unsigned int operation = STAT_OP_SUM)
    {
        CStat<DataType, RecordType>::init(name);
        op = operation;
        Clear();
    }

    void init(const std::string &name, unsigned int operation = STAT_OP_SUM)
    {
        CStat<DataType, RecordType>::init(name);
        op = operation;
        Clear();
    }

    void Clear(bool fStart = true)
    {
        timerCount = 0;
        sampleCount = 0;
        for (int i = 0; i < STATISTICS_NUM_RANGES; i++)
            loc[i] = 0;
        for (int i = 0; i < STATISTICS_NUM_RANGES; i++)
            len[i] = 0;
        for (int i = 0; i < STATISTICS_NUM_RANGES; i++)
            for (int j = 0; j < STATISTICS_SAMPLES; j++)
            {
                history[i][j] = RecordType();
                historyTime[i][j] = 0;
            }
        total = RecordType();
        this->value = RecordType();

        if (fStart)
            Start();
    }

    virtual ~CStatHistory() { Stop(); }
    CStatHistory &operator<<(const DataType &rhs)
    {
        if (op & STAT_INDIVIDUAL)
            timeout(boost::system::error_code()); // If each call is an individual datapoint, simulate a timeout every
        // time data arrives to advance.
        if (op & STAT_OP_SUM)
        {
            this->value += rhs;
            // this->total += rhs;  // Updating total when timer fires
        }
        else if (op & STAT_OP_AVE)
        {
            unsigned int tmp = ++sampleCount;
            if (tmp == 0)
                tmp = 1;
            statAverage(this->value, rhs, tmp);
            //++totalSamples;
            // statAverage(this->total,rhs,totalSamples);
        }
        else if (op & STAT_OP_MAX)
        {
            if (this->value < rhs)
                this->value = rhs;
            // if (this->total < rhs) this->total = rhs;
        }
        else if (op & STAT_OP_MIN)
        {
            if (this->value > rhs)
                this->value = rhs;
            // if (this->total > rhs) this->total = rhs;
        }
        return *this;
    }

    void Start()
    {
        if (!(op & STAT_INDIVIDUAL))
        {
            timerStartSteady = std::chrono::steady_clock::now();
            timerCount = 0;
            wait();
        }
    }

    void Stop()
    {
        if (!(op & STAT_INDIVIDUAL))
        {
            timer.cancel();
        }
    }

    int Series(int series, DataType *array, int len)
    {
        assert(series < STATISTICS_NUM_RANGES);
        if (len > STATISTICS_SAMPLES)
            len = STATISTICS_SAMPLES;

        int pos = loc[series] - STATISTICS_SAMPLES;
        if (pos < 0)
            pos += STATISTICS_SAMPLES;
        for (int i = 0; i < len; i++, pos++) // could be a lot more efficient with 2 memcpy
        {
            if (pos >= STATISTICS_SAMPLES)
                pos -= STATISTICS_SAMPLES;
            array[i] = history[series][pos];
        }

        return len;
    }

    virtual UniValue GetTotal()
    {
        if ((op & STAT_OP_AVE) && (timerCount != 0))
            return UniValue(
                total / timerCount); // If the metric is an average, calculate the average before returning it
        return UniValue(total);
    }

    virtual UniValue GetSeries(const std::string &name, int count)
    {
        for (int series = 0; series < STATISTICS_NUM_RANGES; series++)
        {
            if (name == sampleNames[series])
            {
                UniValue ret(UniValue::VARR);
                if (count < 0)
                    count = 0;
                if (count > len[series])
                    count = len[series];
                for (int i = -1 * (count - 1); i <= 0; i++)
                {
                    const RecordType &sample = History(series, i);
                    ret.push_back((UniValue)sample);
                }
                return ret;
            }
        }
        return NullUniValue; // No series of this name
    }

    // 0 is latest, then pass a negative number for prior
    const RecordType &History(int series, int ago)
    {
        assert(ago <= 0);
        assert(series < STATISTICS_NUM_RANGES);
        assert(-1 * ago <= STATISTICS_SAMPLES);
        int pos = loc[series] - 1 + ago;
        if (pos < 0)
            pos += STATISTICS_SAMPLES;
        assert(pos < STATISTICS_SAMPLES);
        return history[series][pos];
    }

    virtual UniValue GetSeriesTime(const std::string &name, int count)
    {
        for (int series = 0; series < STATISTICS_NUM_RANGES; series++)
        {
            if (name == sampleNames[series])
            {
                UniValue data(UniValue::VARR);
                UniValue times(UniValue::VARR);
                UniValue ret(UniValue::VARR);
                if (count < 0)
                    count = 0;
                if (count > len[series])
                    count = len[series];
                for (int i = -1 * (count - 1); i <= 0; i++)
                {
                    const RecordType &sample = History(series, i);
                    const int64_t &sample_time = HistoryTime(series, i);
                    data.push_back((UniValue)sample);
                    times.push_back((UniValue)sample_time);
                }
                ret.push_back(data);
                ret.push_back(times);
                return ret;
            }
        }
        return NullUniValue; // No series of this name
    }

    // 0 is latest, then pass a negative number for prior
    const int64_t &HistoryTime(int series, int ago)
    {
        assert(ago <= 0);
        assert(series < STATISTICS_NUM_RANGES);
        assert(-1 * ago <= STATISTICS_SAMPLES);
        int pos = loc[series] - 1 + ago;
        if (pos < 0)
            pos += STATISTICS_SAMPLES;
        assert(pos < STATISTICS_SAMPLES);
        assert(series >= 0);
        assert(pos >= 0);
        return historyTime[series][pos];
    }

    void timeout(const boost::system::error_code &e)
    {
        if (e == boost::asio::error::operation_aborted)
        {
            return;
        }
        if (e)
            return;

        // To avoid taking a mutex, I sample and compare.  This sort of thing isn't perfect but acceptable for
        // statistics calc.
        volatile RecordType *sampler = &this->value;
        RecordType samples[2];
        do
        {
            samples[0] = *sampler;
            samples[1] = *sampler;
        } while (samples[0] != samples[1]);

        statReset(this->value, op);
        if ((op & STAT_KEEP_COUNT) == 0)
            sampleCount = 0;

        int64_t cur_time = GetTimeMillis();
        assert(loc[0] < STATISTICS_SAMPLES);
        assert(loc[0] >= 0);
        history[0][loc[0]] = samples[0];
        historyTime[0][loc[0]] = cur_time;
        loc[0]++;
        len[0]++;
        if (loc[0] >= STATISTICS_SAMPLES)
            loc[0] = 0;
        if (len[0] >= STATISTICS_SAMPLES)
            len[0] = STATISTICS_SAMPLES; // full

        timerCount++;

        // Update the "total" count
        if ((op & STAT_OP_SUM) || (op & STAT_OP_AVE))
            total += samples[0];
        else if (op & STAT_OP_MAX)
        {
            if (total < samples[0])
                total = samples[0];
        }
        else if (op & STAT_OP_MIN)
        {
            if (total > samples[0])
                total = samples[0];
        }

        // flow the samples if its time
        for (int i = 0; i < STATISTICS_NUM_RANGES - 1; i++)
        {
            if ((timerCount % interruptIntervals[i]) == 0)
            {
                int start = loc[i];
                RecordType accumulator = RecordType();
                // First time in the loop we need to assign
                start--;
                if (start < 0)
                    start += STATISTICS_SAMPLES; // Wrap around
                accumulator = history[i][start];
                // subsequent times we combine as per the operation
                for (int j = 1; j < operateSampleCount[i]; j++)
                {
                    start--;
                    if (start < 0)
                        start += STATISTICS_SAMPLES; // Wrap around
                    RecordType datapt = history[i][start];
                    if ((op & STAT_OP_SUM) || (op & STAT_OP_AVE))
                        accumulator += datapt;
                    else if (op & STAT_OP_MAX)
                    {
                        if (accumulator < datapt)
                            accumulator = datapt;
                    }
                    else if (op & STAT_OP_MIN)
                    {
                        if (accumulator > datapt)
                            accumulator = datapt;
                    }
                }
                // All done accumulating.  Now store the data in the proper history field -- its going in the next

                // series.
                if (op & STAT_OP_AVE)
                    accumulator /= ((DataType)operateSampleCount[i]);
                assert(i + 1 < STATISTICS_NUM_RANGES);
                assert(loc[i + 1] < STATISTICS_SAMPLES);
                assert(start < STATISTICS_SAMPLES);
                assert(start >= 0);
                assert(loc[i + 1] >= 0);
                history[i + 1][loc[i + 1]] = accumulator;
                // times for accumulated statistics indicate the beginning of the interval
                historyTime[i + 1][loc[i + 1]] = historyTime[i][start];
                loc[i + 1]++;
                len[i + 1]++;
                if (loc[i + 1] >= STATISTICS_SAMPLES)
                    loc[i + 1] = 0; // Wrap around
                if (len[i + 1] >= STATISTICS_SAMPLES)
                    len[i + 1] = STATISTICS_SAMPLES; // full
            }
        }
        if (!(op & STAT_INDIVIDUAL))
            wait();
    }

protected:
    void wait()
    {
        // to account for drift we keep checking back against the start time.
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        auto next = std::chrono::milliseconds((timerCount + 1) * statMinInterval) + timerStartSteady - now;
        timer.expires_from_now(next);
        timer.async_wait(boost::bind(&CStatHistory::timeout, this, boost::asio::placeholders::error));
    }
};


template <class NUM>
class MinValMax
{
public:
    NUM min;
    NUM val;
    NUM max;
    int samples;
    MinValMax() : min(std::numeric_limits<NUM>::max()), val(0), max(std::numeric_limits<NUM>::min()), samples(0) {}
    MinValMax &operator=(const MinValMax &rhs)
    {
        min = rhs.min;
        val = rhs.val;
        max = rhs.max;
        samples = rhs.samples;
        return *this;
    }

    MinValMax &operator=(const volatile MinValMax &rhs)
    {
        min = rhs.min;
        val = rhs.val;
        max = rhs.max;
        samples = rhs.samples;
        return *this;
    }

    bool operator!=(const MinValMax &rhs) const { return !(*this == rhs); }
    bool operator==(const MinValMax &rhs) const
    {
        if (min != rhs.min)
            return false;
        if (val != rhs.val)
            return false;
        if (max != rhs.max)
            return false;
        if (samples != rhs.samples)
            return false;
        return true;
    }

    MinValMax &operator=(const NUM &rhs)
    {
        if (max < rhs)
            max = rhs;
        if (min > rhs)
            min = rhs;
        val = rhs;
        samples++;
        return *this;
    }


    // Probably not meaningful just here to meet the template req
    bool operator>(const MinValMax &rhs) const { return (max > rhs.max); }
    // Probably not meaningful just here to meet the template req
    bool operator<(const MinValMax &rhs) const { return (min > rhs.min); }
    bool operator>(const NUM &rhs) const { return (val > rhs); }
    bool operator<(const NUM &rhs) const { return (val < rhs); }
    // happens when users adds a stat to the system
    MinValMax &operator+=(const NUM &rhs)
    {
        val += rhs;
        if (max < val)
            max = val;
        if (min > val)
            min = val;
        samples++;
        return *this;
    }
    // happens when users adds a stat to the system
    MinValMax &operator-=(const NUM &rhs)
    {
        val -= rhs;
        if (max < rhs)
            max = val;
        if (min > rhs)
            min = val;
        samples++;
        return *this;
    }

    // happens when results are moved from a faster series to a slower one.
    MinValMax &operator+=(const MinValMax &rhs)
    {
        if (rhs.max > max)
            max = rhs.max;
        if (rhs.min < min)
            min = rhs.min;
        //      max += rhs.max;
        //      min += rhs.min;
        val += rhs.val;
        samples += rhs.samples;
        return *this;
    }

    NUM operator/(const NUM &rhs) { return val / rhs; }
    // used in the averaging
    MinValMax &operator/=(const NUM &rhs)
    {
        val /= rhs;
        //      min /= rhs;
        //      max /= rhs;
        return *this;
    }

    operator UniValue() const
    {
        UniValue ret(UniValue::VOBJ);
        ret.push_back(Pair("min", (UniValue)min));
        ret.push_back(Pair("val", (UniValue)val));
        ret.push_back(Pair("max", (UniValue)max));
        return ret;
    }
};

template <typename NUM>
void statAverage(MinValMax<NUM> &tally, const NUM &cur, unsigned int sampleCounts)
{
    statAverage(tally.val, cur, sampleCounts);
    if (cur > tally.max)
        tally.max = cur;
    if (cur < tally.min)
        tally.min = cur;
}

template <typename NUM>
void statReset(MinValMax<NUM> &tally, uint64_t flags)
{
    if (flags & STAT_KEEP)
    {
        tally.min = tally.val;
        tally.max = tally.val;
    }
    else
    {
        tally.min = tally.max = tally.val = NUM();
    }
}


template <class T, int NumBuckets>
class LinearHistogram
{
protected:
    int buckets[NumBuckets];
    T start;
    T end;

public:
    LinearHistogram(T pstart, T pend) : buckets(0), start(pstart), end(pend) {}
};


// Get the named statistic.  Returns NULL if it does not exist
CStatBase *GetStat(char *name);


#endif
