// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "bench.h"
#include <iostream>
#include <sys/time.h>

using namespace benchmark;

std::map<std::string, BenchFunction> BenchRunner::benchmarks;

static double gettimedouble(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec * 0.000001 + tv.tv_sec;
}

BenchRunner::BenchRunner(std::string name, BenchFunction func)
{
    benchmarks.insert(std::make_pair(name, func));
}

void
BenchRunner::RunAll(double elapsedTimeForOne)
{
    std::cout << "Benchmark" << "," << "count" << "," << "min" << "," << "max" << "," << "average" << "\n";

    for (std::map<std::string,BenchFunction>::iterator it = benchmarks.begin();
         it != benchmarks.end(); ++it) {

        State state(it->first, elapsedTimeForOne);
        BenchFunction& func = it->second;
        func(state);
    }
}

bool State::KeepRunning()
{
    double now = gettimedouble();
    if (count == 0) {
        beginTime = now;
    }
    else {
        double elapsedOne = now - lastTime;
        if (elapsedOne < minTime) minTime = elapsedOne;
        if (elapsedOne > maxTime) maxTime = elapsedOne;
    }
    lastTime = now;
    ++count;

    if (now - beginTime < maxElapsed) return true; // Keep going

    --count;

    // Output results
    double average = (now-beginTime)/count;
    std::cout << name << "," << count << "," << minTime << "," << maxTime << "," << average << "\n";

    return false;
}
