#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>

#include <policy/packages.h>

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
            available_coins.insert(COutPoint{MakeTransactionRef(*mtx)->GetHash(), 0});
        }
    }
    LIMITED_WHILE(!available_coins.empty(), 50)
    {
        CMutableTransaction mtx = CMutableTransaction();
        const size_t num_inputs = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, available_coins.size());
        const size_t num_outputs = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 50);
        for (size_t n{0}; n < num_inputs; ++n) {
            auto prevout = available_coins.begin();
            mtx.vin.push_back(CTxIn(*prevout, CScript()));
            available_coins.erase(prevout);
        }
        for (uint32_t n{0}; n < num_outputs; ++n) {
            mtx.vout.push_back(CTxOut(100, P2WSH_OP_TRUE));
        }
        CTransactionRef tx = MakeTransactionRef(mtx);
        txns_in.push_back(tx);
        // Make outputs available to spend
        for (uint32_t n{0}; n < num_outputs; ++n) {
            COutPoint new_coin{tx->GetHash(), n};
            if (fuzzed_data_provider.ConsumeBool()) {
                available_coins.insert(new_coin);
            }
        }
    }
    AncestorPackage packageified(txns_in);
    assert(IsSorted(packageified.Txns()));
    if (packageified.IsAncestorPackage()) {
        for (const auto& tx : packageified.Txns()) {
            assert(IsSorted(*packageified.GetAncestorSet(tx)));
        }
    }
}
} // namespace
