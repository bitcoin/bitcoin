#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/common/Logger.h>
#include <mw/common/Traits.h>
#include <mw/models/tx/Input.h>
#include <mw/models/tx/Output.h>
#include <mw/models/tx/Kernel.h>
#include <mw/models/tx/PegInCoin.h>
#include <mw/models/tx/PegOutCoin.h>
#include <mw/crypto/Bulletproofs.h>
#include <mw/crypto/Schnorr.h>

#include <memory>
#include <vector>

////////////////////////////////////////
// TRANSACTION BODY - Container for all inputs, outputs, and kernels in a transaction or block.
////////////////////////////////////////
class TxBody : public Traits::ISerializable
{
public:
    using CPtr = std::shared_ptr<const TxBody>;

    //
    // Constructors
    //
    TxBody(std::vector<Input> inputs, std::vector<Output> outputs, std::vector<Kernel> kernels)
        : m_inputs(std::move(inputs)), m_outputs(std::move(outputs)), m_kernels(std::move(kernels)) { }
    TxBody(const TxBody& other) = default;
    TxBody(TxBody&& other) noexcept = default;
    TxBody() = default;

    //
    // Destructor
    //
    virtual ~TxBody() = default;

    //
    // Operators
    //
    TxBody& operator=(const TxBody& other) = default;
    TxBody& operator=(TxBody&& other) noexcept = default;

    bool operator==(const TxBody& rhs) const noexcept
    {
        return
            m_inputs == rhs.m_inputs &&
            m_outputs == rhs.m_outputs &&
            m_kernels == rhs.m_kernels;
    }

    //
    // Getters
    //
    const std::vector<Input>& GetInputs() const noexcept { return m_inputs; }
    const std::vector<Output>& GetOutputs() const noexcept { return m_outputs; }
    const std::vector<Kernel>& GetKernels() const noexcept { return m_kernels; }

    std::vector<Commitment> GetKernelCommits() const noexcept { return Commitments::From(m_kernels); }
    std::vector<Commitment> GetInputCommits() const noexcept { return Commitments::From(m_inputs); }
    std::vector<Commitment> GetOutputCommits() const noexcept { return Commitments::From(m_outputs); }

    std::vector<mw::Hash> GetKernelIDs() const noexcept { return Hashes::From(m_kernels); }
    std::vector<mw::Hash> GetSpentIDs() const noexcept
    {
        std::vector<mw::Hash> output_ids;
        std::transform(
            m_inputs.cbegin(), m_inputs.cend(),
            std::back_inserter(output_ids),
            [](const Input& input) { return input.GetOutputID(); });
        return output_ids;
    }
    std::vector<mw::Hash> GetOutputIDs() const noexcept { return Hashes::From(m_outputs); }

    std::vector<PublicKey> GetStealthExcesses() const noexcept {
        std::vector<PublicKey> stealth_excesses;
        for (const Kernel& kernel : m_kernels) {
            if (kernel.HasStealthExcess()) {
                stealth_excesses.push_back(kernel.GetStealthExcess());
            }
        }

        return stealth_excesses;
    }

    std::vector<PegInCoin> GetPegIns() const noexcept;
    CAmount GetPegInAmount() const noexcept;
    std::vector<PegOutCoin> GetPegOuts() const noexcept;
    CAmount GetTotalFee() const noexcept;
    CAmount GetSupplyChange() const noexcept;
    int32_t GetLockHeight() const noexcept;

    //
    // Serialization/Deserialization
    //
    IMPL_SERIALIZABLE(TxBody, obj)
    {
        READWRITE(obj.m_inputs, obj.m_outputs, obj.m_kernels);
    }

    void Validate() const;

private:
    // List of inputs spent by the transaction.
    std::vector<Input> m_inputs;

    // List of outputs the transaction produces.
    std::vector<Output> m_outputs;

    // List of kernels that make up this transaction.
    std::vector<Kernel> m_kernels;
};