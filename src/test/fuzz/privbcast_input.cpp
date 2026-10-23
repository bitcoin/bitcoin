// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <core_io.h>
#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <privbcast/input.h>
#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

FUZZ_TARGET(privbcast_input)
{
    // Core's package checks salt their hash sets from the global generator.
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    const CAmount max_burn{ConsumeMoney(provider)};
    // Half the inputs are two transactions written as text, the second maybe spending the first, so
    // that the package rules are reached; the rest are any text.
    std::string text;
    std::vector<Wtxid> written;
    if (provider.ConsumeBool()) {
        std::optional<CMutableTransaction> first{ConsumeDeserializable<CMutableTransaction>(provider, TX_WITH_WITNESS)};
        std::optional<CMutableTransaction> second{ConsumeDeserializable<CMutableTransaction>(provider, TX_WITH_WITNESS)};
        if (!first || !second) return;
        if (provider.ConsumeBool() && !second->vin.empty()) {
            second->vin[0].prevout = COutPoint{CTransaction{*first}.GetHash(), provider.ConsumeIntegral<uint8_t>()};
        }
        const CTransaction a{*first}, b{*second};
        text = EncodeHexTx(a) + provider.PickValueInArray({" ", "\n", "\t\n "}) + EncodeHexTx(b);
        // Hex without witnesses, of transactions with inputs, has one reading: the one written.
        if (!a.vin.empty() && !b.vin.empty() && !a.HasWitness() && !b.HasWitness()) written = {a.GetWitnessHash(), b.GetWitnessHash()};
    } else {
        text = provider.ConsumeRemainingBytesAsString();
    }

    const auto txs{privbcast::ParseTransactions(text)};
    if (!written.empty()) {
        assert(txs && txs->size() == 2 && (*txs)[0]->GetWitnessHash() == written[0] && (*txs)[1]->GetWitnessHash() == written[1]);
    }
    if (!txs) return;
    assert(txs->size() == 1 || txs->size() == 2);
    if (txs->size() == 2) {
        // The order the two come in changes nothing.
        const auto package{privbcast::CheckPackage((*txs)[0], (*txs)[1], max_burn)};
        const auto swapped{privbcast::CheckPackage((*txs)[1], (*txs)[0], max_burn)};
        assert(package.has_value() == swapped.has_value());
        if (!package) return;
        assert(package->parent == swapped->parent && package->child == swapped->child);
        // A parent and its child as the interface describes them, each fit to broadcast.
        const CTransaction& parent{*package->parent};
        const CTransaction& child{*package->child};
        assert(parent.GetHash() != child.GetHash());
        assert(std::ranges::none_of(parent.vin, [&](const CTxIn& in) { return in.prevout.hash == child.GetHash(); }));
        bool spends_parent{false};
        for (const CTxIn& in : child.vin) {
            if (in.prevout.hash != parent.GetHash()) continue;
            spends_parent = true;
            assert(in.prevout.n < parent.vout.size());
        }
        assert(spends_parent);
        std::set<COutPoint> spent;
        for (const CTxIn& in : parent.vin) spent.insert(in.prevout);
        for (const CTxIn& in : child.vin) assert(!spent.contains(in.prevout));
        assert(privbcast::CheckForBroadcast(parent, max_burn) && privbcast::CheckForBroadcast(child, max_burn));
        assert(int64_t{GetTransactionWeight(parent)} + GetTransactionWeight(child) <= int64_t{MAX_PACKAGE_WEIGHT});
        return;
    }
    const CTransaction& tx{*txs->front()};
    if (!privbcast::CheckForBroadcast(tx, max_burn)) return;

    // What passed is fit to broadcast as the interface describes it.
    TxValidationState state;
    assert(CheckTransaction(tx, state));
    assert(!tx.IsCoinBase());
    assert(GetTransactionWeight(tx) <= MAX_STANDARD_TX_WEIGHT);
    for (const CTxOut& out : tx.vout) {
        assert(!(out.scriptPubKey.IsUnspendable() || !out.scriptPubKey.HasValidOps()) || out.nValue <= max_burn);
    }
    // A higher bound accepts it too.
    assert(privbcast::CheckForBroadcast(tx, MAX_MONEY));
}
