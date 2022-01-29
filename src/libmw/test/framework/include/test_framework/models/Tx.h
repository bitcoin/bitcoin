#pragma once

#include <mw/common/Macros.h>
#include <mw/models/tx/Transaction.h>
#include <mw/models/tx/PegInCoin.h>
#include <mw/crypto/Hasher.h>

#include <test_framework/models/TxInput.h>
#include <test_framework/models/TxOutput.h>

TEST_NAMESPACE

class Tx
{
public:
    Tx(const mw::Transaction::CPtr& pTransaction, const std::vector<TxOutput>& outputs)
        : m_pTransaction(pTransaction), m_outputs(outputs) { }

    static Tx CreatePegIn(const CAmount amount, const CAmount fee = 0);
    static Tx CreatePegOut(const TxOutput& input, const CAmount fee = 0);

    const mw::Transaction::CPtr& GetTransaction() const noexcept { return m_pTransaction; }
    const std::vector<Kernel>& GetKernels() const noexcept { return m_pTransaction->GetKernels(); }
    const std::vector<TxOutput>& GetOutputs() const noexcept { return m_outputs; }

    const BlindingFactor& GetKernelOffset() const noexcept { return m_pTransaction->GetKernelOffset(); }
    const BlindingFactor& GetStealthOffset() const noexcept { return m_pTransaction->GetStealthOffset(); }

    std::vector<PegInCoin> GetPegIns() const noexcept { return m_pTransaction->GetPegIns(); }
    std::vector<PegOutCoin> GetPegOuts() const noexcept { return m_pTransaction->GetPegOuts(); }

    PegInCoin GetPegInCoin() const
    {
        const auto& kernel = m_pTransaction->GetKernels().front();
        return PegInCoin(kernel.GetPegIn(), kernel.GetKernelID());
    }

    PegOutCoin GetPegOutCoin() const
    {
        const auto& kernel = m_pTransaction->GetKernels().front();
        return kernel.GetPegOuts().front();
    }

private:
    mw::Transaction::CPtr m_pTransaction;
    std::vector<TxOutput> m_outputs;
};

END_NAMESPACE