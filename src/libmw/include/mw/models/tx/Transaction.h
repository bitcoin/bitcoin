#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/common/Macros.h>
#include <mw/common/Traits.h>
#include <mw/consensus/Weight.h>
#include <mw/models/crypto/Hash.h>
#include <mw/models/crypto/BigInteger.h>
#include <mw/models/crypto/BlindingFactor.h>
#include <mw/models/tx/TxBody.h>

#include <memory>
#include <numeric>
#include <vector>

MW_NAMESPACE

////////////////////////////////////////
// TRANSACTION - Represents a transaction or merged transactions before they've been included in a block.
////////////////////////////////////////
class Transaction :
    public Traits::IPrintable,
    public Traits::ISerializable,
    public Traits::IHashable
{
public:
    using CPtr = std::shared_ptr<const Transaction>;

    //
    // Constructors
    //
    Transaction(BlindingFactor kernel_offset, BlindingFactor stealth_offset, TxBody body)
        : m_kernelOffset(std::move(kernel_offset)), m_stealthOffset(std::move(stealth_offset)), m_body(std::move(body))
    {
        m_hash = Hashed(*this);
    }
    Transaction(const Transaction& transaction) = default;
    Transaction(Transaction&& transaction) noexcept = default;
    Transaction() = default;

    //
    // Factory
    //
    static mw::Transaction::CPtr Create(
        BlindingFactor kernel_offset,
        BlindingFactor owner_offset,
        std::vector<Input> inputs,
        std::vector<Output> outputs,
        std::vector<Kernel> kernels
    );

    //
    // Destructor
    //
    virtual ~Transaction() = default;

    //
    // Operators
    //
    Transaction& operator=(const Transaction& transaction) = default;
    Transaction& operator=(Transaction&& transaction) noexcept = default;
    bool operator<(const Transaction& transaction) const noexcept { return GetHash() < transaction.GetHash(); }
    bool operator==(const Transaction& transaction) const noexcept { return GetHash() == transaction.GetHash(); }
    bool operator!=(const Transaction& transaction) const noexcept { return GetHash() != transaction.GetHash(); }

    //
    // Getters
    //
    const BlindingFactor& GetKernelOffset() const noexcept { return m_kernelOffset; }
    const BlindingFactor& GetStealthOffset() const noexcept { return m_stealthOffset; }
    const TxBody& GetBody() const noexcept { return m_body; }
    const std::vector<Input>& GetInputs() const noexcept { return m_body.GetInputs(); }
    const std::vector<Output>& GetOutputs() const noexcept { return m_body.GetOutputs(); }
    const std::vector<Kernel>& GetKernels() const noexcept { return m_body.GetKernels(); }
    CAmount GetTotalFee() const noexcept { return m_body.GetTotalFee(); }
    int32_t GetLockHeight() const noexcept { return m_body.GetLockHeight(); }
    uint64_t CalcWeight() const noexcept { return (uint64_t)Weight::Calculate(m_body); }

    std::vector<Commitment> GetKernelCommits() const noexcept { return m_body.GetKernelCommits(); }
    std::vector<Commitment> GetInputCommits() const noexcept { return m_body.GetInputCommits(); }
    std::vector<Commitment> GetOutputCommits() const noexcept { return m_body.GetOutputCommits(); }
    std::vector<PegInCoin> GetPegIns() const noexcept { return m_body.GetPegIns(); }
    CAmount GetPegInAmount() const noexcept { return m_body.GetPegInAmount(); }
    std::vector<PegOutCoin> GetPegOuts() const noexcept { return m_body.GetPegOuts(); }
    CAmount GetSupplyChange() const noexcept { return m_body.GetSupplyChange(); }

    //
    // Serialization/Deserialization
    //
    IMPL_SERIALIZABLE(Transaction, obj)
    {
        READWRITE(obj.m_kernelOffset);
        READWRITE(obj.m_stealthOffset);
        READWRITE(obj.m_body);
        SER_READ(obj, obj.m_hash = Hashed(obj));

        if (ser_action.ForRead()) {
            if (obj.m_body.GetKernels().empty()) {
                throw std::ios_base::failure("Transaction requires at least one kernel");
            }
        }
    }

    //
    // Traits
    //
    std::string Format() const final { return "Tx(" + GetHash().Format() + ")"; }
    const mw::Hash& GetHash() const noexcept final { return m_hash; }

    bool IsStandard() const noexcept;
    void Validate() const;
    
    std::string Print() const noexcept
    {
        auto print_kernel = [](const Kernel& kernel) -> std::string {
            return StringUtil::Format(
                "kern(kernel_id:{}, commit:{}, pegin: {}, pegout: {}, fee: {})",
                kernel.GetKernelID(),
                kernel.GetCommitment(),
                kernel.GetPegIn(),
                kernel.GetPegOutAmount(),
                kernel.GetFee()
            );
        };
        std::string kernels_str = std::accumulate(
            GetKernels().begin(), GetKernels().end(), std::string{},
            [&print_kernel](std::string str, const Kernel& kern) {
                return str.empty() ? print_kernel(kern) : std::move(str) + ", " + print_kernel(kern);
            }
        );

        return StringUtil::Format(
            "tx(hash:{}, offset:{}, kernels:[{}], inputs:{}, outputs:{})",
            GetHash(),
            GetKernelOffset().ToHex(),
            kernels_str,
            GetInputs(),
            GetOutputs()
        );
    }

private:
    // The kernel "offset" k2 excess is k1G after splitting the key k = k1 + k2.
    BlindingFactor m_kernelOffset;

    BlindingFactor m_stealthOffset;

    // The transaction body.
    TxBody m_body;

    mw::Hash m_hash;
};

END_NAMESPACE