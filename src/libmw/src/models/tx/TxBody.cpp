#include <mw/models/tx/TxBody.h>
#include <mw/exceptions/ValidationException.h>
#include <mw/consensus/Params.h>
#include <mw/consensus/Weight.h>

#include <unordered_set>
#include <numeric>

std::vector<PegInCoin> TxBody::GetPegIns() const noexcept
{
    std::vector<PegInCoin> pegins;
    for (const Kernel& kernel : m_kernels) {
        if (kernel.HasPegIn()) {
            pegins.push_back(PegInCoin(kernel.GetPegIn(), kernel.GetKernelID()));
        }
    }

    return pegins;
}

CAmount TxBody::GetPegInAmount() const noexcept
{
    return std::accumulate(
        m_kernels.cbegin(), m_kernels.cend(), (CAmount)0,
        [](const CAmount sum, const auto& kernel) noexcept { return sum + kernel.GetPegIn(); }
    );
}

std::vector<PegOutCoin> TxBody::GetPegOuts() const noexcept
{
    std::vector<PegOutCoin> pegouts;
    for (const Kernel& kernel : m_kernels) {
        for (PegOutCoin pegout : kernel.GetPegOuts()) {
            pegouts.push_back(std::move(pegout));
        }
    }
    return pegouts;
}

CAmount TxBody::GetTotalFee() const noexcept
{
    return std::accumulate(
        m_kernels.cbegin(), m_kernels.cend(), (CAmount)0,
        [](const CAmount sum, const auto& kernel) noexcept { return sum + kernel.GetFee(); }
    );
}

CAmount TxBody::GetSupplyChange() const noexcept
{
    return std::accumulate(
        m_kernels.cbegin(), m_kernels.cend(), (CAmount)0,
        [](const CAmount supply_change, const auto& kernel) noexcept { return supply_change + kernel.GetSupplyChange(); }
    );
}

int32_t TxBody::GetLockHeight() const noexcept
{
    return std::accumulate(
        m_kernels.cbegin(), m_kernels.cend(), (int32_t)0,
        [](const int32_t lock_height, const auto& kernel) noexcept { return std::max(lock_height, kernel.GetLockHeight()); }
    );
}

void TxBody::Validate() const
{
    // Verify weight
    if (Weight::ExceedsMaximum(*this)) {
        ThrowValidation(EConsensusError::BLOCK_WEIGHT);
    }

    // Verify inputs, outputs, kernels, and owner signatures are sorted
    if (!std::is_sorted(m_inputs.cbegin(), m_inputs.cend(), InputSort)
        || !std::is_sorted(m_outputs.cbegin(), m_outputs.cend(), OutputSort)
        || !std::is_sorted(m_kernels.cbegin(), m_kernels.cend(), KernelSort))
    {
        ThrowValidation(EConsensusError::NOT_SORTED);
    }

    auto contains_duplicates = [](const std::vector<mw::Hash>& hashes) -> bool {
        std::unordered_set<mw::Hash> set_hashes;
        std::copy(hashes.begin(), hashes.end(), std::inserter(set_hashes, set_hashes.end()));
        return set_hashes.size() != hashes.size();
    };

    // Verify no duplicate spends
    if (contains_duplicates(GetSpentIDs())) {
        ThrowValidation(EConsensusError::DUPLICATES);
    }

    // Verify no duplicate output IDs
    if (contains_duplicates(GetOutputIDs())) {
        ThrowValidation(EConsensusError::DUPLICATES);
    }

    // Verify no duplicate kernel IDs
    if (contains_duplicates(GetKernelIDs())) {
        ThrowValidation(EConsensusError::DUPLICATES);
    }

    //
    // Verify all signatures
    //
    std::vector<SignedMessage> signatures;
    std::transform(
        m_kernels.cbegin(), m_kernels.cend(), std::back_inserter(signatures),
        [](const Kernel& kernel) { return kernel.BuildSignedMsg(); }
    );

    std::transform(
        m_inputs.cbegin(), m_inputs.cend(), std::back_inserter(signatures),
        [](const Input& input) { return input.BuildSignedMsg(); }
    );

    std::transform(
        m_outputs.cbegin(), m_outputs.cend(), std::back_inserter(signatures),
        [](const Output& output) { return output.BuildSignedMsg(); }
    );

    if (!Schnorr::BatchVerify(signatures)) {
        ThrowValidation(EConsensusError::INVALID_SIG);
    }

    //
    // Verify RangeProofs
    //
    std::vector<ProofData> rangeProofs;
    std::transform(
        m_outputs.cbegin(), m_outputs.cend(), std::back_inserter(rangeProofs),
        [](const Output& output) { return output.BuildProofData(); }
    );
    if (!Bulletproofs::BatchVerify(rangeProofs)) {
        ThrowValidation(EConsensusError::BULLETPROOF);
    }
}