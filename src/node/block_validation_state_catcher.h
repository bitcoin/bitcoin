// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_BLOCK_VALIDATION_STATE_CATCHER_H
#define BITCOIN_NODE_BLOCK_VALIDATION_STATE_CATCHER_H

#include <consensus/validation.h>
#include <validationinterface.h>

#include <memory>
#include <optional>

namespace node {

/** Captures the validation state reported for a specific block.
 *  Callers manage registration and unregistration with ValidationSignals.
 */
class BlockValidationStateCatcher final : public CValidationInterface
{
private:
    uint256 m_hash;

public:
    std::optional<BlockValidationState> m_state;
    explicit BlockValidationStateCatcher(const uint256& hash) : m_hash(hash) {};

protected:
    void BlockChecked(const std::shared_ptr<const CBlock>& block, const BlockValidationState& state) override
    {
        if (block->GetHash() == m_hash) m_state = state;
    }
};

} // namespace node

#endif // BITCOIN_NODE_BLOCK_VALIDATION_STATE_CATCHER_H
