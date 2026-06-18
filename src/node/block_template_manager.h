// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_BLOCK_TEMPLATE_MANAGER_H
#define BITCOIN_NODE_BLOCK_TEMPLATE_MANAGER_H

#include <node/mining_types.h>
#include <util/time.h>

#include <memory>
#include <optional>
#include <string>

class CBlock;
class ChainstateManager;
class CTxMemPool;
class uint256;

namespace interfaces {
struct BlockRef;
} // namespace interfaces

namespace node {
class KernelNotifications;
struct CBlockTemplate;

/**
 * Creates block templates, submits solved blocks, and provides tip-waiting
 * helpers for mining code. Owns the init-time block creation args.
 */
class BlockTemplateManager
{
private:
    CTxMemPool& m_mempool;
    ChainstateManager& m_chainman;
    KernelNotifications& m_notifications;
    const BlockCreateOptions m_block_create_args;

public:
    explicit BlockTemplateManager(CTxMemPool& mempool,
                                  ChainstateManager& chainman,
                                  KernelNotifications& notifications,
                                  BlockCreateOptions block_create_args = {});

    /** @return the block creation args set during node init. */
    const BlockCreateOptions& BlockCreateArgs() const { return m_block_create_args; }

    /** Create a fresh block template, applying init-time defaults to any unset options. */
    std::unique_ptr<CBlockTemplate> CreateNewTemplate(const BlockCreateOptions& options);

    /** Submit a block via ProcessNewBlock and capture validation state.
     *  @return whether the block was accepted as a new valid block. */
    bool SubmitBlock(const std::shared_ptr<const CBlock>& block, std::string& reason, std::string& debug);

    /** Locks cs_main.
     *  @return the active chain tip, or nullopt if none exists. */
    std::optional<interfaces::BlockRef> GetTip();

    /** Wait for the tip to differ from @p current_tip or timeout.
     *  Waits indefinitely during startup for a non-null tip.
     *  @return the current tip, or nullopt if the node is shutting down or
     *  interrupt is set (not when the timeout is reached). */
    std::optional<interfaces::BlockRef> WaitTipChanged(const uint256& current_tip, MillisecondsDouble& timeout, bool& interrupt);

    /**
     * Wait while the best known header extends the current chain tip AND at
     * least one block is being added to the tip every 3 seconds. If the tip is
     * sufficiently far behind, allow up to 20 seconds for the next tip update.
     *
     * It's not safe to keep waiting, because a malicious miner could announce
     * a header and delay revealing the block, causing all other miners using
     * this software to stall. At the same time, we need to balance between the
     * default waiting time being brief, but not ending the cooldown prematurely
     * when a random block is slow to download (or process).
     *
     * The cooldown only applies to createNewBlock(), which is typically called
     * once per connected client. Subsequent templates are provided by
     * waitNext().
     *
     * @param last_tip tip at the start of the cooldown window.
     * @param interrupt_mining set to true to interrupt the cooldown.
     *
     * @returns false if interrupted.
     */
    bool CooldownIfHeadersAhead(const interfaces::BlockRef& last_tip, bool& interrupt_mining);

    /** Interrupt a blocking wait. */
    void InterruptWait(bool& interrupt_wait);

    /** Return a new block template when fees rise to a certain threshold or
     *  after a new tip; return nullptr if timeout is reached. */
    std::unique_ptr<CBlockTemplate> WaitAndCreateNewBlock(
        const std::unique_ptr<CBlockTemplate>& block_template,
        const BlockWaitOptions& wait_options,
        const BlockCreateOptions& create_options,
        bool& interrupt_wait);
};
} // namespace node

#endif // BITCOIN_NODE_BLOCK_TEMPLATE_MANAGER_H
