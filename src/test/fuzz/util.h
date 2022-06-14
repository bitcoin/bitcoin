// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_H
#define BITCOIN_TEST_FUZZ_UTIL_H

#include <amount.h>
#include <arith_uint256.h>
#include <attributes.h>
#include <chainparamsbase.h>
#include <coins.h>
#include <netaddress.h>
#include <netbase.h>
#include <optional.h>
#include <script/script.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <version.h>

#include <cstdint>
#include <string>
#include <vector>

[[ nodiscard ]] inline std::vector<uint8_t> ConsumeRandomLengthByteVector(FuzzedDataProvider& fuzzed_data_provider, const size_t max_length = 4096) noexcept
{
    const std::string s = fuzzed_data_provider.ConsumeRandomLengthString(max_length);
    return {s.begin(), s.end()};
}

[[ nodiscard ]] inline std::vector<std::string> ConsumeRandomLengthStringVector(FuzzedDataProvider& fuzzed_data_provider, const size_t max_vector_size = 16, const size_t max_string_length = 16) noexcept
{
    const size_t n_elements = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, max_vector_size);
    std::vector<std::string> r;
    for (size_t i = 0; i < n_elements; ++i) {
        r.push_back(fuzzed_data_provider.ConsumeRandomLengthString(max_string_length));
    }
    return r;
}

template <typename T>
[[ nodiscard ]] inline std::vector<T> ConsumeRandomLengthIntegralVector(FuzzedDataProvider& fuzzed_data_provider, const size_t max_vector_size = 16) noexcept
{
    const size_t n_elements = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, max_vector_size);
    std::vector<T> r;
    for (size_t i = 0; i < n_elements; ++i) {
        r.push_back(fuzzed_data_provider.ConsumeIntegral<T>());
    }
    return r;
}

template <typename T>
[[ nodiscard ]] inline Optional<T> ConsumeDeserializable(FuzzedDataProvider& fuzzed_data_provider, const size_t max_length = 4096) noexcept
{
    const std::vector<uint8_t> buffer = ConsumeRandomLengthByteVector(fuzzed_data_provider, max_length);
    CDataStream ds{buffer, SER_NETWORK, INIT_PROTO_VERSION};
    T obj;
    try {
        ds >> obj;
    } catch (const std::ios_base::failure&) {
        return nullopt;
    }
    return obj;
}

[[ nodiscard ]] inline opcodetype ConsumeOpcodeType(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return static_cast<opcodetype>(fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, MAX_OPCODE));
}

[[ nodiscard ]] inline CAmount ConsumeMoney(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, MAX_MONEY);
}

[[ nodiscard ]] inline CScript ConsumeScript(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    const std::vector<uint8_t> b = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    return {b.begin(), b.end()};
}

[[ nodiscard ]] inline CScriptNum ConsumeScriptNum(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return CScriptNum{fuzzed_data_provider.ConsumeIntegral<int64_t>()};
}


[[ nodiscard ]] inline uint256 ConsumeUInt256(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    const std::vector<unsigned char> v256 = fuzzed_data_provider.ConsumeBytes<unsigned char>(sizeof(uint256));
    if (v256.size() != sizeof(uint256)) {
        return {};
    }
    return uint256{v256};
}

[[ nodiscard ]] inline arith_uint256 ConsumeArithUInt256(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return UintToArith256(ConsumeUInt256(fuzzed_data_provider));
}

template <typename T>
[[ nodiscard ]] bool MultiplicationOverflow(const T i, const T j) noexcept
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
[[ nodiscard ]] bool AdditionOverflow(const T i, const T j) noexcept
{
    static_assert(std::is_integral<T>::value, "Integral required.");
    if (std::numeric_limits<T>::is_signed) {
        return (i > 0 && j > std::numeric_limits<T>::max() - i) ||
               (i < 0 && j < std::numeric_limits<T>::min() - i);
    }
    return std::numeric_limits<T>::max() - i < j;
}

[[ nodiscard ]] inline bool ContainsSpentInput(const CTransaction& tx, const CCoinsViewCache& inputs) noexcept
{
    for (const CTxIn& tx_in : tx.vin) {
        const Coin& coin = inputs.AccessCoin(tx_in.prevout);
        if (coin.IsSpent()) {
            return true;
        }
    }
    return false;
}

/**
 * Returns a byte vector of specified size regardless of the number of remaining bytes available
 * from the fuzzer. Pads with zero value bytes if needed to achieve the specified size.
 */
[[ nodiscard ]] inline std::vector<uint8_t> ConsumeFixedLengthByteVector(FuzzedDataProvider& fuzzed_data_provider, const size_t length) noexcept
{
    std::vector<uint8_t> result(length);
    const std::vector<uint8_t> random_bytes = fuzzed_data_provider.ConsumeBytes<uint8_t>(length);
    if (!random_bytes.empty()) {
        std::memcpy(result.data(), random_bytes.data(), random_bytes.size());
    }
    return result;
}

CNetAddr ConsumeNetAddr(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    const Network network = fuzzed_data_provider.PickValueInArray({Network::NET_IPV4, Network::NET_IPV6, Network::NET_INTERNAL, Network::NET_ONION});
    CNetAddr net_addr;
    if (network == Network::NET_IPV4) {
        const in_addr v4_addr = {
            .s_addr = fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
        net_addr = CNetAddr{v4_addr};
    } else if (network == Network::NET_IPV6) {
        if (fuzzed_data_provider.remaining_bytes() >= 16) {
            in6_addr v6_addr = {};
            memcpy(v6_addr.s6_addr, fuzzed_data_provider.ConsumeBytes<uint8_t>(16).data(), 16);
            net_addr = CNetAddr{v6_addr, fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
        }
    } else if (network == Network::NET_INTERNAL) {
        net_addr.SetInternal(fuzzed_data_provider.ConsumeBytesAsString(32));
    } else if (network == Network::NET_ONION) {
        net_addr.SetSpecial(fuzzed_data_provider.ConsumeBytesAsString(32));
    }
    return net_addr;
}

CSubNet ConsumeSubNet(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return {ConsumeNetAddr(fuzzed_data_provider), fuzzed_data_provider.ConsumeIntegral<uint8_t>()};
}

void InitializeFuzzingContext(const std::string& chain_name = CBaseChainParams::REGTEST)
{
    static const BasicTestingSetup basic_testing_setup{chain_name, {"-nodebuglogfile"}};
}

#endif // BITCOIN_TEST_FUZZ_UTIL_H
