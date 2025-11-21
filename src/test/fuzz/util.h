// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_H
#define BITCOIN_TEST_FUZZ_UTIL_H

#include <addresstype.h>
#include <arith_uint256.h>
#include <coins.h>
#include <compat/compat.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <key.h>
#include <merkleblock.h>
#include <pow.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <uint256.h>
#include <validation.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

class PeerManager;

template <typename... Callables>
size_t CallOneOf(FuzzedDataProvider& fuzzed_data_provider, Callables... callables)
{
    constexpr size_t call_size{sizeof...(callables)};
    static_assert(call_size >= 1);
    const size_t call_index{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, call_size - 1)};

    size_t i{0};
    ((i++ == call_index ? callables() : void()), ...);
    return call_size;
}

template <typename Collection>
auto& PickValue(FuzzedDataProvider& fuzzed_data_provider, Collection& col)
{
    auto sz{col.size()};
    assert(sz >= 1);
    auto it = col.begin();
    std::advance(it, fuzzed_data_provider.ConsumeIntegralInRange<decltype(sz)>(0, sz - 1));
    return *it;
}

template<typename B = uint8_t>
[[nodiscard]] inline std::vector<B> ConsumeRandomLengthByteVector(FuzzedDataProvider& fuzzed_data_provider, const std::optional<size_t>& max_length = std::nullopt) noexcept
{
    static_assert(sizeof(B) == 1);
    const std::string s = max_length ?
                              fuzzed_data_provider.ConsumeRandomLengthString(*max_length) :
                              fuzzed_data_provider.ConsumeRandomLengthString();
    std::vector<B> ret(s.size());
    std::copy(s.begin(), s.end(), reinterpret_cast<char*>(ret.data()));
    return ret;
}

[[nodiscard]] inline std::vector<bool> ConsumeRandomLengthBitVector(FuzzedDataProvider& fuzzed_data_provider, const std::optional<size_t>& max_length = std::nullopt) noexcept
{
    return BytesToBits(ConsumeRandomLengthByteVector(fuzzed_data_provider, max_length));
}

[[nodiscard]] inline DataStream ConsumeDataStream(FuzzedDataProvider& fuzzed_data_provider, const std::optional<size_t>& max_length = std::nullopt) noexcept
{
    return DataStream{ConsumeRandomLengthByteVector(fuzzed_data_provider, max_length)};
}

[[nodiscard]] inline std::vector<std::string> ConsumeRandomLengthStringVector(FuzzedDataProvider& fuzzed_data_provider, const size_t max_vector_size = 16, const size_t max_string_length = 16) noexcept
{
    const size_t n_elements = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, max_vector_size);
    std::vector<std::string> r;
    r.reserve(n_elements);
    for (size_t i = 0; i < n_elements; ++i) {
        r.push_back(fuzzed_data_provider.ConsumeRandomLengthString(max_string_length));
    }
    return r;
}

template <typename T>
[[nodiscard]] inline std::vector<T> ConsumeRandomLengthIntegralVector(FuzzedDataProvider& fuzzed_data_provider, const size_t max_vector_size = 16) noexcept
{
    const size_t n_elements = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, max_vector_size);
    std::vector<T> r;
    r.reserve(n_elements);
    for (size_t i = 0; i < n_elements; ++i) {
        r.push_back(fuzzed_data_provider.ConsumeIntegral<T>());
    }
    return r;
}

template <typename P>
[[nodiscard]] P ConsumeDeserializationParams(FuzzedDataProvider& fuzzed_data_provider) noexcept;

template <typename T, typename P>
[[nodiscard]] std::optional<T> ConsumeDeserializable(FuzzedDataProvider& fuzzed_data_provider, const P& params, const std::optional<size_t>& max_length = std::nullopt) noexcept
{
    const std::vector<uint8_t> buffer{ConsumeRandomLengthByteVector(fuzzed_data_provider, max_length)};
    DataStream ds{buffer};
    T obj;
    try {
        ds >> params(obj);
    } catch (const std::ios_base::failure&) {
        return std::nullopt;
    }
    return obj;
}

template <typename T>
[[nodiscard]] inline std::optional<T> ConsumeDeserializable(FuzzedDataProvider& fuzzed_data_provider, const std::optional<size_t>& max_length = std::nullopt) noexcept
{
    const std::vector<uint8_t> buffer = ConsumeRandomLengthByteVector(fuzzed_data_provider, max_length);
    DataStream ds{buffer};
    T obj;
    try {
        ds >> obj;
    } catch (const std::ios_base::failure&) {
        return std::nullopt;
    }
    return obj;
}

