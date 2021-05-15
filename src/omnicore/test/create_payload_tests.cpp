#include <omnicore/createpayload.h>

#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <stdint.h>
#include <vector>
#include <string>

BOOST_FIXTURE_TEST_SUITE(omnicore_create_payload_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(payload_simple_send)
{
    // Simple send [type 0, version 0]
    std::vector<unsigned char> vch = CreatePayload_SimpleSend(
        static_cast<uint32_t>(1),          // property: MSC
        static_cast<int64_t>(100000000));  // amount to transfer: 1.0 MSC (in willets)

    BOOST_CHECK_EQUAL(HexStr(vch), "00000000000000010000000005f5e100");
}

BOOST_AUTO_TEST_CASE(payload_send_to_owners)
{
    // Send to owners [type 3, version 0] (same property)
    std::vector<unsigned char> vch = CreatePayload_SendToOwners(
        static_cast<uint32_t>(1),          // property: OMNI
        static_cast<int64_t>(100000000),   // amount to transfer: 1.0 OMNI (in willets)
        static_cast<uint32_t>(1));         // property: OMNI

    BOOST_CHECK_EQUAL(HexStr(vch), "00000003000000010000000005f5e100");
}

BOOST_AUTO_TEST_CASE(payload_send_to_owners_v1)
{
    // Send to owners [type 3, version 1] (cross property)
    std::vector<unsigned char> vch = CreatePayload_SendToOwners(
        static_cast<uint32_t>(1),          // property: OMNI
        static_cast<int64_t>(100000000),   // amount to transfer: 1.0 OMNI (in willets)
        static_cast<uint32_t>(3));         // property: SP#3

    BOOST_CHECK_EQUAL(HexStr(vch), "00010003000000010000000005f5e10000000003");
}

BOOST_AUTO_TEST_CASE(payload_send_all)
{
    // Send to owners [type 4, version 0]
    std::vector<unsigned char> vch = CreatePayload_SendAll(
        static_cast<uint8_t>(2));          // ecosystem: Test

    BOOST_CHECK_EQUAL(HexStr(vch), "0000000402");
}

BOOST_AUTO_TEST_CASE(payload_dex_offer)
{
    // Sell tokens for bitcoins [type 20, version 1]
    std::vector<unsigned char> vch = CreatePayload_DExSell(
        static_cast<uint32_t>(1),         // property: MSC
        static_cast<int64_t>(100000000),  // amount to transfer: 1.0 MSC (in willets)
        static_cast<int64_t>(20000000),   // amount desired: 0.2 BTC (in satoshis)
        static_cast<uint8_t>(10),         // payment window in blocks
        static_cast<int64_t>(10000),      // commitment fee in satoshis
        static_cast<uint8_t>(1));         // sub-action: new offer

    BOOST_CHECK_EQUAL(HexStr(vch),
        "00010014000000010000000005f5e1000000000001312d000a000000000000271001");
}

BOOST_AUTO_TEST_CASE(payload_meta_dex_new_trade)
{
    // Trade tokens for tokens [type 25, version 0]
    std::vector<unsigned char> vch = CreatePayload_MetaDExTrade(
        static_cast<uint32_t>(1),          // property: MSC
        static_cast<int64_t>(250000000),   // amount for sale: 2.5 MSC
        static_cast<uint32_t>(31),         // property desired: TetherUS
        static_cast<int64_t>(5000000000)); // amount desired: 50.0 TetherUS

    BOOST_CHECK_EQUAL(HexStr(vch),
        "0000001900000001000000000ee6b2800000001f000000012a05f200");
}

BOOST_AUTO_TEST_CASE(payload_meta_dex_cancel_at_price)
{
    // Trade tokens for tokens [type 26, version 0]
    std::vector<unsigned char> vch = CreatePayload_MetaDExCancelPrice(
        static_cast<uint32_t>(1),          // property: MSC
        static_cast<int64_t>(250000000),   // amount for sale: 2.5 MSC
        static_cast<uint32_t>(31),         // property desired: TetherUS
        static_cast<int64_t>(5000000000)); // amount desired: 50.0 TetherUS

    BOOST_CHECK_EQUAL(HexStr(vch),
        "0000001a00000001000000000ee6b2800000001f000000012a05f200");
}

BOOST_AUTO_TEST_CASE(payload_meta_dex_cancel_pair)
{
    // Trade tokens for tokens [type 27, version 0]
    std::vector<unsigned char> vch = CreatePayload_MetaDExCancelPair(
        static_cast<uint32_t>(1),          // property: MSC
        static_cast<uint32_t>(31));        // property desired: TetherUS

    BOOST_CHECK_EQUAL(HexStr(vch),
        "0000001b000000010000001f");
}

BOOST_AUTO_TEST_CASE(payload_meta_dex_cancel_ecosystem)
{
    // Trade tokens for tokens [type 28, version 0]
    std::vector<unsigned char> vch = CreatePayload_MetaDExCancelEcosystem(
        static_cast<uint8_t>(1));          // ecosystem: Main

    BOOST_CHECK_EQUAL(HexStr(vch),
        "0000001c01");
}

BOOST_AUTO_TEST_CASE(payload_accept_dex_offer)
{
    // Purchase tokens with bitcoins [type 22, version 0]
    std::vector<unsigned char> vch = CreatePayload_DExAccept(
        static_cast<uint32_t>(1),          // property: MSC
        static_cast<int64_t>(130000000));  // amount to transfer: 1.3 MSC (in willets)

    BOOST_CHECK_EQUAL(HexStr(vch), "00000016000000010000000007bfa480");
}

BOOST_AUTO_TEST_CASE(payload_create_property)
{
    // Create property [type 50, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceFixed(
        static_cast<uint8_t>(1),             // ecosystem: main
        static_cast<uint16_t>(1),            // property type: indivisible tokens
        static_cast<uint32_t>(0),            // previous property: none
        std::string("Companies"),            // category
        std::string("Bitcoin Mining"),       // subcategory
        std::string("Quantum Miner"),        // label
        std::string("builder.bitwatch.co"),  // website
        std::string(""),                     // additional information
        static_cast<int64_t>(1000000));      // number of units to create

    BOOST_CHECK_EQUAL(HexStr(vch),
        "0000003201000100000000436f6d70616e69657300426974636f696e204d696e696e67"
        "005175616e74756d204d696e6572006275696c6465722e62697477617463682e636f00"
        "0000000000000f4240");
}

