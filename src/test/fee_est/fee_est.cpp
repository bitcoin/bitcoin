// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <policy/fees.h>
#include <policy/fees_input.h>
#include <util/translation.h>

#include <univalue.h>

#include <fstream>
#include <random>
#include <stdio.h>

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

std::string Usage()
{
    std::string usage = "Usage: fee_est [options] <estlog.txt>\n";
    usage += HelpMessageGroup("Options:");
    usage += HelpMessageOpt("-?, -help", "This help message");
    usage += HelpMessageOpt("-ograph=<graph.html>", "Generate target vs feerate graph output after replaying transaction log.");
    usage += HelpMessageOpt("-odat=<fee_estimates.dat>", "Save fee estimator state after after replaying transaction log.");
    usage += HelpMessageOpt("-idat=<fee_estimates.dat>", "Load fee estimator state after before replaying transaction log.");
    usage += HelpMessageOpt("-dat=<fee_estimates.dat>", "Shortcut for -idat=<fee_estimates.dat> -odat=<fee_estimates.dat>");
    usage += HelpMessageOpt("-cross", "Treat half the transactions from the log as test data and print cross-validation statistics.");
    return usage;
}

struct Options {
    std::string logFileIn;
    std::string dataFileIn;
    std::string dataFileOut;
    std::string graphFileOut;
    bool cross = false;
    bool help = false;
    bool error = false;
};

Options ParseCommandLine(int argc, char** argv)
{
    auto match = [&](const char* arg, const char* name, const char** value = nullptr) {
        if (arg[0] != '-') return false;
        size_t n = strlen(name);
        if (strncmp(arg + 1, name, n) != 0) return false;
        if (value) return (*value = arg[n + 1] == '=' ? arg + n + 2 : nullptr) != nullptr;
        return arg[n + 1] == '\0';
    };

    Options options;
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        const char* value;
        if (match(arg, "ograph", &value)) {
            options.graphFileOut = value;
        } else if (match(arg, "odat", &value)) {
            options.dataFileOut = value;
        } else if (match(arg, "idat", &value)) {
            options.dataFileIn = value;
        } else if (match(arg, "dat", &value)) {
            options.dataFileIn = options.dataFileOut = value;
        } else if (match(arg, "cross")) {
            options.cross = true;
        } else if (match(arg, "help") || match(arg, "h") || match(arg, "?")) {
            options.help = true;
        } else if (arg[0] != '-' && options.logFileIn.empty()) {
            options.logFileIn = arg;
        } else {
            fprintf(stderr, "Error: unexpected argument: '%s'\n", arg);
            options.error = true;
        }
    }

    if (options.logFileIn.empty() && !options.help) {
        fprintf(stderr, "Error: missing required log file argument.\n");
        options.error = true;
    }

    return options;
}

struct TxData {
    TxData(CAmount fee, size_t size, int height) : fee(fee), size(size), height(height) {}
    CAmount fee;
    size_t size;
    int height;
    int expectedBlocks = -1;
    int actualBlocks = -1;
};

using TxMap = std::map<uint256, TxData>;

const char* const GRAPH_HTML = R"(<!DOCTYPE html>
<meta charset="utf-8">
<title>Fee Graph</title>
<script src="https://d3js.org/d3.v3.min.js"></script>
<script src="https://cdnjs.cloudflare.com/ajax/libs/vega/3.0.0-beta.30/vega.js"></script>
<script src="https://cdnjs.cloudflare.com/ajax/libs/vega-lite/2.0.0-beta.2/vega-lite.js"></script>
<script src="https://cdnjs.cloudflare.com/ajax/libs/vega-embed/3.0.0-beta.14/vega-embed.js"></script>
<style media="screen">
.vega-actions a {
    margin-right: 5px;
}
</style>
<div id=vis></div>
<script>
var spec = {
  "$schema": "https://vega.github.io/schema/vega-lite/v2.json",
  "width": 800,
  "height": 600,
  "layer": [
    {
        "data": {
            "values": %s
        },
        "mark": "tick",
        "encoding": {
            "x": {"field": "blocks", "type": "quantitative"},
            "y": {"field": "feeRate", "type": "quantitative"},
            "color": {"field": "height", "type": "quantitative"}
        }
    },
    {
      "data": {
        "values": %s
      },
      "encoding": {
        "x": {"field": "blocks", "type": "quantitative"},
        "y": {"field": "feeRate", "type": "quantitative"},
        "color": {"field": "mode", "type": "nominal"}
      },
      "mark": "line",
    }
  ]
}
vega.embed("#vis", spec);
</script>
)";

