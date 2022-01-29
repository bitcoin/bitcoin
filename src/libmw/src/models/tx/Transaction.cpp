#include <mw/models/tx/Transaction.h>
#include <mw/consensus/KernelSumValidator.h>
#include <mw/consensus/StealthSumValidator.h>

using namespace mw;

Transaction::CPtr Transaction::Create(
    BlindingFactor kernel_offset,
    BlindingFactor stealth_offset,
    std::vector<Input> inputs,
    std::vector<Output> outputs,
    std::vector<Kernel> kernels)
{
    std::sort(inputs.begin(), inputs.end(), InputSort);
    std::sort(outputs.begin(), outputs.end(), OutputSort);
    std::sort(kernels.begin(), kernels.end(), KernelSort);

    return std::make_shared<mw::Transaction>(
        std::move(kernel_offset),
        std::move(stealth_offset),
        TxBody{
            std::move(inputs),
            std::move(outputs),
            std::move(kernels)
        }
    );
}

bool Transaction::IsStandard() const noexcept
{
    for (const Input& input : GetInputs()) {
        if (!input.IsStandard()) {
            return false;
        }
    }

    for (const Kernel& kernel : GetKernels()) {
        if (!kernel.IsStandard()) {
            return false;
        }
    }

    for (const Output& output : GetOutputs()) {
        if (!output.IsStandard()) {
            return false;
        }
    }

    return true;
}

void Transaction::Validate() const
{
    m_body.Validate();

    KernelSumValidator::ValidateForTx(*this);
    StealthSumValidator::Validate(m_stealthOffset, m_body);
}