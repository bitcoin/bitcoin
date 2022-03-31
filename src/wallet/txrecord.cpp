#include <wallet/txrecord.h>
#include <wallet/wallet.h>
#include <wallet/txrecord.h>
#include <wallet/wallet.h>

#include <chain.h>
#include <core_io.h>
#include <interfaces/chain.h>
#include <key_io.h>
#include <util/check.h>

#include <chrono>

using namespace std::chrono;

bool WalletTxRecord::UpdateStatusIfNeeded(const uint256& block_hash)
{
    assert(!block_hash.IsNull());

    // Check if update is needed
    if (status.m_cur_block_hash == block_hash && !status.needsUpdate) {
        return false;
    }

    // Try locking the wallet
    TRY_LOCK(m_pWallet->cs_wallet, locked_wallet);
    if (!locked_wallet) {
        return false;
    }

    const int last_block_height = m_pWallet->GetLastBlockHeight();
    int64_t block_time = -1;
    CHECK_NONFATAL(m_pWallet->chain().findBlock(m_pWallet->GetLastBlockHash(), interfaces::FoundBlock().time(block_time)));

    // Sort order, unrecorded transactions sort to the top
    // Sub-components sorted with standard outputs first, MWEB outputs second, then MWEB pegouts third.
    std::string idx = strprintf("0%03d", 0);
    if (index) {
        if (index->type() == typeid(int)) {
            idx = strprintf("0%03d", boost::get<int>(*index));
        } else if (index->type() == typeid(mw::Hash)) {
            idx = "1" + boost::get<mw::Hash>(*index).ToHex();
        } else {
            const PegoutIndex& pegout_idx = boost::get<PegoutIndex>(*index);
            idx = "2" + pegout_idx.kernel_id.ToHex() + strprintf("%03d", pegout_idx.pos);
        }
    }

    int block_height = m_wtx->m_confirm.block_height > 0 ? m_wtx->m_confirm.block_height : std::numeric_limits<int>::max();
    int blocks_to_maturity = m_wtx->GetBlocksToMaturity();
    
    status.sortKey = strprintf("%010d-%01d-%010u-%s",
                               block_height,
                               m_wtx->IsCoinBase() ? 1 : 0,
                               m_wtx->nTimeReceived,
                               idx);
    status.countsForBalance = m_wtx->IsTrusted() && !(blocks_to_maturity > 0);
    status.depth = m_wtx->GetDepthInMainChain();
    status.m_cur_block_hash = block_hash;

    if (m_wtx->IsInMainChain()) {
        status.matures_in = blocks_to_maturity;
    }

    const int64_t time_since_epoch = (int64_t)duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    const bool up_to_date = (time_since_epoch - block_time < MAX_BLOCK_TIME_GAP);
    if (up_to_date && !m_wtx->IsFinal()) {
        if (m_wtx->tx->nLockTime < LOCKTIME_THRESHOLD) {
            status.status = WalletTxStatus::OpenUntilBlock;
            status.open_for = m_wtx->tx->nLockTime - last_block_height;
        } else {
            status.status = WalletTxStatus::OpenUntilDate;
            status.open_for = m_wtx->tx->nLockTime;
        }
    }

    // For generated transactions, determine maturity
    else if (type == WalletTxRecord::Type::Generated || m_wtx->IsHogEx()) {
        if (blocks_to_maturity > 0) {
            status.status = WalletTxStatus::Immature;

            if (!m_wtx->IsInMainChain()) {
                status.status = WalletTxStatus::NotAccepted;
            }
        } else {
            status.status = WalletTxStatus::Confirmed;
        }
    } else {
        if (status.depth < 0) {
            status.status = WalletTxStatus::Conflicted;
        } else if (status.depth == 0) {
            status.status = WalletTxStatus::Unconfirmed;
            if (m_wtx->isAbandoned())
                status.status = WalletTxStatus::Abandoned;
        } else if (status.depth < RecommendedNumConfirmations) {
            status.status = WalletTxStatus::Confirming;
        } else {
            status.status = WalletTxStatus::Confirmed;
        }
    }

    status.needsUpdate = false;
    return true;
}

