#pragma once

#include <mw/exceptions/ValidationException.h>
#include <mw/crypto/Pedersen.h>
#include <mw/models/tx/TxBody.h>
#include <mw/models/tx/Transaction.h>
#include <mw/models/tx/UTXO.h>
#include <mw/common/Logger.h>
#include <cstdlib>

class KernelSumValidator
{
public:
    // Makes sure the sums of all utxo commitments minus the total supply
    // equals the sum of all kernel excesses and the total offset.
    // This is to be used only when validating the entire state.
    //
    // Throws a ValidationException if the utxo sum != kernel sum.
    static void ValidateState(
        const std::vector<Commitment>& utxo_commitments,
        const std::vector<Kernel>& kernels,
        const BlindingFactor& total_offset)
    {
        // Sum all utxo commitments - expected supply.
        int64_t total_mweb_supply = 0;
        for (const Kernel& kernel : kernels) {
            total_mweb_supply += kernel.GetSupplyChange();

            // Total supply can never go below 0
            if (total_mweb_supply < 0) {
                ThrowValidation(EConsensusError::BLOCK_SUMS);
            }
        }

        ValidateSums(
            {},
            utxo_commitments,
            Commitments::From(kernels),
            total_offset,
            total_mweb_supply
        );
    }

    static void ValidateForBlock(
        const TxBody& body,
        const BlindingFactor& total_offset,
        const BlindingFactor& prev_total_offset)
    {
        BlindingFactor block_offset = total_offset;
        if (!prev_total_offset.IsZero()) {
            block_offset = Pedersen::AddBlindingFactors({block_offset}, {prev_total_offset});
        }

        ValidateSums(
            body.GetInputCommits(),
            body.GetOutputCommits(),
            body.GetKernelCommits(),
            block_offset,
            body.GetSupplyChange()
        );
    }

    static void ValidateForTx(const mw::Transaction& tx)
    {
        ValidateSums(
            tx.GetInputCommits(),
            tx.GetOutputCommits(),
            tx.GetKernelCommits(),
            tx.GetKernelOffset(),
            tx.GetSupplyChange()
        );
    }

private:
    static void ValidateSums(
        const std::vector<Commitment>& input_commits,
        const std::vector<Commitment>& output_commits,
        const std::vector<Commitment>& kernel_commits,
        const BlindingFactor& offset,
        const int64_t coins_added)
    {
        // Calculate UTXO nonce sum
        Commitment sum_utxo_commitment = Pedersen::AddCommitments(output_commits, input_commits);
        if (coins_added > 0) {
            sum_utxo_commitment = Pedersen::AddCommitments(
                { sum_utxo_commitment }, { Commitment::Transparent(coins_added) }
            );
        } else if (coins_added < 0) {
            sum_utxo_commitment = Pedersen::AddCommitments(
                { sum_utxo_commitment, Commitment::Transparent(std::abs(coins_added)) }
            );
        }

        // Calculate total kernel excess
        Commitment sum_excess_commitment = Pedersen::AddCommitments(kernel_commits);
        if (!offset.IsZero()) {
            sum_excess_commitment = Pedersen::AddCommitments(
                { sum_excess_commitment, Commitment::Blinded(offset, 0) }
            );
        }

        if (sum_utxo_commitment != sum_excess_commitment) {
            LOG_ERROR_F(
                "UTXO sum {} does not match kernel excess sum {}.",
                sum_utxo_commitment,
                sum_excess_commitment
            );
            ThrowValidation(EConsensusError::BLOCK_SUMS);
        }
    }
};