BOOST_AUTO_TEST_CASE(payload_create_property_empty)
{
    // Create property [type 50, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceFixed(
        static_cast<uint8_t>(1),         // ecosystem: main
        static_cast<uint16_t>(1),        // property type: indivisible tokens
        static_cast<uint32_t>(0),        // previous property: none
        std::string(""),                 // category
        std::string(""),                 // subcategory
        std::string(""),                 // label
        std::string(""),                 // website
        std::string(""),                 // additional information
        static_cast<int64_t>(1000000));  // number of units to create

    BOOST_CHECK_EQUAL(vch.size(), 24);
}

BOOST_AUTO_TEST_CASE(payload_create_property_full)
{
    // Create property [type 50, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceFixed(
        static_cast<uint8_t>(1),         // ecosystem: main
        static_cast<uint16_t>(1),        // property type: indivisible tokens
        static_cast<uint32_t>(0),        // previous property: none
        std::string(700, 'x'),           // category
        std::string(700, 'x'),           // subcategory
        std::string(700, 'x'),           // label
        std::string(700, 'x'),           // website
        std::string(700, 'x'),           // additional information
        static_cast<int64_t>(1000000));  // number of units to create

    BOOST_CHECK_EQUAL(vch.size(), 1299);
}

BOOST_AUTO_TEST_CASE(payload_create_crowdsale)
{
    // Create crowdsale [type 51, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceVariable(
        static_cast<uint8_t>(1),             // ecosystem: main
        static_cast<uint16_t>(1),            // property type: indivisible tokens
        static_cast<uint32_t>(0),            // previous property: none
        std::string("Companies"),            // category
        std::string("Bitcoin Mining"),       // subcategory
        std::string("Quantum Miner"),        // label
        std::string("builder.bitwatch.co"),  // website
        std::string(""),                     // additional information
        static_cast<uint32_t>(1),            // property desired: MSC
        static_cast<int64_t>(100),           // tokens per unit vested
        static_cast<uint64_t>(7731414000L),  // deadline: 31 Dec 2214 23:00:00 UTC
        static_cast<uint8_t>(10),            // early bird bonus: 10 % per week
        static_cast<uint8_t>(12));           // issuer bonus: 12 %

    BOOST_CHECK_EQUAL(HexStr(vch),
        "0000003301000100000000436f6d70616e69657300426974636f696e204d696e696e67"
        "005175616e74756d204d696e6572006275696c6465722e62697477617463682e636f00"
        "0000000001000000000000006400000001ccd403f00a0c");
}

