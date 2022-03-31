#pragma once

#include <amount.h>
#include <key.h>
#include <mw/models/block/Block.h>
#include <mw/models/tx/Transaction.h>
#include <mw/models/wallet/Coin.h>
#include <mw/models/wallet/StealthAddress.h>
#include <mw/wallet/Keychain.h>
#include <streams.h>
#include <util/strencodings.h>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <map>
#include <set>

class CWallet;

namespace MWEB {

class Wallet
{
    CWallet* m_pWallet;
    std::map<mw::Hash, mw::Coin> m_coins;

public:
    Wallet(CWallet* pWallet)
        : m_pWallet(pWallet) {}

    bool IsChange(const StealthAddress& address) const;
    bool GetCoin(const mw::Hash& output_id, mw::Coin& coin) const;

    // Loops through the transactions in the wallet and attempts to fill
    // in missing information for mw::Coin's, in particular the spend key.
    // Intended to be called after unlocking an encrypted wallet.
    bool UpgradeCoins();

    std::vector<mw::Coin> RewindOutputs(const CTransaction& tx);
    bool RewindOutput(const Output& output, mw::Coin& coin);

    bool GetStealthAddress(const mw::Coin& coin, StealthAddress& address) const;
    bool GetStealthAddress(const uint32_t index, StealthAddress& address) const;

    void LoadToWallet(const mw::Coin& coin);
    void SaveToWallet(const std::vector<mw::Coin>& coins);

private:
    mw::Keychain::Ptr GetKeychain() const;
};

struct WalletTxInfo
{
    // When connecting a block, if an output is found that belongs to us,
    // we check if we have a CWalletTx that created it.
    // If none is found, then we assume it is a newly received coin,
    // so we create an empty transaction and store the received coin here.
    boost::optional<mw::Coin> received_coin;

    // When connecting a block, if one of the wallet's coins is spent,
    // we check if we have a CWalletTx that spent it.
    // If none is found, then we assume it was spent by another wallet,
    // so we create an empty Transaction and store the spent hash here.
    boost::optional<mw::Hash> spent_input;

    mutable uint256 hash;

    WalletTxInfo()
        : received_coin(boost::none), spent_input(boost::none), hash() { }
    WalletTxInfo(mw::Coin received)
        : received_coin(std::move(received)), spent_input(boost::none), hash(received_coin->output_id.vec()) {}
    WalletTxInfo(mw::Hash spent)
        : received_coin(boost::none), spent_input(std::move(spent)), hash(spent_input->vec()) {}

    SERIALIZE_METHODS(WalletTxInfo, obj)
    {
        bool received = !!obj.received_coin;
        READWRITE(received);

        if (received) {
            mw::Coin coin;
            SER_WRITE(obj, coin = *obj.received_coin);
            READWRITE(coin);
            SER_READ(obj, obj.hash = uint256(coin.output_id.vec()));
            SER_READ(obj, obj.received_coin = boost::make_optional<mw::Coin>(std::move(coin)));
        } else {
            mw::Hash output_id;
            SER_WRITE(obj, output_id = *obj.spent_input);
            READWRITE(output_id);
            SER_READ(obj, obj.hash = uint256(output_id.vec()));
            SER_READ(obj, obj.spent_input = boost::make_optional<mw::Hash>(std::move(output_id)));
        }
    }

    static WalletTxInfo FromHex(const std::string& str)
    {
        std::vector<uint8_t> bytes = ParseHex(str);
        CDataStream stream((const char*)bytes.data(), (const char*)bytes.data() + bytes.size(), SER_DISK, PROTOCOL_VERSION);

        WalletTxInfo wtx_info;
        stream >> wtx_info;
        return wtx_info;
    }

    std::string ToHex() const
    {
        CDataStream stream(SER_DISK, PROTOCOL_VERSION);
        stream << *this;
        return HexStr(std::vector<uint8_t>{stream.begin(), stream.end()});
    }

    const uint256& GetHash() const {
        return hash;
    }
};

} // namespace MWEB