template <typename WeakEnumType, size_t size>
[[nodiscard]] WeakEnumType ConsumeWeakEnum(FuzzedDataProvider& fuzzed_data_provider, const WeakEnumType (&all_types)[size]) noexcept
{
    return fuzzed_data_provider.ConsumeBool() ?
               fuzzed_data_provider.PickValueInArray<WeakEnumType>(all_types) :
               WeakEnumType(fuzzed_data_provider.ConsumeIntegral<std::underlying_type_t<WeakEnumType>>());
}

[[nodiscard]] inline opcodetype ConsumeOpcodeType(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return static_cast<opcodetype>(fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, MAX_OPCODE));
}

[[nodiscard]] CAmount ConsumeMoney(FuzzedDataProvider& fuzzed_data_provider, const std::optional<CAmount>& max = std::nullopt) noexcept;

[[nodiscard]] int64_t ConsumeTime(FuzzedDataProvider& fuzzed_data_provider, const std::optional<int64_t>& min = std::nullopt, const std::optional<int64_t>& max = std::nullopt) noexcept;

[[nodiscard]] CMutableTransaction ConsumeTransaction(FuzzedDataProvider& fuzzed_data_provider, const std::optional<std::vector<Txid>>& prevout_txids, const int max_num_in = 10, const int max_num_out = 10) noexcept;

[[nodiscard]] CScriptWitness ConsumeScriptWitness(FuzzedDataProvider& fuzzed_data_provider, const size_t max_stack_elem_size = 32) noexcept;

[[nodiscard]] CScript ConsumeScript(FuzzedDataProvider& fuzzed_data_provider, const bool maybe_p2wsh = false) noexcept;

[[nodiscard]] uint32_t ConsumeSequence(FuzzedDataProvider& fuzzed_data_provider) noexcept;

[[nodiscard]] inline CScriptNum ConsumeScriptNum(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return CScriptNum{fuzzed_data_provider.ConsumeIntegral<int64_t>()};
}

[[nodiscard]] inline uint160 ConsumeUInt160(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    const std::vector<uint8_t> v160 = fuzzed_data_provider.ConsumeBytes<uint8_t>(160 / 8);
    if (v160.size() != 160 / 8) {
        return {};
    }
    return uint160{v160};
}

[[nodiscard]] inline uint256 ConsumeUInt256(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    const std::vector<uint8_t> v256 = fuzzed_data_provider.ConsumeBytes<uint8_t>(256 / 8);
    if (v256.size() != 256 / 8) {
        return {};
    }
    return uint256{v256};
}

[[nodiscard]] inline arith_uint256 ConsumeArithUInt256(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return UintToArith256(ConsumeUInt256(fuzzed_data_provider));
}

[[nodiscard]] inline arith_uint256 ConsumeArithUInt256InRange(FuzzedDataProvider& fuzzed_data_provider, const arith_uint256& min, const arith_uint256& max) noexcept
{
    assert(min <= max);
    const arith_uint256 range = max - min;
    const arith_uint256 value = ConsumeArithUInt256(fuzzed_data_provider);
    arith_uint256 result = value;
    // Avoid division by 0, in case range + 1 results in overflow.
    if (range != ~arith_uint256(0)) {
        const arith_uint256 quotient = value / (range + 1);
        result = value - (quotient * (range + 1));
    }
    result += min;
    assert(result >= min && result <= max);
    return result;
}

[[nodiscard]] std::map<COutPoint, Coin> ConsumeCoins(FuzzedDataProvider& fuzzed_data_provider) noexcept;

[[nodiscard]] CTxDestination ConsumeTxDestination(FuzzedDataProvider& fuzzed_data_provider) noexcept;

[[nodiscard]] CKey ConsumePrivateKey(FuzzedDataProvider& fuzzed_data_provider, std::optional<bool> compressed = std::nullopt) noexcept;

template <typename T>
[[nodiscard]] bool MultiplicationOverflow(const T i, const T j) noexcept
{
    static_assert(std::is_integral_v<T>, "Integral required.");
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

[[nodiscard]] bool ContainsSpentInput(const CTransaction& tx, const CCoinsViewCache& inputs) noexcept;

/**
 * Sets errno to a value selected from the given std::array `errnos`.
 */
template <typename T, size_t size>
void SetFuzzedErrNo(FuzzedDataProvider& fuzzed_data_provider, const std::array<T, size>& errnos)
{
    errno = fuzzed_data_provider.PickValueInArray(errnos);
}

/*
 * Sets a fuzzed errno in the range [0, 133 (EHWPOISON)]. Can be used from functions emulating
 * standard library functions that set errno, or in other contexts where the value of errno
 * might be relevant for the execution path that will be taken.
 */
inline void SetFuzzedErrNo(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    errno = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 133);
}

