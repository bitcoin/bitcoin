#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>

#include <policy/ancestor_packages.h>

#include <set>
#include <vector>

namespace {
FUZZ_TARGET(ancestorpackage)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::vector<CTransactionRef> txns_in;
    // Avoid repeat coins, as they may cause transactions to conflict
    std::set<COutPoint> available_coins;
    for (auto i{0}; i < 100; ++i) {
        if (auto mtx{ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider)}) {
            available_coins.insert(COutPoint{MakeTransactionRef(*mtx)->GetHash(), fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, 10)});
        }
    }
    // Create up to 50 transactions with variable inputs and outputs.
    LIMITED_WHILE(!available_coins.empty() && fuzzed_data_provider.ConsumeBool(), 50)
    {
        // If we're making the "bottom" tx, spend all available coins to make an ancestor package,
        // and exit this loop afterwards.
        const bool make_bottom_tx{fuzzed_data_provider.ConsumeBool()};

        CMutableTransaction mtx = CMutableTransaction();
        const size_t num_inputs = make_bottom_tx ? available_coins.size() :
            fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, available_coins.size());
        const size_t num_outputs = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 50);
        for (size_t n{0}; n < num_inputs; ++n) {
            auto prevout = available_coins.begin();
            mtx.vin.emplace_back(*prevout, CScript());
            available_coins.erase(prevout);
        }
        for (uint32_t n{0}; n < num_outputs; ++n) {
            mtx.vout.emplace_back(100, P2WSH_OP_TRUE);
        }
        CTransactionRef tx = MakeTransactionRef(mtx);

        if (txns_in.empty()) {
            txns_in.emplace_back(tx);
        } else {
            // Place tx in a random spot in the vector, swapping the existing tx at that index to the
            // back, so the package is not necessarily sorted.
            const size_t index = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, txns_in.size() - 1);
            txns_in.emplace_back(txns_in.at(index));
            txns_in.at(index) = tx;
        }

        // Make outputs available to spend, unless we want this to be the last tx and have just
        // swept available_coins.
        if (!make_bottom_tx) {
            for (uint32_t n{0}; n < num_outputs; ++n) {
                COutPoint new_coin{tx->GetHash(), n};
                // Always add the first output to ensure each tx has an outgoing output we can use
                // to create an ancestor package.
                if (n > 0 && fuzzed_data_provider.ConsumeBool()) {
                    available_coins.insert(new_coin);
                }
            }
        }
    }

    // AncestorPackage may need to topologically sort txns_in. Find bugs in topological sort and
    // skipping as a result of MarkAsInMempool() and IsDanglingWithDescendants() functions.
    AncestorPackage packageified(txns_in);
    Assert(IsTopoSortedPackage(packageified.Txns()));
    if (packageified.IsAncestorPackage()) {
        // Optionally MarkAsInMempool() the first n transactions. These must be at the beginning of the
        // package as it doesn't make sense for a transaction to be in mempool if its ancestors
        // aren't as well.  The ith transaction is not necessarily an ancestor of the i+1th
        // transaction, but just call MarkAsInMempool() for transactions 1...n to keep things simple.
        // For the rest of the transactions, optionally call IsDanglingWithDescendants().
        bool skipping = true;
        for (const auto& tx : packageified.Txns()) {
            if (skipping) {
                packageified.MarkAsInMempool(tx);
                if (fuzzed_data_provider.ConsumeBool()) {
                    skipping = false;
                }
            } else {
                // Not skipping anymore. Maybe do a IsDanglingWithDescendants().
                if (fuzzed_data_provider.ConsumeBool()) {
                    packageified.IsDanglingWithDescendants(tx);
                }
            }
        }
        Assert(IsTopoSortedPackage(packageified.FilteredTxns()));
        for (const auto& tx : packageified.FilteredTxns()) {
            assert(IsTopoSortedPackage(*packageified.FilteredAncestorSet(tx)));
        }
    }
}
} // namespace
