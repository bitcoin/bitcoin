#include <mweb/mweb_wallet.h>
#include <wallet/wallet.h>
#include <wallet/coincontrol.h>
#include <util/bip32.h>

using namespace MWEB;

std::vector<mw::Coin> Wallet::RewindOutputs(const CTransaction& tx)
{
    std::vector<mw::Coin> coins;
    for (const CTxOutput& txout : tx.GetOutputs()) {
        if (txout.IsMWEB()) {
            mw::Coin mweb_coin;
            if (RewindOutput(tx.mweb_tx.m_transaction, txout.ToMWEB(), mweb_coin)) {
                coins.push_back(mweb_coin);
            }
        }
    }

    return coins;
}

bool Wallet::RewindOutput(const boost::variant<mw::Block::CPtr, mw::Transaction::CPtr>& parent,
        const mw::Hash& output_id, mw::Coin& coin)
{
    if (GetCoin(output_id, coin) && coin.IsMine()) {
        return true;
    }

    mw::Keychain::Ptr keychain = GetKeychain();
    if (!keychain) {
        return false;
    }

    bool rewound = false;
    if (parent.type() == typeid(mw::Block::CPtr)) {
        const mw::Block::CPtr& block = boost::get<mw::Block::CPtr>(parent);
        for (const Output& output : block->GetOutputs()) {
            if (output.GetOutputID() == output_id) {
                rewound = keychain->RewindOutput(output, coin);
                break;
            }
        }
    } else {
        const mw::Transaction::CPtr& tx = boost::get<mw::Transaction::CPtr>(parent);
        for (const Output& output : tx->GetOutputs()) {
            if (output.GetOutputID() == output_id) {
                rewound = keychain->RewindOutput(output, coin);
                break;
            }
        }
    }

    if (rewound) {
        m_coins[coin.output_id] = coin;
        WalletBatch(m_pWallet->GetDatabase()).WriteMWEBCoin(coin);
    }

    return rewound;
}

bool Wallet::IsChange(const StealthAddress& address) const
{
    StealthAddress change_addr;
    return GetStealthAddress(mw::CHANGE_INDEX, change_addr) && change_addr == address;
}

bool Wallet::GetStealthAddress(const mw::Coin& coin, StealthAddress& address) const
{
    if (coin.HasAddress()) {
        address = *coin.address;
        return true;
    }

    return GetStealthAddress(coin.address_index, address);
}

bool Wallet::GetStealthAddress(const uint32_t index, StealthAddress& address) const
{
    mw::Keychain::Ptr keychain = GetKeychain();
    if (!keychain || index == mw::UNKNOWN_INDEX) {
        return false;
    }

    address = keychain->GetStealthAddress(index);
    return true;
}

void Wallet::LoadToWallet(const mw::Coin& coin)
{
    m_coins[coin.output_id] = coin;
}

void Wallet::SaveToWallet(const std::vector<mw::Coin>& coins)
{
    WalletBatch batch(m_pWallet->GetDatabase());
    for (const mw::Coin& coin : coins) {
        m_coins[coin.output_id] = coin;
        batch.WriteMWEBCoin(coin);
    }
}

bool Wallet::GetCoin(const mw::Hash& output_id, mw::Coin& coin) const
{
    auto iter = m_coins.find(output_id);
    if (iter != m_coins.end()) {
        coin = iter->second;
        return true;
    }

    return false;
}

mw::Keychain::Ptr Wallet::GetKeychain() const
{
    auto spk_man = m_pWallet->GetScriptPubKeyMan(OutputType::MWEB, false);
    return spk_man ? spk_man->GetMWEBKeychain() : nullptr;
}