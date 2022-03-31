#pragma once

#include <mweb/mweb_transact.h>
#include <policy/fees.h>
#include <script/standard.h>
#include <wallet/coincontrol.h>
#include <wallet/reserve.h>
#include <wallet/wallet.h>
#include <util/translation.h>

struct InProcessTx {
    CMutableTransaction tx{};
    std::vector<CRecipient> recipients;
    CAmount recipient_amount;

    std::unique_ptr<ReserveDestination> reserve_dest{nullptr};
    DestinationAddr change_addr{};
    int change_position{-1}; // MW: TODO - Change this to OutputIndex
    int bytes{0};

    CCoinControl coin_control;
    CoinSelectionParams coin_selection_params{};
    std::vector<COutputCoin> available_coins{};
    std::set<CInputCoin> selected_coins{};
    CAmount value_selected{0};
    bool bnb_used{false};

    FeeCalculation fee_calc{};
    CAmount fee_needed{0};
    CAmount total_fee{0};
    unsigned int subtract_fee_from_amount{0};

    // MWEB
    uint64_t mweb_weight{0};
    MWEB::TxType mweb_type{MWEB::TxType::LTC_TO_LTC};
    bool change_on_mweb{false};
    CAmount mweb_fee{0}; // Portion of the fee that will be paid by an MWEB kernel
};

class CreateTxError : public std::runtime_error
{
public:
    CreateTxError(const bilingual_str& error)
        : std::runtime_error(error.original), m_error(error) {}
    ~CreateTxError() = default;

    const bilingual_str& GetError() const { return m_error; }

private:
    bilingual_str m_error;
};

struct AssembledTx {
    CTransactionRef tx;
    CAmount fee;
    FeeCalculation fee_calc; // MW: TODO - Really just needs FeeReason
    int change_position;
};

class TxAssembler
{
    CWallet& m_wallet;

public:
    explicit TxAssembler(CWallet& wallet)
        : m_wallet(wallet) {}

    Optional<AssembledTx> AssembleTx(
        const std::vector<CRecipient>& recipients,
        const CCoinControl& coin_control,
        const int nChangePosRequest,
        const bool sign,
        bilingual_str& errorOut
    );

private:
    AssembledTx CreateTransaction(
        const std::vector<CRecipient>& recipients,
        const CCoinControl& coin_control,
        const int nChangePosRequest,
        const bool sign
    );

    void CreateTransaction_Locked(
        InProcessTx& new_tx,
        const int nChangePosRequest,
        const bool sign
    );

    void VerifyRecipients(const std::vector<CRecipient>& recipients);

    // Adds the output to 'txNew' for each non-MWEB recipient.
    // If the recipient is marked as 'fSubtractFeeFromAmount', 'nFeeRet' will be deducted from the amount.
    //
    // Returns false and populates the 'error' message if an error occurs.
    // Currently, this will only occur when a recipient amount is considered dust.
    void AddRecipientOutputs(InProcessTx& new_tx);
    void AddChangeOutput(InProcessTx& new_tx, const CAmount& nChange) const;
    void AddTxInputs(InProcessTx& new_tx) const;

    uint32_t GetLocktimeForNewTransaction() const;
    bool IsCurrentForAntiFeeSniping(interfaces::Chain& chain, const uint256& block_hash) const;

    //
    // Coin Selection
    //
    void InitCoinSelectionParams(InProcessTx& new_tx) const;
    bool AttemptCoinSelection(
        InProcessTx& new_tx,
        const CAmount& nTargetValue
    ) const;
    bool SelectCoins(
        InProcessTx& new_tx,
        const CAmount& nTargetValue,
        CoinSelectionParams& coin_selection_params
    ) const;

    // Reduce fee to only the needed amount if possible. This
    // prevents potential overpayment in fees if the coins
    // selected to meet nFeeNeeded result in a transaction that
    // requires less fee than the prior iteration.
    void ReduceFee(InProcessTx& new_tx) const;

    int64_t CalculateMaximumTxSize(const InProcessTx& new_tx) const;

    void UpdateChangeAddress(InProcessTx& new_tx) const;
    OutputType GetChangeType(const InProcessTx& new_tx) const;
};