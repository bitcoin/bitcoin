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

bool ContainsPegIn(const TxType& mweb_type, const std::set<CInputCoin>& input_coins);
bool IsChangeOnMWEB(const CWallet& wallet, const MWEB::TxType& mweb_type, const std::vector<CRecipient>& recipients, const CTxDestination& dest_change);
uint64_t CalcMWEBWeight(const MWEB::TxType& mweb_type, const bool change_on_mweb, const std::vector<CRecipient>& recipients);
int64_t CalcPegOutBytes(const TxType& mweb_type, const std::vector<CRecipient>& recipients);

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
        const CAmount& ltc_change,
        const bool include_mweb_change,
        bilingual_str& error
    );

private:
    static mw::Recipient BuildChangeRecipient();
};

}