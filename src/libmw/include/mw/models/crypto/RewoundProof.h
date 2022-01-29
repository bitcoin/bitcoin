#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/models/crypto/SecretKey.h>
#include <mw/models/crypto/ProofMessage.h>

class RewoundProof
{
public:
    RewoundProof(const uint64_t amount, std::unique_ptr<SecretKey>&& pBlindingFactor, ProofMessage&& proofMessage)
        : m_amount(amount), m_pBlindingFactor(std::move(pBlindingFactor)), m_proofMessage(std::move(proofMessage))
    {

    }

    uint64_t GetAmount() const noexcept { return m_amount; }
    const std::unique_ptr<SecretKey>& GetBlindingFactor() const noexcept { return m_pBlindingFactor; }
    const ProofMessage& GetProofMessage() const noexcept { return m_proofMessage; }

    bool operator==(const RewoundProof& rhs) const noexcept
    {
        return
            m_amount == rhs.m_amount &&
            m_pBlindingFactor->GetBigInt() == rhs.m_pBlindingFactor->GetBigInt() &&
            m_proofMessage.GetBytes() == rhs.m_proofMessage.GetBytes();
    }

private:
    uint64_t m_amount;
    std::unique_ptr<SecretKey> m_pBlindingFactor;
    ProofMessage m_proofMessage;
};