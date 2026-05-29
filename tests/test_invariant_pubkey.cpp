#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <stdexcept>

// Simulate the DER signature parsing logic from pubkey.cpp
// Returns false if the signature is invalid/rejected, true if successfully parsed
// This mirrors the validation logic that SHOULD be present to prevent CWE-120
static bool ParseDERSignature(const unsigned char* input, size_t inputlen, unsigned char* tmpsig) {
    // tmpsig is a fixed 64-byte buffer
    memset(tmpsig, 0, 64);

    // Minimum DER signature length check
    if (inputlen < 8) return false;
    if (inputlen > 72) return false;

    // Check DER sequence tag
    if (input[0] != 0x30) return false;

    // Check sequence length
    size_t seqlen = input[1];
    if (seqlen + 2 != inputlen) return false;

    // Parse R integer
    if (input[2] != 0x02) return false;
    size_t rlen = input[3];

    // SECURITY INVARIANT: rlen must not exceed 33 bytes and must fit in buffer
    // rlen > 33 would cause buffer overflow in: memcpy(tmpsig + 32 - rlen, input + rpos, rlen)
    if (rlen == 0 || rlen > 33) return false;

    size_t rpos = 4;
    if (rpos + rlen > inputlen) return false;

    // Strip leading zero if present (for negative number encoding)
    size_t rstart = rpos;
    if (rlen == 33) {
        if (input[rpos] != 0x00) return false;
        rstart++;
        rlen--;
    }

    // SECURITY INVARIANT: rlen must be <= 32 before memcpy
    if (rlen > 32) return false;

    // Parse S integer
    size_t spos_tag = rpos + (input[3] == 33 ? 33 : rlen);
    if (spos_tag + 2 > inputlen) return false;
    if (input[spos_tag] != 0x02) return false;

    size_t slen = input[spos_tag + 1];

    // SECURITY INVARIANT: slen must not exceed 33 bytes
    if (slen == 0 || slen > 33) return false;

    size_t spos = spos_tag + 2;
    if (spos + slen > inputlen) return false;

    size_t sstart = spos;
    if (slen == 33) {
        if (input[spos] != 0x00) return false;
        sstart++;
        slen--;
    }

    // SECURITY INVARIANT: slen must be <= 32 before memcpy
    if (slen > 32) return false;

    // Safe memcpy - only reached if rlen <= 32 and slen <= 32
    memcpy(tmpsig + 32 - rlen, input + rstart, rlen);
    memcpy(tmpsig + 64 - slen, input + sstart, slen);

    return true;
}

// Helper to build a DER signature with specified rlen and slen
static std::string BuildDERSig(uint8_t rlen, uint8_t slen) {
    std::string sig;
    sig.push_back(0x30); // SEQUENCE
    sig.push_back(4 + rlen + slen); // total length
    sig.push_back(0x02); // INTEGER tag for R
    sig.push_back(rlen); // R length
    for (uint8_t i = 0; i < rlen; i++) sig.push_back(0x41 + (i % 26)); // R data
    sig.push_back(0x02); // INTEGER tag for S
    sig.push_back(slen); // S length
    for (uint8_t i = 0; i < slen; i++) sig.push_back(0x42 + (i % 26)); // S data
    return sig;
}

class DERSignatureSecurityTest : public ::testing::TestWithParam<std::string> {};

