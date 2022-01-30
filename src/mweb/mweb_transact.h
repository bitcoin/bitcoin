#pragma once

#include <mweb/mweb_models.h>
#include <mweb/mweb_wallet.h>
#include <wallet/coinselection.h>
#include <wallet/wallet.h>
#include <boost/optional.hpp>
#include <numeric>

namespace MWEB {

enum class TxType {
    LTC_TO_LTC,
    MWEB_TO_MWEB,
    PEGIN,
    PEGOUT // NOTE: It's possible pegout transactions will also have pegins, but they will still be classified as PEGOUT
};

TxType GetTxType(const std::vector<CRecipient>& recipients, const std::vector<CInputCoin>& input_coins);

class Transact
{
public:
    static bool CreateTx(
        const std::shared_ptr<MWEB::Wallet>& mweb_wallet,
        CMutableTransaction& transaction,
        const std::vector<CInputCoin>& selected_coins,
        const std::vector<CRecipient>& recipients,
        const CAmount& ltc_fee,
        const CAmount& mweb_fee,
        const bool include_mweb_change
    );

private:
    static std::vector<mw::Coin> GetInputCoins(const std::vector<CInputCoin>& inputs);
    static CAmount GetMWEBInputAmount(const std::vector<CInputCoin>& inputs);
    static CAmount GetLTCInputAmount(const std::vector<CInputCoin>& inputs);
    static CAmount GetMWEBRecipientAmount(const std::vector<CRecipient>& recipients);
    static bool UpdatePegInOutput(CMutableTransaction& transaction, const PegInCoin& pegin);
};

}