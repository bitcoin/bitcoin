#include <mweb/mweb_transact.h>
#include <key_io.h>

#include <mw/models/tx/PegOutCoin.h>
#include <mw/wallet/TxBuilder.h>

using namespace MWEB;

TxType MWEB::GetTxType(const std::vector<CRecipient>& recipients, const std::vector<CInputCoin>& input_coins)
{
    assert(!recipients.empty());

    static auto is_ltc = [](const CInputCoin& input) { return !input.IsMWEB(); };
    static auto is_mweb = [](const CInputCoin& input) { return input.IsMWEB(); };

    if (recipients.front().IsMWEB()) {
        // If any inputs are non-MWEB inputs, this is a peg-in transaction.
        // Otherwise, it's a simple MWEB-to-MWEB transaction.
        if (std::any_of(input_coins.cbegin(), input_coins.cend(), is_ltc)) {
            return TxType::PEGIN;
        } else {
            return TxType::MWEB_TO_MWEB;
        }
    } else {
        // If any inputs are MWEB inputs, this is a peg-out transaction.
        // NOTE: This does not exclude the possibility that it's also pegging-in in addition to the pegout.
        // Otherwise, if there are no MWEB inputs, it's a simple LTC-to-LTC transaction.
        if (std::any_of(input_coins.cbegin(), input_coins.cend(), is_mweb)) {
            return TxType::PEGOUT;
        } else {
            return TxType::LTC_TO_LTC;
        }
    }
}

bool Transact::CreateTx(
    const std::shared_ptr<MWEB::Wallet>& mweb_wallet,
    CMutableTransaction& transaction,
    const std::vector<CInputCoin>& selected_coins,
    const std::vector<CRecipient>& recipients,
    const CAmount& ltc_fee,
    const CAmount& mweb_fee,
    const bool include_mweb_change)
{
    TxType type = MWEB::GetTxType(recipients, selected_coins);
    if (type == TxType::LTC_TO_LTC) {
        return true;
    }

    // Add recipients
    std::vector<mw::Recipient> receivers;
    std::vector<PegOutCoin> pegouts;
    for (const CRecipient& recipient : recipients) {
        CAmount recipient_amount = recipient.nAmount;
        if (recipient.fSubtractFeeFromAmount) {
            recipient_amount -= ltc_fee;
        }

        if (recipient.IsMWEB()) {
            receivers.push_back(mw::Recipient{recipient_amount, recipient.GetMWEBAddress()});
        } else {
            PegOutCoin pegout_recipient(
                recipient_amount,
                recipient.receiver.GetScript()
            );
            pegouts.push_back(std::move(pegout_recipient));
        }
    }

    // Calculate pegin_amount
    boost::optional<CAmount> pegin_amount = boost::none;
    CAmount ltc_input_amount = GetLTCInputAmount(selected_coins);
    if (ltc_input_amount > 0) {
        assert(ltc_fee < ltc_input_amount);
        pegin_amount = (ltc_input_amount - ltc_fee + mweb_fee); // MW: TODO - There could also be LTC change
    }

    // Add Change
    if (include_mweb_change) {
        CAmount recipient_amount = std::accumulate(
            recipients.cbegin(), recipients.cend(), CAmount(0),
            [ltc_fee](CAmount amount, const CRecipient& recipient) {
                return amount + (recipient.nAmount - (recipient.fSubtractFeeFromAmount ? ltc_fee : 0));
            }
        );

        CAmount change_amount = (pegin_amount.value_or(0) + GetMWEBInputAmount(selected_coins)) - (recipient_amount + mweb_fee);
        if (change_amount < 0) {
            return false;
        }
        StealthAddress change_address;
        if (!mweb_wallet->GetStealthAddress(mw::CHANGE_INDEX, change_address)) {
            return false;
        }

        receivers.push_back(mw::Recipient{change_amount, change_address});
    }

    // Create transaction
    std::vector<mw::Coin> input_coins = GetInputCoins(selected_coins);
    std::vector<mw::Coin> output_coins;

    try {
        transaction.mweb_tx = TxBuilder::BuildTx(
            input_coins,
            receivers,
            pegouts,
            pegin_amount,
            mweb_fee,
            output_coins
        );
    } catch (std::exception&) {
        return false;
    }

    if (!output_coins.empty()) {
        mweb_wallet->SaveToWallet(output_coins);
    }

    // Update pegin output
    auto pegins = transaction.mweb_tx.GetPegIns();
    if (!pegins.empty()) {
        UpdatePegInOutput(transaction, pegins.front());
    }

    return true;
}

std::vector<mw::Coin> Transact::GetInputCoins(const std::vector<CInputCoin>& inputs)
{
    std::vector<mw::Coin> input_coins;
    for (const auto& coin : inputs) {
        if (coin.IsMWEB()) {
            input_coins.push_back(coin.GetMWEBCoin());
        }
    }

    return input_coins;
}

CAmount Transact::GetMWEBInputAmount(const std::vector<CInputCoin>& inputs)
{
    return std::accumulate(
        inputs.cbegin(), inputs.cend(), CAmount(0),
        [](CAmount amt, const CInputCoin& input) { return amt + (input.IsMWEB() ? input.GetAmount() : 0); });
}

CAmount Transact::GetLTCInputAmount(const std::vector<CInputCoin>& inputs)
{
    return std::accumulate(
        inputs.cbegin(), inputs.cend(), CAmount(0),
        [](CAmount amt, const CInputCoin& input) { return amt + (input.IsMWEB() ? 0 : input.GetAmount()); });
}

CAmount Transact::GetMWEBRecipientAmount(const std::vector<CRecipient>& recipients)
{
    return std::accumulate(
        recipients.cbegin(), recipients.cend(), CAmount(0),
        [](CAmount amt, const CRecipient& recipient) { return amt + (recipient.IsMWEB() ? recipient.nAmount : 0); });
}

bool Transact::UpdatePegInOutput(CMutableTransaction& transaction, const PegInCoin& pegin)
{
    for (size_t i = 0; i < transaction.vout.size(); i++) {
        if (IsPegInOutput(CTransaction(transaction).GetOutput(i))) {
            transaction.vout[i].nValue = pegin.GetAmount();
            transaction.vout[i].scriptPubKey = GetScriptForPegin(pegin.GetKernelID());
            return true;
        }
    }

    return false;
}