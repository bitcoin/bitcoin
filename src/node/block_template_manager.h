// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_BLOCK_TEMPLATE_MANAGER_H
#define BITCOIN_NODE_BLOCK_TEMPLATE_MANAGER_H

#include <node/mining_types.h>

#include <memory>

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

public:
    explicit BlockTemplateManager(CTxMemPool& mempool,
                                  ChainstateManager& chainman);

    /** Create a fresh block template. */
    std::unique_ptr<CBlockTemplate> CreateNewTemplate(const BlockCreateOptions& options);
};
} // namespace node

#endif // BITCOIN_NODE_BLOCK_TEMPLATE_MANAGER_H
