#pragma once

#include <mw/crypto/Pedersen.h>
#include <mw/models/tx/Transaction.h>
#include <cassert>

class Aggregation
{
public:
    //
    // Aggregates multiple transactions into 1.
    //
    static mw::Transaction::CPtr Aggregate(const std::vector<mw::Transaction::CPtr>& transactions)
    {
        if (transactions.empty()) {
            return std::make_shared<mw::Transaction>();
        }

        if (transactions.size() == 1) {
            return transactions.front();
        }

        std::vector<Input> inputs;
        std::vector<Output> outputs;
        std::vector<Kernel> kernels;
        std::vector<BlindingFactor> kernel_offsets;
        std::vector<BlindingFactor> stealth_offsets;

        // collect all the inputs, outputs, kernels, and owner sigs from the txs
        for (const mw::Transaction::CPtr& pTransaction : transactions) {
            inputs.insert(
                inputs.end(),
                pTransaction->GetInputs().begin(),
                pTransaction->GetInputs().end()
            );

            outputs.insert(
                outputs.end(),
                pTransaction->GetOutputs().begin(),
                pTransaction->GetOutputs().end()
            );

            kernels.insert(
                kernels.end(),
                pTransaction->GetKernels().begin(),
                pTransaction->GetKernels().end()
            );

            kernel_offsets.push_back(pTransaction->GetKernelOffset());
            stealth_offsets.push_back(pTransaction->GetStealthOffset());
        }

        // Sum the offsets up to give us an aggregate offsets for the transaction.
        BlindingFactor kernel_offset = Pedersen::AddBlindingFactors(kernel_offsets);
        BlindingFactor stealth_offset = Pedersen::AddBlindingFactors(stealth_offsets);

        // Build a new aggregate tx
        return mw::Transaction::Create(
            std::move(kernel_offset),
            std::move(stealth_offset),
            std::move(inputs),
            std::move(outputs),
            std::move(kernels)
        );
    }
};