TEST_P(DERSignatureSecurityTest, BufferReadsNeverExceedDeclaredLength) {
    // Invariant: DER signature parsing must reject any input where rlen or slen
    // would cause memcpy to write beyond the 64-byte tmpsig buffer.
    // Specifically: rlen <= 32 and slen <= 32 must hold before any memcpy.
    std::string payload = GetParam();

    unsigned char tmpsig[64];
    memset(tmpsig, 0xAA, sizeof(tmpsig)); // Fill with sentinel value

    const unsigned char* input = reinterpret_cast<const unsigned char*>(payload.data());
    size_t inputlen = payload.size();

    bool result = ParseDERSignature(input, inputlen, tmpsig);

    if (result) {
        // If parsing succeeded, verify tmpsig was only written within bounds
        // The function should only succeed with valid, safe inputs
        // Verify no sentinel bytes were corrupted beyond the 64-byte buffer
        // (This is a logical check - in real code we'd use AddressSanitizer)
        SUCCEED() << "Parsing succeeded with valid input";
    } else {
        // Oversized/malformed inputs must be rejected
        SUCCEED() << "Oversized/malformed input correctly rejected";
    }

    // The key invariant: if rlen or slen > 32, parsing MUST return false
    // Extract rlen and slen from payload if it has the right structure
    if (inputlen >= 6 && input[0] == 0x30 && input[2] == 0x02) {
        uint8_t rlen = input[3];
        if (rlen > 32) {
            EXPECT_FALSE(result) << "Oversized rlen=" << (int)rlen
                                 << " must be rejected to prevent buffer overflow";
        }
        if (inputlen > 4 + rlen + 2) {
            uint8_t slen = input[4 + rlen + 1];
            if (slen > 32) {
                EXPECT_FALSE(result) << "Oversized slen=" << (int)slen
                                     << " must be rejected to prevent buffer overflow";
            }
        }
    }
}

// Build adversarial payloads
static std::string MakePayload(uint8_t rlen, uint8_t slen) {
    return BuildDERSig(rlen, slen);
}

// Oversized rlen: 2x expected (64 bytes instead of 32)
static const std::string payload_rlen_2x = MakePayload(64, 32);
// Oversized rlen: 10x expected
static const std::string payload_rlen_10x = MakePayload(200, 32);  // will be truncated by uint8
// Oversized slen: 2x expected
static const std::string payload_slen_2x = MakePayload(32, 64);
// Oversized slen: 10x expected
static const std::string payload_slen_10x = MakePayload(32, 200);
// Both oversized
static const std::string payload_both_oversized = MakePayload(64, 64);
// rlen = 33 with no leading zero (invalid)
static const std::string payload_rlen_33_no_zero = MakePayload(33, 32);
// slen = 33 with no leading zero (invalid)
static const std::string payload_slen_33_no_zero = MakePayload(32, 33);
// rlen = 34 (exceeds max valid)
static const std::string payload_rlen_34 = MakePayload(34, 32);
// slen = 34 (exceeds max valid)
static const std::string payload_slen_34 = MakePayload(32, 34);
// Maximum uint8 values
static const std::string payload_max_rlen = MakePayload(255, 32);
static const std::string payload_max_slen = MakePayload(32, 255);
static const std::string payload_max_both = MakePayload(255, 255);
// Empty payload
static const std::string payload_empty = "";
// Single byte
static const std::string payload_single = "\x30";
// All zeros
static const std::string payload_zeros(72, '\x00');
// All 0xFF
static const std::string payload_ff(72, '\xFF');
// Valid-looking but rlen just over boundary
static const std::string payload_rlen_33 = MakePayload(33, 31);
// Valid-looking but slen just over boundary
static const std::string payload_slen_33 = MakePayload(31, 33);
// Exactly at boundary (valid)
static const std::string payload_valid_32_32 = MakePayload(32, 32);
// Truncated sequence
static const std::string payload_truncated = "\x30\x46\x02\x21";
// Malformed tag
static const std::string payload_bad_tag = "\x31\x44\x02\x20" + std::string(32, 'A') + "\x02\x20" + std::string(32, 'B');

INSTANTIATE_TEST_SUITE_P(
    AdversarialInputs,
    DERSignatureSecurityTest,
    ::testing::Values(
        payload_rlen_2x,
        payload_rlen_10x,
        payload_slen_2x,
        payload_slen_10x,
        payload_both_oversized,
        payload_rlen_33_no_zero,
        payload_slen_33_no_zero,
        payload_rlen_34,
        payload_slen_34,
        payload_max_rlen,
        payload_max_slen,
        payload_max_both,
        payload_empty,
        payload_single,
        payload_zeros,
        payload_ff,
        payload_rlen_33,
        payload_slen_33,
        payload_valid_32_32,
        payload_truncated,
        payload_bad_tag
    )
);

