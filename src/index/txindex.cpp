// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/txindex.h>
#include <util.h>
#include <validation.h>

std::unique_ptr<TxIndex> g_txindex;

TxIndex::TxIndex(size_t n_cache_size, bool f_memory, bool f_wipe)
    : m_db(MakeUnique<TxIndex::DB>(n_cache_size, f_memory, f_wipe))
{}

bool TxIndex::Init()
{
    LOCK(cs_main);

    // Attempt to migrate txindex from the old database to the new one. Even if
    // chain_tip is null, the node could be reindexing and we still want to
    // delete txindex records in the old database.
    if (!m_db->MigrateData(*pblocktree, chainActive.GetLocator())) {
        return false;
    }

    return BaseIndex::Init();
}

bool TxIndex::WriteBlock(const CBlock& block, const CBlockIndex* pindex)
{
    CDiskTxPos pos(pindex->GetBlockPos(), GetSizeOfCompactSize(block.vtx.size()));
    std::vector<std::pair<uint256, CDiskTxPos>> vPos;
    vPos.reserve(block.vtx.size());
    for (const auto& tx : block.vtx) {
        vPos.emplace_back(tx->GetHash(), pos);
        pos.nTxOffset += ::GetSerializeSize(*tx, SER_DISK, CLIENT_VERSION);
    }
    return m_db->WriteTxs(vPos);
}

BaseIndexDB& TxIndex::GetDB() const { return *m_db; }

bool TxIndex::FindTx(const uint256& tx_hash, uint256& block_hash, CTransactionRef& tx) const
{
    CDiskTxPos postx;
    if (!m_db->ReadTxPos(tx_hash, postx)) {
        return false;
    }

    CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
    if (file.IsNull()) {
        return error("%s: OpenBlockFile failed", __func__);
    }
    CBlockHeader header;
    try {
        file >> header;
        if (fseek(file.Get(), postx.nTxOffset, SEEK_CUR)) {
            return error("%s: fseek(...) failed", __func__);
        }
        file >> tx;
    } catch (const std::exception& e) {
        return error("%s: Deserialize or I/O error - %s", __func__, e.what());
    }
    if (tx->GetHash() != tx_hash) {
        return error("%s: txid mismatch", __func__);
    }
    block_hash = header.GetHash();
    return true;
}
