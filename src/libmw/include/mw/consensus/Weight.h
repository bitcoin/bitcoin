#pragma once

#include <mw/consensus/Params.h>
#include <mw/models/tx/TxBody.h>
#include <numeric>
#include <utility>

class Weight
{
public:
    static size_t Calculate(const TxBody& tx_body)
    {
        size_t input_weight = std::accumulate(
            tx_body.GetInputs().begin(), tx_body.GetInputs().end(), (size_t)0,
            [](size_t sum, const Input& input) {
                return sum + CalcInputWeight(input.GetExtraData());
            }
        );

        size_t kernel_weight = std::accumulate(
            tx_body.GetKernels().begin(), tx_body.GetKernels().end(), (size_t)0,
            [](size_t sum, const Kernel& kernel) {
                size_t kern_weight = CalcKernelWeight(
                    kernel.HasStealthExcess(),
                    kernel.GetPegOuts(),
                    kernel.GetExtraData()
                );
                return sum + kern_weight;
            }
        );

        size_t output_weight = std::accumulate(
            tx_body.GetOutputs().begin(), tx_body.GetOutputs().end(), (size_t)0,
            [](size_t sum, const Output& output) {
                return sum + CalcOutputWeight(output.HasStandardFields(), output.GetExtraData());
            }
        );

        return input_weight + kernel_weight + output_weight;
    }

    static bool ExceedsMaximum(const TxBody& tx_body)
    {
        return tx_body.GetInputs().size() > mw::MAX_NUM_INPUTS || Calculate(tx_body) > mw::MAX_BLOCK_WEIGHT;
    }

    static size_t CalcInputWeight(const std::vector<uint8_t>& extra_data)
    {
        return ExtraBytesToWeight(extra_data.size());
    }

    static size_t CalcKernelWeight(
        const bool has_stealth_excess,
        const CScript& pegout_script = CScript{},
        const std::vector<uint8_t>& extra_data = {})
    {
        // Kernels with a stealth excess cost extra.
        size_t base_weight = has_stealth_excess ? mw::KERNEL_WITH_STEALTH_WEIGHT : mw::BASE_KERNEL_WEIGHT;

        return base_weight + ExtraBytesToWeight(pegout_script.size()) + ExtraBytesToWeight(extra_data.size());
    }

    static size_t CalcKernelWeight(
        const bool has_stealth_excess,
        const std::vector<PegOutCoin>& pegouts = {},
        const std::vector<uint8_t>& extra_data = {})
    {
        // Kernels with a stealth excess cost extra.
        size_t base_weight = has_stealth_excess ? mw::KERNEL_WITH_STEALTH_WEIGHT : mw::BASE_KERNEL_WEIGHT;

        size_t pegout_weight = std::accumulate(
            pegouts.begin(), pegouts.end(), (size_t)0,
            [](size_t sum, const PegOutCoin& pegout) {
                return sum + ExtraBytesToWeight(pegout.GetScriptPubKey().size());
            }
        );

        return base_weight + pegout_weight + ExtraBytesToWeight(extra_data.size());
    }

    // Outputs have a weight of 'BASE_OUTPUT_WEIGHT' plus 2 weight for standard fields, and 1 weight for every 'BYTES_PER_WEIGHT' extra_data bytes
    static size_t CalcOutputWeight(const bool standard_fields, const std::vector<uint8_t>& extra_data = {})
    {
        size_t base_weight = standard_fields ? mw::STANDARD_OUTPUT_WEIGHT : mw::BASE_OUTPUT_WEIGHT;
        return base_weight + ExtraBytesToWeight(extra_data.size());
    }

private:
    static size_t ExtraBytesToWeight(const size_t extra_bytes)
    {
        return (extra_bytes + (mw::BYTES_PER_WEIGHT - 1)) / mw::BYTES_PER_WEIGHT;
    }
};