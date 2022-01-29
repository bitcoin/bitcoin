#include <test_framework/models/Tx.h>
#include <test_framework/TxBuilder.h>
#include <mw/crypto/Blinds.h>
#include <mw/crypto/SecretKeys.h>
#include <mw/crypto/Pedersen.h>

test::Tx test::Tx::CreatePegIn(const CAmount amount, const CAmount fee)
{
    return test::TxBuilder()
        .AddPeginKernel(amount, fee)
        .AddOutput(amount, SecretKey::Random(), StealthAddress::Random())
        .Build();
}

test::Tx test::Tx::CreatePegOut(const test::TxOutput& input, const CAmount fee)
{
    return test::TxBuilder()
        .AddInput(input)
        .AddPegoutKernel(input.GetAmount() - fee, fee, true)
        .Build();
}