BOOST_AUTO_TEST_CASE(payload_create_crowdsale_empty)
{
    // Create crowdsale [type 51, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceVariable(
        static_cast<uint8_t>(1),            // ecosystem: main
        static_cast<uint16_t>(1),           // property type: indivisible tokens
        static_cast<uint32_t>(0),           // previous property: none
        std::string(""),                    // category
        std::string(""),                    // subcategory
        std::string(""),                    // label
        std::string(""),                    // website
        std::string(""),                    // additional information
        static_cast<uint32_t>(1),           // property desired: MSC
        static_cast<int64_t>(100),          // tokens per unit vested
        static_cast<uint64_t>(7731414000L), // deadline: 31 Dec 2214 23:00:00 UTC
        static_cast<uint8_t>(10),           // early bird bonus: 10 % per week
        static_cast<uint8_t>(12));          // issuer bonus: 12 %

    BOOST_CHECK_EQUAL(vch.size(), 38);
}

BOOST_AUTO_TEST_CASE(payload_create_crowdsale_full)
{
    // Create crowdsale [type 51, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceVariable(
        static_cast<uint8_t>(1),            // ecosystem: main
        static_cast<uint16_t>(1),           // property type: indivisible tokens
        static_cast<uint32_t>(0),           // previous property: none
        std::string(700, 'x'),              // category
        std::string(700, 'x'),              // subcategory
        std::string(700, 'x'),              // label
        std::string(700, 'x'),              // website
        std::string(700, 'x'),              // additional information
        static_cast<uint32_t>(1),           // property desired: MSC
        static_cast<int64_t>(100),          // tokens per unit vested
        static_cast<uint64_t>(7731414000L), // deadline: 31 Dec 2214 23:00:00 UTC
        static_cast<uint8_t>(10),           // early bird bonus: 10 % per week
        static_cast<uint8_t>(12));          // issuer bonus: 12 %

    BOOST_CHECK_EQUAL(vch.size(), 1313);
}

BOOST_AUTO_TEST_CASE(payload_close_crowdsale)
{
    // Close crowdsale [type 53, version 0]
    std::vector<unsigned char> vch = CreatePayload_CloseCrowdsale(
        static_cast<uint32_t>(9));  // property: SP #9

    BOOST_CHECK_EQUAL(HexStr(vch), "0000003500000009");
}

BOOST_AUTO_TEST_CASE(payload_create_managed_property)
{
    // create managed property [type 54, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceManaged(
        static_cast<uint8_t>(1),             // ecosystem: main
        static_cast<uint16_t>(1),            // property type: indivisible tokens
        static_cast<uint32_t>(0),            // previous property: none
        std::string("Companies"),            // category
        std::string("Bitcoin Mining"),       // subcategory
        std::string("Quantum Miner"),        // label
        std::string("builder.bitwatch.co"),  // website
        std::string(""));                    // additional information

    BOOST_CHECK_EQUAL(HexStr(vch),
        "0000003601000100000000436f6d70616e69657300426974636f696e204d696e696e67"
        "005175616e74756d204d696e6572006275696c6465722e62697477617463682e636f00"
        "00");
}

BOOST_AUTO_TEST_CASE(payload_create_managed_property_empty)
{
    // create managed property [type 54, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceManaged(
        static_cast<uint8_t>(1),   // ecosystem: main
        static_cast<uint16_t>(1),  // property type: indivisible tokens
        static_cast<uint32_t>(0),  // previous property: none
        std::string(""),           // category
        std::string(""),           // subcategory
        std::string(""),           // label
        std::string(""),           // website
        std::string(""));          // additional information

    BOOST_CHECK_EQUAL(vch.size(), 16);
}

BOOST_AUTO_TEST_CASE(payload_create_managed_property_full)
{
    // create managed property [type 54, version 0]
    std::vector<unsigned char> vch = CreatePayload_IssuanceManaged(
        static_cast<uint8_t>(1),   // ecosystem: main
        static_cast<uint16_t>(1),  // property type: indivisible tokens
        static_cast<uint32_t>(0),  // previous property: none
        std::string(700, 'x'),     // category
        std::string(700, 'x'),     // subcategory
        std::string(700, 'x'),     // label
        std::string(700, 'x'),     // website
        std::string(700, 'x'));    // additional information

    BOOST_CHECK_EQUAL(vch.size(), 1291);
}

