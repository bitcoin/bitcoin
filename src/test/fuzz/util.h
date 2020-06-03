// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_H
#define BITCOIN_TEST_FUZZ_UTIL_H

#include <amount.h>
#include <arith_uint256.h>
#include <attributes.h>
#include <coins.h>
#include <consensus/consensus.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/standard.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <txmempool.h>
#include <uint256.h>
#include <version.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

NODISCARD inline std::vector<uint8_t> ConsumeRandomLengthByteVector(FuzzedDataProvider& fuzzed_data_provider, const size_t max_length = 4096) noexcept
{
    const std::string s = fuzzed_data_provider.ConsumeRandomLengthString(max_length);
    return {s.begin(), s.end()};
}

NODISCARD inline CDataStream ConsumeDataStream(FuzzedDataProvider& fuzzed_data_provider, const size_t max_length = 4096) noexcept
{
    return {ConsumeRandomLengthByteVector(fuzzed_data_provider, max_length), SER_NETWORK, INIT_PROTO_VERSION};
}

NODISCARD inline std::vector<std::string> ConsumeRandomLengthStringVector(FuzzedDataProvider& fuzzed_data_provider, const size_t max_vector_size = 16, const size_t max_string_length = 16) noexcept
{
    const size_t n_elements = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, max_vector_size);
    std::vector<std::string> r;
    for (size_t i = 0; i < n_elements; ++i) {
        r.push_back(fuzzed_data_provider.ConsumeRandomLengthString(max_string_length));
    }
    return r;
}

template <typename T>
NODISCARD inline std::vector<T> ConsumeRandomLengthIntegralVector(FuzzedDataProvider& fuzzed_data_provider, const size_t max_vector_size = 16) noexcept
{
    const size_t n_elements = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, max_vector_size);
    std::vector<T> r;
    for (size_t i = 0; i < n_elements; ++i) {
        r.push_back(fuzzed_data_provider.ConsumeIntegral<T>());
    }
    return r;
}

template <typename T>
NODISCARD inline std::optional<T> ConsumeDeserializable(FuzzedDataProvider& fuzzed_data_provider, const size_t max_length = 4096) noexcept
{
    const std::vector<uint8_t> buffer = ConsumeRandomLengthByteVector(fuzzed_data_provider, max_length);
    CDataStream ds{buffer, SER_NETWORK, INIT_PROTO_VERSION};
    T obj;
    try {
        ds >> obj;
    } catch (const std::ios_base::failure&) {
        return std::nullopt;
    }
    return obj;
}

NODISCARD inline opcodetype ConsumeOpcodeType(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return static_cast<opcodetype>(fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, MAX_OPCODE));
}

NODISCARD inline CAmount ConsumeMoney(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, MAX_MONEY);
}

NODISCARD inline CScript ConsumeScript(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    const std::vector<uint8_t> b = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    return {b.begin(), b.end()};
}

NODISCARD inline CScriptNum ConsumeScriptNum(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return CScriptNum{fuzzed_data_provider.ConsumeIntegral<int64_t>()};
}

NODISCARD inline uint160 ConsumeUInt160(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    const std::vector<uint8_t> v160 = fuzzed_data_provider.ConsumeBytes<uint8_t>(160 / 8);
    if (v160.size() != 160 / 8) {
        return {};
    }
    return uint160{v160};
}

NODISCARD inline uint256 ConsumeUInt256(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    const std::vector<uint8_t> v256 = fuzzed_data_provider.ConsumeBytes<uint8_t>(256 / 8);
    if (v256.size() != 256 / 8) {
        return {};
    }
    return uint256{v256};
}

NODISCARD inline arith_uint256 ConsumeArithUInt256(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return UintToArith256(ConsumeUInt256(fuzzed_data_provider));
}

NODISCARD inline CTxMemPoolEntry ConsumeTxMemPoolEntry(FuzzedDataProvider& fuzzed_data_provider, const CTransaction& tx) noexcept
{
    // Avoid:
    // policy/feerate.cpp:28:34: runtime error: signed integer overflow: 34873208148477500 * 1000 cannot be represented in type 'long'
    //
    // Reproduce using CFeeRate(348732081484775, 10).GetFeePerK()
    const CAmount fee = std::min<CAmount>(ConsumeMoney(fuzzed_data_provider), std::numeric_limits<CAmount>::max() / static_cast<CAmount>(100000));
    assert(MoneyRange(fee));
    const int64_t time = fuzzed_data_provider.ConsumeIntegral<int64_t>();
    const unsigned int entry_height = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
    const bool spends_coinbase = fuzzed_data_provider.ConsumeBool();
    const unsigned int sig_op_cost = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, MAX_BLOCK_SIGOPS_COST);
    return CTxMemPoolEntry{MakeTransactionRef(tx), fee, time, entry_height, spends_coinbase, sig_op_cost, {}};
}

NODISCARD inline CTxDestination ConsumeTxDestination(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    CTxDestination tx_destination;
    switch (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 5)) {
    case 0: {
        tx_destination = CNoDestination{};
        break;
    }
    case 1: {
        tx_destination = PKHash{ConsumeUInt160(fuzzed_data_provider)};
        break;
    }
    case 2: {
        tx_destination = ScriptHash{ConsumeUInt160(fuzzed_data_provider)};
        break;
    }
    case 3: {
        tx_destination = WitnessV0ScriptHash{ConsumeUInt256(fuzzed_data_provider)};
        break;
    }
    case 4: {
        tx_destination = WitnessV0KeyHash{ConsumeUInt160(fuzzed_data_provider)};
        break;
    }
    case 5: {
        WitnessUnknown witness_unknown{};
        witness_unknown.version = fuzzed_data_provider.ConsumeIntegral<int>();
        const std::vector<uint8_t> witness_unknown_program_1 = fuzzed_data_provider.ConsumeBytes<uint8_t>(40);
        witness_unknown.length = witness_unknown_program_1.size();
        std::copy(witness_unknown_program_1.begin(), witness_unknown_program_1.end(), witness_unknown.program);
        tx_destination = witness_unknown;
        break;
    }
    }
    return tx_destination;
}

template <typename T>
NODISCARD bool MultiplicationOverflow(const T i, const T j) noexcept
{
    static_assert(std::is_integral<T>::value, "Integral required.");
    if (std::numeric_limits<T>::is_signed) {
        if (i > 0) {
            if (j > 0) {
                return i > (std::numeric_limits<T>::max() / j);
            } else {
                return j < (std::numeric_limits<T>::min() / i);
            }
        } else {
            if (j > 0) {
                return i < (std::numeric_limits<T>::min() / j);
            } else {
                return i != 0 && (j < (std::numeric_limits<T>::max() / i));
            }
        }
    } else {
        return j != 0 && i > std::numeric_limits<T>::max() / j;
    }
}

template <class T>
NODISCARD bool AdditionOverflow(const T i, const T j) noexcept
{
    static_assert(std::is_integral<T>::value, "Integral required.");
    if (std::numeric_limits<T>::is_signed) {
        return (i > 0 && j > std::numeric_limits<T>::max() - i) ||
               (i < 0 && j < std::numeric_limits<T>::min() - i);
    }
    return std::numeric_limits<T>::max() - i < j;
}

NODISCARD inline bool ContainsSpentInput(const CTransaction& tx, const CCoinsViewCache& inputs) noexcept
{
    for (const CTxIn& tx_in : tx.vin) {
        const Coin& coin = inputs.AccessCoin(tx_in.prevout);
        if (coin.IsSpent()) {
            return true;
        }
    }
    return false;
}

#endif // BITCOIN_TEST_FUZZ_UTIL_H
