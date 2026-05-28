// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/block_template_manager.h>

#include <node/miner.h>
#include <node/mining_args.h>
#include <validation.h>

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

} // namespace node
