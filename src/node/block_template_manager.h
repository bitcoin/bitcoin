// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_BLOCK_TEMPLATE_MANAGER_H
#define BITCOIN_NODE_BLOCK_TEMPLATE_MANAGER_H

#include <node/mining_types.h>

#include <memory>

class CBlock;
class ChainstateManager;
class CTxMemPool;

namespace node {
struct CBlockTemplate;

/** Creates block templates. */
class BlockTemplateManager
{
private:
    CTxMemPool& m_mempool;
    ChainstateManager& m_chainman;
    const BlockCreateOptions m_init_block_create_options;

public:
    explicit BlockTemplateManager(CTxMemPool& mempool,
                                  ChainstateManager& chainman,
                                  BlockCreateOptions init_block_create_options = {});

    /** @return a copy of the block create options set during node init. */
    BlockCreateOptions GetInitBlockCreateOptions() const { return m_init_block_create_options; }

    /** Create a fresh block template. */
    std::unique_ptr<CBlockTemplate> CreateNewTemplate(const BlockCreateOptions& options);

    /** Submit a block via ProcessNewBlock and capture validation state. */
    bool SubmitBlock(const std::shared_ptr<const CBlock>& block, bool* new_block, std::string& reason, std::string& debug);
};
} // namespace node

#endif // BITCOIN_NODE_BLOCK_TEMPLATE_MANAGER_H
