// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/block_template_manager.h>

#include <consensus/validation.h>
#include <node/miner.h>
#include <node/mining_args.h>
#include <primitives/block.h>
#include <uint256.h>
#include <util/check.h>
#include <validation.h>
#include <validationinterface.h>

#include <utility>

namespace node {

BlockTemplateManager::BlockTemplateManager(CTxMemPool& mempool, ChainstateManager& chainman,
                                           BlockCreateOptions block_create_args)
    : m_mempool(mempool), m_chainman(chainman), m_block_create_args(std::move(block_create_args))
{
}

std::unique_ptr<CBlockTemplate> BlockTemplateManager::CreateNewTemplate(const BlockCreateOptions& options)
{
    return BlockAssembler{m_chainman.ActiveChainstate(), &m_mempool, options}.CreateNewBlock();
}

namespace {
class SubmitBlockStateCatcher final : public CValidationInterface
{
public:
    uint256 m_hash;
    bool m_found{false};
    BlockValidationState m_state;

    explicit SubmitBlockStateCatcher(const uint256& hash) : m_hash{hash} {}

protected:
    void BlockChecked(const std::shared_ptr<const CBlock>& block, const BlockValidationState& state) override
    {
        if (block->GetHash() != m_hash) return;
        // ProcessNewBlock emits BlockChecked synchronously while holding cs_main,
        // so SubmitBlock can read these fields after ProcessNewBlock returns
        // without extra synchronization.
        m_found = true;
        m_state = state;
    }
};
} // namespace

bool BlockTemplateManager::SubmitBlock(const std::shared_ptr<const CBlock>& block, std::string& reason, std::string& debug)
{
    reason.clear();
    debug.clear();

    // This follows the submitblock RPC's validation-state capture pattern, but
    // is intentionally kept separate from the RPC implementation. The RPC entry
    // point decodes hex, formats BIP22/JSONRPC results, and calls
    // UpdateUncommittedBlockStructures() for legacy witness handling. IPC
    // callers submit already-formed blocks and need bool + reason/debug
    // results.
    auto sc = std::make_shared<SubmitBlockStateCatcher>(block->GetHash());
    CHECK_NONFATAL(m_chainman.m_options.signals)->RegisterSharedValidationInterface(sc);
    bool new_block;
    bool accepted = m_chainman.ProcessNewBlock(block, /*force_processing=*/true, /*min_pow_checked=*/true, /*new_block=*/&new_block);
    // No queue drain is needed. The BlockChecked notification used above is
    // emitted synchronously by ProcessNewBlock, unlike most validation signals.
    CHECK_NONFATAL(m_chainman.m_options.signals)->UnregisterSharedValidationInterface(sc);

    if (!new_block && accepted) {
        reason = "duplicate";
    } else if (!accepted && (!sc->m_found || sc->m_state.IsValid())) {
        // ProcessNewBlock can fail without a validation result, for example
        // from an activation or system error. It can also fail after a valid
        // BlockChecked result. In these cases the validation result is
        // inconclusive.
        reason = "inconclusive";
    } else if (!sc->m_found) {
        // The block was accepted but not connected, for example if it does not
        // have more work than the current tip.
        reason = "inconclusive";
    } else if (!sc->m_state.IsValid()) {
        reason = sc->m_state.GetRejectReason();
        debug = sc->m_state.GetDebugMessage();
    }
    const bool result{accepted && new_block && reason.empty()};
    CHECK_NONFATAL(result == reason.empty());
    return result;
}

} // namespace node
