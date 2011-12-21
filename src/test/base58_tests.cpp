#include <boost/test/unit_test.hpp>

#include "main.h"
#include "wallet.h"
#include "util.h"

BOOST_AUTO_TEST_SUITE(base58_tests)

// TODO:
// EncodeBase58Check
// DecodeBase58Check
// CBase58Data
//    bool SetString(const char* psz)
    // bool SetString(const std::string& str)
    // std::string ToString() const
    // int CompareTo(const CBase58Data& b58) const
    // bool operator==(const CBase58Data& b58) const
    // bool operator<=(const CBase58Data& b58) const
    // bool operator>=(const CBase58Data& b58) const
    // bool operator< (const CBase58Data& b58) const
    // bool operator> (const CBase58Data& b58) const

// CBitcoinAddress
    // bool SetHash160(const uint160& hash160)
    // bool SetPubKey(const std::vector<unsigned char>& vchPubKey)
    // bool IsValid() const
    // CBitcoinAddress()
    // CBitcoinAddress(uint160 hash160In)
    // CBitcoinAddress(const std::vector<unsigned char>& vchPubKey)
    // CBitcoinAddress(const std::string& strAddress)
    // CBitcoinAddress(const char* pszAddress)
    // uint160 GetHash160() const

#define U(x) (reinterpret_cast<const unsigned char*>(x))
static struct {
    const unsigned char *data;
    int size;
} vstrIn[] = {
{U(""), 0},
{U("\x61"), 1},
{U("\x62\x62\x62"), 3},
{U("\x63\x63\x63"), 3},
{U("\x73\x69\x6d\x70\x6c\x79\x20\x61\x20\x6c\x6f\x6e\x67\x20\x73\x74\x72\x69\x6e\x67"), 20},
{U("\x00\xeb\x15\x23\x1d\xfc\xeb\x60\x92\x58\x86\xb6\x7d\x06\x52\x99\x92\x59\x15\xae\xb1\x72\xc0\x66\x47"), 25},
{U("\x51\x6b\x6f\xcd\x0f"), 5},
{U("\xbf\x4f\x89\x00\x1e\x67\x02\x74\xdd"), 9},
{U("\x57\x2e\x47\x94"), 4},
{U("\xec\xac\x89\xca\xd9\x39\x23\xc0\x23\x21"), 10},
{U("\x10\xc8\x51\x1e"), 4},
{U("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"), 10},
};

const char *vstrOut[] = {
"",
"2g",
"a3gV",
"aPEr",
"2cFupjhnEsSn59qHXstmK2ffpLv2",
"1NS17iag9jJgTHD1VXjvLCEnZuQ3rJDE9L",
"ABnLTmg",
"3SEo3LWLoPntC",
"3EFU7m",
"EJDM8drfXA6uyA",
"Rt5zm",
"1111111111"
};

BOOST_AUTO_TEST_CASE(base58_EncodeBase58)
{
    for (int i=0; i<sizeof(vstrIn)/sizeof(vstrIn[0]); i++)
    {
        BOOST_CHECK_EQUAL(EncodeBase58(vstrIn[i].data, vstrIn[i].data + vstrIn[i].size), vstrOut[i]);
    }
}

BOOST_AUTO_TEST_CASE(base58_DecodeBase58)
{
    std::vector<unsigned char> result;
    for (int i=0; i<sizeof(vstrIn)/sizeof(vstrIn[0]); i++)
    {
        std::vector<unsigned char> expected(vstrIn[i].data, vstrIn[i].data + vstrIn[i].size);
        BOOST_CHECK(DecodeBase58(vstrOut[i], result));
        BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
    }
    BOOST_CHECK(!DecodeBase58("invalid", result));
}

BOOST_AUTO_TEST_SUITE_END()