// Maximum number of randomly sampled transactions to show on graph.
const size_t SAMPLE_TXS = 50000;

// Drop 1/20th of sampled transactions with highest feerates if they are
// outliers that would change the scale of the graph.
const int DROP_FRAC = 20;

void WriteGraph(const std::string& filename,
    const TxMap& txMap,
    const std::vector<unsigned int>& targets,
    CBlockPolicyEstimator& estimator)
{
    UniValue fees(UniValue::VARR);

    int64_t maxFeeRate = 0;
    const unsigned int maxTarget = estimator.getMaxTarget();
    for (bool conservative : {false, true}) {
        for (unsigned int target : targets) {
            if (target > maxTarget) continue;
            UniValue fee(UniValue::VOBJ);
            fee.pushKV("mode", conservative ? "conservative" : "default");
            fee.pushKV("blocks", int(target));
            CAmount feeRate = estimator.estimateSmartFee(target, nullptr /* fee_calc */, conservative).GetFeePerK();
            fee.pushKV("feeRate", feeRate);
            fees.push_back(fee);
            maxFeeRate = std::max(maxFeeRate, feeRate);
        }
    }

    std::vector<const TxMap::value_type*> sample;
    sample.reserve(SAMPLE_TXS);
    int i = 0;
    std::minstd_rand randint;
    for (const auto& entry : txMap) {
        if (entry.second.actualBlocks > int(maxTarget)) {
            continue;
        } else if (sample.size() < SAMPLE_TXS) {
            sample.emplace_back(&entry);
        } else {
            size_t j = randint() % i;
            if (j < SAMPLE_TXS) {
                sample[j] = &entry;
            }
        }
        ++i;
    }

    if (!sample.empty()) {
        std::vector<CAmount> feeRates;
        feeRates.reserve(sample.size());
        for (const auto& entry : sample) {
            feeRates.emplace_back(CFeeRate(entry->second.fee, entry->second.size).GetFeePerK());
        }
        auto nth = feeRates.begin() + sample.size() / DROP_FRAC;
        std::nth_element(feeRates.begin(), nth, feeRates.end(), std::greater<int64_t>());
        maxFeeRate = std::max(maxFeeRate, *nth);
    }

    UniValue txs(UniValue::VARR);
    for (const auto& entry : sample) {
        if (entry->second.actualBlocks > 0) {
            CAmount feeRate = CFeeRate(entry->second.fee, entry->second.size).GetFeePerK();
            if (feeRate <= maxFeeRate) {
                UniValue tx(UniValue::VOBJ);
                tx.pushKV("feeRate", feeRate);
                tx.pushKV("blocks", entry->second.actualBlocks);
                tx.pushKV("height", entry->second.height);
                txs.push_back(std::move(tx));
            }
        }
    }

    std::ofstream file(filename);
    file << strprintf(GRAPH_HTML, txs.write(), fees.write());
}

bool UpdateCross(TxMap::value_type& tx, const std::vector<unsigned int>& targets, CBlockPolicyEstimator& estimator)
{
    if (*tx.first.begin() == 0) {
        auto it = std::lower_bound(targets.begin(), targets.end(), nullptr, [&](unsigned int target, std::nullptr_t) {
            CAmount estFee = estimator.estimateSmartFee(target, nullptr /* fee_calc */, false /* conservative */)
                                 .GetFee(tx.second.size);
            return estFee <= 1 || tx.second.fee < estFee;
        });
        tx.second.expectedBlocks = it == targets.end() ? std::numeric_limits<int>::max() : int(*it);
        return false;
    }
    return true;
}