BOOST_AUTO_TEST_CASE(payload_grant_tokens)
{
    // Grant tokens [type 55, version 0]
    std::vector<unsigned char> vch = CreatePayload_Grant(
        static_cast<uint32_t>(8),                  // property: SP #8
        static_cast<int64_t>(1000),                // number of units to issue
        std::string("First Milestone Reached!"));  // additional information

    BOOST_CHECK_EQUAL(HexStr(vch),
        "000000370000000800000000000003e84669727374204d696c6573746f6e6520526561"
        "636865642100");
}

BOOST_AUTO_TEST_CASE(payload_grant_tokens_empty)
{
    // Grant tokens [type 55, version 0]
    std::vector<unsigned char> vch = CreatePayload_Grant(
        static_cast<uint32_t>(8),                  // property: SP #8
        static_cast<int64_t>(1000),                // number of units to issue
        std::string(""));                          // additional information

    BOOST_CHECK_EQUAL(vch.size(), 17);
}

BOOST_AUTO_TEST_CASE(payload_grant_tokens_full)
{
    // Grant tokens [type 55, version 0]
    std::vector<unsigned char> vch = CreatePayload_Grant(
        static_cast<uint32_t>(8),                  // property: SP #8
        static_cast<int64_t>(1000),                // number of units to issue
        std::string(700, 'x'));                    // additional information

    BOOST_CHECK_EQUAL(vch.size(), 272);
}

BOOST_AUTO_TEST_CASE(payload_revoke_tokens)
{
    // Revoke tokens [type 56, version 0]
    std::vector<unsigned char> vch = CreatePayload_Revoke(
        static_cast<uint32_t>(8),                                   // property: SP #8
        static_cast<int64_t>(1000),                                 // number of units to revoke
        std::string("Redemption of tokens for Bob, Thanks Bob!"));  // additional information

    BOOST_CHECK_EQUAL(HexStr(vch),
        "000000380000000800000000000003e8526564656d7074696f6e206f6620746f6b656e"
        "7320666f7220426f622c205468616e6b7320426f622100");
}

BOOST_AUTO_TEST_CASE(payload_revoke_tokens_empty)
{
    // Revoke tokens [type 56, version 0]
    std::vector<unsigned char> vch = CreatePayload_Revoke(
        static_cast<uint32_t>(8),    // property: SP #8
        static_cast<int64_t>(1000),  // number of units to revoke
        std::string(""));            // additional information

    BOOST_CHECK_EQUAL(vch.size(), 17);
}

BOOST_AUTO_TEST_CASE(payload_revoke_tokens_full)
{
    // Revoke tokens [type 56, version 0]
    std::vector<unsigned char> vch = CreatePayload_Revoke(
        static_cast<uint32_t>(8),    // property: SP #8
        static_cast<int64_t>(1000),  // number of units to revoke
        std::string(700, 'x'));      // additional information

    BOOST_CHECK_EQUAL(vch.size(), 272);
}

BOOST_AUTO_TEST_CASE(payload_change_property_manager)
{
    // Change property manager [type 70, version 0]
    std::vector<unsigned char> vch = CreatePayload_ChangeIssuer(
        static_cast<uint32_t>(13));  // property: SP #13

    BOOST_CHECK_EQUAL(HexStr(vch), "000000460000000d");
}

BOOST_AUTO_TEST_CASE(payload_enable_freezing)
{
    // Enable freezing [type 71, version 0]
    std::vector<unsigned char> vch = CreatePayload_EnableFreezing(
        static_cast<uint32_t>(4));                 // property: SP #4

    BOOST_CHECK_EQUAL(HexStr(vch), "0000004700000004");
}

BOOST_AUTO_TEST_CASE(payload_disable_freezing)
{
    // Disable freezing [type 72, version 0]
    std::vector<unsigned char> vch = CreatePayload_DisableFreezing(
        static_cast<uint32_t>(4));                 // property: SP #4

    BOOST_CHECK_EQUAL(HexStr(vch), "0000004800000004");
}

