#include "omnicore/createtx.h"
#include "omnicore/rpcvalues.h"

#include "primitives/transaction.h"
#include "pubkey.h"
#include "rpcprotocol.h"
#include "script/script.h"
#include "utilstrencodings.h"

#include "json/json_spirit_value.h"

#include <stdint.h>
#include <string>

#include <boost/test/unit_test.hpp>

using json_spirit::Value;
using json_spirit::Object;

BOOST_AUTO_TEST_SUITE(omnicore_rpc_value_tests)

static Value ValueFromString(const std::string& str)
{
    Value value;
    BOOST_CHECK(json_spirit::read_string(str, value));
    return value;
}

BOOST_AUTO_TEST_CASE(rpcvalues_address)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParseAddress(Value("1f2dj45pxYb8BCW5sSbCgJ5YvXBfSapeX")), "1f2dj45pxYb8BCW5sSbCgJ5YvXBfSapeX");
    BOOST_CHECK_EQUAL(ParseAddress(Value("1ARjWDkZ7kT9fwjPrjcQyvbXDkEySzKHwu")), "1ARjWDkZ7kT9fwjPrjcQyvbXDkEySzKHwu");
    BOOST_CHECK_EQUAL(ParseAddress(Value("14gu4rLFn72uCBagKqxdinnVTTiPmnDehk")), "14gu4rLFn72uCBagKqxdinnVTTiPmnDehk");
    BOOST_CHECK_EQUAL(ParseAddress(Value("3MbYQMMmSkC3AgWkj9FMo5LsPTW1zBTwXL")), "3MbYQMMmSkC3AgWkj9FMo5LsPTW1zBTwXL");
    // Invalid CBitcoinAddress
    BOOST_CHECK_THROW(ParseAddress(Value("NotAnBase58EncodedAdddress")), Object);
    BOOST_CHECK_THROW(ParseAddress(Value("14gu4rLFn72uCBagKqxdinnVTTiPmnDehj")), Object);
    BOOST_CHECK_THROW(ParseAddress(Value("9EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParseAddress(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParseAddress(ValueFromString("null")), std::runtime_error);
    BOOST_CHECK_THROW(ParseAddress(ValueFromString("123456789")), std::runtime_error);
    BOOST_CHECK_THROW(ParseAddress(ValueFromString("0.1")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_address_or_empty)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParseAddressOrEmpty(Value()), "");
    BOOST_CHECK_EQUAL(ParseAddressOrEmpty(Value("")), "");
    BOOST_CHECK_EQUAL(ParseAddressOrEmpty(ValueFromString("null")), "");
    BOOST_CHECK_EQUAL(ParseAddressOrEmpty(Value("1f2dj45pxYb8BCW5sSbCgJ5YvXBfSapeX")), "1f2dj45pxYb8BCW5sSbCgJ5YvXBfSapeX");
    // Invalid CBitcoinAddress
    BOOST_CHECK_THROW(ParseAddressOrEmpty(Value("*")), Object);
    BOOST_CHECK_THROW(ParseAddressOrEmpty(Value("asdrf234")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParseAddressOrEmpty(ValueFromString("0.1")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_address_or_wildcard)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParseAddressOrWildcard(Value("*")), "*");
    BOOST_CHECK_EQUAL(ParseAddressOrWildcard(Value("3MbYQMMmSkC3AgWkj9FMo5LsPTW1zBTwXL")), "3MbYQMMmSkC3AgWkj9FMo5LsPTW1zBTwXL");
    // Invalid CBitcoinAddress
    BOOST_CHECK_THROW(ParseAddressOrWildcard(Value("")), Object);
    BOOST_CHECK_THROW(ParseAddressOrWildcard(Value("-")), Object);
    BOOST_CHECK_THROW(ParseAddressOrWildcard(Value("9EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParseAddressOrWildcard(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParseAddressOrWildcard(ValueFromString("null")), std::runtime_error);
    BOOST_CHECK_THROW(ParseAddressOrWildcard(ValueFromString("123456789")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_property_id)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParsePropertyId(ValueFromString("1")), uint32_t(1));
    BOOST_CHECK_EQUAL(ParsePropertyId(ValueFromString("31")), uint32_t(31));
    BOOST_CHECK_EQUAL(ParsePropertyId(ValueFromString("2147483651")), uint32_t(2147483651U));
    BOOST_CHECK_EQUAL(ParsePropertyId(ValueFromString("4294967295")), uint32_t(4294967295U));
    // Out of range
    BOOST_CHECK_THROW(ParsePropertyId(ValueFromString("0")), Object);
    BOOST_CHECK_THROW(ParsePropertyId(ValueFromString("4294967296")), Object);
    BOOST_CHECK_THROW(ParsePropertyId(ValueFromString("-2")), Object);
    BOOST_CHECK_THROW(ParsePropertyId(ValueFromString("9223372036854775807")), Object);
    BOOST_CHECK_THROW(ParsePropertyId(ValueFromString("9223372036854775809")), Object);
    BOOST_CHECK_THROW(ParsePropertyId(ValueFromString("18446744073709551615")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParsePropertyId(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParsePropertyId(Value("123")), std::runtime_error);
    BOOST_CHECK_THROW(ParsePropertyId(ValueFromString("null")), std::runtime_error);
    BOOST_CHECK_THROW(ParsePropertyId(ValueFromString("1.0")), std::runtime_error);
    BOOST_CHECK_THROW(ParsePropertyId(ValueFromString("-0.99")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_indivisible_amounts)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParseAmount(Value("1"), false), int64_t(1));
    BOOST_CHECK_EQUAL(ParseAmount(Value("100000000"), false), int64_t(100000000));
    BOOST_CHECK_EQUAL(ParseAmount(Value("2147483651"), false), int64_t(2147483651LL));
    BOOST_CHECK_EQUAL(ParseAmount(Value("9223372036854775807"), false), int64_t(9223372036854775807LL));
    // Out of range
    BOOST_CHECK_THROW(ParseAmount(Value("0"), false), Object);
    BOOST_CHECK_THROW(ParseAmount(Value("-2"), false), Object);
    BOOST_CHECK_THROW(ParseAmount(Value("-4294967297"), false), Object);
    BOOST_CHECK_THROW(ParseAmount(Value("9223372036854775808"), false), Object);
    BOOST_CHECK_THROW(ParseAmount(Value("-9223372036854775808"), false), Object);
    BOOST_CHECK_THROW(ParseAmount(Value("18446744073709551610"), false), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParseAmount(Value(), false), std::runtime_error);
    BOOST_CHECK_THROW(ParseAmount(ValueFromString("null"), false), std::runtime_error);
    BOOST_CHECK_THROW(ParseAmount(ValueFromString("9999"), false), std::runtime_error);    
    BOOST_CHECK_THROW(ParseAmount(ValueFromString("2.50000000"), false), std::runtime_error);
    BOOST_CHECK_THROW(ParseAmount(ValueFromString("9223372036854775807"), false), std::runtime_error);
    BOOST_CHECK_THROW(ParseAmount(ValueFromString("-9223372036854775808"), false), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_divisible_amounts)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParseAmount(Value("0.00000001"), true), int64_t(1));
    BOOST_CHECK_EQUAL(ParseAmount(Value("1"), true), int64_t(100000000));
    BOOST_CHECK_EQUAL(ParseAmount(Value("55555.555555"), true), int64_t(5555555555500L));
    BOOST_CHECK_EQUAL(ParseAmount(Value("21000000.0"), true), int64_t(2100000000000000L));
    BOOST_CHECK_EQUAL(ParseAmount(Value("21000000.00000009"), true), int64_t(2100000000000009L));
    BOOST_CHECK_EQUAL(ParseAmount(Value("92233720368.54775807"), true), int64_t(9223372036854775807LL));
    // Out of range
    BOOST_CHECK_THROW(ParseAmount(Value("0"), true), Object);
    BOOST_CHECK_THROW(ParseAmount(Value("0.000000001"), true), Object);
    BOOST_CHECK_THROW(ParseAmount(Value("0.000000009"), true), Object);
    BOOST_CHECK_THROW(ParseAmount(Value("-0.00000001"), true), Object);
    BOOST_CHECK_THROW(ParseAmount(Value("CAFEBABE"), true), Object);
    BOOST_CHECK_THROW(ParseAmount(Value("0x30303030"), true), Object);
    BOOST_CHECK_THROW(ParseAmount(Value("92233720368.54775808"), true), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParseAmount(Value(), false), std::runtime_error);
    BOOST_CHECK_THROW(ParseAmount(ValueFromString("null"), false), std::runtime_error);
    BOOST_CHECK_THROW(ParseAmount(ValueFromString("4294967299"), false), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_amount_by_type_mix)
{
    int typeIndivisible = 1;
    int typeDivisible = 2;
    BOOST_CHECK_EQUAL(ParseAmount(Value("2976579765"), false), ParseAmount(Value("2976579765"), typeIndivisible));
    BOOST_CHECK_EQUAL(ParseAmount(Value("4286316807.99194360"), typeDivisible), ParseAmount(Value("4286316807.99194360"), true));
    BOOST_CHECK_EQUAL(ParseAmount(Value("3221344269"), typeIndivisible), ParseAmount(Value("32.21344269"), true));
    BOOST_CHECK_EQUAL(ParseAmount(Value("8.00"), typeDivisible), ParseAmount(Value("800000000"), false));
}

BOOST_AUTO_TEST_CASE(rpcvalues_payment_window)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParseDExPaymentWindow(ValueFromString("1")), uint8_t(1));
    BOOST_CHECK_EQUAL(ParseDExPaymentWindow(ValueFromString("127")), uint8_t(127));
    BOOST_CHECK_EQUAL(ParseDExPaymentWindow(ValueFromString("255")), uint8_t(255));
    // Out of range
    BOOST_CHECK_THROW(ParseDExPaymentWindow(ValueFromString("0")), Object);
    BOOST_CHECK_THROW(ParseDExPaymentWindow(ValueFromString("256")), Object);
    BOOST_CHECK_THROW(ParseDExPaymentWindow(ValueFromString("-256")), Object);
    BOOST_CHECK_THROW(ParseDExPaymentWindow(ValueFromString("-128")), Object);
    BOOST_CHECK_THROW(ParseDExPaymentWindow(ValueFromString("1027")), Object);
    BOOST_CHECK_THROW(ParseDExPaymentWindow(ValueFromString("4294967297")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParseDExPaymentWindow(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParseDExPaymentWindow(Value("1")), std::runtime_error);
    BOOST_CHECK_THROW(ParseDExPaymentWindow(ValueFromString("null")), std::runtime_error);
    BOOST_CHECK_THROW(ParseDExPaymentWindow(ValueFromString("2554.9995")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_dex_fee)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParseDExFee(Value("0")), int64_t(0));
    BOOST_CHECK_EQUAL(ParseDExFee(Value("0.00000")), int64_t(0));
    BOOST_CHECK_EQUAL(ParseDExFee(Value("0.00000001")), int64_t(1));
    BOOST_CHECK_EQUAL(ParseDExFee(Value("777")), int64_t(77700000000L));
    BOOST_CHECK_EQUAL(ParseDExFee(Value("92233720368.54772323")), int64_t(9223372036854772323L));
    // Invalid types
    BOOST_CHECK_THROW(ParseDExFee(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParseDExFee(ValueFromString("3")), std::runtime_error);
    BOOST_CHECK_THROW(ParseDExFee(ValueFromString("null")), std::runtime_error);
    BOOST_CHECK_THROW(ParseDExFee(ValueFromString("2554.9995")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_dex_action)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParseDExAction(ValueFromString("1")), uint8_t(1));
    BOOST_CHECK_EQUAL(ParseDExAction(ValueFromString("2")), uint8_t(2));
    BOOST_CHECK_EQUAL(ParseDExAction(ValueFromString("3")), uint8_t(3));
    // Out of range
    BOOST_CHECK_THROW(ParseDExAction(ValueFromString("0")), Object);
    BOOST_CHECK_THROW(ParseDExAction(ValueFromString("4")), Object);
    BOOST_CHECK_THROW(ParseDExAction(ValueFromString("-1")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParseDExAction(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParseDExAction(Value("2")), std::runtime_error);
    BOOST_CHECK_THROW(ParseDExAction(ValueFromString("null")), std::runtime_error);
    BOOST_CHECK_THROW(ParseDExAction(ValueFromString("3.0")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_ecosystem)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParseEcosystem(ValueFromString("1")), uint8_t(1));
    BOOST_CHECK_EQUAL(ParseEcosystem(ValueFromString("2")), uint8_t(2));
    // Out of range
    BOOST_CHECK_THROW(ParseEcosystem(ValueFromString("0")), Object);
    BOOST_CHECK_THROW(ParseEcosystem(ValueFromString("3")), Object);
    BOOST_CHECK_THROW(ParseEcosystem(ValueFromString("-257")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParseEcosystem(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParseEcosystem(Value("1")), std::runtime_error);
    BOOST_CHECK_THROW(ParseEcosystem(ValueFromString("null")), std::runtime_error);
    BOOST_CHECK_THROW(ParseEcosystem(ValueFromString("1.999999999")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_property_type)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParsePropertyType(ValueFromString("1")), uint16_t(1));
    BOOST_CHECK_EQUAL(ParsePropertyType(ValueFromString("2")), uint16_t(2));
    // Out of range
    BOOST_CHECK_THROW(ParsePropertyType(ValueFromString("0")), Object);
    BOOST_CHECK_THROW(ParsePropertyType(ValueFromString("3")), Object);
    BOOST_CHECK_THROW(ParsePropertyType(ValueFromString("-0")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParsePropertyType(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParsePropertyType(Value("2")), std::runtime_error);
    BOOST_CHECK_THROW(ParsePropertyType(ValueFromString("null")), std::runtime_error);
    BOOST_CHECK_THROW(ParsePropertyType(ValueFromString("1.50")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_previous_property_id)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParsePreviousPropertyId(ValueFromString("0")), uint32_t(0));
    // Out of range
    BOOST_CHECK_THROW(ParsePreviousPropertyId(ValueFromString("1")), Object);
    BOOST_CHECK_THROW(ParsePreviousPropertyId(ValueFromString("-1")), Object);
    BOOST_CHECK_THROW(ParsePreviousPropertyId(ValueFromString("9223372036854775808")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParsePreviousPropertyId(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParsePreviousPropertyId(Value("0")), std::runtime_error);
    BOOST_CHECK_THROW(ParsePreviousPropertyId(ValueFromString("null")), std::runtime_error);
    BOOST_CHECK_THROW(ParsePreviousPropertyId(ValueFromString("0.0")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_text)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParseText(Value("")), "");
    BOOST_CHECK_EQUAL(ParseText(Value("0")), "0");
    BOOST_CHECK_EQUAL(ParseText(Value("-997.77")), "-997.77");
    BOOST_CHECK_EQUAL(ParseText(Value("Omni Core Tokens")), "Omni Core Tokens");
    BOOST_CHECK_EQUAL(ParseText(Value("TBM@e[$Rn)S$a..grahn n{d_}KWe940M-}z<^]{_Z^{~<IQr@Lt81Vq,'v1nseUUm>T'b<lryT^q(B'!FAO}*{E`"
            "rc)]U[6XpriKassU/)TcO/WQQAkPaK'BJx[l(OCyf24ImF16K`em0n_JSy;w:O0qC]H~lV==,~p0[,@;J?0%Dk+tUV1Z>MYqSu0q^}-V{gG>|kPj(4I"
            "3,dDUT3p/3b|<ua;~:VVa,!Y;!Ur$k8rOt &'/?qM(=C-i\bkj%")), "TBM@e[$Rn)S$a..grahn n{d_}KWe940M-}z<^]{_Z^{~<IQr@Lt81Vq,'"
            "v1nseUUm>T'b<lryT^q(B'!FAO}*{E`rc)]U[6XpriKassU/)TcO/WQQAkPaK'BJx[l(OCyf24ImF16K`em0n_JSy;w:O0qC]H~lV==,~p0[,@;J?0%"
            "Dk+tUV1Z>MYqSu0q^}-V{gG>|kPj(4I3,dDUT3p/3b|<ua;~:VVa,!Y;!Ur$k8rOt &'/?qM(=C-i\bkj%");
    // Out of range
    BOOST_CHECK_THROW(ParseText(Value("12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
            "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234"
            "567890123456789012345678901234567890 <- 240 ... 256!")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParseText(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParseText(ValueFromString("0")), std::runtime_error);
    BOOST_CHECK_THROW(ParseText(ValueFromString("null")), std::runtime_error);
    BOOST_CHECK_THROW(ParseText(ValueFromString("4815162342")), std::runtime_error);
    BOOST_CHECK_THROW(ParseText(ValueFromString("99.")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_deadline)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParseDeadline(ValueFromString("1231006505")), int64_t(1231006505));
    BOOST_CHECK_EQUAL(ParseDeadline(ValueFromString("2147483648")), int64_t(2147483648LL));
    // Out of range
    BOOST_CHECK_THROW(ParseDeadline(ValueFromString("-1")), Object);
    BOOST_CHECK_THROW(ParseDeadline(ValueFromString("-2342342342434562345")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParseDeadline(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParseDeadline(Value("11010102332")), std::runtime_error);
    BOOST_CHECK_THROW(ParseDeadline(ValueFromString("null")), std::runtime_error);
    BOOST_CHECK_THROW(ParseDeadline(ValueFromString("9224323423.25153")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_early_bird_bonus)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParseEarlyBirdBonus(ValueFromString("0")), uint8_t(0));
    BOOST_CHECK_EQUAL(ParseEarlyBirdBonus(ValueFromString("1")), uint8_t(1));
    BOOST_CHECK_EQUAL(ParseEarlyBirdBonus(ValueFromString("107")), uint8_t(107));
    BOOST_CHECK_EQUAL(ParseEarlyBirdBonus(ValueFromString("255")), uint8_t(255));
    // Out of range
    BOOST_CHECK_THROW(ParseEarlyBirdBonus(ValueFromString("-1")), Object);
    BOOST_CHECK_THROW(ParseEarlyBirdBonus(ValueFromString("256")), Object);
    BOOST_CHECK_THROW(ParseEarlyBirdBonus(ValueFromString("45345634745691")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParseEarlyBirdBonus(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParseEarlyBirdBonus(Value("-50")), std::runtime_error);
    BOOST_CHECK_THROW(ParseEarlyBirdBonus(ValueFromString("null")), std::runtime_error);
    BOOST_CHECK_THROW(ParseEarlyBirdBonus(ValueFromString("255.0000000000001")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_issuer_bonus)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParseIssuerBonus(ValueFromString("0")), uint8_t(0));
    BOOST_CHECK_EQUAL(ParseIssuerBonus(ValueFromString("1")), uint8_t(1));
    BOOST_CHECK_EQUAL(ParseIssuerBonus(ValueFromString("208")), uint8_t(208));
    BOOST_CHECK_EQUAL(ParseIssuerBonus(ValueFromString("255")), uint8_t(255));
    // Out of range
    BOOST_CHECK_THROW(ParseIssuerBonus(ValueFromString("-1")), Object);
    BOOST_CHECK_THROW(ParseIssuerBonus(ValueFromString("256")), Object);
    BOOST_CHECK_THROW(ParseIssuerBonus(ValueFromString("65537")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParseIssuerBonus(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParseIssuerBonus(Value("232.230")), std::runtime_error);
    BOOST_CHECK_THROW(ParseIssuerBonus(ValueFromString("null")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_meta_dex_action)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParseMetaDExAction(ValueFromString("1")), uint8_t(1));
    BOOST_CHECK_EQUAL(ParseMetaDExAction(ValueFromString("2")), uint8_t(2));
    BOOST_CHECK_EQUAL(ParseMetaDExAction(ValueFromString("3")), uint8_t(3));
    BOOST_CHECK_EQUAL(ParseMetaDExAction(ValueFromString("4")), uint8_t(4));
    // Out of range
    BOOST_CHECK_THROW(ParseMetaDExAction(ValueFromString("0")), Object);
    BOOST_CHECK_THROW(ParseMetaDExAction(ValueFromString("-1")), Object);
    BOOST_CHECK_THROW(ParseMetaDExAction(ValueFromString("5")), Object);
    BOOST_CHECK_THROW(ParseMetaDExAction(ValueFromString("-3253335464")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParseMetaDExAction(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParseMetaDExAction(Value("4")), std::runtime_error);
    BOOST_CHECK_THROW(ParseMetaDExAction(ValueFromString("null")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_transaction)
{
    // Valid input
    CTransaction txParsed = ParseTransaction(Value("07000000000100000000000000000a6a086f6d6e6961746f6d00000000"));
    BOOST_CHECK_EQUAL(txParsed.nVersion, 7);
    BOOST_CHECK_EQUAL(txParsed.vin.size(), 0);
    BOOST_CHECK_EQUAL(txParsed.vout.size(), 1);
    BOOST_CHECK_EQUAL(txParsed.nLockTime, 0);
    BOOST_CHECK_NO_THROW(ParseTransaction(Value()));
    BOOST_CHECK_NO_THROW(ParseTransaction(Value("")));
    BOOST_CHECK_NO_THROW(ParseTransaction(ValueFromString("null")));
    // Invalid input
    BOOST_CHECK_THROW(ParseTransaction(Value("null")), Object);
    BOOST_CHECK_THROW(ParseTransaction(Value("01007")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParseTransaction(ValueFromString("0100000000")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_mutable_transaction)
{
    // Valid input
    CMutableTransaction txParsed = ParseMutableTransaction(Value(
            "0100000001343b5a6728370da71c439bd032a8bb920b6ae1dc459a016a4b11c6afe7d0332a0000000000ffffffff00ffffffff"));
    BOOST_CHECK_EQUAL(txParsed.nVersion, 1);
    BOOST_CHECK_EQUAL(txParsed.vin.size(), 1);
    BOOST_CHECK_EQUAL(txParsed.vout.size(), 0);
    BOOST_CHECK_EQUAL(txParsed.nLockTime, 0xFFFFFFFF);
    BOOST_CHECK_NO_THROW(ParseTransaction(Value()));
    BOOST_CHECK_NO_THROW(ParseTransaction(Value("")));
    BOOST_CHECK_NO_THROW(ParseTransaction(ValueFromString("null")));
    // Invalid input
    BOOST_CHECK_THROW(ParseMutableTransaction(Value("null")), Object);    
    BOOST_CHECK_THROW(ParseMutableTransaction(Value("12355")), Object);    
    // Invalid types
    BOOST_CHECK_THROW(ParseMutableTransaction(ValueFromString("300.5")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_address_or_pubkey)
{
    // Valid input
    std::string validPubKey("02f3e471222bb57a7d416c82bf81c627bfcd2bdc47f36e763ae69935bba4601ece");
    BOOST_CHECK(ParsePubKeyOrAddress(Value(validPubKey)) == CPubKey(ParseHex(validPubKey)));
    // Invalid input
    std::string nonWalletAddress("1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa");
    std::string invalidPubKey("02f3e471222bb57a7d416c82bf81c627bfcd2bdc47f36e763ae69935bba4601ed0");
    BOOST_CHECK_THROW(ParsePubKeyOrAddress(Value(nonWalletAddress)), Object); // pubkey not in wallet
    BOOST_CHECK_THROW(ParsePubKeyOrAddress(Value(invalidPubKey)), Object); // not a valid ECDSA point
    // Invalid types
    BOOST_CHECK_THROW(ParsePubKeyOrAddress(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParsePubKeyOrAddress(ValueFromString("020202020202007")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_output_index)
{
    // Valid input
    BOOST_CHECK_EQUAL(ParseOutputIndex(ValueFromString("0")), uint32_t(0));
    BOOST_CHECK_EQUAL(ParseOutputIndex(ValueFromString("2")), uint32_t(2));
    BOOST_CHECK_EQUAL(ParseOutputIndex(ValueFromString("55")), uint32_t(55));
    // Out of range
    BOOST_CHECK_THROW(ParseOutputIndex(ValueFromString("-1")), Object);
    // Invalid types
    BOOST_CHECK_THROW(ParseOutputIndex(Value()), std::runtime_error);
    BOOST_CHECK_THROW(ParseOutputIndex(Value("123")), std::runtime_error);
    BOOST_CHECK_THROW(ParseOutputIndex(ValueFromString("null")), std::runtime_error);
    BOOST_CHECK_THROW(ParseOutputIndex(ValueFromString("1.0")), std::runtime_error);
    BOOST_CHECK_THROW(ParseOutputIndex(ValueFromString("-0.99")), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpcvalues_prevtxs_empty)
{
    std::vector<PrevTxsEntry> prevTxs = ParsePrevTxs(ValueFromString("[]"));
    BOOST_CHECK_EQUAL(0, prevTxs.size());
}

BOOST_AUTO_TEST_CASE(rpcvalues_prevtxs_one)
{
    std::string validPrevTx(
        "["
            "{"
                "\"txid\":\"c7de70ef10a1c3d3f8a446337bc1460e70893e61b1d1adb79cced09b16e14c6d\","
                "\"vout\":4,"
                "\"scriptPubKey\":\"76a914b25aa453f8d633b92db2bbb8f552170324d1e00d88ac\","
                "\"value\":0.00152810"
            "}"
        "]");

    std::vector<PrevTxsEntry> prevTxs = ParsePrevTxs(ValueFromString(validPrevTx));
    std::vector<unsigned char> script = ParseHex("76a914b25aa453f8d633b92db2bbb8f552170324d1e00d88ac");

    BOOST_CHECK_EQUAL(1, prevTxs.size());
    BOOST_CHECK(uint256("c7de70ef10a1c3d3f8a446337bc1460e70893e61b1d1adb79cced09b16e14c6d") == prevTxs[0].outPoint.hash);
    BOOST_CHECK_EQUAL(4, prevTxs[0].outPoint.n);
    BOOST_CHECK(CScript(script.begin(), script.end()) == prevTxs[0].txOut.scriptPubKey);
    BOOST_CHECK_EQUAL(152810LL, prevTxs[0].txOut.nValue);
}

BOOST_AUTO_TEST_CASE(rpcvalues_prevtxs_two)
{
    std::string validPrevTx(
        "["
        "    {"
        "        \"scriptPubKey\":           \"a914ea1153af4beaf36067175ab8c9ef34b4bfec419e87\","
        "        \"value\": 0.03347963,"
        "        \"txid\": \"ab283ae1c1d71468a98eb850c39f688e1ffa022f7dfbc1c4ac58fc56a272167c\","
        "        \"vout\": 0"
        "    }, {"
                "\"txid\":\"1137e11996ea3fe690394bc02cef422d10310b6042c956ca7eb95f24277b6fea\","
                "\"vout\":0,"
                "\"scriptPubKey\":\"51210394abb6fe6d492495f855ab3fa29acd6be04f347d17597985da0e8c70a26036f22102f8acc5e6654559ca12408fdc6184aa68db62083a6c1f448894845c3f7dfed78552ae\","
                "\"value\":7.0"
            "}"
        "]");

    std::vector<PrevTxsEntry> prevTxs = ParsePrevTxs(ValueFromString(validPrevTx));
    std::vector<unsigned char> scriptA = ParseHex("a914ea1153af4beaf36067175ab8c9ef34b4bfec419e87");
    std::vector<unsigned char> scriptB = ParseHex("51210394abb6fe6d492495f855ab3fa29acd6be04f347d17597985da0e8c70a26036f22102f8acc5e6654559ca12408fdc6184aa68db62083a6c1f448894845c3f7dfed78552ae");

    BOOST_CHECK_EQUAL(2, prevTxs.size());
    BOOST_CHECK(uint256("ab283ae1c1d71468a98eb850c39f688e1ffa022f7dfbc1c4ac58fc56a272167c") == prevTxs[0].outPoint.hash);
    BOOST_CHECK(uint256("1137e11996ea3fe690394bc02cef422d10310b6042c956ca7eb95f24277b6fea") == prevTxs[1].outPoint.hash);
    BOOST_CHECK_EQUAL(0, prevTxs[0].outPoint.n);
    BOOST_CHECK_EQUAL(0, prevTxs[1].outPoint.n);
    BOOST_CHECK(CScript(scriptA.begin(), scriptA.end()) == prevTxs[0].txOut.scriptPubKey);
    BOOST_CHECK(CScript(scriptB.begin(), scriptB.end()) == prevTxs[1].txOut.scriptPubKey);
    BOOST_CHECK_EQUAL(3347963LL, prevTxs[0].txOut.nValue);
    BOOST_CHECK_EQUAL(700000000LL, prevTxs[1].txOut.nValue);
}

BOOST_AUTO_TEST_CASE(rpcvalues_prevtxs_invalid)
{
    // missing txid
    BOOST_CHECK_THROW(ParsePrevTxs(ValueFromString(
        "["
            "{"
                "\"vout\":4,"
                "\"scriptPubKey\":\"76a9145b21669fe9375599bb65919cb9efb9d349f33b5a88ac\","
                "\"value\":0.00155220"
            "}"
        "]")), Object);
    // missing vout
    BOOST_CHECK_THROW(ParsePrevTxs(ValueFromString(
        "["
            "{"
                "\"txid\":\"75cb6a2e2853c850baeb20a91b7015ef61d2b6886e26866a25f88bc71280b6f9\","
                "\"scriptPubKey\":\"76a9145b21669fe9375599bb65919cb9efb9d349f33b5a88ac\","
                "\"value\":0.00155220"
            "}"
        "]")), Object);
    // missing scriptPubKey
    BOOST_CHECK_THROW(ParsePrevTxs(ValueFromString(
        "["
            "{"
                "\"txid\":\"75cb6a2e2853c850baeb20a91b7015ef61d2b6886e26866a25f88bc71280b6f9\","
                "\"vout\":4,"
                "\"value\":0.00155220"
            "}"
        "]")), Object);
    // missing value
    BOOST_CHECK_THROW(ParsePrevTxs(ValueFromString(
        "["
            "{"
                "\"txid\":\"75cb6a2e2853c850baeb20a91b7015ef61d2b6886e26866a25f88bc71280b6f9\","
                "\"vout\":4,"
                "\"scriptPubKey\":\"76a9145b21669fe9375599bb65919cb9efb9d349f33b5a88ac\""
            "}"
        "]")), Object);
    // invalid txid
    BOOST_CHECK_THROW(ParsePrevTxs(ValueFromString(
        "["
            "{"
                "\"txid\":1234,"
                "\"vout\":4,"
                "\"scriptPubKey\":\"76a9145b21669fe9375599bb65919cb9efb9d349f33b5a88ac\","
                "\"value\":0.00155220"
            "}"
        "]")), Object);
    // invalid vout
    BOOST_CHECK_THROW(ParsePrevTxs(ValueFromString(
        "["
            "{"
                "\"txid\":\"75cb6a2e2853c850baeb20a91b7015ef61d2b6886e26866a25f88bc71280b6f9\","
                "\"vout\":-3,"
                "\"scriptPubKey\":\"76a9145b21669fe9375599bb65919cb9efb9d349f33b5a88ac\","
                "\"value\":0.00155220" 
            "}"
        "]")), Object);
    // invalid script
    BOOST_CHECK_THROW(ParsePrevTxs(ValueFromString(
        "["
            "{"
                "\"txid\":\"75cb6a2e2853c850baeb20a91b7015ef61d2b6886e26866a25f88bc71280b6f9\","
                "\"vout\":4,"
                "\"scriptPubKey\":\"76a9145b21669fe9375599bb65919cb9efb9d349f33b5a88a\","
                "\"value\":0.00155220" 
            "}"
        "]")), Object);
    // invalid value
    BOOST_CHECK_THROW(ParsePrevTxs(ValueFromString(
        "["
            "{"
                "\"txid\":\"75cb6a2e2853c850baeb20a91b7015ef61d2b6886e26866a25f88bc71280b6f9\","
                "\"vout\":4,"
                "\"scriptPubKey\":\"76a9145b21669fe9375599bb65919cb9efb9d349f33b5a88a\","
                "\"value\":-0.00155220" 
            "}"
        "]")), Object);
    // invalid value
    BOOST_CHECK_THROW(ParsePrevTxs(ValueFromString(
        "["
            "{"
                "\"txid\":\"75cb6a2e2853c850baeb20a91b7015ef61d2b6886e26866a25f88bc71280b6f9\","
                "\"vout\":4,"
                "\"scriptPubKey\":\"76a9145b21669fe9375599bb65919cb9efb9d349f33b5a88a\","
                "\"value\":21000001" 
            "}"
        "]")), Object);
}

BOOST_AUTO_TEST_SUITE_END()
