#pragma once

#include <mw/common/Macros.h>
#include <mw/models/tx/Output.h>
#include <mw/models/wallet/StealthAddress.h>
#include <mw/crypto/Bulletproofs.h>
#include <mw/crypto/Pedersen.h>

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
        BlindingFactor raw_blind;
        Output output = Output::Create(&raw_blind, sender_privkey, receiver_addr, amount);
        BlindingFactor blind_switch = Pedersen::BlindSwitch(raw_blind, amount);

        return TxOutput{std::move(blind_switch), amount, std::move(output)};
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