void PrintCross(const TxMap& txMap, CBlockPolicyEstimator&)
{
    int nonTestTxs = 0;
    int unconfirmedTestTxs = 0;
    int outlierTestTxs = 0;
    int keptTestTxs = 0;
    int64_t errsq = 0;
    for (const auto& entry : txMap) {
        if (entry.second.expectedBlocks == -1) {
            ++nonTestTxs;
        } else if (entry.second.actualBlocks == -1) {
            ++unconfirmedTestTxs;
        } else if (entry.second.expectedBlocks == std::numeric_limits<int>::max()) {
            ++outlierTestTxs;
        } else {
            ++keptTestTxs;
            int err = entry.second.expectedBlocks - entry.second.actualBlocks;
            errsq += err * err;
        }
    }
    printf("Non-test txs: %i\n", nonTestTxs);
    printf("Test txs: %i total (%i kept, %i discarded unconfirmed, %i discarded outliers)\n",
        keptTestTxs + unconfirmedTestTxs + outlierTestTxs, keptTestTxs, unconfirmedTestTxs, outlierTestTxs);
    if (keptTestTxs > 0) {
        printf("Mean squared error: %g\n", double(errsq) / double(keptTestTxs));
    }
}

int main(int argc, char** argv)
{
    Options options = ParseCommandLine(argc, argv);

    if (options.help) {
        printf("%s", Usage().c_str());
        return EXIT_SUCCESS;
    } else if (options.error) {
        fprintf(stderr, "Try `fee_est -h` for more information.\n");
        return EXIT_FAILURE;
    } else if (options.dataFileOut.empty() && options.graphFileOut.empty() && !options.cross) {
        fprintf(stderr, "Warning: No output options specified. Try -ograph, -odat, -cross options, or `fee_est -h` "
                        "for more information.\n");
    }

    CBlockPolicyEstimator estimator;
    FeeEstInput input(estimator);

    if (!options.dataFileIn.empty() && !input.readData(fs::PathFromString(options.dataFileIn))) {
        fprintf(stderr, "Error: failed to load fee estimate data file '%s'\n", options.dataFileOut.c_str());
        return 1;
    }

    const std::vector<unsigned int> targets = estimator.GetUniqueTargets();
    TxMap txMap;

    auto filter = [&](UniValue& value) {
        bool keep = true;
        const UniValue& tx = value["tx"];
        if (tx.isObject()) {
            auto inserted = txMap.emplace(uint256S(tx["hash"].get_str()),
                TxData(tx["fee"].getInt<int64_t>(), tx["size"].getInt<uint32_t>(), tx["height"].getInt<int>()));
            if (options.cross && !UpdateCross(*inserted.first, targets, estimator)) {
                keep = false;
            }
        }

        const UniValue& block = value["block"];
        if (block.isObject()) {
            int blockHeight = block["height"].getInt<int>();
            const auto& block_txs = value["txs"].getValues();
            for (const UniValue& block_tx : block_txs) {
                auto it = txMap.find(uint256S(block_tx["hash"].get_str()));
                if (it != txMap.end()) {
                    it->second.actualBlocks = blockHeight - block_tx["height"].getInt<int>();
                }
            }
        }

        return keep;
    };

    if (!input.readLog(fs::PathFromString(options.logFileIn), std::ref(filter))) {
        fprintf(stderr, "Error: failed to load fee data log file '%s'\n", options.logFileIn.c_str());
        return EXIT_FAILURE;
    }

    if (!options.dataFileOut.empty() && !input.writeData(fs::PathFromString(options.dataFileOut))) {
        fprintf(stderr, "Error: failed to write fee estimate data file '%s'\n", options.dataFileOut.c_str());
        return EXIT_FAILURE;
    }

    if (!options.graphFileOut.empty()) {
        WriteGraph(options.graphFileOut, txMap, targets, estimator);
        return EXIT_FAILURE;
    }

    if (options.cross) {
        PrintCross(txMap, estimator);
    }

    return EXIT_SUCCESS;
}
