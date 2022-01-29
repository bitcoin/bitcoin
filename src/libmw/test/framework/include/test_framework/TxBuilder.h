#pragma once

#include <test_framework/models/Tx.h>

#include <mw/crypto/Blinds.h>
#include <mw/crypto/Bulletproofs.h>
#include <mw/models/wallet/StealthAddress.h>

TEST_NAMESPACE

//
// Builds transactions for use with automated tests using random blinding factors.
// To make the transaction valid, TxBuilder keeps track of all blinding factors used,
// and adjusts the tx offset to make a valid transaction.
//
class TxBuilder
{
public:
    TxBuilder();

    TxBuilder& AddInput(const TxOutput& input);
    TxBuilder& AddInput(const CAmount amount);
    TxBuilder& AddInput(
        const CAmount amount,
        const SecretKey& privkey,
        const BlindingFactor& blind,
        const mw::Hash& output_id
    );

    TxBuilder& AddOutput(const CAmount amount);
    TxBuilder& AddOutput(
        const CAmount amount,
        const SecretKey& sender_privkey,
        const StealthAddress& receiver_addr
    );

    TxBuilder& AddPlainKernel(const CAmount fee, const bool add_stealth_excess = false);
    TxBuilder& AddPeginKernel(const CAmount amount, const boost::optional<CAmount>& fee = boost::none, const bool add_stealth_excess = false);
    TxBuilder& AddPegoutKernel(const CAmount amount, const CAmount fee, const bool add_stealth_excess = false);

    Tx Build();

private:
    CAmount m_amount;
    Blinds m_kernelOffset;
    Blinds m_stealthOffset;

    std::vector<Input> m_inputs;
    std::vector<TxOutput> m_outputs;
    std::vector<Kernel> m_kernels;
};

END_NAMESPACE