/**
 * Returns a byte vector of specified size regardless of the number of remaining bytes available
 * from the fuzzer. Pads with zero value bytes if needed to achieve the specified size.
 */
template<typename B = uint8_t>
[[nodiscard]] inline std::vector<B> ConsumeFixedLengthByteVector(FuzzedDataProvider& fuzzed_data_provider, const size_t length) noexcept
{
    static_assert(sizeof(B) == 1);
    auto random_bytes = fuzzed_data_provider.ConsumeBytes<B>(length);
    random_bytes.resize(length);
    return random_bytes;
}

class FuzzedFileProvider
{
    FuzzedDataProvider& m_fuzzed_data_provider;
    int64_t m_offset = 0;

public:
    FuzzedFileProvider(FuzzedDataProvider& fuzzed_data_provider) : m_fuzzed_data_provider{fuzzed_data_provider}
    {
    }

    FILE* open();

    static ssize_t read(void* cookie, char* buf, size_t size);

    static ssize_t write(void* cookie, const char* buf, size_t size);

    static int seek(void* cookie, int64_t* offset, int whence);

    static int close(void* cookie);
};

#define WRITE_TO_STREAM_CASE(type, consume) \
    [&] {                                   \
        type o = consume;                   \
        stream << o;                        \
    }
template <typename Stream>
void WriteToStream(FuzzedDataProvider& fuzzed_data_provider, Stream& stream) noexcept
{
    while (fuzzed_data_provider.ConsumeBool()) {
        try {
            CallOneOf(
                fuzzed_data_provider,
                WRITE_TO_STREAM_CASE(bool, fuzzed_data_provider.ConsumeBool()),
                WRITE_TO_STREAM_CASE(int8_t, fuzzed_data_provider.ConsumeIntegral<int8_t>()),
                WRITE_TO_STREAM_CASE(uint8_t, fuzzed_data_provider.ConsumeIntegral<uint8_t>()),
                WRITE_TO_STREAM_CASE(int16_t, fuzzed_data_provider.ConsumeIntegral<int16_t>()),
                WRITE_TO_STREAM_CASE(uint16_t, fuzzed_data_provider.ConsumeIntegral<uint16_t>()),
                WRITE_TO_STREAM_CASE(int32_t, fuzzed_data_provider.ConsumeIntegral<int32_t>()),
                WRITE_TO_STREAM_CASE(uint32_t, fuzzed_data_provider.ConsumeIntegral<uint32_t>()),
                WRITE_TO_STREAM_CASE(int64_t, fuzzed_data_provider.ConsumeIntegral<int64_t>()),
                WRITE_TO_STREAM_CASE(uint64_t, fuzzed_data_provider.ConsumeIntegral<uint64_t>()),
                WRITE_TO_STREAM_CASE(std::string, fuzzed_data_provider.ConsumeRandomLengthString(32)),
                WRITE_TO_STREAM_CASE(std::vector<uint8_t>, ConsumeRandomLengthIntegralVector<uint8_t>(fuzzed_data_provider)));
        } catch (const std::ios_base::failure&) {
            break;
        }
    }
}

#define READ_FROM_STREAM_CASE(type) \
    [&] {                           \
        type o;                     \
        stream >> o;                \
    }
template <typename Stream>
void ReadFromStream(FuzzedDataProvider& fuzzed_data_provider, Stream& stream) noexcept
{
    while (fuzzed_data_provider.ConsumeBool()) {
        try {
            CallOneOf(
                fuzzed_data_provider,
                READ_FROM_STREAM_CASE(bool),
                READ_FROM_STREAM_CASE(int8_t),
                READ_FROM_STREAM_CASE(uint8_t),
                READ_FROM_STREAM_CASE(int16_t),
                READ_FROM_STREAM_CASE(uint16_t),
                READ_FROM_STREAM_CASE(int32_t),
                READ_FROM_STREAM_CASE(uint32_t),
                READ_FROM_STREAM_CASE(int64_t),
                READ_FROM_STREAM_CASE(uint64_t),
                READ_FROM_STREAM_CASE(std::string),
                READ_FROM_STREAM_CASE(std::vector<uint8_t>));
        } catch (const std::ios_base::failure&) {
            break;
        }
    }
}

inline void FinalizeHeader(CBlockHeader& header, const ChainstateManager& chainman)
{
    while (!CheckProofOfWork(header.GetHash(), header.nBits, chainman.GetParams().GetConsensus())) {
        ++(header.nNonce);
    }
}

#endif // BITCOIN_TEST_FUZZ_UTIL_H
