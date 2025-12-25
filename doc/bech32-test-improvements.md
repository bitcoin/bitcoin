# Bech32 Unit Test Improvements

This document outlines a plan to improve unit test coverage for the Bech32 encoding implementation in Bitcoin Core, based on [BIP-173](https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki).

## Background

Bech32 is the address format used for native SegWit addresses (BIP-173) and Taproot addresses via Bech32m (BIP-350). Comprehensive test coverage is critical as encoding errors can result in loss of funds.

### Current Test Coverage

The existing tests in `src/test/bech32_tests.cpp` cover:

| Test Case | Description |
|-----------|-------------|
| `bech32_testvectors_valid` | 7 valid Bech32 strings from BIP-173 |
| `bech32m_testvectors_valid` | 7 valid Bech32m strings from BIP-350 |
| `bech32_testvectors_invalid` | 16 invalid Bech32 cases with error detection |
| `bech32m_testvectors_invalid` | 16 invalid Bech32m cases with error detection |

Additionally, fuzz tests in `src/test/fuzz/bech32.cpp` provide:
- Random decode with re-encode verification
- Random round-trip testing (encode → decode → verify)

## BIP-173 Specification Requirements

### Bech32 String Structure

```
[HRP] [SEPARATOR='1'] [DATA (base32)] [CHECKSUM (6 chars)]
```

### Key Constraints from BIP-173

| Constraint | Specification |
|------------|---------------|
| Maximum length | 90 characters |
| HRP length | 1-83 US-ASCII characters |
| HRP character range | ASCII 33-126 (excluding uppercase for encoding) |
| Separator | Always `1` (last occurrence in string) |
| Data characters | 32-character set: `qpzry9x8gf2tvdw0s3jn54khce6mua7l` |
| Checksum | 6 characters, BCH error-correcting code |
| Case | Encoders MUST output lowercase; decoders reject mixed case |

### Error Detection Guarantees

BIP-173 specifies that the checksum guarantees detection of:
- **Any error affecting at most 4 characters** (< 1 in 10^9 failure rate)
- Reasonable detection rates for 5+ errors

## Proposed New Tests

### 1. Round-Trip Encoding Tests

**Purpose:** Verify that encoding arbitrary data and decoding it returns the original values.

```cpp
BOOST_AUTO_TEST_CASE(bech32_roundtrip)
{
    // Test with known HRP and data values
    // Verify: Decode(Encode(hrp, data)) == {hrp, data}
}
```

Test cases:
- Empty data (HRP + checksum only)
- Single byte data
- Maximum length data (at 90 char limit)
- Various HRP lengths

### 2. HRP Boundary Tests

**Purpose:** Test edge cases for Human-Readable Part.

```cpp
BOOST_AUTO_TEST_CASE(bech32_hrp_boundaries)
{
    // Minimum HRP (1 character)
    // Maximum HRP length
    // HRP with boundary ASCII characters (33 '!' and 126 '~')
    // HRP containing '1' (valid, uses last '1' as separator)
}
```

### 3. Character Set Validation

**Purpose:** Verify the complete 32-character encoding alphabet.

```cpp
BOOST_AUTO_TEST_CASE(bech32_charset_mapping)
{
    // Verify each of the 32 valid characters
    // Test: CHARSET[i] decodes back to i for all i in [0,31]
}
```

The BIP-173 character set:
```
Value:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
Char:   q  p  z  r  y  9  x  8  g  f  2  t  v  d  w  0

Value: 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
Char:   s  3  j  n  5  4  k  h  c  e  6  m  u  a  7  l
```

### 4. Maximum Length Boundary Tests

**Purpose:** Test behavior at the 90-character limit.

```cpp
BOOST_AUTO_TEST_CASE(bech32_length_limits)
{
    // Exactly 90 characters (valid)
    // 91 characters (invalid - too long)
    // Maximum data with minimum HRP
    // Maximum HRP with minimum data
}
```

### 5. Error Detection Tests

**Purpose:** Verify the checksum detects errors as specified in BIP-173.

```cpp
BOOST_AUTO_TEST_CASE(bech32_error_detection)
{
    // Single character substitution (1 error)
    // Two character substitution (2 errors)
    // Three character errors
    // Four character errors (guaranteed detection)
    // Adjacent character transposition
}
```

### 6. Multiple Separator Tests

**Purpose:** Verify correct handling when HRP contains '1'.

```cpp
BOOST_AUTO_TEST_CASE(bech32_multiple_separators)
{
    // HRP like "a]1[b" where HRP contains '1'
    // Verify last '1' is used as separator
}
```

