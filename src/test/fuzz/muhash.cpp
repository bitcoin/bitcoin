// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <crypto/muhash.h>
#include <span.h>
#include <uint256.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>
#include <vector>

namespace {

/** Class to represent 6144-bit numbers using arith_uint256 code.
 *
 * 6144 is sufficient to represent the product of two 3072-bit numbers. */
class arith_uint6144 : public base_uint<6144> {
public:
    arith_uint6144(uint64_t x) : base_uint{x} {}

    /** Construct an arith_uint6144 from any multiple of 4 bytes in LE notation,
     *  up to 768 bytes. */
    arith_uint6144(Span<const uint8_t> bytes) : base_uint{}
    {
        assert(bytes.size() % 4 == 0);
        assert(bytes.size() <= 768);
        for (unsigned i = 0; i * 4 < bytes.size(); ++i) {
            pn[i] = ReadLE32(bytes.data() + 4 * i);
        }
    }

    /** Serialize an arithm_uint6144 to any multiply of 4 bytes in LE notation,
     *  on the condition that the represented number fits. */
    void Serialize(Span<uint8_t> bytes) {
        assert(bytes.size() % 4 == 0);
        assert(bytes.size() <= 768);
        for (unsigned i = 0; i * 4 < bytes.size(); ++i) {
            WriteLE32(bytes.data() + 4 * i, pn[i]);
        }
        for (unsigned i = bytes.size() / 4; i * 4 < 768; ++i) {
            assert(pn[i] == 0);
        }
    };
};

/** The MuHash3072 modulus (2**3072 - 1103717) as 768 LE8 bytes. */
constexpr std::array<const uint8_t, 768> MODULUS_BYTES = {
    155,  40, 239, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const arith_uint6144 ZERO{0};
const arith_uint6144 ONE{1};
const arith_uint6144 MODULUS{MODULUS_BYTES};

/** Update value to be the modulus of the input modulo MODULUS. */
void Reduce(arith_uint6144& value)
{
    arith_uint6144 tmp = value;
    tmp /= MODULUS;
    tmp *= MODULUS;
    value -= tmp;
}

} // namespace

FUZZ_TARGET(num3072_mul)
{
    // Test multiplication
    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    // Read two 3072-bit numbers from fuzz input, and construct arith_uint6144
    // and Num3072 objects with the read values.
    uint16_t data_a_len = provider.ConsumeIntegralInRange(0, 384);
    uint8_t data_a[384] = {0};
    provider.ConsumeData(data_a, data_a_len);
    arith_uint6144 a_uint{data_a};
    Num3072 a_num{data_a};

    uint8_t data_b[384] = {0};
    provider.ConsumeData(data_b, 384);
    arith_uint6144 b_uint{data_b};
    Num3072 b_num{data_b};

    // Multiply the first number with the second, in both representations.
    a_num.Multiply(b_num);
    a_uint *= b_uint;
    Reduce(a_uint);

    // Serialize both to bytes and compare.
    uint8_t buf_num[384], buf_uint[384];
    a_num.ToBytes(buf_num);
    a_uint.Serialize(buf_uint);
    assert(Span(buf_num) == Span(buf_uint));
}

FUZZ_TARGET(num3072_inv)
{
    // Test inversion

    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    // Read a 3072-bit number from fuzz input, and construct arith_uint6144
    // and Num3072 objects with the read values.
    uint8_t data[384] = {0};
    provider.ConsumeData(data, 384);
    Num3072 num{data};
    arith_uint6144 uint{data};

    // Bail out if the number has no inverse.
    if ((uint == ZERO) || (uint == MODULUS)) return;

    // Compute the inverse of the Num3072 object.
    Num3072 inv;
    inv.SetToOne();
    inv.Divide(num);

    // Convert the computed inverse to arith_uint6144.
    uint8_t buf[384];
    inv.ToBytes(buf);
    arith_uint6144 uint_inv{buf};

    // Multiply the original and the inverse, and expect 1.
    uint *= uint_inv;
    Reduce(uint);
    assert(uint == ONE);
}

FUZZ_TARGET(muhash)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::vector<uint8_t> data{ConsumeRandomLengthByteVector(fuzzed_data_provider)};
    std::vector<uint8_t> data2{ConsumeRandomLengthByteVector(fuzzed_data_provider)};

    MuHash3072 muhash;

    muhash.Insert(data);
    muhash.Insert(data2);

    const std::string initial_state_hash{"dd5ad2a105c2d29495f577245c357409002329b9f4d6182c0af3dc2f462555c8"};
    uint256 out;
    uint256 out2;
    CallOneOf(
        fuzzed_data_provider,
        [&] {
            // Test that MuHash result is consistent independent of order of operations
            muhash.Finalize(out);

            muhash = MuHash3072();
            muhash.Insert(data2);
            muhash.Insert(data);
            muhash.Finalize(out2);
        },
        [&] {
            // Test that multiplication with the initial state never changes the finalized result
            muhash.Finalize(out);
            MuHash3072 muhash3;
            muhash3 *= muhash;
            muhash3.Finalize(out2);
        },
        [&] {
            // Test that dividing a MuHash by itself brings it back to it's initial state

            // See note about clang + self-assignment in test/uint256_tests.cpp
            #if defined(__clang__)
            #    pragma clang diagnostic push
            #    pragma clang diagnostic ignored "-Wself-assign-overloaded"
            #endif

            muhash /= muhash;

            #if defined(__clang__)
            #    pragma clang diagnostic pop
            #endif

            muhash.Finalize(out);
            out2 = uint256S(initial_state_hash);
        },
        [&] {
            // Test that removing all added elements brings the object back to it's initial state
            muhash.Remove(data);
            muhash.Remove(data2);
            muhash.Finalize(out);
            out2 = uint256S(initial_state_hash);
        });
    assert(out == out2);
}
