// Copyright(C) 2011 - 2020 The Litecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LITECOIN_MWEB_MODELS_H
#define LITECOIN_MWEB_MODELS_H

#include <amount.h>
#include <mw/models/block/Block.h>
#include <mw/models/tx/Transaction.h>
#include <serialize.h>
#include <tinyformat.h>

#include <memory>
#include <vector>

namespace MWEB {

/// <summary>
/// A convenience wrapper around a possibly-null extension block.
/// </summary>
struct Block {
    mw::Block::CPtr m_block;

    Block() = default;
    Block(const mw::Block::CPtr& block)
        : m_block(block) {}

    CAmount GetTotalFee() const noexcept
    {
        return IsNull() ? 0 : m_block->GetTotalFee();
    }

    CAmount GetSupplyChange() const noexcept
    {
        return IsNull() ? 0 : m_block->GetSupplyChange();
    }

    mw::Hash GetHash() const noexcept
    {
        return IsNull() ? mw::Hash{} : m_block->GetHeader()->GetHash();
    }

    mw::Header::CPtr GetMWEBHeader() const noexcept
    {
        return IsNull() ? mw::Header::CPtr{nullptr} : m_block->GetHeader();
    }

    int32_t GetHeight() const noexcept
    {
        return IsNull() ? -1 : m_block->GetHeight();
    }

    std::vector<mw::Hash> GetSpentIDs() const
    {
        if (IsNull()) {
            return std::vector<mw::Hash>{};
        }

        std::vector<mw::Hash> spent_ids;
        for (const Input& input : m_block->GetInputs()) {
            spent_ids.push_back(input.GetOutputID());
        }

        return spent_ids;
    }

    std::vector<mw::Hash> GetOutputIDs() const
    {
        if (IsNull()) {
            return std::vector<mw::Hash>{};
        }

        std::vector<mw::Hash> output_ids;
        for (const Output& output : m_block->GetOutputs()) {
            output_ids.push_back(output.GetOutputID());
        }

        return output_ids;
    }

    std::set<mw::Hash> GetKernelIDs() const
    {
        if (IsNull()) {
            return std::set<mw::Hash>{};
        }

        std::set<mw::Hash> kernel_ids;
        for (const Kernel& kernel : m_block->GetKernels()) {
            kernel_ids.insert(kernel.GetKernelID());
        }

        return kernel_ids;
    }

    SERIALIZE_METHODS(Block, obj)
    {
        READWRITE(WrapOptionalPtr(obj.m_block));
    }

    bool IsNull() const noexcept { return m_block == nullptr; }
    void SetNull() noexcept { m_block.reset(); }
};

/// <summary>
/// A convenience wrapper around a possibly-null MWEB transcation.
/// </summary>
struct Tx {
    mw::Transaction::CPtr m_transaction;

    Tx() = default;
    Tx(const mw::Transaction::CPtr& tx)
        : m_transaction(tx) {}

    std::set<mw::Hash> GetSpentIDs() const noexcept
    {
        if (IsNull()) {
            return std::set<mw::Hash>{};
        }

        std::set<mw::Hash> spent_ids;
        for (const Input& input : m_transaction->GetInputs()) {
            spent_ids.insert(input.GetOutputID());
        }

        return spent_ids;
    }

    std::set<mw::Hash> GetKernelIDs() const noexcept
    {
        if (IsNull()) {
            return std::set<mw::Hash>{};
        }

        std::set<mw::Hash> kernel_ids;
        for (const Kernel& kernel : m_transaction->GetKernels()) {
            kernel_ids.insert(kernel.GetKernelID());
        }

        return kernel_ids;
    }

    std::set<mw::Hash> GetOutputIDs() const noexcept
    {
        if (IsNull()) {
            return std::set<mw::Hash>{};
        }

        std::set<mw::Hash> output_ids;
        for (const Output& output : m_transaction->GetOutputs()) {
            output_ids.insert(output.GetOutputID());
        }

        return output_ids;
    }

    std::vector<PegInCoin> GetPegIns() const noexcept
    {
        if (IsNull()) {
            return std::vector<PegInCoin>{};
        }

        return m_transaction->GetPegIns();
    }

    bool HasPegOut() const noexcept
    {
        if (IsNull()) {
            return false;
        }

        const std::vector<Kernel>& kernels = m_transaction->GetKernels();
        return std::any_of(
            kernels.cbegin(), kernels.cend(),
            [](const Kernel& kernel) { return kernel.HasPegOut(); }
        );
    }

    std::vector<PegOutCoin> GetPegOuts() const noexcept
    {
        if (IsNull()) {
            return std::vector<PegOutCoin>{};
        }

        return m_transaction->GetPegOuts();
    }

    uint64_t GetMWEBWeight() const noexcept
    {
        return IsNull() ? 0 : m_transaction->CalcWeight();
    }

    CAmount GetFee() const noexcept
    {
        return IsNull() ? 0 : CAmount(m_transaction->GetTotalFee());
    }

    int32_t GetLockHeight() const noexcept
    {
        return IsNull() ? 0 : m_transaction->GetLockHeight();
    }

    bool GetOutput(const mw::Hash& output_id, Output& output) const noexcept
    {
        if (IsNull()) {
            return false;
        }

        for (const Output& o : m_transaction->GetOutputs()) {
            if (o.GetOutputID() == output_id) {
                output = o;
                return true;
            }
        }

        return false;
    }

    SERIALIZE_METHODS(Tx, obj)
    {
        READWRITE(WrapOptionalPtr(obj.m_transaction));
    }

    bool IsNull() const noexcept { return m_transaction == nullptr; }
    void SetNull() noexcept { m_transaction.reset(); }

    std::string ToString() const
    {
        return IsNull() ? "" : m_transaction->Print();
    }
};

} // namespace MWEB

#endif // LITECOIN_MWEB_MODELS_H