### 7. Case Sensitivity Tests

**Purpose:** Comprehensive mixed-case rejection testing.

```cpp
BOOST_AUTO_TEST_CASE(bech32_case_sensitivity)
{
    // All uppercase (valid for decoding)
    // All lowercase (valid)
    // Mixed case in HRP only
    // Mixed case in data only
    // Mixed case spanning HRP and data
}
```

### 8. Polymod/Checksum Direct Tests

**Purpose:** Test the checksum algorithm directly.

```cpp
BOOST_AUTO_TEST_CASE(bech32_checksum_properties)
{
    // Verify encoding constant (1 for Bech32, 0x2bc830a3 for Bech32m)
    // Verify checksum changes with any input modification
    // Verify checksum is exactly 6 characters
}
```

## Implementation Plan

### Phase 1: Foundation Tests
- [ ] Add round-trip encoding test with basic cases
- [ ] Add character set validation test
- [ ] Add length boundary tests

### Phase 2: Error Detection Tests
- [ ] Add single-error detection test
- [ ] Add multi-error detection tests (2, 3, 4 errors)
- [ ] Add transposition detection test

### Phase 3: Edge Cases
- [ ] Add HRP boundary tests
- [ ] Add multiple separator tests
- [ ] Add comprehensive case sensitivity tests

### Phase 4: Documentation & Cleanup
- [ ] Add comments referencing BIP-173 sections
- [ ] Ensure consistent test naming
- [ ] Update this document with completion status

## Test Vectors from BIP-173

### Valid Bech32 Strings (already tested)
- `A12UEL5L`
- `a12uel5l`
- `an83characterlonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio1tt5tgs`
- `abcdef1qpzry9x8gf2tvdw0s3jn54khce6mua7lmqqqxw`
- `11qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqc8247j`
- `split1checkupstagehandshakeupstreamerranterredcaperred2y9e3w`
- `?1ezyfcl`

### Invalid Bech32 Strings (already tested)
| String | Reason |
|--------|--------|
| `\x20 + "1nwldj5"` | HRP character out of range |
| `\x7f + "1axkwrx"` | HRP character out of range |
| `an84characterslonghumanreadablepartthatcontains...` | Overall max length exceeded |
| `pzry9x0s0muk` | No separator |
| `1pzry9x0s0muk` | Empty HRP |
| `x1b4n0q5v` | Invalid data character (b) |
| `li1dgmt3` | Too short checksum |
| `A1G7SGD8` | Checksum calculated with uppercase |

### Additional Test Vectors to Add

**Valid addresses from BIP-173:**
```
BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4
bc1pw508d6qejxtdg4y5r3zarvary0c5xw7kw508d6qejxtdg4y5r3zarvary0c5xw7k7grplx
bc1sw50qa3jx3s
bc1zw508d6qejxtdg4y5r3zarvaryvg6kdaj
tb1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3q0sl5k7
tb1qqqqqp399et2xygdj5xreqhjjvcmzhxw4aywxecjdzew6hylgvsesrxh6hy
```

**Invalid addresses from BIP-173:**
| Address | Reason |
|---------|--------|
| `tc1qw508d6qejxtdg4y5r3zarvary0c5xw7kg3g4ty` | Invalid HRP |
| `bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t5` | Invalid checksum |
| `BC13W508D6QEJXTDG4Y5R3ZARVARY0C5XW7KN40WF2` | Invalid witness version |
| `bc1rw5uspcuh` | Invalid program length |
| `bc1zw508d6qejxtdg4y5r3zarvaryvqyzf3du` | Zero padding of more than 4 bits |
| `bc1gmk9yu` | Empty data section |

## References

- [BIP-173: Base32 address format for native v0-16 witness outputs](https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki)
- [BIP-350: Bech32m format for v1+ witness addresses](https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki)
- [Bitcoin Core bech32.cpp](../src/bech32.cpp)
- [Bitcoin Core bech32_tests.cpp](../src/test/bech32_tests.cpp)

## Contributing

When adding new tests:

1. Follow existing code style in `bech32_tests.cpp`
2. Use `BOOST_AUTO_TEST_CASE` for new test cases
3. Reference the relevant BIP-173/350 section in comments
4. Ensure tests pass locally: `ctest --test-dir build -R bech32`
5. Run the full test suite before submitting

## Author

Bitcoin Brisbane - Contributing to Bitcoin Core test coverage