BOOST_AUTO_TEST_CASE(payload_freeze_tokens)
{
    // Freeze tokens [type 185, version 0]
    std::vector<unsigned char> vch = CreatePayload_FreezeTokens(
        static_cast<uint32_t>(4),                                   // property: SP #4
        static_cast<int64_t>(1000),                                 // amount to freeze (unused)
        std::string("1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));         // reference address

    BOOST_CHECK_EQUAL(HexStr(vch),
        "000000b90000000400000000000003e800946cb2e08075bcbaf157e47bcb67eb2b2339d242");
}

BOOST_AUTO_TEST_CASE(payload_unfreeze_tokens)
{
    // Freeze tokens [type 186, version 0]
    std::vector<unsigned char> vch = CreatePayload_UnfreezeTokens(
        static_cast<uint32_t>(4),                                   // property: SP #4
        static_cast<int64_t>(1000),                                 // amount to freeze (unused)
        std::string("1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));         // reference address

    BOOST_CHECK_EQUAL(HexStr(vch),
        "000000ba0000000400000000000003e800946cb2e08075bcbaf157e47bcb67eb2b2339d242");
}

BOOST_AUTO_TEST_CASE(payload_add_delegate)
{
    // Enable freezing [type 73, version 0]
    std::vector<unsigned char> vch = CreatePayload_AddDelegate(
        static_cast<uint32_t>(21));                 // property: SP #21

    BOOST_CHECK_EQUAL(HexStr(vch), "0000004900000015");
}

BOOST_AUTO_TEST_CASE(payload_remove_delegate)
{
    // Enable freezing [type 74, version 0]
    std::vector<unsigned char> vch = CreatePayload_RemoveDelegate(
        static_cast<uint32_t>(21));                 // property: SP #21

    BOOST_CHECK_EQUAL(HexStr(vch), "0000004a00000015");
}

BOOST_AUTO_TEST_CASE(payload_anydata)
{
    // Freeze tokens [type 200, version 0]
    std::vector<unsigned char> vch = CreatePayload_AnyData(
        ParseHex("646578782032303230"));                            // data

    BOOST_CHECK_EQUAL(HexStr(vch),
        "000000c8646578782032303230");
}

BOOST_AUTO_TEST_CASE(payload_feature_deactivation)
{
    // Omni Core feature activation [type 65533, version 65535]
    std::vector<unsigned char> vch = CreatePayload_DeactivateFeature(
        static_cast<uint16_t>(1));        // feature identifier: 1 (OP_RETURN)

    BOOST_CHECK_EQUAL(HexStr(vch), "fffffffd0001");
}

BOOST_AUTO_TEST_CASE(payload_feature_activation)
{
    // Omni Core feature activation [type 65534, version 65535]
    std::vector<unsigned char> vch = CreatePayload_ActivateFeature(
        static_cast<uint16_t>(1),        // feature identifier: 1 (OP_RETURN)
        static_cast<uint32_t>(370000),   // activation block
        static_cast<uint32_t>(999));     // min client version

    BOOST_CHECK_EQUAL(HexStr(vch), "fffffffe00010005a550000003e7");
}

BOOST_AUTO_TEST_CASE(payload_omnicore_alert_block)
{
    // Omni Core client notification [type 65535, version 65535]
    std::vector<unsigned char> vch = CreatePayload_OmniCoreAlert(
        static_cast<int32_t>(1),            // alert target: by block number
        static_cast<uint64_t>(300000),      // expiry value: 300000
        static_cast<std::string>("test"));  // alert message: test

    BOOST_CHECK_EQUAL(HexStr(vch), "ffffffff0001000493e07465737400");
}

BOOST_AUTO_TEST_CASE(payload_omnicore_alert_blockexpiry)
{
    // Omni Core client notification [type 65535, version 65535]
    std::vector<unsigned char> vch = CreatePayload_OmniCoreAlert(
        static_cast<int32_t>(2),            // alert target: by block time
        static_cast<uint64_t>(1439528630),  // expiry value: 1439528630
        static_cast<std::string>("test"));  // alert message: test

    BOOST_CHECK_EQUAL(HexStr(vch), "ffffffff000255cd76b67465737400");
}

BOOST_AUTO_TEST_CASE(payload_omnicore_alert_minclient)
{
    // Omni Core client notification [type 65535, version 65535]
    std::vector<unsigned char> vch = CreatePayload_OmniCoreAlert(
        static_cast<int32_t>(3),            // alert target: by client version
        static_cast<uint64_t>(900100),      // expiry value: v0.0.9.1
        static_cast<std::string>("test"));  // alert message: test

    BOOST_CHECK_EQUAL(HexStr(vch), "ffffffff0003000dbc047465737400");
}

BOOST_AUTO_TEST_SUITE_END()
