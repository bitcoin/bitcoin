#pragma once

#include <mweb/mweb_models.h>
#include <mweb/mweb_wallet.h>
#include <mw/models/wallet/Recipient.h>
#include <wallet/coinselection.h>
#include <wallet/wallet.h>
#include <boost/optional.hpp>
#include <numeric>

// Forward Declarations
struct InProcessTx;

namespace MWEB {

enum class TxType {
    LTC_TO_LTC,
    MWEB_TO_MWEB,
    PEGIN,
    PEGOUT // NOTE: It's possible pegout transactions will also have pegins, but they will still be classified as PEGOUT
};

TxType GetTxType(const std::vector<CRecipient>& recipients, const std::set<CInputCoin>& input_coins);
bool ContainsPegIn(const TxType& mweb_type, const std::set<CInputCoin>& input_coins);
bool IsChangeOnMWEB(const CWallet& wallet, const MWEB::TxType& mweb_type, const std::vector<CRecipient>& recipients, const CTxDestination& dest_change);
uint64_t CalcMWEBWeight(const MWEB::TxType& mweb_type, const bool change_on_mweb, const std::vector<CRecipient>& recipients);
int64_t CalcPegOutBytes(const TxType& mweb_type, const std::vector<CRecipient>& recipients);

class Transact
{
    const CWallet& m_wallet;

public:
    Transact(const CWallet& wallet)
        : m_wallet(wallet) {}

    void AddMWEBTx(InProcessTx& new_tx);

private:
    mw::Recipient BuildChangeRecipient(
        const InProcessTx& new_tx,
        const boost::optional<CAmount>& pegin_amount,
        const CAmount& ltc_change
    );
};

}