const uint256& WalletTxRecord::GetTxHash() const
{
    assert(m_wtx != nullptr);
    return m_wtx->GetHash();
}

std::string WalletTxRecord::GetTxString() const
{
    assert(m_wtx != nullptr);

    if (m_wtx->IsPartialMWEB()) {
        if (m_wtx->mweb_wtx_info->received_coin) {
            const mw::Coin& received_coin = *m_wtx->mweb_wtx_info->received_coin;
            return strprintf("MWEB Output(ID=%s, amount=%d)", received_coin.output_id.ToHex(), received_coin.amount);
        } else if (m_wtx->mweb_wtx_info->spent_input) {
            return strprintf("MWEB Input(ID=%s)\n", m_wtx->mweb_wtx_info->spent_input->ToHex());
        }
    }

    return m_wtx->tx->ToString();
}

int64_t WalletTxRecord::GetTxTime() const
{
    assert(m_wtx != nullptr);
    return m_wtx->GetTxTime();
}

std::string WalletTxRecord::GetComponentIndex() const
{
    std::string idx;
    if (this->index) {
        if (this->index->type() == typeid(int)) {
            idx = std::to_string(boost::get<int>(*index));
        } else if (index->type() == typeid(mw::Hash)) {
            idx = "MWEB Output (" + boost::get<mw::Hash>(*index).ToHex() + ")";
        } else {
            const PegoutIndex& pegout_idx = boost::get<PegoutIndex>(*index);
            idx = "Pegout (" + pegout_idx.kernel_id.ToHex() + ":" + std::to_string(pegout_idx.pos) + ")";
        }
    }

    return idx;
}

std::string WalletTxRecord::GetType() const
{
    switch (this->type) {
        case Other: return "Other";
        case Generated: return "Generated";
        case SendToAddress: return "SendToAddress";
        case SendToOther: return "SendToOther";
        case RecvWithAddress: return "RecvWithAddress";
        case RecvFromOther: return "RecvFromOther";
        case SendToSelf: return "SendToSelf";
    }

    assert(false);
}

UniValue WalletTxRecord::ToUniValue() const
{
    UniValue entry(UniValue::VOBJ);

    entry.pushKV("type", GetType());
    entry.pushKV("amount", ValueFromAmount(GetAmount()));
    entry.pushKV("net", ValueFromAmount(GetNet()));

    CTxDestination destination = DecodeDestination(this->address);
    if (m_wtx->IsFromMe(ISMINE_WATCH_ONLY) || (IsValidDestination(destination) && (m_pWallet->IsMine(destination) & ISMINE_WATCH_ONLY))) {
        entry.pushKV("involvesWatchonly", true);
    }

    if (IsValidDestination(destination)) {
        const auto* address_book_entry = m_pWallet->FindAddressBookEntry(destination);
        if (address_book_entry) {
            entry.pushKV("label", address_book_entry->GetLabel());
        }
    }

    if (!this->address.empty()) {
        entry.pushKV("address", this->address);
    }

    if (this->index) {
        if (this->index->type() == typeid(int)) {
            entry.pushKV("vout", boost::get<int>(*this->index));
        } else if (this->index->type() == typeid(mw::Hash)) {
            entry.pushKV("mweb_out", boost::get<mw::Hash>(*this->index).ToHex());
        } else {
            const PegoutIndex& pegout_idx = boost::get<PegoutIndex>(*index);
            entry.pushKV("pegout", pegout_idx.kernel_id.ToHex() + ":" + std::to_string(pegout_idx.pos));
        }
    }

    if (this->debit > 0) {
        entry.pushKV("fee", ValueFromAmount(-this->fee));
        entry.pushKV("abandoned", m_wtx->isAbandoned());
    }

    return entry;
}