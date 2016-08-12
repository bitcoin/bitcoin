// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/server.h"
#include "stats/stats.h"
#include "util.h"
#include "utilstrencodings.h"

#include <stdint.h>

#include <univalue.h>

UniValue getmempoolstats(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            "getmempoolstats\n"
            "\nReturns the collected mempool statistics.\n"
            "\nThe samples are segregated in multiple precision groups.\n"
            "\nSamples time interval is not guaranteed to be constant, hence there\n"
            "is a time delta in each sample relative to the last sample.\n"
            "\nResult:\n"
            "  [\n"
            "    {\n"
            "      \"sample_interval\"    : \"interval\",      (numeric) Interval target in seconds\n"
            "      \"time_from\"          : \"timestamp\",     (numeric) Timestamp, first sample\n"
            "      \"samples\"            : [\n"
            "                                 [<delta_in_secs>,<tx_count>,<dynamic_mem_usage>,<min_fee_per_k>],\n"
            "                                 [<delta_in_secs>,<tx_count>,<dynamic_mem_usage>,<min_fee_per_k>],\n"
            "                                 ...\n"
            "                             ]\n"
            "    }\n"
            "    ,...\n"
            "  ]\n"
            "\nExamples:\n" +
            HelpExampleCli("getmempoolstats", "") + HelpExampleRpc("getmempoolstats", ""));

    // get stats from the core stats model
    uint64_t timeFrom = 0;
    std::vector<unsigned int> groups = CStats::DefaultStats()->mempoolCollector->getPrecisionGroupsAndIntervals();

    UniValue groupsUni(UniValue::VARR);
    for (unsigned int i = 0; i < groups.size(); i++) {

        MempoolSamplesVector samples = CStats::DefaultStats()->mempoolCollector->getSamplesForPrecision(i, timeFrom);

        // use "flat" json encoding for performance reasons
        UniValue samplesObj(UniValue::VARR);
        for (const struct CStatsMempoolSample& sample : samples) {
            UniValue singleSample(UniValue::VARR);
            singleSample.push_back(UniValue((uint64_t)sample.timeDelta));
            singleSample.push_back(UniValue((uint64_t)sample.txCount));
            singleSample.push_back(UniValue((uint64_t)sample.dynMemUsage));
            singleSample.push_back(UniValue(sample.minFeePerK));
            samplesObj.push_back(singleSample);
        }

        UniValue sampleGroup(UniValue::VOBJ);
        sampleGroup.push_back(Pair("sample_interval", (int)groups[i]));
        sampleGroup.push_back(Pair("time_from", timeFrom));
        sampleGroup.push_back(Pair("samples", samplesObj));

        groupsUni.push_back(sampleGroup);
    }

    return groupsUni;
}

static const CRPCCommand commands[] =
{
    //  category              name                      actor (function)         argNames
    //  --------------------- ------------------------  -----------------------  ----------
        {"stats",             "getmempoolstats",        &getmempoolstats,        {}},

};

void RegisterStatsRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
