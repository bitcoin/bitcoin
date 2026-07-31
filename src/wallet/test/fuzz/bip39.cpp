// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <wallet/bip39.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>

namespace wallet::bip39 {
namespace {

struct ValidMnemonic {
    size_t word_count;
    std::string_view checksum_word;
};

// Zero-entropy mnemonics use "abandon" for every entropy word. The final
// checksum word depends on the mnemonic length.
constexpr std::array<ValidMnemonic, 5> VALID_MNEMONICS{{
    {12, "about"},
    {15, "address"},
    {18, "agent"},
    {21, "admit"},
    {24, "art"},
}};
constexpr std::array<char, 6> ASCII_WHITESPACE{' ', '\t', '\n', '\r', '\f', '\v'};

std::string ConsumeValidMnemonic(FuzzedDataProvider& provider)
{
    const ValidMnemonic parameters{provider.PickValueInArray(VALID_MNEMONICS)};
    const size_t separator_size{provider.ConsumeIntegralInRange<size_t>(1, 4)};
    std::string separator;
    separator.reserve(separator_size);
    for (size_t i{0}; i < separator_size; ++i) {
        separator += provider.PickValueInArray(ASCII_WHITESPACE);
    }

    std::string mnemonic;
    if (provider.ConsumeBool()) mnemonic += separator;
    for (size_t word_index{1}; word_index < parameters.word_count; ++word_index) {
        mnemonic += "abandon";
        mnemonic += separator;
    }
    mnemonic += parameters.checksum_word;
    if (provider.ConsumeBool()) mnemonic += separator;
    return mnemonic;
}

} // namespace

FUZZ_TARGET(bip39)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    const std::string mnemonic{provider.ConsumeRemainingBytesAsString()};
    (void)DecodeMnemonic(mnemonic, /*passphrase=*/{});
}

FUZZ_TARGET(bip39_valid)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    const std::string mnemonic{ConsumeValidMnemonic(provider)};
    std::string passphrase{provider.ConsumeRemainingBytesAsString()};
    for (char& ch : passphrase) {
        ch = static_cast<char>(static_cast<unsigned char>(ch) & 0x7f);
    }

    const auto decoded{DecodeMnemonic(mnemonic, passphrase)};
    assert(decoded.has_value() || decoded.error() == Error::InvalidSeed);
}

} // namespace wallet::bip39
