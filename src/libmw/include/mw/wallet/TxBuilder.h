#pragma once

#include <mw/models/crypto/BlindingFactor.h>
#include <mw/models/tx/Transaction.h>
#include <mw/models/tx/PegInCoin.h>
#include <mw/models/tx/PegOutCoin.h>
#include <mw/models/wallet/Coin.h>
#include <mw/models/wallet/Recipient.h>

class TxBuilder
{
    struct Inputs
    {
        BlindingFactor total_blind;
        SecretKey total_key;
        std::vector<Input> inputs;
    };

    struct Outputs
    {
        BlindingFactor total_blind;
        SecretKey total_key;
        std::vector<Output> outputs;
        std::vector<mw::Coin> coins;
    };

public:
    static mw::Transaction::CPtr BuildTx(
        const std::vector<mw::Coin>& input_coins,
        const std::vector<mw::Recipient>& recipients,
        const std::vector<PegOutCoin>& pegouts,
        const boost::optional<CAmount>& pegin_amount,
        const CAmount fee,
        std::vector<mw::Coin>& output_coins
    );

private:
    static Inputs CreateInputs(const std::vector<mw::Coin>& input_coins);
    static Outputs CreateOutputs(const std::vector<mw::Recipient>& recipients);
    static CAmount TotalAmount(const std::vector<mw::Coin>& coins);
};