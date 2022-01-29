#pragma once

#include <mw/common/Macros.h>
#include <mw/models/tx/Output.h>
#include <mw/models/wallet/StealthAddress.h>
#include <mw/crypto/Bulletproofs.h>

TEST_NAMESPACE

class TxOutput
{
public:
    TxOutput(BlindingFactor&& blindingFactor, const uint64_t amount, Output&& output)
        : m_blindingFactor(std::move(blindingFactor)), m_amount(amount), m_output(std::move(output)) { }

    static TxOutput Create(
        const SecretKey& sender_privkey,
        const StealthAddress& receiver_addr,
        const uint64_t amount)
    {
        BlindingFactor blinding_factor;
        Output output = Output::Create(&blinding_factor, sender_privkey, receiver_addr, amount);

        return TxOutput{ std::move(blinding_factor), amount, std::move(output) };
    }

    const BlindingFactor& GetBlind() const noexcept { return m_blindingFactor; }
    uint64_t GetAmount() const noexcept { return m_amount; }
    const Output& GetOutput() const noexcept { return m_output; }
    const Commitment& GetCommitment() const noexcept { return m_output.GetCommitment(); }
    const mw::Hash& GetOutputID() const noexcept { return m_output.GetOutputID(); }

private:
    BlindingFactor m_blindingFactor;
    uint64_t m_amount;
    Output m_output;
};

END_NAMESPACE