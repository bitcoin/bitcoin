// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/block_template_manager.h>

#include <node/miner.h>
#include <validation.h>

namespace node {

BlockTemplateManager::BlockTemplateManager(CTxMemPool& mempool, ChainstateManager& chainman)
    : m_mempool(mempool), m_chainman(chainman)
{
}

std::unique_ptr<CBlockTemplate> BlockTemplateManager::CreateNewTemplate(const BlockCreateOptions& options)
{
    return BlockAssembler{m_chainman.ActiveChainstate(), &m_mempool, options}.CreateNewBlock();
}

} // namespace node
