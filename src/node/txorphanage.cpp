// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txorphanage.h>
#include <node/txorphanage_impl.h>

namespace node{

TxOrphanage::TxOrphanage() : m_impl{std::make_unique<TxOrphanageImpl>()} {}
TxOrphanage::~TxOrphanage() = default;

bool TxOrphanage::AddTx(const CTransactionRef& tx, NodeId peer)
{
    return m_impl->AddTx(tx, peer);
}

bool TxOrphanage::AddAnnouncer(const Wtxid& wtxid, NodeId peer)
{
    return m_impl->AddAnnouncer(wtxid, peer);
}

int TxOrphanage::EraseTx(const Wtxid& wtxid)
{
    return m_impl->EraseTx(wtxid);
}

void TxOrphanage::EraseForPeer(NodeId peer)
{
    m_impl->EraseForPeer(peer);
}

void TxOrphanage::LimitOrphans()
{
    m_impl->LimitOrphans();
}

void TxOrphanage::AddChildrenToWorkSet(const CTransaction& tx, FastRandomContext& rng)
{
    m_impl->AddChildrenToWorkSet(tx, rng);
}

bool TxOrphanage::HaveTx(const Wtxid& wtxid) const
{
    return m_impl->HaveTx(wtxid);
}

CTransactionRef TxOrphanage::GetTx(const Wtxid& wtxid) const
{
    return m_impl->GetTx(wtxid);
}

bool TxOrphanage::HaveTxFromPeer(const Wtxid& wtxid, NodeId peer) const
{
    return m_impl->HaveTxFromPeer(wtxid, peer);
}

CTransactionRef TxOrphanage::GetTxToReconsider(NodeId peer)
{
    return m_impl->GetTxToReconsider(peer);
}

bool TxOrphanage::HaveTxToReconsider(NodeId peer)
{
    return m_impl->HaveTxToReconsider(peer);
}

void TxOrphanage::EraseForBlock(const CBlock& block)
{
    m_impl->EraseForBlock(block);
}

std::vector<CTransactionRef> TxOrphanage::GetChildrenFromSamePeer(const CTransactionRef& parent, NodeId nodeid) const
{
    return m_impl->GetChildrenFromSamePeer(parent, nodeid);
}

std::vector<TxOrphanage::OrphanTxBase> TxOrphanage::GetOrphanTransactions() const
{
    return m_impl->GetOrphanTransactions();
}

void TxOrphanage::SanityCheck() const
{
    m_impl->SanityCheck();
}

size_t TxOrphanage::Size() const
{
    return m_impl->CountUniqueOrphans();
}

int64_t TxOrphanage::TotalOrphanUsage() const
{
    return m_impl->TotalOrphanUsage();
}

int64_t TxOrphanage::UsageByPeer(NodeId peer) const
{
    return m_impl->UsageFromPeer(peer);
}
} // namespace node
