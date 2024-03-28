#include <base58.h>
#include <boost/test/unit_test.hpp>

#include <string>

BOOST_AUTO_TEST_SUITE(base58_prefix_decode_test)

BOOST_AUTO_TEST_CASE(base58_prefix_decode_testvectors_valid)
{
    const std::string base58_address_std = "12higDjoCCNXSA95xZMWUdPvXNmkAduhWv";

    static const unsigned char CASES[] = {
        0x00, // Mainnet pay to public key hash
        0x01, // Largest range of prefixes
        0x02, // Second largest range of prefixes
        0x03,
        0x04,
        0x05, // Mainnet script hash
        0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
        0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,
        0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41,
        0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B,
        0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55,
        0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x6A, 0x6B, 0x6C, 0x6D, 0x6E,
        0x6F, // Testnet pubkey hash
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83,
        0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D,
        0x8E, 0x8F, 0x90,
        0xC4, // Testnet script hash
        0xFF  // End of possible range of inputs
    };

    static const std::vector<char> RANGES[] = {
        {'1'},
        {'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','m','n','o'},
        {'o','p','q','r','s','t','u','v','w','x','y','z','2'},
        {'2'},
        {'2','3'},
        {'3'},
        {'3'},{'3','4'},{'4'},{'4','5'},{'5'},{'5'},{'5','6'},{'6'},{'6','7'},{'7'}, // Prefix values 6-15
        {'7'},{'7','8'},{'8'},{'8','9'},{'9'},{'9'},{'9','A'},{'A'},{'A','B'},{'B'}, // Prefix values 16-25
        {'B'},{'B','C'},{'C'},{'C','D'},{'D'},{'D'},{'D','E'},{'E'},{'E','F'},{'F'}, // Prefix values 26-35
        {'F'},{'F','G'},{'G'},{'G','H'},{'H'},{'H'},{'H','J'},{'J'},{'J','K'},{'K'}, // Prefix values 36-45
        {'K'},{'K','L'},{'L'},{'L','M'},{'M'},{'M'},{'M','N'},{'N'},{'N','P'},{'P'}, // Prefix values 46-55
        {'P'},{'P','Q'},{'Q'},{'Q','R'},{'R'},{'R'},{'R','S'},{'S'},{'S','T'},{'T'}, // Prefix values 56-65
        {'T'},{'T','U'},{'U'},{'U','V'},{'V'},{'V'},{'V','W'},{'W'},{'W','X'},{'X'}, // Prefix values 66-75
        {'X'},{'X','Y'},{'Y'},{'Y','Z'},{'Z'},{'Z'},{'Z','a'},{'a'},{'a','b'},{'b'}, // Prefix values 76-85
        {'b','c'},{'c'},{'c'},{'c','d'},{'d'},{'d','e'},{'e'},{'e'},{'e','f'},{'f'}, // Prefix values 86-95
        {'f','g'},{'g'},{'g'},{'g','h'},{'h'},{'h','i'},{'i'},{'i'},{'i','j'},{'j'}, // Prefix values 96-105
        {'j','k'},{'k'},{'k'},{'k','m'},{'m'},{'m','n'},{'n'},{'n'},{'n','o'},{'o'}, // Prefix values 106-115
        {'o','p'},{'p'},{'p'},{'p','q'},{'q'},{'q','r'},{'r'},{'r'},{'r','s'},{'s'}, // Prefix values 116-125
        {'s','t'},{'t'},{'t'},{'t','u'},{'u'},{'u','v'},{'v'},{'v'},{'v','w'},{'w'}, // Prefix values 126-135
        {'w','x'},{'x'},{'x'},{'x','y'},{'y'},{'y','z'},{'z'},{'z'},{'z','2'},       // Prefix values 136-144
        {'2'},
        {'2'}
    };


    static_assert(std::size(CASES) == std::size(RANGES), "Base58 CASES and RANGES should have the same length");

    std::vector<unsigned char> base58_data;
    bool valid_base58 = DecodeBase58(base58_address_std, base58_data, 25);
    BOOST_CHECK(valid_base58  == true);

    /* variable output 25 byte decoded base58 cases */
    int i = 0;
    for (const unsigned version_byte : CASES) {
        std::vector<char> prefixes = Base58PrefixesFromVersionByte(base58_data.size(), version_byte);
        std::vector<char> range = RANGES[i];
        BOOST_CHECK_MESSAGE(prefixes == range, "Base58 prefix decoder, version byte " << (uint8_t)version_byte << " prefixes " << std::string(prefixes.begin(),prefixes.end()) << " != " << std::string(range.begin(),range.end()) << " with a total len. of 25");
        ++i;
    }

    /* static output 25 byte decoded base58 cases */
    for (unsigned short version_byte = 0x91; version_byte <= 0xFF; version_byte++) {
        std::vector<char> prefixes = Base58PrefixesFromVersionByte(base58_data.size(), version_byte);
        std::vector<char> expected(1,'2');
        BOOST_CHECK_MESSAGE(prefixes == expected, "Base58 prefix decoder, version byte " << (uint8_t)version_byte << " prefixes " << std::string(prefixes.begin(),prefixes.end()) << " != " << std::string(expected.begin(),expected.end()) << " with a total len. of 25");
    }
}

BOOST_AUTO_TEST_SUITE_END()
