#include <mw/wallet/TxBuilder.h>

#include <mw/consensus/Weight.h>
#include <mw/crypto/Blinds.h>
#include <mw/exceptions/InsufficientFundsException.h>
#include <numeric>

mw::Transaction::CPtr TxBuilder::BuildTx(
    const std::vector<mw::Coin>& input_coins,
    const std::vector<mw::Recipient>& recipients,
    const std::vector<PegOutCoin>& pegouts,
    const boost::optional<CAmount>& pegin_amount,
    const CAmount fee)
{
    CAmount pegout_total = std::accumulate(
        pegouts.cbegin(), pegouts.cend(), (CAmount)0,
        [](CAmount sum, const PegOutCoin& pegout) { return sum + pegout.GetAmount(); }
    );

    CAmount recipient_total = std::accumulate(
        recipients.cbegin(), recipients.cend(), (CAmount)0,
        [](CAmount sum, const mw::Recipient& recipient) { return sum + recipient.amount; }
    );

    // Get input coins
    LOG_INFO_F(
        "Creating Txs: Inputs({}), pegins({}), pegouts({}), recipient_total({}), fee({})",
        TotalAmount(input_coins),
        pegin_amount.value_or(0),
        pegout_total,
        recipient_total,
        fee
    );
    if ((TotalAmount(input_coins) + pegin_amount.value_or(0)) != (pegout_total + recipient_total + fee)) {
        ThrowInsufficientFunds("Total amount mismatch");
    }

    // Sign inputs
    TxBuilder::Inputs inputs = CreateInputs(input_coins);

    // Create outputs
    TxBuilder::Outputs outputs = CreateOutputs(recipients);

    // Total kernel offset is split between raw kernel_offset and the kernel's blinding factor.
    // sum(output.blind) - sum(input.blind) = kernel_offset + sum(kernel.blind)
    BlindingFactor kernel_offset = BlindingFactor::Random();
    BlindingFactor kernel_blind = Blinds()
        .Add(outputs.total_blind)
        .Sub(inputs.total_blind)
        .Sub(kernel_offset)
        .Total();

    // MW: TODO - This is only needed for peg-ins or when no change
    SecretKey stealth_blind = SecretKey::Random();

    // Create the kernel
    Kernel kernel = Kernel::Create(
        kernel_blind,
        boost::make_optional(stealth_blind),
        fee,
        pegin_amount,
        pegouts,
        boost::none
    );

    // Calculate stealth_offset
    BlindingFactor stealth_offset = Blinds()
        .Add(outputs.total_key)
        .Add(inputs.total_key)
        .Sub(stealth_blind)
        .Total();

    // Build the transaction
    return mw::Transaction::Create(
        std::move(kernel_offset),
        std::move(stealth_offset),
        std::move(inputs.inputs),
        std::move(outputs.outputs),
        std::vector<Kernel>{ std::move(kernel) }
    );
}

TxBuilder::Inputs TxBuilder::CreateInputs(const std::vector<mw::Coin>& input_coins)
{
    Blinds blinds;
    Blinds keys;
    std::vector<Input> inputs;
    std::transform(
        input_coins.cbegin(), input_coins.cend(), std::back_inserter(inputs),
        [&blinds, &keys](const mw::Coin& input_coin) {
            assert(!!input_coin.blind);
            assert(!!input_coin.key);

            BlindingFactor blind = Pedersen::BlindSwitch(input_coin.blind.value(), input_coin.amount);
            SecretKey ephemeral_key = SecretKey::Random();
            Input input = Input::Create(
                input_coin.output_id,
                Commitment::Blinded(blind, input_coin.amount),
                ephemeral_key,
                input_coin.key.value()
            );

            blinds.Add(blind);
            keys.Add(ephemeral_key);
            keys.Sub(input_coin.key.value());
            return input;
        }
    );

    return TxBuilder::Inputs{
        blinds.Total(),
        SecretKey(keys.Total().data()),
        std::move(inputs)
    };
}

TxBuilder::Outputs TxBuilder::CreateOutputs(const std::vector<mw::Recipient>& recipients)
{
    Blinds output_blinds;
    Blinds output_keys;
    std::vector<Output> outputs;
    std::transform(
        recipients.cbegin(), recipients.cend(), std::back_inserter(outputs),
        [&output_blinds, &output_keys](const mw::Recipient& recipient) {
            BlindingFactor blind;
            SecretKey ephemeral_key = SecretKey::Random();
            Output output = Output::Create(
                &blind,
                ephemeral_key,
                recipient.address,
                recipient.amount
            );

            output_blinds.Add(blind);
            output_keys.Add(ephemeral_key);
            return output;
        }
    );

    return TxBuilder::Outputs{
        output_blinds.Total(),
        SecretKey(output_keys.Total().data()),
        std::move(outputs)
    };
}

CAmount TxBuilder::TotalAmount(const std::vector<mw::Coin>& coins)
{
    return std::accumulate(
        coins.cbegin(), coins.cend(), (CAmount)0,
        [](CAmount total, const mw::Coin& coin) { return total + coin.amount; });
}