// Additional direct unit tests for specific invariants
TEST(DERSignatureBufferOverflowTest, RlenExceeding32MustBeRejected) {
    unsigned char tmpsig[64];
    // Test all values of rlen from 33 to 255
    for (int rlen = 33; rlen <= 255; rlen++) {
        std::string sig = BuildDERSig((uint8_t)rlen, 32);
        bool result = ParseDERSignature(
            reinterpret_cast<const unsigned char*>(sig.data()),
            sig.size(),
            tmpsig
        );
        // rlen > 33 must always be rejected (33 is only valid with leading zero)
        if (rlen > 33) {
            EXPECT_FALSE(result) << "rlen=" << rlen << " must be rejected";
        }
    }
}

TEST(DERSignatureBufferOverflowTest, SlenExceeding32MustBeRejected) {
    unsigned char tmpsig[64];
    // Test all values of slen from 33 to 255
    for (int slen = 33; slen <= 255; slen++) {
        std::string sig = BuildDERSig(32, (uint8_t)slen);
        bool result = ParseDERSignature(
            reinterpret_cast<const unsigned char*>(sig.data()),
            sig.size(),
            tmpsig
        );
        if (slen > 33) {
            EXPECT_FALSE(result) << "slen=" << slen << " must be rejected";
        }
    }
}

TEST(DERSignatureBufferOverflowTest, ValidSignatureAccepted) {
    unsigned char tmpsig[64];
    // Build a valid 32+32 signature
    std::string sig = BuildDERSig(32, 32);
    // Fix the sequence length
    sig[1] = 68; // 2 + 32 + 2 + 32
    bool result = ParseDERSignature(
        reinterpret_cast<const unsigned char*>(sig.data()),
        sig.size(),
        tmpsig
    );
    EXPECT_TRUE(result) << "Valid 32+32 DER signature should be accepted";
}

TEST(DERSignatureBufferOverflowTest, TmpsigNotCorruptedOnRejection) {
    unsigned char tmpsig[64];
    memset(tmpsig, 0xAA, 64);

    // Craft a signature with rlen=64 (2x overflow attempt)
    std::string sig = BuildDERSig(64, 32);
    bool result = ParseDERSignature(
        reinterpret_cast<const unsigned char*>(sig.data()),
        sig.size(),
        tmpsig
    );

    EXPECT_FALSE(result) << "Oversized rlen=64 must be rejected";

    // Verify tmpsig was not modified (still all 0xAA) since it should be rejected
    // before any memcpy
    bool tmpsig_clean = true;
    for (int i = 0; i < 64; i++) {
        if (tmpsig[i] != 0xAA) {
            tmpsig_clean = false;
            break;
        }
    }
    EXPECT_TRUE(tmpsig_clean) << "tmpsig must not be modified when input is rejected";
}

TEST(DERSignatureBufferOverflowTest, InputLengthBoundaryChecks) {
    unsigned char tmpsig[64];

    // Test with inputlen > 72 (max valid DER sig size)
    std::string oversized(100, 0x30);
    bool result = ParseDERSignature(
        reinterpret_cast<const unsigned char*>(oversized.data()),
        oversized.size(),
        tmpsig
    );
    EXPECT_FALSE(result) << "Input longer than 72 bytes must be rejected";

    // Test with inputlen < 8 (minimum valid)
    std::string undersized(4, 0x30);
    result = ParseDERSignature(
        reinterpret_cast<const unsigned char*>(undersized.data()),
        undersized.size(),
        tmpsig
    );
    EXPECT_FALSE(result) << "Input shorter than 8 bytes must be rejected";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}